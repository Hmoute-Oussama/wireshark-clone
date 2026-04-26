#pragma once

#include <QString>
#include <QStringList>

typedef struct st_mysql MYSQL;

struct DatabaseConfig {
    QString host = "127.0.0.1";
    int port = 3306;
    QString databaseName = "wireshark_clone";
    QString userName = "root";
    QString password;
};

class DbManager {
public:
    DbManager();
    ~DbManager();

    bool open(const DatabaseConfig &config, QString *errorMessage = nullptr);
    MYSQL *connection() const;
    static DatabaseConfig defaultConfig();
    static QStringList availableDrivers();

private:
    bool initializeSchema(QString *errorMessage);

    MYSQL *m_connection = nullptr;
};
