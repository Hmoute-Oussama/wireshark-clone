#pragma once

#include <QThread>
#include <QStringList>
#include <pcap.h>
#include "PacketData.h"

class PacketCapture : public QThread {
    Q_OBJECT

public:
    explicit PacketCapture(QObject *parent = nullptr);
    ~PacketCapture() override;

    QStringList getDeviceList() const;
    void startCapture(const QString &deviceName);
    void openPcap(const QString &fileName);
    void stopCapture();

    static bool savePackets(const QString &fileName, const QVector<PacketData> &packets);

signals:
    void packetCaptured(PacketData packet);
    void captureError(const QString &errorMsg);

protected:
    void run() override;

private:
    pcap_t *handle = nullptr;
    QString selectedDevice;
    QString pcapFileName;
    bool capturing = false;
    bool isOffline = false;
    uint32_t packetCount = 0;
};
