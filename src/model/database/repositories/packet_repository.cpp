#include "packet_repository.hpp"

#include <mysql.h>

namespace {

QString mysqlError(MYSQL *connection) {
    if (!connection) {
        return "MySQL connection handle is null.";
    }
    return QString::fromLocal8Bit(mysql_error(connection));
}

QString escapeString(MYSQL *connection, const QString &value) {
    const QByteArray utf8 = value.toUtf8();
    QByteArray escaped(utf8.size() * 2 + 1, '\0');
    const auto length = mysql_real_escape_string(
        connection,
        escaped.data(),
        utf8.constData(),
        static_cast<unsigned long>(utf8.size()));
    escaped.resize(static_cast<int>(length));
    return QString::fromUtf8(escaped);
}

QByteArray escapeBytes(MYSQL *connection, const QByteArray &value) {
    QByteArray escaped(value.size() * 2 + 1, '\0');
    const auto length = mysql_real_escape_string(
        connection,
        escaped.data(),
        value.constData(),
        static_cast<unsigned long>(value.size()));
    escaped.resize(static_cast<int>(length));
    return escaped;
}

}

PacketRepository::PacketRepository(DbManager &dbManager)
    : m_dbManager(dbManager) {
}

bool PacketRepository::save(const PacketEntity &packet, QString *errorMessage) {
    MYSQL *connection = m_dbManager.connection();
    if (!connection) {
        if (errorMessage) {
            *errorMessage = "MySQL connection is not open.";
        }
        return false;
    }

    const QString timestamp = escapeString(connection, packet.timestamp);
    const QString srcMac = escapeString(connection, packet.srcMac);
    const QString dstMac = escapeString(connection, packet.dstMac);
    const QString source = escapeString(connection, packet.source);
    const QString destination = escapeString(connection, packet.destination);
    const QString protocol = escapeString(connection, packet.protocol);
    const QByteArray rawData = escapeBytes(connection, packet.rawData);

    const QString query = QString(
        "INSERT INTO captured_packets ("
        "packet_number, timestamp_text, packet_length, raw_data, "
        "src_mac, dst_mac, ether_type, ip_version, ip_header_len, ttl, "
        "ip_protocol, ip_total_len, source_addr, destination_addr, "
        "protocol_label, src_port, dst_port, tcp_seq, tcp_ack, "
        "tcp_data_offset, tcp_flags, tcp_window, udp_length, "
        "icmp_type, icmp_code"
        ") VALUES ("
        "%1, '%2', %3, '%4', "
        "'%5', '%6', %7, %8, %9, %10, "
        "%11, %12, '%13', '%14', "
        "'%15', %16, %17, %18, %19, "
        "%20, %21, %22, %23, "
        "%24, %25"
        ") ON DUPLICATE KEY UPDATE "
        "timestamp_text = VALUES(timestamp_text), "
        "packet_length = VALUES(packet_length), "
        "raw_data = VALUES(raw_data), "
        "protocol_label = VALUES(protocol_label)")
        .arg(packet.packetNumber)
        .arg(timestamp)
        .arg(packet.length)
        .arg(QString::fromLatin1(rawData))
        .arg(srcMac)
        .arg(dstMac)
        .arg(packet.etherType)
        .arg(packet.ipVersion)
        .arg(packet.ipHeaderLen)
        .arg(packet.ttl)
        .arg(packet.ipProtocol)
        .arg(packet.ipTotalLen)
        .arg(source)
        .arg(destination)
        .arg(protocol)
        .arg(packet.srcPort)
        .arg(packet.dstPort)
        .arg(packet.tcpSeq)
        .arg(packet.tcpAck)
        .arg(packet.tcpDataOffset)
        .arg(packet.tcpFlags)
        .arg(packet.tcpWindow)
        .arg(packet.udpLength)
        .arg(packet.icmpType)
        .arg(packet.icmpCode);

    if (mysql_query(connection, query.toUtf8().constData()) != 0) {
        if (errorMessage) {
            *errorMessage = mysqlError(connection);
        }
        return false;
    }

    return true;
}
