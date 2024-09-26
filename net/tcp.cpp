#include "net/tcp.h"

#include <arpa/inet.h>
#include <string.h>

#include "estd/print.h"

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

void tcpRecv(NicDevice*, uint8_t* buffer, size_t size) {
    if (size < sizeof(TcpHeader)) {
        return;
    }

    TcpHeader* tcpHeader = reinterpret_cast<TcpHeader*>(buffer);
    if (tcpHeader->dataOffset() * 4 > size) return;

    // Echo the message to the console
    size_t dataLen = size - sizeof(TcpHeader);
    char* s = new char[dataLen + 1];
    memcpy(s, buffer + tcpHeader->dataOffset() * 4, dataLen);
    s[dataLen] = '\0';
    println("tcp net message: {}", s);
}
