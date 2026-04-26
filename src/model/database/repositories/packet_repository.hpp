#pragma once

#include <QString>
#include "../db_manager.hpp"
#include "../models/packet_entity.hpp"

class PacketRepository {
public:
    explicit PacketRepository(DbManager &dbManager);

    bool save(const PacketEntity &packet, QString *errorMessage = nullptr);

private:
    DbManager &m_dbManager;
};
