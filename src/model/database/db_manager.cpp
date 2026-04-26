#include "db_manager.hpp"

#include <mysql.h>

namespace {

QString mysqlError(MYSQL *connection) {
    if (!connection) {
        return "MySQL connection handle is null.";
    }
    return QString::fromLocal8Bit(mysql_error(connection));
}

}

DbManager::DbManager() = default;

DbManager::~DbManager() {
    if (m_connection) {
        mysql_close(m_connection);
        m_connection = nullptr;
    }
}

bool DbManager::open(const DatabaseConfig &config, QString *errorMessage) {
    if (m_connection) {
        mysql_close(m_connection);
        m_connection = nullptr;
    }

    m_connection = mysql_init(nullptr);
    if (!m_connection) {
        if (errorMessage) {
            *errorMessage = "mysql_init() failed.";
        }
        return false;
    }

    const unsigned int timeoutSeconds = 5;
    mysql_options(m_connection, MYSQL_OPT_CONNECT_TIMEOUT, &timeoutSeconds);
    mysql_options(m_connection, MYSQL_SET_CHARSET_NAME, "utf8mb4");

    if (!mysql_real_connect(
            m_connection,
            config.host.toLocal8Bit().constData(),
            config.userName.toLocal8Bit().constData(),
            config.password.toLocal8Bit().constData(),
            config.databaseName.toLocal8Bit().constData(),
            static_cast<unsigned int>(config.port),
            nullptr,
            0)) {
        if (errorMessage) {
            *errorMessage = mysqlError(m_connection);
        }
        mysql_close(m_connection);
        m_connection = nullptr;
        return false;
    }

    return initializeSchema(errorMessage);
}

MYSQL *DbManager::connection() const {
    return m_connection;
}

DatabaseConfig DbManager::defaultConfig() {
    DatabaseConfig config;

    const QString host = qEnvironmentVariable("MYSQL_HOST");
    const QString port = qEnvironmentVariable("MYSQL_PORT");
    const QString db = qEnvironmentVariable("MYSQL_DB");
    const QString user = qEnvironmentVariable("MYSQL_USER");
    const QString password = qEnvironmentVariable("MYSQL_PASSWORD");

    if (!host.isEmpty()) {
        config.host = host;
    }
    if (!port.isEmpty()) {
        config.port = port.toInt();
    }
    if (!db.isEmpty()) {
        config.databaseName = db;
    }
    if (!user.isEmpty()) {
        config.userName = user;
    }
    if (!password.isEmpty()) {
        config.password = password;
    }

    return config;
}

QStringList DbManager::availableDrivers() {
    return {"MYSQL_C_API (libmysql)"};
}

bool DbManager::initializeSchema(QString *errorMessage) {
    static const char *sql =
        "CREATE TABLE IF NOT EXISTS captured_packets ("
        "id BIGINT AUTO_INCREMENT PRIMARY KEY,"
        "packet_number INT NOT NULL,"
        "timestamp_text VARCHAR(64) NOT NULL,"
        "packet_length INT NOT NULL,"
        "raw_data LONGBLOB NOT NULL,"
        "src_mac VARCHAR(32) NOT NULL,"
        "dst_mac VARCHAR(32) NOT NULL,"
        "ether_type INT NOT NULL,"
        "ip_version INT NOT NULL,"
        "ip_header_len INT NOT NULL,"
        "ttl INT NOT NULL,"
        "ip_protocol INT NOT NULL,"
        "ip_total_len INT NOT NULL,"
        "source_addr VARCHAR(255) NOT NULL,"
        "destination_addr VARCHAR(255) NOT NULL,"
        "protocol_label VARCHAR(64) NOT NULL,"
        "src_port INT NOT NULL,"
        "dst_port INT NOT NULL,"
        "tcp_seq BIGINT NOT NULL,"
        "tcp_ack BIGINT NOT NULL,"
        "tcp_data_offset INT NOT NULL,"
        "tcp_flags INT NOT NULL,"
        "tcp_window INT NOT NULL,"
        "udp_length INT NOT NULL,"
        "icmp_type INT NOT NULL,"
        "icmp_code INT NOT NULL,"
        "created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,"
        "UNIQUE KEY uniq_packet_number (packet_number)"
        ")";

    if (mysql_query(m_connection, sql) != 0) {
        if (errorMessage) {
            *errorMessage = mysqlError(m_connection);
        }
        return false;
    }

    return true;
}
