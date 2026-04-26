#include "database_service.hpp"

DatabaseService::DatabaseService()
    : m_config(DbManager::defaultConfig()),
      m_packetRepository(m_dbManager) {
}

void DatabaseService::setDatabaseConfig(const DatabaseConfig &config) {
    m_config = config;
}

DatabaseConfig DatabaseService::databaseConfig() const {
    return m_config;
}

bool DatabaseService::testConnection(QString *errorMessage) {
    return m_dbManager.open(m_config, errorMessage);
}

bool DatabaseService::savePacket(const PacketData &packet, QString *errorMessage) {
    if (!m_dbManager.open(m_config, errorMessage)) {
        return false;
    }

    return m_packetRepository.save(toEntity(packet), errorMessage);
}

PacketEntity DatabaseService::toEntity(const PacketData &packet) {
    PacketEntity entity;
    entity.packetNumber = packet.number;
    entity.timestamp = packet.timestamp;
    entity.length = packet.length;
    entity.rawData = packet.rawData;
    entity.srcMac = packet.srcMac;
    entity.dstMac = packet.dstMac;
    entity.etherType = packet.etherType;
    entity.ipVersion = packet.ipVersion;
    entity.ipHeaderLen = packet.ipHeaderLen;
    entity.ttl = packet.ttl;
    entity.ipProtocol = packet.ipProtocol;
    entity.ipTotalLen = packet.ipTotalLen;
    entity.source = packet.source;
    entity.destination = packet.destination;
    entity.protocol = packet.protocol;
    entity.srcPort = packet.srcPort;
    entity.dstPort = packet.dstPort;
    entity.tcpSeq = packet.tcpSeq;
    entity.tcpAck = packet.tcpAck;
    entity.tcpDataOffset = packet.tcpDataOffset;
    entity.tcpFlags = packet.tcpFlags;
    entity.tcpWindow = packet.tcpWindow;
    entity.udpLength = packet.udpLength;
    entity.icmpType = packet.icmpType;
    entity.icmpCode = packet.icmpCode;
    return entity;
}
