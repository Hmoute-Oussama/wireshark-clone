#pragma once

#include <QThread>
#include <QByteArray>
#include <QStringList>
#include <pcap.h>

struct CapturedPacket {
    uint32_t number = 0;
    uint32_t length = 0;
    timeval timestamp{};
    QByteArray rawData;
};

Q_DECLARE_METATYPE(CapturedPacket)

class PacketCapture : public QThread {
    Q_OBJECT

public:
    explicit PacketCapture(QObject *parent = nullptr);
    ~PacketCapture() override;

    QStringList getDeviceList() const;
    void startCapture(const QString &deviceName);
    void stopCapture();

signals:
    void packetCaptured(CapturedPacket packet);
    void captureError(const QString &errorMsg);

protected:
    void run() override;

private:
    pcap_t *handle = nullptr;
    QString selectedDevice;
    bool capturing = false;
    uint32_t packetCount = 0;
};
