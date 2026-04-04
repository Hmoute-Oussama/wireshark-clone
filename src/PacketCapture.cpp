#include "PacketCapture.h"
#include <QDateTime>
#include <winsock2.h>
#include <ws2tcpip.h>

struct EthernetHeader {
    uint8_t destMac[6];
    uint8_t srcMac[6];
    uint16_t etherType;
};

struct IpHeader {
    uint8_t  versionAndHeaderLen;
    uint8_t  tos;
    uint16_t totalLength;
    uint16_t id;
    uint16_t flagsAndOffset;
    uint8_t  ttl;
    uint8_t  protocol;
    uint16_t checksum;
    uint32_t srcAddr;
    uint32_t destAddr;
};

PacketCapture::PacketCapture(QObject *parent) : QThread(parent) {}

PacketCapture::~PacketCapture() {
    stopCapture();
    wait();
}

QStringList PacketCapture::getDeviceList() const {
    QStringList devices;
    pcap_if_t *allDevs;
    char errBuf[PCAP_ERRBUF_SIZE];

    if (pcap_findalldevs(&allDevs, errBuf) == -1) {
        return devices;
    }

    for (pcap_if_t *d = allDevs; d != nullptr; d = d->next) {
        QString name = QString::fromLocal8Bit(d->name);
        QString description = d->description
            ? QString::fromLocal8Bit(d->description)
            : "No description";
        devices.append(name + " (" + description + ")");
    }

    pcap_freealldevs(allDevs);
    return devices;
}

void PacketCapture::startCapture(const QString &deviceName) {
    selectedDevice = deviceName;
    packetCount = 0;
    capturing = true;
    start();
}

void PacketCapture::stopCapture() {
    capturing = false;
    if (handle) {
        pcap_breakloop(handle);
    }
}

void PacketCapture::run() {
    char errBuf[PCAP_ERRBUF_SIZE];

    handle = pcap_open_live(
        selectedDevice.toLocal8Bit().constData(),
        65536,   // snapshot length (capture full packets)
        1,       // promiscuous mode
        1000,    // read timeout in ms
        errBuf
    );

    if (!handle) {
        emit captureError(QString("Failed to open device: %1").arg(errBuf));
        return;
    }

    struct pcap_pkthdr *header;
    const u_char *data;

    while (capturing) {
        int result = pcap_next_ex(handle, &header, &data);

        if (result == 1) {
            PacketData pkt;
            pkt.number = ++packetCount;
            pkt.length = header->len;
            pkt.rawData = QByteArray(reinterpret_cast<const char*>(data), header->caplen);

            // Timestamp
            QDateTime dt = QDateTime::fromSecsSinceEpoch(header->ts.tv_sec);
            pkt.timestamp = dt.toString("hh:mm:ss.") +
                            QString::number(header->ts.tv_usec / 1000).rightJustified(3, '0');

            // Parse Ethernet header
            if (header->caplen >= sizeof(EthernetHeader)) {
                const EthernetHeader *eth = reinterpret_cast<const EthernetHeader*>(data);
                uint16_t etherType = ntohs(eth->etherType);

                if (etherType == 0x0800 &&
                    header->caplen >= sizeof(EthernetHeader) + sizeof(IpHeader)) {
                    // IPv4
                    const IpHeader *ip = reinterpret_cast<const IpHeader*>(
                        data + sizeof(EthernetHeader));

                    struct in_addr srcAddr, destAddr;
                    srcAddr.s_addr = ip->srcAddr;
                    destAddr.s_addr = ip->destAddr;

                    char srcStr[INET_ADDRSTRLEN], destStr[INET_ADDRSTRLEN];
                    inet_ntop(AF_INET, &srcAddr, srcStr, INET_ADDRSTRLEN);
                    inet_ntop(AF_INET, &destAddr, destStr, INET_ADDRSTRLEN);

                    pkt.source = QString(srcStr);
                    pkt.destination = QString(destStr);

                    switch (ip->protocol) {
                        case 6:  pkt.protocol = "TCP"; break;
                        case 17: pkt.protocol = "UDP"; break;
                        case 1:  pkt.protocol = "ICMP"; break;
                        default: pkt.protocol = QString("IP(%1)").arg(ip->protocol); break;
                    }
                } else if (etherType == 0x0806) {
                    pkt.source = "ARP";
                    pkt.destination = "ARP";
                    pkt.protocol = "ARP";
                } else if (etherType == 0x86DD) {
                    pkt.source = "IPv6";
                    pkt.destination = "IPv6";
                    pkt.protocol = "IPv6";
                } else {
                    pkt.source = "Unknown";
                    pkt.destination = "Unknown";
                    pkt.protocol = QString("0x%1").arg(etherType, 4, 16, QChar('0'));
                }
            }

            emit packetCaptured(pkt);
        } else if (result == -1) {
            emit captureError(QString("Capture error: %1").arg(pcap_geterr(handle)));
            break;
        }
        // result == 0 means timeout, just loop again
    }

    pcap_close(handle);
    handle = nullptr;
}
