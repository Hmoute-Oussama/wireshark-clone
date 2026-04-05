#pragma once

#include <QVector>
#include <QString>

#include "../PacketData.h"

class FilterService {
public:
    bool matches(const PacketData &packet, const QString &expression, QString *errorMessage = nullptr) const;
    QVector<PacketData> apply(const QVector<PacketData> &packets, const QString &expression, QString *errorMessage = nullptr) const;
    bool isValid(const QString &expression, QString *errorMessage = nullptr) const;
};
