#pragma once

#include <QByteArray>
#include <QString>
#include <cstdint>

struct PacketEntity {
    uint32_t packetNumber = 0;
    QString timestamp;
    uint32_t length = 0;
    QByteArray rawData;
    QString srcMac;
    QString dstMac;
    uint16_t etherType = 0;
    uint8_t ipVersion = 0;
    uint8_t ipHeaderLen = 0;
    uint8_t ttl = 0;
    uint8_t ipProtocol = 0;
    uint16_t ipTotalLen = 0;
    QString source;
    QString destination;
    QString protocol;
    uint16_t srcPort = 0;
    uint16_t dstPort = 0;
    uint32_t tcpSeq = 0;
    uint32_t tcpAck = 0;
    uint8_t tcpDataOffset = 0;
    uint8_t tcpFlags = 0;
    uint16_t tcpWindow = 0;
    uint16_t udpLength = 0;
    uint8_t icmpType = 0;
    uint8_t icmpCode = 0;
};
