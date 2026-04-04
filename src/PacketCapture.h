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
    void stopCapture();

signals:
    void packetCaptured(PacketData packet);
    void captureError(const QString &errorMsg);

protected:
    void run() override;

private:
    pcap_t *handle = nullptr;
    QString selectedDevice;
    bool capturing = false;
    uint32_t packetCount = 0;
};
