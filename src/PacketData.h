#pragma once

#include <QString>
#include <QByteArray>
#include <cstdint>

struct PacketData {
    uint32_t number;
    QString timestamp;
    QString source;
    QString destination;
    QString protocol;
    uint32_t length;
    QByteArray rawData;
};
