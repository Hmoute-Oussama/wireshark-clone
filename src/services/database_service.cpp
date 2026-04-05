#include "database_service.hpp"

#include <QDir>
#include <QFileInfo>
#include <QMutexLocker>

#include "../../third_party/sqlite/include/sqlite3.h"

namespace {
bool setError(sqlite3 *database, QString *errorMessage, const QString &fallbackMessage) {
    if (errorMessage) {
        if (database != nullptr) {
            *errorMessage = QString::fromUtf8(sqlite3_errmsg(database));
        } else {
            *errorMessage = fallbackMessage;
        }
    }
    return false;
}
}  // namespace

DatabaseService::DatabaseService(QObject *parent) : QObject(parent) {}

DatabaseService::~DatabaseService() {
    close();
}

bool DatabaseService::open(const QString &databasePath, QString *errorMessage) {
    QMutexLocker locker(&mutex_);

    if (database_ != nullptr && databasePath_ == databasePath) {
        return true;
    }

    close();

    QFileInfo fileInfo(databasePath);
    if (!fileInfo.dir().exists()) {
        fileInfo.dir().mkpath(".");
    }

    sqlite3 *db = nullptr;
    const int result = sqlite3_open(databasePath.toUtf8().constData(), &db);
    if (result != SQLITE_OK) {
        const QString message = db != nullptr
            ? QString::fromUtf8(sqlite3_errmsg(db))
            : QString("Failed to open database '%1'.").arg(databasePath);
        if (db != nullptr) {
            sqlite3_close(db);
        }
        if (errorMessage) {
            *errorMessage = message;
        }
        return false;
    }

    database_ = db;
    databasePath_ = databasePath;
    return initializeSchema(errorMessage);
}

void DatabaseService::close() {
    QMutexLocker locker(&mutex_);

    if (database_ != nullptr) {
        sqlite3_close(database_);
        database_ = nullptr;
    }

    databasePath_.clear();
}

bool DatabaseService::isOpen() const {
    QMutexLocker locker(&mutex_);
    return database_ != nullptr;
}

bool DatabaseService::initializeSchema(QString *errorMessage) {
    return execute(
        "CREATE TABLE IF NOT EXISTS packets ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "capture_number INTEGER NOT NULL,"
        "timestamp TEXT NOT NULL,"
        "source TEXT NOT NULL,"
        "destination TEXT NOT NULL,"
        "protocol TEXT NOT NULL,"
        "length INTEGER NOT NULL,"
        "raw_data BLOB NOT NULL,"
        "device_name TEXT"
        ");"
        "CREATE INDEX IF NOT EXISTS idx_packets_protocol ON packets(protocol);"
        "CREATE INDEX IF NOT EXISTS idx_packets_timestamp ON packets(timestamp);",
        errorMessage);
}

bool DatabaseService::storePacket(const PacketData &packet, const QString &deviceName, QString *errorMessage) {
    return storePackets({packet}, deviceName, errorMessage);
}

bool DatabaseService::storePackets(const QVector<PacketData> &packets, const QString &deviceName, QString *errorMessage) {
    QMutexLocker locker(&mutex_);

    if (database_ == nullptr) {
        return setError(database_, errorMessage, "Database is not open.");
    }

    if (packets.isEmpty()) {
        if (errorMessage) {
            errorMessage->clear();
        }
        return true;
    }

    sqlite3_stmt *statement = nullptr;
    const char *sql =
        "INSERT INTO packets (capture_number, timestamp, source, destination, protocol, length, raw_data, device_name) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?);";

    if (sqlite3_prepare_v2(database_, sql, -1, &statement, nullptr) != SQLITE_OK) {
        return setError(database_, errorMessage, "Failed to prepare insert statement.");
    }

    const bool transactionStarted = sqlite3_exec(database_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) == SQLITE_OK;

    for (const PacketData &packet : packets) {
        sqlite3_bind_int64(statement, 1, static_cast<sqlite3_int64>(packet.number));
        sqlite3_bind_text(statement, 2, packet.timestamp.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 3, packet.source.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 4, packet.destination.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 5, packet.protocol.toUtf8().constData(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(statement, 6, static_cast<sqlite3_int64>(packet.length));
        sqlite3_bind_blob(statement, 7, packet.rawData.constData(), packet.rawData.size(), SQLITE_TRANSIENT);
        sqlite3_bind_text(statement, 8, deviceName.toUtf8().constData(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(statement) != SQLITE_DONE) {
            sqlite3_finalize(statement);
            if (transactionStarted) {
                sqlite3_exec(database_, "ROLLBACK;", nullptr, nullptr, nullptr);
            }
            return setError(database_, errorMessage, "Failed to store packet.");
        }

        sqlite3_reset(statement);
        sqlite3_clear_bindings(statement);
    }

    sqlite3_finalize(statement);

    if (transactionStarted) {
        sqlite3_exec(database_, "COMMIT;", nullptr, nullptr, nullptr);
    }

    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}

QVector<PacketData> DatabaseService::loadPackets(int limit, int offset, const QString &protocolFilter, QString *errorMessage) const {
    QMutexLocker locker(&mutex_);
    QVector<PacketData> packets;

    if (database_ == nullptr) {
        setError(database_, errorMessage, "Database is not open.");
        return packets;
    }

    QString sql =
        "SELECT capture_number, timestamp, source, destination, protocol, length, raw_data "
        "FROM packets ";
    if (!protocolFilter.trimmed().isEmpty()) {
        sql += "WHERE lower(protocol) = lower(?) ";
    }
    sql += "ORDER BY id DESC LIMIT ? OFFSET ?;";

    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(database_, sql.toUtf8().constData(), -1, &statement, nullptr) != SQLITE_OK) {
        setError(database_, errorMessage, "Failed to prepare select statement.");
        return {};
    }

    int bindIndex = 1;
    if (!protocolFilter.trimmed().isEmpty()) {
        sqlite3_bind_text(statement, bindIndex++, protocolFilter.toUtf8().constData(), -1, SQLITE_TRANSIENT);
    }
    sqlite3_bind_int(statement, bindIndex++, qMax(1, limit));
    sqlite3_bind_int(statement, bindIndex, qMax(0, offset));

    while (sqlite3_step(statement) == SQLITE_ROW) {
        PacketData packet;
        packet.number = static_cast<uint32_t>(sqlite3_column_int64(statement, 0));
        packet.timestamp = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(statement, 1)));
        packet.source = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(statement, 2)));
        packet.destination = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(statement, 3)));
        packet.protocol = QString::fromUtf8(reinterpret_cast<const char *>(sqlite3_column_text(statement, 4)));
        packet.length = static_cast<uint32_t>(sqlite3_column_int64(statement, 5));

        const void *blob = sqlite3_column_blob(statement, 6);
        const int blobSize = sqlite3_column_bytes(statement, 6);
        packet.rawData = QByteArray(static_cast<const char *>(blob), blobSize);

        packets.push_back(packet);
    }

    sqlite3_finalize(statement);

    if (errorMessage) {
        errorMessage->clear();
    }
    return packets;
}

int DatabaseService::packetCount(QString *errorMessage) const {
    QMutexLocker locker(&mutex_);

    if (database_ == nullptr) {
        setError(database_, errorMessage, "Database is not open.");
        return 0;
    }

    sqlite3_stmt *statement = nullptr;
    if (sqlite3_prepare_v2(database_, "SELECT COUNT(*) FROM packets;", -1, &statement, nullptr) != SQLITE_OK) {
        setError(database_, errorMessage, "Failed to count packets.");
        return 0;
    }

    int count = 0;
    if (sqlite3_step(statement) == SQLITE_ROW) {
        count = sqlite3_column_int(statement, 0);
    }
    sqlite3_finalize(statement);

    if (errorMessage) {
        errorMessage->clear();
    }
    return count;
}

bool DatabaseService::clearPackets(QString *errorMessage) {
    return execute("DELETE FROM packets;", errorMessage);
}

bool DatabaseService::execute(const QString &sql, QString *errorMessage) const {
    QMutexLocker locker(&mutex_);

    if (database_ == nullptr) {
        return setError(database_, errorMessage, "Database is not open.");
    }

    char *sqliteError = nullptr;
    const int result = sqlite3_exec(database_, sql.toUtf8().constData(), nullptr, nullptr, &sqliteError);
    if (result != SQLITE_OK) {
        if (errorMessage) {
            *errorMessage = sqliteError != nullptr
                ? QString::fromUtf8(sqliteError)
                : QString("SQLite error code %1.").arg(result);
        }
        if (sqliteError != nullptr) {
            sqlite3_free(sqliteError);
        }
        return false;
    }

    if (errorMessage) {
        errorMessage->clear();
    }
    return true;
}
