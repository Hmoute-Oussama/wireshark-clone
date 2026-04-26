#include "PacketCapture.h"

PacketCapture::PacketCapture(QObject *parent) : QThread(parent) {
    qRegisterMetaType<CapturedPacket>("CapturedPacket");
}

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

    for (pcap_if_t *d = allDevs; d; d = d->next) {
        QString name = QString::fromLocal8Bit(d->name);
        QString desc = d->description
            ? QString::fromLocal8Bit(d->description)
            : QStringLiteral("No description");
        devices.append(name + " (" + desc + ")");
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
        65536, 1, 1000, errBuf);

    if (!handle) {
        emit captureError(QString("Failed to open device: %1").arg(errBuf));
        return;
    }

    pcap_pkthdr *header;
    const u_char *data;

    while (capturing) {
        int result = pcap_next_ex(handle, &header, &data);

        if (result == 1) {
            CapturedPacket packet;
            packet.number = ++packetCount;
            packet.length = header->len;
            packet.timestamp = header->ts;
            packet.rawData = QByteArray(reinterpret_cast<const char *>(data),
                                        static_cast<int>(header->caplen));

            emit packetCaptured(packet);
        } else if (result == -1) {
            emit captureError(
                QString("Capture error: %1").arg(pcap_geterr(handle)));
            break;
        }
    }

    pcap_close(handle);
    handle = nullptr;
}
