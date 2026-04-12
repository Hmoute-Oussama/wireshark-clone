#include "FilterEngine.h"
#include <QRegularExpression>

FilterEngine::FilterResult FilterEngine::validate(const QString &filterText) {
    if (filterText.trimmed().isEmpty()) return FilterResult::Empty;
    
    auto rules = parse(filterText);
    if (rules.isEmpty()) return FilterResult::Invalid;
    
    return FilterResult::Valid;
}

bool FilterEngine::matches(const PacketData &packet, const QString &filterText) {
    QString trimmed = filterText.trimmed().toLower();
    if (trimmed.isEmpty()) return true;

    auto rules = parse(trimmed);
    if (rules.isEmpty()) return false;

    for (const auto &rule : rules) {
        if (!evaluateRule(packet, rule)) return false;
    }
    
    return true;
}

QList<FilterEngine::FilterRule> FilterEngine::parse(const QString &filterText) {
    QList<FilterRule> rules;
    QString text = filterText.trimmed().toLower();
    if (text.isEmpty()) return rules;

    QStringList protocols = {"tcp", "udp", "icmp", "arp", "ip", "ipv6"};
    if (protocols.contains(text)) {
        FilterRule r;
        r.field = text;
        r.isProtocolOnly = true;
        rules.append(r);
        return rules;
    }

    QRegularExpression re(R"(^([\w\.]+)\s*(==|!=)\s*(.*)$)");
    QRegularExpressionMatch match = re.match(text);

    if (match.hasMatch()) {
        FilterRule r;
        r.field = match.captured(1);
        r.op    = match.captured(2);
        r.value = match.captured(3).trimmed();
        
        QStringList validFields = {
            "ip.src", "ip.dst", "ip.addr", 
            "tcp.port", "udp.port", "port",
            "eth.src", "eth.dst", "eth.addr",
            "frame.len"
        };
        
        if (validFields.contains(r.field)) {
            rules.append(r);
        }
    }

    return rules;
}

bool FilterEngine::evaluateRule(const PacketData &packet, const FilterRule &rule) {
    if (rule.isProtocolOnly) {
        if (rule.field == "ip")   return packet.ipVersion == 4;
        if (rule.field == "ipv6") return packet.ipVersion == 6 || packet.protocol == "IPv6";
        return packet.protocol.toLower() == rule.field;
    }

    QString fieldValue;
    bool isNumeric = false;
    uint32_t packetNumValue = 0;

    if (rule.field == "ip.src") {
        fieldValue = packet.source;
    } else if (rule.field == "ip.dst") {
        fieldValue = packet.destination;
    } else if (rule.field == "ip.addr") {
        bool match = (packet.source == rule.value || packet.destination == rule.value);
        return (rule.op == "==") ? match : !match;
    } else if (rule.field == "tcp.port") {
        if (packet.protocol != "TCP") return false;
        packetNumValue = packet.srcPort;
        isNumeric = true;
    } else if (rule.field == "udp.port") {
        if (packet.protocol != "UDP") return false;
        packetNumValue = packet.srcPort;
        isNumeric = true;
    } else if (rule.field == "port") {
        bool match = (QString::number(packet.srcPort) == rule.value || 
                      QString::number(packet.dstPort) == rule.value);
        return (rule.op == "==") ? match : !match;
    } else if (rule.field == "eth.src") {
        fieldValue = packet.srcMac;
    } else if (rule.field == "eth.dst") {
        fieldValue = packet.dstMac;
    } else if (rule.field == "frame.len") {
        packetNumValue = packet.length;
        isNumeric = true;
    }

    if (isNumeric) {
        uint32_t filterValue = rule.value.toUInt();
        bool match = false;
        if (rule.field == "tcp.port" || rule.field == "udp.port" || rule.field == "port") {
            match = (packet.srcPort == filterValue || packet.dstPort == filterValue);
        } else {
            match = (packetNumValue == filterValue);
        }
        return (rule.op == "==") ? match : !match;
    } else {
        bool match = (fieldValue.toLower() == rule.value.toLower());
        return (rule.op == "==") ? match : !match;
    }

    return false;
}
