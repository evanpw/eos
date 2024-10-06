#include "net/tcp.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/new.h"
#include "klibc.h"
#include "net/ip.h"
#include "net/network_interface.h"
#include "panic.h"
#include "scheduler.h"
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

struct TcpControlBlock {
    TcpControlBlock();

    Spinlock lock;

    // Used to identify a control block in the rest of the kernel, without exposing the
    // internal details of the control block
    TcpHandle handle;

    TcpState state;
    IpAddress localIp;
    uint16_t localPort;
    IpAddress remoteIp;
    uint16_t remotePort;
    uint32_t iss;  // initial send sequence number
    uint32_t irs;  // initial receive sequence number

    struct {
        uint32_t unacked;  // first unacknowledged sequence number
        uint32_t next;     // next sequence number to send
        uint32_t window;   // size of the send window
    } send;

    struct {
        uint32_t next;    // next sequence number expected on an incoming segment
        uint32_t window;  // size of the receive window
    } recv;

    static constexpr size_t RECV_BUFFER_SIZE = 64 * KiB - 1;

    uint8_t recvBuffer[RECV_BUFFER_SIZE];
    uint32_t recvBufferUsed() { return sizeof(recvBuffer) - recv.window; }
    bool recvBufferEmpty() { return recvBufferUsed() == 0; }

    // Used to wait for certain conditions to occur
    estd::shared_ptr<Blocker> connectionEstablished;
    estd::shared_ptr<Blocker> dataAvailable;

    TcpControlBlock* next;
};

static Spinlock* tcpLock = nullptr;
static TcpControlBlock* tcbList = nullptr;
static TcpHandle nextHandle;
static constexpr uint16_t MIN_EPHEMERAL_PORT = 32768;
static constexpr uint16_t MAX_PORT = 65535;

void tcpInit() {
    tcpLock = new Spinlock();
    nextHandle = 1;
}

uint16_t getEphemeralPort() {
    SpinlockLocker locker(*tcpLock);

    // Scan over the ephemeral port range to find the next available port
    for (uint32_t port = MIN_EPHEMERAL_PORT; port <= MAX_PORT; ++port) {
        TcpControlBlock* tcb = tcbList;
        bool found = false;
        while (tcb != nullptr) {
            SpinlockLocker tcbLocker(tcb->lock);
            if (tcb->localPort == port) {
                found = true;
                break;
            }

            tcb = tcb->next;
        }

        if (!found) return port;
    }

    panic("no available ephemeral ports");
}

uint64_t createHandle() {
    SpinlockLocker locker(*tcpLock);
    return nextHandle++;
}

TcpControlBlock::TcpControlBlock() {
    handle = createHandle();
    state = TcpState::CLOSED;
    localIp = IpAddress();
    localPort = 0;
    remoteIp = IpAddress();
    remotePort = 0;
    iss = 0;
    irs = 0;
    send.unacked = 0;
    send.next = 0;
    send.window = 0;
    recv.next = 0;
    recv.window = RECV_BUFFER_SIZE;
    connectionEstablished.assign(new Blocker);
    dataAvailable.assign(new Blocker);
    next = nullptr;
}

void tcbInsert(TcpControlBlock* tcb) {
    SpinlockLocker locker(*tcpLock);
    SpinlockLocker tcbLocker(tcb->lock);

    tcb->next = tcbList;
    tcbList = tcb;
}

void tcbRemove(TcpControlBlock* tcb) {
    SpinlockLocker locker(*tcpLock);

    if (tcbList == tcb) {
        tcbList = tcb->next;
        return;
    }

    TcpControlBlock* prev = tcbList;
    while (prev && prev->next != tcb) {
        prev = prev->next;
    }

    if (prev) {
        prev->next = tcb->next;
    }
}

// Returns the tcb in the locked state, must be unlocked by the caller
TcpControlBlock* tcbLookup(TcpHandle handle) {
    SpinlockLocker locker(*tcpLock);

    TcpControlBlock* tcb = tcbList;
    while (tcb != nullptr) {
        tcb->lock.lock();
        if (tcb->handle == handle) {
            return tcb;
        }

        TcpControlBlock* next = tcb->next;
        tcb->lock.unlock();
        tcb = next;
    }

    return nullptr;
}

// Returns the tcb in the locked state, must be unlocked by the caller
TcpControlBlock* tcbLookup(uint16_t localPort) {
    SpinlockLocker locker(*tcpLock);

    TcpControlBlock* tcb = tcbList;
    while (tcb != nullptr) {
        tcb->lock.lock();
        if (tcb->localPort == localPort) {
            return tcb;
        }

        TcpControlBlock* next = tcb->next;
        tcb->lock.unlock();
        tcb = next;
    }

    return nullptr;
}

// Returns the tcb in the locked state, must be unlocked by the caller
TcpControlBlock* tcbLookup(uint16_t localPort, IpAddress remoteIp, uint16_t remotePort) {
    SpinlockLocker locker(*tcpLock);

    TcpControlBlock* tcb = tcbList;
    while (tcb != nullptr) {
        tcb->lock.lock();
        if (tcb->localPort == localPort) {
            if (tcb->state == TcpState::LISTEN) {
                return tcb;
            }

            if ((tcb->remoteIp == remoteIp) && (tcb->remotePort == remotePort)) {
                return tcb;
            }
        }

        TcpControlBlock* next = tcb->next;
        tcb->lock.unlock();
        tcb = next;
    }

    return nullptr;
}

void tcpRecvListen(NetworkInterface* netif, TcpControlBlock* tcb, IpHeader* ipHeader,
                   TcpHeader* tcpHeader, uint8_t*, size_t) {
    ASSERT(tcb->lock.isLocked());
    ASSERT(tcpHeader->syn());

    // Setup the new connection
    tcb->state = TcpState::SYN_RECEIVED;
    tcb->localIp = netif->ipAddress();
    tcb->remotePort = tcpHeader->sourcePort();
    tcb->remoteIp = ipHeader->sourceIp();
    tcb->iss = 0;
    tcb->send.unacked = tcb->iss;
    tcb->send.next = tcb->iss;
    tcb->send.window = tcpHeader->windowSize();
    tcb->irs = tcpHeader->seqNum();
    tcb->recv.next = tcb->irs + 1;  // SYN consumes one sequence number
    tcb->recv.window = TcpControlBlock::RECV_BUFFER_SIZE;

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
    ipSend(netif, tcb->remoteIp, IpProtocol::Tcp, &response, sizeof(TcpHeader));

    // The SYN consumes one sequence number
    tcb->send.next++;
}

void tcpRecvEstablished(NetworkInterface* netif, TcpControlBlock* tcb,
                        TcpHeader* tcpHeader, uint8_t* data, size_t dataLen);

void tcpRecvSynReceived(NetworkInterface* netif, TcpControlBlock* tcb,
                        TcpHeader* tcpHeader, uint8_t* data, size_t dataLen) {
    ASSERT(tcb->lock.isLocked());
    ASSERT(tcpHeader->ack());
    ASSERT(tcpHeader->seqNum() == tcb->recv.next);
    ASSERT(tcpHeader->ackNum() == tcb->send.next);

    tcb->state = TcpState::ESTABLISHED;
    tcb->send.unacked = tcpHeader->ackNum();
    tcb->send.window = tcpHeader->windowSize();

    // Handle TCP Fast Open (initial data in the ACK packet)
    if (tcpHeader->dataOffset() * 4 > sizeof(TcpHeader)) {
        tcpRecvEstablished(netif, tcb, tcpHeader, data, dataLen);
    }

    sys.scheduler().wakeThreads(tcb->connectionEstablished);
}

void tcpRecvEstablished(NetworkInterface* netif, TcpControlBlock* tcb,
                        TcpHeader* tcpHeader, uint8_t* data, size_t dataLen) {
    ASSERT(tcb->lock.isLocked());
    ASSERT(tcpHeader->seqNum() == tcb->recv.next);
    ASSERT(tcpHeader->ackNum() == tcb->send.next);

    // If we received more data than we can store, just truncate (and ACK only the
    // portion that we handled)
    size_t origDataLen = dataLen;
    dataLen = min<size_t>(dataLen, tcb->recv.window);

    if (dataLen > 0) {
        memcpy(tcb->recvBuffer + tcb->recvBufferUsed(), data, dataLen);
        tcb->recv.window -= dataLen;
        sys.scheduler().wakeThreads(tcb->dataAvailable);
    }

    tcb->send.window = tcpHeader->windowSize();
    tcb->send.unacked = tcpHeader->ackNum();
    tcb->recv.next += dataLen;

    // If the other side is finished, continue acking this segment, and wait for the
    // local user to close the connection
    if (tcpHeader->fin()) {
        tcb->state = TcpState::CLOSE_WAIT;
        tcb->recv.next++;  // ACK the FIN, which consumes one sequence number
    }

    // If we get a simple ACK with no data and no FIN, then we don't need to reply
    if (origDataLen == 0 && !tcpHeader->fin()) {
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

    response.fillChecksum(tcb->localIp, tcb->remoteIp, sizeof(TcpHeader));
    ipSend(netif, tcb->remoteIp, IpProtocol::Tcp, &response, sizeof(TcpHeader));
}

void tcpRecvFinWait2(NetworkInterface* netif, TcpControlBlock* tcb, TcpHeader* tcpHeader,
                     uint8_t* data, size_t dataLen);

void tcpRecvFinWait1(NetworkInterface* netif, TcpControlBlock* tcb, TcpHeader* tcpHeader,
                     uint8_t* data, size_t dataLen) {
    ASSERT(tcb->lock.isLocked());
    ASSERT(tcpHeader->ack());
    ASSERT(tcpHeader->seqNum() == tcb->recv.next);
    ASSERT(tcpHeader->ackNum() == tcb->send.next);

    tcb->state = TcpState::FIN_WAIT_2;
    tcb->send.unacked = tcpHeader->ackNum();
    tcb->send.window = tcpHeader->windowSize();

    // The remote side can send the ACK and FIN in the same packet
    if (tcpHeader->fin()) {
        tcpRecvFinWait2(netif, tcb, tcpHeader, data, dataLen);
    }
}

void tcpRecvFinWait2(NetworkInterface* netif, TcpControlBlock* tcb, TcpHeader* tcpHeader,
                     uint8_t*, size_t) {
    ASSERT(tcb->lock.isLocked());
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
    ipSend(netif, tcb->remoteIp, IpProtocol::Tcp, &response, sizeof(TcpHeader));
}

void tcpRecvLastAck(NetworkInterface*, TcpControlBlock* tcb, TcpHeader* tcpHeader,
                    uint8_t*, size_t) {
    ASSERT(tcb->lock.isLocked());
    ASSERT(tcpHeader->ack());
    ASSERT(tcpHeader->seqNum() == tcb->recv.next);
    ASSERT(tcpHeader->ackNum() == tcb->send.next);

    // Connection is closed, listen for a new connection
    tcb->state = TcpState::CLOSED;
}

void tcpRecvSynSent(NetworkInterface* netif, TcpControlBlock* tcb, TcpHeader* tcpHeader,
                    uint8_t*, size_t) {
    ASSERT(tcb->lock.isLocked());
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
    ipSend(netif, tcb->remoteIp, IpProtocol::Tcp, &response, sizeof(TcpHeader));

    tcb->state = TcpState::ESTABLISHED;
    sys.scheduler().wakeThreads(tcb->connectionEstablished);
}

void tcpRecv(NetworkInterface* netif, IpHeader* ipHeader, uint8_t* buffer, size_t size) {
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
            tcpRecvListen(netif, tcb, ipHeader, tcpHeader, data, dataLen);
            break;

        case TcpState::SYN_RECEIVED:
            tcpRecvSynReceived(netif, tcb, tcpHeader, data, dataLen);
            break;

        case TcpState::ESTABLISHED:
            tcpRecvEstablished(netif, tcb, tcpHeader, data, dataLen);
            break;

        case TcpState::FIN_WAIT_1:
            tcpRecvFinWait1(netif, tcb, tcpHeader, data, dataLen);
            break;

        case TcpState::FIN_WAIT_2:
            tcpRecvFinWait2(netif, tcb, tcpHeader, data, dataLen);
            break;

        case TcpState::LAST_ACK:
            tcpRecvLastAck(netif, tcb, tcpHeader, data, dataLen);
            break;

        case TcpState::SYN_SENT:
            tcpRecvSynSent(netif, tcb, tcpHeader, data, dataLen);
            break;

        default:
            break;
    }

    tcb->lock.unlock();
}

//// High-level kernel-mode API
bool tcpWaitForConnection(TcpHandle handle) {
    // Lookup the connection
    TcpControlBlock* tcb = tcbLookup(handle);
    if (!tcb) return false;

    while (true) {
        // TODO: add a timeout
        switch (tcb->state) {
            case TcpState::SYN_SENT:
            case TcpState::SYN_RECEIVED:
                // Wait for the connection to be established
                sys.scheduler().sleepThread(tcb->connectionEstablished, &tcb->lock);
                break;

            case TcpState::ESTABLISHED:
            case TcpState::CLOSE_WAIT:
                tcb->lock.unlock();
                return true;

            default:
                // Error: connection is closed or closing
                tcb->lock.unlock();
                return false;
        }
    }
}

TcpHandle tcpConnect(NetworkInterface* netif, IpAddress destIp, uint16_t destPort) {
    // Create and initialize the control block
    TcpControlBlock* tcb = new TcpControlBlock;
    tcb->localIp = netif->ipAddress();
    tcb->localPort = getEphemeralPort();
    tcb->remoteIp = destIp;
    tcb->remotePort = destPort;
    tcbInsert(tcb);

    tcb->lock.lock();

    // Send the initial SYN
    TcpHeader header;
    header.setSourcePort(tcb->localPort);
    header.setDestPort(tcb->remotePort);
    header.setSeqNum(tcb->send.next);
    header.setSyn();
    header.setWindowSize(tcb->recv.window);
    header.fillChecksum(tcb->localIp, tcb->remoteIp, sizeof(TcpHeader));

    tcb->state = TcpState::SYN_SENT;
    tcb->send.next++;  // SYN consumes one sequence number
    TcpHandle handle = tcb->handle;
    tcb->lock.unlock();

    // Will block until sent (may have to wait for ARP resolution)
    ipSend(netif, tcb->remoteIp, IpProtocol::Tcp, &header, sizeof(TcpHeader), true);

    return handle;
}

TcpHandle tcpListen(NetworkInterface* netif, uint16_t port) {
    // Make sure the port is available
    TcpControlBlock* existing = tcbLookup(port);
    if (existing) {
        existing->lock.unlock();
        return InvalidTcpHandle;
    }

    // Create and initialize the control block
    TcpControlBlock* tcb = new TcpControlBlock;
    tcb->localIp = netif->ipAddress();
    tcb->localPort = port;
    tcbInsert(tcb);

    SpinlockLocker tcbLocker(tcb->lock);

    tcb->state = TcpState::LISTEN;
    return tcb->handle;
}

bool tcpSend(NetworkInterface* netif, TcpHandle handle, const void* buffer, size_t size,
             bool push) {
    // Lookup the connection
    TcpControlBlock* tcb = tcbLookup(handle);
    if (!tcb) {
        return false;
    }

    bool ready = false;
    while (!ready) {
        // TODO: add a timeout
        switch (tcb->state) {
            case TcpState::SYN_SENT:
            case TcpState::SYN_RECEIVED:
                // Wait for the connection to be established
                sys.scheduler().sleepThread(tcb->connectionEstablished, &tcb->lock);
                break;

            case TcpState::ESTABLISHED:
            case TcpState::CLOSE_WAIT:
                // Send the data now
                ready = true;
                break;

            default:
                // Error: connection is closed or closing
                tcb->lock.unlock();
                return false;
        }
    }

    size_t packetSize = sizeof(TcpHeader) + size;
    uint8_t* packet = new uint8_t[packetSize];

    // Construct the header
    TcpHeader* tcpHeader = new (packet) TcpHeader;
    tcpHeader->setSourcePort(tcb->localPort);
    tcpHeader->setDestPort(tcb->remotePort);
    tcpHeader->setSeqNum(tcb->send.next);
    tcpHeader->setAckNum(tcb->recv.next);
    tcpHeader->setAck();
    if (push) tcpHeader->setPsh();
    tcpHeader->setWindowSize(tcb->recv.window);

    // Copy the payload into the packet
    memcpy(tcpHeader->data(), buffer, size);
    tcpHeader->fillChecksum(tcb->localIp, tcb->remoteIp, packetSize);

    tcb->send.next += size;
    tcb->lock.unlock();

    // Will block until sent (may have to wait for ARP resolution)
    ipSend(netif, tcb->remoteIp, IpProtocol::Tcp, packet, packetSize, true);

    return true;
}

ssize_t tcpRecv(NetworkInterface* netif, TcpHandle handle, void* buffer, size_t size) {
    // Lookup the connection
    TcpControlBlock* tcb = tcbLookup(handle);
    if (!tcb) return -1;

    bool ready = false;
    while (!ready) {
        // TODO: add a timeout
        switch (tcb->state) {
            case TcpState::LISTEN:
            case TcpState::SYN_SENT:
            case TcpState::SYN_RECEIVED:
                // Wait for the connection to be established
                sys.timer().sleep(1, &tcb->lock);
                break;

            case TcpState::ESTABLISHED:
            case TcpState::FIN_WAIT_1:
            case TcpState::FIN_WAIT_2:
                // Wait until we receive a push or the recv window is full
                if (!tcb->recvBufferEmpty()) {
                    ready = true;
                } else {
                    sys.timer().sleep(1, &tcb->lock);
                }
                break;

            case TcpState::CLOSE_WAIT:
                // No more data is coming, but we can read what's left in the buffer
                if (!tcb->recvBufferEmpty()) {
                    ready = true;
                } else {
                    tcb->lock.unlock();
                    return 0;
                }
                break;

            default:
                // Error: connection is closed or closing
                tcb->lock.unlock();
                return -1;
        }
    }

    // Copy the data from the receive buffer to the caller's buffer
    size_t readSize = min<size_t>(size, tcb->recvBufferUsed());
    memcpy(buffer, tcb->recvBuffer, readSize);

    // Shift the remaining data to the front of the buffer
    memcpy(tcb->recvBuffer, tcb->recvBuffer + readSize, tcb->recvBufferUsed() - readSize);
    uint32_t prevWindow = tcb->recv.window;
    tcb->recv.window += readSize;

    // If moving from a zero (or near-zero) window to a non-zero window, send a window
    // update to the remote side
    if ((prevWindow < 1500 && tcb->recv.window >= 1500) ||
        (prevWindow == 0 && tcb->recv.window > 0)) {
        TcpHeader header;
        header.setSourcePort(tcb->localPort);
        header.setDestPort(tcb->remotePort);
        header.setSeqNum(tcb->send.next);
        header.setAckNum(tcb->recv.next);
        header.setAck();
        header.setWindowSize(tcb->recv.window);
        header.fillChecksum(tcb->localIp, tcb->remoteIp, sizeof(TcpHeader));
        tcb->lock.unlock();

        // Will block until sent (may have to wait for ARP resolution)
        ipSend(netif, tcb->remoteIp, IpProtocol::Tcp, &header, sizeof(TcpHeader), true);
    } else {
        tcb->lock.unlock();
    }

    return readSize;
}

bool tcpClose(NetworkInterface* netif, TcpHandle handle) {
    // Lookup the connection
    TcpControlBlock* tcb = tcbLookup(handle);
    if (!tcb) return false;

    switch (tcb->state) {
        case TcpState::LISTEN:
        case TcpState::SYN_SENT:
            // Delete the control block and enter CLOSED state (no need to send a FIN)
            // Any pending reads or writes will return an error
            // TODO: use shared ptrs for cleanup rather than leaking the TCB
            tcbRemove(tcb);
            tcb->state = TcpState::CLOSED;
            tcb->lock.unlock();
            return true;

        case TcpState::SYN_RECEIVED:
        case TcpState::ESTABLISHED:
        case TcpState::CLOSE_WAIT:
            break;

        default:
            // Error: already closed or closing
            return false;
    }

    // Send a FIN and transition state to wait for remote side
    TcpHeader header;
    header.setSourcePort(tcb->localPort);
    header.setDestPort(tcb->remotePort);
    header.setSeqNum(tcb->send.next);
    header.setAckNum(tcb->recv.next);
    header.setAck();
    header.setFin();
    header.setWindowSize(tcb->recv.window);
    header.fillChecksum(tcb->localIp, tcb->remoteIp, sizeof(TcpHeader));

    if (tcb->state == TcpState::SYN_RECEIVED || tcb->state == TcpState::ESTABLISHED) {
        tcb->state = TcpState::FIN_WAIT_1;
    } else {
        tcb->state = TcpState::LAST_ACK;
    }
    tcb->send.next++;  // FIN consumes one sequence number
    tcb->lock.unlock();

    // Will block until sent (may have to wait for ARP resolution)
    ipSend(netif, tcb->remoteIp, IpProtocol::Tcp, &header, sizeof(TcpHeader), true);

    return true;
}
