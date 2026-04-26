#include "packet_controller.hpp"

#include <QDateTime>
#include <winsock2.h>
#include <ws2tcpip.h>

namespace {

#pragma pack(push, 1)

struct EthernetHeader {
    uint8_t destMac[6];
    uint8_t srcMac[6];
    uint16_t etherType;
};

struct IpHeader {
    uint8_t versionAndHeaderLen;
    uint8_t tos;
    uint16_t totalLength;
    uint16_t id;
    uint16_t flagsAndOffset;
    uint8_t ttl;
    uint8_t protocol;
    uint16_t checksum;
    uint32_t srcAddr;
    uint32_t destAddr;
};

struct TcpHeader {
    uint16_t srcPort;
    uint16_t dstPort;
    uint32_t seqNum;
    uint32_t ackNum;
    uint8_t dataOffset;
    uint8_t flags;
    uint16_t windowSize;
    uint16_t checksum;
    uint16_t urgentPtr;
};

struct UdpHeader {
    uint16_t srcPort;
    uint16_t dstPort;
    uint16_t length;
    uint16_t checksum;
};

struct IcmpHeader {
    uint8_t type;
    uint8_t code;
    uint16_t checksum;
};

#pragma pack(pop)

QString formatMac(const uint8_t *mac) {
    return QString("%1:%2:%3:%4:%5:%6")
        .arg(mac[0], 2, 16, QChar('0'))
        .arg(mac[1], 2, 16, QChar('0'))
        .arg(mac[2], 2, 16, QChar('0'))
        .arg(mac[3], 2, 16, QChar('0'))
        .arg(mac[4], 2, 16, QChar('0'))
        .arg(mac[5], 2, 16, QChar('0'));
}

QString ipToStr(uint32_t addr) {
    in_addr address;
    address.s_addr = addr;
    char buffer[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &address, buffer, INET_ADDRSTRLEN);
    return QString(buffer);
}

}  // namespace

PacketController::PacketController(QObject *parent)
    : QObject(parent), m_captureEngine(this) {
    qRegisterMetaType<PacketData>("PacketData");

    connect(&m_captureEngine, &PacketCapture::packetCaptured,
            this, &PacketController::processCapturedPacket);
    connect(&m_captureEngine, &PacketCapture::captureError,
            this, &PacketController::captureError);
}

QStringList PacketController::getDeviceList() const {
    return m_captureEngine.getDeviceList();
}

void PacketController::startCapture(const QString &deviceName) {
    m_captureEngine.startCapture(deviceName);
}

void PacketController::stopCapture() {
    m_captureEngine.stopCapture();
}

void PacketController::setDatabaseConfig(const DatabaseConfig &config) {
    m_databaseService.setDatabaseConfig(config);
}

DatabaseConfig PacketController::databaseConfig() const {
    return m_databaseService.databaseConfig();
}

QStringList PacketController::availableDatabaseDrivers() const {
    return DbManager::availableDrivers();
}

bool PacketController::testDatabaseConnection(QString *errorMessage) {
    return m_databaseService.testConnection(errorMessage);
}

bool PacketController::savePacket(const PacketData &packet, QString *errorMessage) {
    return m_databaseService.savePacket(packet, errorMessage);
}

void PacketController::processCapturedPacket(CapturedPacket packet) {
    emit packetCaptured(buildPacketData(packet));
}

PacketData PacketController::buildPacketData(const CapturedPacket &packet) const {
    PacketData pkt;
    pkt.number = packet.number;
    pkt.length = packet.length;
    pkt.rawData = packet.rawData;

    const QDateTime dt = QDateTime::fromSecsSinceEpoch(packet.timestamp.tv_sec);
    pkt.timestamp = dt.toString("hh:mm:ss.") +
                    QString::number(packet.timestamp.tv_usec / 1000)
                        .rightJustified(3, '0');

    const auto *data =
        reinterpret_cast<const u_char *>(packet.rawData.constData());
    const auto caplen = static_cast<uint32_t>(packet.rawData.size());

    if (caplen < sizeof(EthernetHeader)) {
        return pkt;
    }

    const auto *eth = reinterpret_cast<const EthernetHeader *>(data);
    const uint16_t etherType = ntohs(eth->etherType);

    pkt.srcMac = formatMac(eth->srcMac);
    pkt.dstMac = formatMac(eth->destMac);
    pkt.etherType = etherType;

    if (etherType == 0x0800) {
        const uint32_t ethLen = sizeof(EthernetHeader);
        if (caplen < ethLen + sizeof(IpHeader)) {
            return pkt;
        }

        const auto *ip = reinterpret_cast<const IpHeader *>(data + ethLen);
        pkt.ipVersion = ip->versionAndHeaderLen >> 4;
        pkt.ipHeaderLen = (ip->versionAndHeaderLen & 0x0F) * 4;
        pkt.ttl = ip->ttl;
        pkt.ipProtocol = ip->protocol;
        pkt.ipTotalLen = ntohs(ip->totalLength);
        pkt.source = ipToStr(ip->srcAddr);
        pkt.destination = ipToStr(ip->destAddr);

        const uint32_t ipEnd = ethLen + pkt.ipHeaderLen;

        switch (ip->protocol) {
        case 1:
            pkt.protocol = "ICMP";
            if (caplen >= ipEnd + sizeof(IcmpHeader)) {
                const auto *icmp =
                    reinterpret_cast<const IcmpHeader *>(data + ipEnd);
                pkt.icmpType = icmp->type;
                pkt.icmpCode = icmp->code;
            }
            break;
        case 6:
            pkt.protocol = "TCP";
            if (caplen >= ipEnd + sizeof(TcpHeader)) {
                const auto *tcp =
                    reinterpret_cast<const TcpHeader *>(data + ipEnd);
                pkt.srcPort = ntohs(tcp->srcPort);
                pkt.dstPort = ntohs(tcp->dstPort);
                pkt.tcpSeq = ntohl(tcp->seqNum);
                pkt.tcpAck = ntohl(tcp->ackNum);
                pkt.tcpDataOffset = (tcp->dataOffset >> 4) * 4;
                pkt.tcpFlags = tcp->flags & 0x3F;
                pkt.tcpWindow = ntohs(tcp->windowSize);
            }
            break;
        case 17:
            pkt.protocol = "UDP";
            if (caplen >= ipEnd + sizeof(UdpHeader)) {
                const auto *udp =
                    reinterpret_cast<const UdpHeader *>(data + ipEnd);
                pkt.srcPort = ntohs(udp->srcPort);
                pkt.dstPort = ntohs(udp->dstPort);
                pkt.udpLength = ntohs(udp->length);
            }
            break;
        default:
            pkt.protocol = QString("IP(%1)").arg(ip->protocol);
            break;
        }
    } else if (etherType == 0x0806) {
        pkt.source = pkt.srcMac;
        pkt.destination = pkt.dstMac;
        pkt.protocol = "ARP";
    } else if (etherType == 0x86DD) {
        pkt.source = "IPv6";
        pkt.destination = "IPv6";
        pkt.protocol = "IPv6";
    } else {
        pkt.source = pkt.srcMac;
        pkt.destination = pkt.dstMac;
        pkt.protocol = QString("0x%1").arg(etherType, 4, 16, QChar('0'));
    }

    return pkt;
}
