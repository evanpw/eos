#include "net/dns.h"

#include <arpa/inet.h>

#include "estd/bits.h"
#include "estd/vector.h"
#include "net/udp.h"
#include "spinlock.h"
#include "system.h"
#include "timer.h"

uint16_t DnsHeader::id() const { return ntohs(_id); }
bool DnsHeader::isReply() const { return _qr; }
DnsOpcode DnsHeader::opcode() const { return (DnsOpcode)_opcode; }
bool DnsHeader::authoritativeAnswer() const { return _aa; }
bool DnsHeader::truncated() const { return _tc; }
bool DnsHeader::recursionDesired() const { return _rd; }
bool DnsHeader::recursionAvailable() const { return _ra; }
DnsResponseCode DnsHeader::rcode() const { return (DnsResponseCode)_rcode; }
uint16_t DnsHeader::flags() const { return ntohs(_flags); }
uint16_t DnsHeader::qdcount() const { return ntohs(_qdcount); }
uint16_t DnsHeader::ancount() const { return ntohs(_ancount); }
uint16_t DnsHeader::nscount() const { return ntohs(_nscount); }
uint16_t DnsHeader::arcount() const { return ntohs(_arcount); }

void DnsHeader::setId(uint16_t value) { _id = htons(value); }
void DnsHeader::setOpcode(DnsOpcode value) { _opcode = (uint16_t)value; }
void DnsHeader::setRecursionDesired() { _rd = 1; }
void DnsHeader::setQdcount(uint16_t value) { _qdcount = htons(value); }
void DnsHeader::setAncount(uint16_t value) { _ancount = htons(value); }
void DnsHeader::setNscount(uint16_t value) { _nscount = htons(value); }
void DnsHeader::setArcount(uint16_t value) { _arcount = htons(value); }

class QuestionBuilder {
    size_t _count = 0;
    estd::vector<uint8_t> _bytes;

public:
    bool add(const char* hostname, DnsRecordType type, DnsRecordClass cls) {
        // The hostname is encoded as a list of labels, each preceded by a length byte
        const char* p = hostname;
        while (*p) {
            const char* nextDot = strchr(p, '.');
            size_t len = nextDot ? nextDot - p : strlen(p);

            // Maximum label length is 63 bytes
            if (len >= 64) return false;

            _bytes.push_back(len);
            for (size_t i = 0; i < len; ++i) {
                _bytes.push_back(*p++);
            }

            // Skip the dot
            if (*p == '.') ++p;
        }

        // The list of labels is null-terminated
        _bytes.push_back(0);

        // Then comes the record type, 2 bytes in network byte order
        _bytes.push_back(highBits((uint16_t)type, 8));
        _bytes.push_back(lowBits((uint16_t)type, 8));

        // And the class code, in the same format
        _bytes.push_back(highBits((uint16_t)cls, 8));
        _bytes.push_back(lowBits((uint16_t)cls, 8));

        return true;
    };

    size_t count() const { return _count; }
    const uint8_t* data() const { return _bytes.data(); }
    size_t size() const { return _bytes.size(); }
};

struct DnsQuestion {
    const char* hostname;
    DnsRecordType type;
    DnsRecordClass cls;
};

struct DnsRecord {
    const char* name;
    DnsRecordType type;
    DnsRecordClass cls;
    uint32_t ttl;
    uint16_t rdlength;
    const uint8_t* rdata;

    IpAddress ip() const {
        ASSERT(type == DnsRecordType::A);
        ASSERT(rdlength == 4);
        return IpAddress(rdata[0], rdata[1], rdata[2], rdata[3]);
    }
};

class ResponseParser {
    const uint8_t* parseName(const uint8_t* packet, const uint8_t* start,
                             estd::vector<char>& result) {
        const uint8_t* p = start;

        // Domain names are sequence of labels ending in a zero byte, a pointer, or a
        // sequence of labels ending with a pointer
        while (*p) {
            // Separate the labels by dots
            if (!result.empty()) {
                result.push_back('.');
            }

            // If the high 2 bits are set, then this is a pointer to a previous name
            if (highBits(*p, 2) == 0b11) {
                uint8_t upperByte = lowBits(*p++, 6);
                uint8_t lowerByte = *p++;
                uint16_t offset = concatBits(upperByte, lowerByte);

                parseName(packet, packet + offset, result);
                return p;
            }

            // Otherwise, this is a label
            uint8_t len = *p++;
            ASSERT(len < 64);
            for (size_t i = 0; i < len; ++i) {
                result.push_back(*p++);
            }
        }

        // Skip the null terminator of the sequence of labels
        ASSERT(*p == 0);
        return ++p;
    }

    const uint8_t* parseQuestions(const uint8_t* packet, const uint8_t* start,
                                  size_t count) {
        const uint8_t* p = start;

        for (size_t i = 0; i < count; ++i) {
            estd::vector<char> hostnameBuffer;
            p = parseName(packet, p, hostnameBuffer);

            // Parse the type and class
            uint16_t typeRaw;
            memcpy(&typeRaw, p, 2);
            DnsRecordType type = (DnsRecordType)ntohs(typeRaw);
            p += 2;

            uint16_t clsRaw;
            memcpy(&clsRaw, p, 2);
            DnsRecordClass cls = (DnsRecordClass)ntohs(clsRaw);
            p += 2;

            // Make a copy of the hostname as a null-terminated string
            char* hostname = new char[hostnameBuffer.size() + 1];
            memcpy(hostname, hostnameBuffer.data(), hostnameBuffer.size());
            hostname[hostnameBuffer.size()] = '\0';
            _hostnames.push_back(hostname);

            // Construct the question structure and add it to the list
            questions.push_back(DnsQuestion{_hostnames.back(), type, cls});
        }

        return p;
    }

    const uint8_t* parseAnswers(const uint8_t* packet, const uint8_t* start,
                                size_t count) {
        const uint8_t* p = start;

        for (size_t i = 0; i < count; ++i) {
            estd::vector<char> hostnameBuffer;
            p = parseName(packet, p, hostnameBuffer);

            // Parse the remaining fixed-length fields
            uint16_t typeRaw;
            memcpy(&typeRaw, p, 2);
            DnsRecordType type = (DnsRecordType)ntohs(typeRaw);
            p += 2;

            uint16_t clsRaw;
            memcpy(&clsRaw, p, 2);
            DnsRecordClass cls = (DnsRecordClass)ntohs(clsRaw);
            p += 2;

            uint32_t ttlRaw;
            memcpy(&ttlRaw, p, 4);
            uint32_t ttl = ntohl(ttlRaw);
            p += 4;

            uint16_t rdlengthRaw;
            memcpy(&rdlengthRaw, p, 2);
            uint16_t rdlength = ntohs(rdlengthRaw);
            p += 2;

            const uint8_t* rdata = p;

            // Make a copy of the hostname as a null-terminated string
            char* hostname = new char[hostnameBuffer.size() + 1];
            memcpy(hostname, hostnameBuffer.data(), hostnameBuffer.size());
            hostname[hostnameBuffer.size()] = '\0';
            _hostnames.push_back(hostname);

            // Construct the record structure and add it to the list
            answers.push_back(
                DnsRecord{_hostnames.back(), type, cls, ttl, rdlength, rdata});

            // Skip the variable-length data
            p += rdlength;
        }

        return p;
    }

    estd::vector<const char*> _hostnames;

public:
    ResponseParser(const uint8_t* packet) {
        estd::vector<char> hostnameBuffer;

        const DnsHeader* header = reinterpret_cast<const DnsHeader*>(packet);
        const uint8_t* p = packet + sizeof(DnsHeader);

        p = parseQuestions(packet, p, header->qdcount());
        p = parseAnswers(packet, p, header->ancount());
    }

    ~ResponseParser() {
        for (auto str : _hostnames) {
            delete[] str;
        }
    }

    // Not copyable or movable
    ResponseParser(const ResponseParser&) = delete;
    ResponseParser& operator=(const ResponseParser&) = delete;

    estd::vector<DnsQuestion> questions;
    estd::vector<DnsRecord> answers;
};

struct CacheEntry {
    const char* hostname = nullptr;
    IpAddress ip;
    CacheEntry* next = nullptr;

    ~CacheEntry() { delete[] hostname; }
};

static Spinlock* dnsLock;
static CacheEntry* dnsCache;

void dnsInit() {
    dnsLock = new Spinlock();
    dnsCache = nullptr;
}

bool dnsLookupCached(const char* hostname, IpAddress* result) {
    SpinlockLocker locker(*dnsLock);

    CacheEntry* entry = dnsCache;
    while (entry != nullptr) {
        // TODO: case-insensitive comparison
        if (strcmp(hostname, entry->hostname) == 0) {
            *result = entry->ip;
            return true;
        }

        entry = entry->next;
    }

    return false;
}

static void dnsInsert(const char* hostname, IpAddress ip) {
    SpinlockLocker locker(*dnsLock);

    // If we already have an entry for this ip, just update the mac
    CacheEntry* entry = dnsCache;
    while (entry != nullptr) {
        // TODO: case-insensitive comparison
        if (strcmp(hostname, entry->hostname) == 0) {
            entry->ip = ip;
            return;
        }

        entry = entry->next;
    }

    // Otherwise, add it to the list
    CacheEntry* newEntry = new CacheEntry;
    newEntry->hostname = strdup(hostname);
    newEntry->ip = ip;
    newEntry->next = dnsCache;
    dnsCache = newEntry;
}

void dnsQuery(NetworkInterface* netif, IpAddress dnsServer, const char* hostname) {
    QuestionBuilder questions;
    questions.add(hostname, DnsRecordType::A, DnsRecordClass::IN);

    size_t packetSize = sizeof(DnsHeader) + questions.size();
    uint8_t* packet = new uint8_t[packetSize];

    DnsHeader* header = new (packet) DnsHeader{};
    header->setId(0x1234);
    header->setOpcode(DnsOpcode::Query);
    header->setRecursionDesired();
    header->setQdcount(1);
    memcpy(packet + sizeof(DnsHeader), questions.data(), questions.size());

    udpSend(netif, dnsServer, 10053, 53, packet, packetSize, true);

    delete[] packet;
}

IpAddress dnsResolve(NetworkInterface* netif, IpAddress dnsServer, const char* hostname,
                     bool blocking) {
    IpAddress result;
    if (dnsLookupCached(hostname, &result)) {
        return result;
    }

    if (!blocking) {
        return IpAddress();
    }

    // If not in cache, we have to send a dns request
    dnsQuery(netif, dnsServer, hostname);

    // Wait for the reply
    do {
        // TODO: create a thread blocker for this
        sys.timer().sleep(10);
        // TODO: re-send after a timeout
    } while (!dnsLookupCached(hostname, &result));

    return result;
}

void dnsRecv(NetworkInterface*, uint8_t* buffer, size_t size) {
    if (size < sizeof(DnsHeader)) {
        return;
    }

    DnsHeader* header = reinterpret_cast<DnsHeader*>(buffer);
    if (!header->isReply()) return;
    if (header->rcode() != DnsResponseCode::NoError) return;
    if (header->id() != 0x1234) return;
    if (header->qdcount() != 1) return;
    if (header->ancount() == 0) return;

    ResponseParser parser(buffer);
    for (auto& answer : parser.answers) {
        if (answer.type == DnsRecordType::A) {
            dnsInsert(parser.questions[0].hostname, answer.ip());
        }
    }
}
