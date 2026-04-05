#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVector>

#include "../PacketCapture.h"
#include "../PacketData.h"

class CaptureService : public QObject {
    Q_OBJECT

public:
    explicit CaptureService(QObject *parent = nullptr);
    ~CaptureService() override;

    QStringList availableDevices() const;
    bool start(const QString &deviceName);
    void stop();
    void clearHistory();
    void setHistoryLimit(qsizetype limit);

    bool isCapturing() const;
    QString currentDevice() const;
    quint64 capturedPacketCount() const;
    const QVector<PacketData> &packets() const;

signals:
    void captureStarted(const QString &deviceName);
    void captureStopped(quint64 packetCount);
    void packetCaptured(const PacketData &packet);
    void errorOccurred(const QString &message);

private slots:
    void onPacketCaptured(PacketData packet);
    void onCaptureError(const QString &message);

private:
    PacketCapture captureEngine_;
    QVector<PacketData> packets_;
    QString currentDevice_;
    bool capturing_ = false;
    quint64 capturedPacketCount_ = 0;
    qsizetype historyLimit_ = 10000;
};
