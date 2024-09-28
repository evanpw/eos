#include "net/tcp.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/new.h"
#include "estd/print.h"
#include "net/ip.h"
#include "net/nic_device.h"
#include "spinlock.h"
#include "system.h"
#include "timer.h"

uint16_t TcpHeader::computeChecksum(IpAddress srcIp, IpAddress destIp, size_t totalLen) {
    ASSERT(totalLen >= dataOffset() * 4);

    uint32_t sum = 0;

    uint8_t* bytes = reinterpret_cast<uint8_t*>(this);
    for (size_t i = 0; i + 1 < totalLen; i += 2) {
        uint16_t word = (bytes[i] << 8) | bytes[i + 1];
        sum += word;
    }

    // Handle an odd-sized buffer by padding with a zero byte
    if (totalLen % 2) {
        sum += (bytes[totalLen - 1] << 8);
    }

    // Subtract off the embedded checksum
    sum -= ntohs(_checksum);

    // Add in the pseudo-header
    sum += ntohs(lowBits(srcIp, 16));
    sum += ntohs(highBits(srcIp, 16));
    sum += ntohs(lowBits(destIp, 16));
    sum += ntohs(highBits(destIp, 16));
    sum += (uint16_t)IpProtocol::Tcp;
    sum += (uint16_t)totalLen;

    // Include the overflow
    while (sum >> 16) {
        sum = (sum & 0xFFFF) + (sum >> 16);
    }

    // Ones complement
    return ~sum;
}

bool TcpHeader::verifyChecksum(IpHeader* ipHeader) {
    size_t totalLen = ipHeader->totalLen() - ipHeader->headerLen() * 4;
    uint16_t cksum = computeChecksum(ipHeader->sourceIp(), ipHeader->destIp(), totalLen);
    return cksum == ntohs(_checksum);
}

void TcpHeader::fillChecksum(IpAddress srcIp, IpAddress destIp, size_t totalLen) {
    _checksum = htons(computeChecksum(srcIp, destIp, totalLen));
}

uint16_t TcpHeader::sourcePort() { return ntohs(_sourcePort); }
uint16_t TcpHeader::destPort() { return ntohs(_destPort); }
uint32_t TcpHeader::seqNum() { return ntohl(_seqNum); }
uint32_t TcpHeader::ackNum() { return ntohl(_ackNum); }
uint8_t TcpHeader::dataOffset() { return _dataOffset; }
uint8_t TcpHeader::fin() { return _fin; }
uint8_t TcpHeader::syn() { return _syn; }
uint8_t TcpHeader::rst() { return _rst; }
uint8_t TcpHeader::psh() { return _psh; }
uint8_t TcpHeader::ack() { return _ack; }
uint8_t TcpHeader::urg() { return _urg; }
uint8_t TcpHeader::ece() { return _ece; }
uint8_t TcpHeader::cwr() { return _cwr; }
uint16_t TcpHeader::windowSize() { return ntohs(_windowSize); }
uint16_t TcpHeader::checksum() { return ntohs(_checksum); }
uint16_t TcpHeader::urgentPointer() { return ntohs(_urgentPointer); }
uint8_t* TcpHeader::data() { return _data; }

void TcpHeader::setSourcePort(uint16_t value) { _sourcePort = htons(value); }
void TcpHeader::setDestPort(uint16_t value) { _destPort = htons(value); }
void TcpHeader::setSeqNum(uint32_t value) { _seqNum = htonl(value); }
void TcpHeader::setAckNum(uint32_t value) { _ackNum = htonl(value); }
void TcpHeader::setDataOffset(uint8_t value) { _dataOffset = value; }
void TcpHeader::setFin() { _fin = 1; }
void TcpHeader::setSyn() { _syn = 1; }
void TcpHeader::setRst() { _rst = 1; }
void TcpHeader::setPsh() { _psh = 1; }
void TcpHeader::setAck() { _ack = 1; }
void TcpHeader::setUrg() { _urg = 1; }
void TcpHeader::setEce() { _ece = 1; }
void TcpHeader::setCwr() { _cwr = 1; }
void TcpHeader::setWindowSize(uint16_t value) { _windowSize = htons(value); }
void TcpHeader::setChecksum(uint16_t value) { _checksum = htons(value); }
void TcpHeader::setUrgentPointer(uint16_t value) { _urgentPointer = htons(value); }

enum class TcpState {
    CLOSED,
    LISTEN,
    SYN_SENT,
    SYN_RECEIVED,
    ESTABLISHED,
    FIN_WAIT_1,
    FIN_WAIT_2,
    CLOSE_WAIT,
    CLOSING,
    LAST_ACK,
    TIME_WAIT
};

// TODO: needs a lock
struct TcpControlBlock {
    TcpState state;

    IpAddress localIp;
    uint16_t localPort;

    IpAddress remoteIp;
    uint16_t remotePort;

    uint32_t iss;  // initial send sequence number

    struct {
        uint32_t unacked;  // first unacknowledged sequence number
        uint32_t next;     // next sequence number to send
        uint32_t window;   // size of the send window
    } send;

    uint32_t irs;  // initial receive sequence number

    struct {
        uint32_t next;    // next sequence number expected on an incoming segment
        uint32_t window;  // size of the receive window
    } recv;

    TcpControlBlock* next;
};

static TcpControlBlock* tcbList = nullptr;
static Spinlock* tcpLock = nullptr;

void tcbInsert(TcpControlBlock* tcb);

void tcpInit() {
    tcpLock = new Spinlock();

    // Open a socket for listening on port 22
    TcpControlBlock* tcb = new TcpControlBlock;
    tcb->state = TcpState::LISTEN;
    tcb->localPort = 22;
    tcb->remoteIp = 0;
    tcb->remotePort = 0;
    tcbInsert(tcb);
}

void tcbInsert(TcpControlBlock* tcb) {
    SpinlockLocker locker(*tcpLock);

    tcb->next = tcbList;
    tcbList = tcb;
}

TcpControlBlock* tcbLookup(uint16_t localPort, IpAddress remoteIp, uint16_t remotePort) {
    SpinlockLocker locker(*tcpLock);

    TcpControlBlock* tcb = tcbList;
    while (tcb != nullptr) {
        if (tcb->localPort == localPort) {
            if (tcb->state == TcpState::LISTEN) {
                return tcb;
            }

            if ((tcb->remoteIp == remoteIp) && (tcb->remotePort == remotePort)) {
                return tcb;
            }
        }

        tcb = tcb->next;
    }

    return nullptr;
}

void tcpRecvListen(NicDevice* nic, TcpControlBlock* tcb, IpHeader* ipHeader,
                   TcpHeader* tcpHeader, uint8_t*, size_t) {
    ASSERT(tcpHeader->syn());

    // Setup the new connection
    tcb->state = TcpState::SYN_RECEIVED;
    tcb->localIp = nic->ipAddress();
    tcb->remotePort = tcpHeader->sourcePort();
    tcb->remoteIp = ipHeader->sourceIp();
    tcb->iss = 0;
    tcb->send.unacked = tcb->iss;
    tcb->send.next = tcb->iss;
    tcb->send.window = tcpHeader->windowSize();
    tcb->irs = tcpHeader->seqNum();
    tcb->recv.next = tcb->irs + 1;  // SYN consumes one sequence number
    tcb->recv.window = 64 * KiB - 1;

    // Reply with SYN-ACK
    TcpHeader response;
    response.setSourcePort(tcb->localPort);
    response.setDestPort(tcb->remotePort);
    response.setSeqNum(tcb->send.next);
    response.setAckNum(tcb->recv.next);
    response.setSyn();
    response.setAck();
    response.setWindowSize(tcb->recv.window);
    response.fillChecksum(tcb->localIp, tcb->remoteIp, sizeof(TcpHeader));
    ipSend(nic, tcb->remoteIp, IpProtocol::Tcp, &response, sizeof(TcpHeader));

    // The SYN consumes one sequence number
    tcb->send.next++;
}

void tcpRecvEstablished(NicDevice* nic, TcpControlBlock* tcb, TcpHeader* tcpHeader,
                        uint8_t* data, size_t dataLen);

void tcpRecvSynReceived(NicDevice* nic, TcpControlBlock* tcb, TcpHeader* tcpHeader,
                        uint8_t* data, size_t dataLen) {
    ASSERT(tcpHeader->ack());
    ASSERT(tcpHeader->seqNum() == tcb->recv.next);
    ASSERT(tcpHeader->ackNum() == tcb->send.next);

    tcb->state = TcpState::ESTABLISHED;
    tcb->send.unacked = tcpHeader->ackNum();
    tcb->send.window = tcpHeader->windowSize();

    // Handle TCP Fast Open (initial data in the ACK packet)
    if (tcpHeader->dataOffset() * 4 > sizeof(TcpHeader)) {
        tcpRecvEstablished(nic, tcb, tcpHeader, data, dataLen);
    }
}

void tcpRecvEstablished(NicDevice* nic, TcpControlBlock* tcb, TcpHeader* tcpHeader,
                        uint8_t* data, size_t dataLen) {
    ASSERT(tcpHeader->seqNum() == tcb->recv.next);
    ASSERT(tcpHeader->ackNum() == tcb->send.next);

    if (dataLen > 0) {
        // Echo the message to the console
        char* s = new char[dataLen + 1];
        memcpy(s, data, dataLen);
        s[dataLen] = '\0';
        println("tcp net message: {}", s);
    }

    tcb->state = TcpState::ESTABLISHED;
    tcb->send.window = tcpHeader->windowSize();
    tcb->send.unacked = tcpHeader->ackNum();
    tcb->recv.next += dataLen;

    // If we get a simple ACK with no data and no FIN, then we don't need to reply
    if (dataLen == 0 && !tcpHeader->fin()) {
        ASSERT(tcpHeader->ack());
        return;
    }

    // Acknowledge the message
    TcpHeader response;
    response.setSourcePort(tcb->localPort);
    response.setDestPort(tcb->remotePort);
    response.setSeqNum(tcb->send.next);
    response.setAckNum(tcb->recv.next);
    response.setAck();
    response.setWindowSize(tcb->recv.window);

    // If the remote side has sent a FIN, start tearing down the connection
    if (tcpHeader->fin()) {
        tcb->state = TcpState::LAST_ACK;
        tcb->recv.next++;  // received FIN consumes one sequence number
        tcb->send.next++;  // sent FIN also consumes one sequence number

        // Acknowledge their FIN and send our own
        response.setAckNum(tcb->recv.next);
        response.setFin();
    }

    response.fillChecksum(tcb->localIp, tcb->remoteIp, sizeof(TcpHeader));
    ipSend(nic, tcb->remoteIp, IpProtocol::Tcp, &response, sizeof(TcpHeader));
}

void tcpRecvFinWait2(NicDevice* nic, TcpControlBlock* tcb, TcpHeader* tcpHeader,
                     uint8_t* data, size_t dataLen);

void tcpRecvFinWait1(NicDevice* nic, TcpControlBlock* tcb, TcpHeader* tcpHeader,
                     uint8_t* data, size_t dataLen) {
    ASSERT(tcpHeader->ack());
    ASSERT(tcpHeader->seqNum() == tcb->recv.next);
    ASSERT(tcpHeader->ackNum() == tcb->send.next);

    tcb->state = TcpState::FIN_WAIT_2;
    tcb->send.unacked = tcpHeader->ackNum();
    tcb->send.window = tcpHeader->windowSize();

    // The remote side can send the ACK and FIN in the same packet
    if (tcpHeader->fin()) {
        tcpRecvFinWait2(nic, tcb, tcpHeader, data, dataLen);
    }
}

void tcpRecvFinWait2(NicDevice* nic, TcpControlBlock* tcb, TcpHeader* tcpHeader, uint8_t*,
                     size_t) {
    ASSERT(tcpHeader->fin());
    ASSERT(tcpHeader->seqNum() == tcb->recv.next);

    tcb->state = TcpState::TIME_WAIT;
    tcb->recv.next++;  // FIN consumes one sequence number

    // Acknowledge the final FIN
    TcpHeader response;
    response.setSourcePort(tcb->localPort);
    response.setDestPort(tcb->remotePort);
    response.setSeqNum(tcb->send.next);
    response.setAckNum(tcb->recv.next);
    response.setAck();
    response.setWindowSize(tcb->recv.window);
    response.fillChecksum(tcb->localIp, tcb->remoteIp, sizeof(TcpHeader));
    ipSend(nic, tcb->remoteIp, IpProtocol::Tcp, &response, sizeof(TcpHeader));
}

void tcpRecvLastAck(NicDevice*, TcpControlBlock* tcb, TcpHeader* tcpHeader, uint8_t*,
                    size_t) {
    ASSERT(tcpHeader->ack());
    ASSERT(tcpHeader->seqNum() == tcb->recv.next);
    ASSERT(tcpHeader->ackNum() == tcb->send.next);

    // Connection is closed, listen for a new connection
    tcb->state = TcpState::CLOSED;
}

void tcpRecvSynSent(NicDevice* nic, TcpControlBlock* tcb, TcpHeader* tcpHeader, uint8_t*,
                    size_t) {
    ASSERT(tcpHeader->syn());
    ASSERT(tcpHeader->ack());
    ASSERT(tcpHeader->ackNum() == tcb->send.next);

    // Finish initializing the control block
    tcb->state = TcpState::SYN_RECEIVED;
    tcb->send.window = tcpHeader->windowSize();
    tcb->irs = tcpHeader->seqNum();
    tcb->recv.next = tcb->irs + 1;  // SYN consumes one sequence number

    // Reply with ACK
    TcpHeader response;
    response.setSourcePort(tcb->localPort);
    response.setDestPort(tcb->remotePort);
    response.setSeqNum(tcb->send.next);
    response.setAckNum(tcb->recv.next);
    response.setAck();
    response.setWindowSize(tcb->recv.window);
    response.fillChecksum(tcb->localIp, tcb->remoteIp, sizeof(TcpHeader));
    ipSend(nic, tcb->remoteIp, IpProtocol::Tcp, &response, sizeof(TcpHeader));

    tcb->state = TcpState::ESTABLISHED;
}

void tcpRecv(NicDevice* nic, IpHeader* ipHeader, uint8_t* buffer, size_t size) {
    if (size < sizeof(TcpHeader)) {
        return;
    }

    TcpHeader* tcpHeader = reinterpret_cast<TcpHeader*>(buffer);
    if (tcpHeader->dataOffset() * 4 > size) return;
    if (!tcpHeader->verifyChecksum(ipHeader)) return;

    TcpControlBlock* tcb =
        tcbLookup(tcpHeader->destPort(), ipHeader->sourceIp(), tcpHeader->sourcePort());
    if (!tcb) return;

    size_t headerSize = tcpHeader->dataOffset() * 4;
    uint8_t* data = buffer + headerSize;
    size_t dataLen = size - headerSize;

    switch (tcb->state) {
        case TcpState::LISTEN:
            tcpRecvListen(nic, tcb, ipHeader, tcpHeader, data, dataLen);
            break;

        case TcpState::SYN_RECEIVED:
            tcpRecvSynReceived(nic, tcb, tcpHeader, data, dataLen);
            break;

        case TcpState::ESTABLISHED:
            tcpRecvEstablished(nic, tcb, tcpHeader, data, dataLen);
            break;

        case TcpState::FIN_WAIT_1:
            tcpRecvFinWait1(nic, tcb, tcpHeader, data, dataLen);
            break;

        case TcpState::FIN_WAIT_2:
            tcpRecvFinWait2(nic, tcb, tcpHeader, data, dataLen);
            break;

        case TcpState::LAST_ACK:
            tcpRecvLastAck(nic, tcb, tcpHeader, data, dataLen);
            break;

        case TcpState::SYN_SENT:
            tcpRecvSynSent(nic, tcb, tcpHeader, data, dataLen);
            break;

        default:
            break;
    }
}

TcpControlBlock* tcpConnect(NicDevice* nic, IpAddress destIp, uint16_t destPort) {
    // Setup the control block
    TcpControlBlock* tcb = new TcpControlBlock;
    tcb->state = TcpState::SYN_SENT;
    tcb->localIp = nic->ipAddress();
    tcb->localPort = 12345;
    tcb->remoteIp = destIp;
    tcb->remotePort = destPort;
    tcb->iss = 0;
    tcb->send.unacked = tcb->iss;
    tcb->send.next = tcb->iss;
    tcb->send.window = 0;
    tcb->irs = 0;
    tcb->recv.next = 0;
    tcb->recv.window = 64 * KiB - 1;
    tcbInsert(tcb);

    // Send the initial SYN
    TcpHeader header;
    header.setSourcePort(tcb->localPort);
    header.setDestPort(tcb->remotePort);
    header.setSeqNum(tcb->send.next);
    header.setSyn();
    header.setWindowSize(tcb->recv.window);
    header.fillChecksum(tcb->localIp, tcb->remoteIp, sizeof(TcpHeader));
    ipSend(nic, tcb->remoteIp, IpProtocol::Tcp, &header, sizeof(TcpHeader));

    // The SYN consumes one sequence number
    tcb->send.next++;

    return tcb;
}

void tcpSend(NicDevice* nic, TcpControlBlock* tcb, const uint8_t* data, size_t size) {
    // TODO: this is a race condition
    while (tcb->state != TcpState::ESTABLISHED) {
        System::timer().sleep(1);
    }

    // Construct the tcp header and send the data
    size_t packetSize = sizeof(TcpHeader) + size;
    uint8_t* packet = new uint8_t[packetSize];

    TcpHeader* tcpHeader = new (packet) TcpHeader;
    tcpHeader->setSourcePort(tcb->localPort);
    tcpHeader->setDestPort(tcb->remotePort);
    tcpHeader->setSeqNum(tcb->send.next);
    tcpHeader->setAckNum(tcb->recv.next);
    tcpHeader->setAck();
    tcpHeader->setPsh();
    tcpHeader->setWindowSize(tcb->recv.window);
    memcpy(tcpHeader->data(), data, size);
    tcpHeader->fillChecksum(tcb->localIp, tcb->remoteIp, packetSize);
    ipSend(nic, tcb->remoteIp, IpProtocol::Tcp, packet, packetSize);

    tcb->send.next += size;
}

void tcpClose(NicDevice* nic, TcpControlBlock* tcb) {
    if (tcb->state == TcpState::CLOSED) return;

    ASSERT(tcb->state == TcpState::ESTABLISHED);

    // Send the initial FIN
    TcpHeader response;
    response.setSourcePort(tcb->localPort);
    response.setDestPort(tcb->remotePort);
    response.setSeqNum(tcb->send.next);
    response.setAckNum(tcb->recv.next);
    response.setAck();
    response.setFin();
    response.setWindowSize(tcb->recv.window);
    response.fillChecksum(tcb->localIp, tcb->remoteIp, sizeof(TcpHeader));
    ipSend(nic, tcb->remoteIp, IpProtocol::Tcp, &response, sizeof(TcpHeader));

    tcb->send.next++;  // FIN consumes on sequence number
}
