#pragma once

#include <QString>
#include <QStringList>
#include "../PacketData.h"

class FilterEngine {
public:
    enum class FilterResult {
        Valid,
        Invalid,
        Empty
    };

    struct FilterRule {
        QString field;
        QString op;
        QString value;
        bool isProtocolOnly = false;
    };

    static FilterResult validate(const QString &filterText);
    static bool matches(const PacketData &packet, const QString &filterText);

private:
    static QList<FilterRule> parse(const QString &filterText);
    static bool evaluateRule(const PacketData &packet, const FilterRule &rule);
};
