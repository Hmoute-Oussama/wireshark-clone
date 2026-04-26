#pragma once

#include "../model/database/db_manager.hpp"
#include "../model/database/repositories/packet_repository.hpp"
#include "../PacketData.h"
#include <QString>

class DatabaseService {
public:
    DatabaseService();

    void setDatabaseConfig(const DatabaseConfig &config);
    DatabaseConfig databaseConfig() const;
    bool testConnection(QString *errorMessage = nullptr);
    bool savePacket(const PacketData &packet, QString *errorMessage = nullptr);

private:
    static PacketEntity toEntity(const PacketData &packet);

    DatabaseConfig m_config;
    DbManager m_dbManager;
    PacketRepository m_packetRepository;
};
