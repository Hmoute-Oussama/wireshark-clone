#pragma once

#include <QObject>
#include <QRecursiveMutex>
#include <QString>
#include <QVector>

#include "../PacketData.h"

struct sqlite3;

class DatabaseService : public QObject {
    Q_OBJECT

public:
    explicit DatabaseService(QObject *parent = nullptr);
    ~DatabaseService() override;

    bool open(const QString &databasePath, QString *errorMessage = nullptr);
    void close();
    bool isOpen() const;

    bool initializeSchema(QString *errorMessage = nullptr);
    bool storePacket(const PacketData &packet, const QString &deviceName = QString(), QString *errorMessage = nullptr);
    bool storePackets(const QVector<PacketData> &packets, const QString &deviceName = QString(), QString *errorMessage = nullptr);
    QVector<PacketData> loadPackets(int limit = 1000, int offset = 0, const QString &protocolFilter = QString(),
                                    QString *errorMessage = nullptr) const;
    int packetCount(QString *errorMessage = nullptr) const;
    bool clearPackets(QString *errorMessage = nullptr);

private:
    bool execute(const QString &sql, QString *errorMessage) const;

    mutable QRecursiveMutex mutex_;
    sqlite3 *database_ = nullptr;
    QString databasePath_;
};
