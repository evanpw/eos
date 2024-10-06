#pragma once

#include <stddef.h>
#include <stdint.h>

#include "net/ip.h"

enum class DnsResponseCode : uint8_t {
    NoError = 0,     // No error condition
    FormErr = 1,     // Format error
    ServFail = 2,    // Server failure
    NXDomain = 3,    // Non-existent domain
    NotImp = 4,      // Not implemented
    Refused = 5,     // Query refused
    YXDomain = 6,    // Name exists when it should not
    YXRRSet = 7,     // RR set exists when it should not
    NXRRSet = 8,     // RR set that should exist does not
    NotAuth = 9,     // Server not authoritative for zone
    NotZone = 10,    // Name not in zone
    BadVers = 16,    // Bad OPT version
    BadSig = 16,     // TSIG signature failure
    BadKey = 17,     // Key not recognized
    BadTime = 18,    // Signature out of time window
    BadMode = 19,    // Bad TKEY mode
    BadName = 20,    // Duplicate key name
    BadAlg = 21,     // Algorithm not supported
    BadTrunc = 22,   // Bad truncation
    BadCookie = 23,  // Bad cookie
};

enum class DnsOpcode : uint8_t {
    Query = 0,     // Standard query
    InvQuery = 1,  // Inverse query
    Status = 2,    // Server status request
    Notify = 4,    // Notify
    Update = 5,    // Dynamic update
    Dso = 6,       // DNS Stateful Operations
};

enum class DnsRecordType : uint16_t {
    A = 1,       // IPv4 address
    NS = 2,      // Name server
    CNAME = 5,   // Canonical name
    SOA = 6,     // Start of authority
    PTR = 12,    // Pointer
    MX = 15,     // Mail exchange
    TXT = 16,    // Text
    AAAA = 28,   // IPv6 address
    SRV = 33,    // Service locator
    OPT = 41,    // EDNS0 option
    AXFR = 252,  // Transfer zone of authority
    ANY = 255,   // Any record
};

enum class DnsRecordClass : uint16_t {
    IN = 1,  // Internet
    CS = 2,  // CSNET
    CH = 3,  // CHAOS
    HS = 4,  // Hesiod
};

class DnsHeader {
    uint16_t _id;  // transaction ID
    union {
        struct {
            uint16_t _rd : 1;      // recursion desired
            uint16_t _tc : 1;      // truncated response
            uint16_t _aa : 1;      // authoritative answer
            uint16_t _opcode : 4;  // operation code
            uint16_t _qr : 1;      // query/response flag
            uint16_t _rcode : 4;   // response code
            uint16_t _z : 3;       // reserved
            uint16_t _ra : 1;      // recursion available
        };
        uint16_t _flags;
    };
    uint16_t _qdcount;  // number of entries in the question section
    uint16_t _ancount;  // number of resource records in the answer section
    uint16_t _nscount;  // number of resource records in the authority records section
    uint16_t _arcount;  // number of resource records in the additional records section

public:
    uint16_t id() const;
    bool isReply() const;
    DnsOpcode opcode() const;
    bool authoritativeAnswer() const;
    bool truncated() const;
    bool recursionDesired() const;
    bool recursionAvailable() const;
    DnsResponseCode rcode() const;
    uint16_t flags() const;
    uint16_t qdcount() const;
    uint16_t ancount() const;
    uint16_t nscount() const;
    uint16_t arcount() const;

    void setId(uint16_t value);
    void setOpcode(DnsOpcode value);
    void setRecursionDesired();
    void setQdcount(uint16_t value);
    void setAncount(uint16_t value);
    void setNscount(uint16_t value);
    void setArcount(uint16_t value);
};

static_assert(sizeof(DnsHeader) == 12);

void dnsInit();
IpAddress dnsResolve(IpAddress dnsServer, const char* hostname, bool blocking = false);
void dnsRecv(uint8_t* buffer, size_t size);
