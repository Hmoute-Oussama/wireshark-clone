#pragma once

#include <QObject>
#include <QStringList>
#include "../PacketCapture.h"
#include "../PacketData.h"
#include "../services/database_service.hpp"

class PacketController : public QObject {
    Q_OBJECT

public:
    explicit PacketController(QObject *parent = nullptr);

    QStringList getDeviceList() const;
    void startCapture(const QString &deviceName);
    void stopCapture();
    void setDatabaseConfig(const DatabaseConfig &config);
    DatabaseConfig databaseConfig() const;
    QStringList availableDatabaseDrivers() const;
    bool testDatabaseConnection(QString *errorMessage = nullptr);
    bool savePacket(const PacketData &packet, QString *errorMessage = nullptr);

signals:
    void packetCaptured(PacketData packet);
    void captureError(const QString &errorMsg);

private slots:
    void processCapturedPacket(CapturedPacket packet);

private:
    PacketData buildPacketData(const CapturedPacket &packet) const;

    PacketCapture m_captureEngine;
    DatabaseService m_databaseService;
};
