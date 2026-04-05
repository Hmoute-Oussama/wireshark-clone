#include "filter_service.hpp"

#include <QByteArray>
#include <QStringList>

namespace {
enum class TokenType {
    Condition,
    And,
    Or,
    Not,
    LeftParen,
    RightParen
};

struct Token {
    TokenType type;
    QString text;
};

QVector<Token> tokenize(const QString &expression, QString *errorMessage) {
    QVector<Token> tokens;
    QString current;
    bool inQuotes = false;

    auto flushCondition = [&]() {
        if (!current.trimmed().isEmpty()) {
            tokens.push_back({TokenType::Condition, current.trimmed()});
        }
        current.clear();
    };

    for (int i = 0; i < expression.size(); ++i) {
        const QChar ch = expression[i];

        if (ch == '"') {
            inQuotes = !inQuotes;
            current += ch;
            continue;
        }

        if (inQuotes) {
            current += ch;
            continue;
        }

        if (ch.isSpace()) {
            flushCondition();
            continue;
        }

        if (ch == '(') {
            flushCondition();
            tokens.push_back({TokenType::LeftParen, "("});
        } else if (ch == ')') {
            flushCondition();
            tokens.push_back({TokenType::RightParen, ")"});
        } else if (ch == '&' && i + 1 < expression.size() && expression[i + 1] == '&') {
            flushCondition();
            tokens.push_back({TokenType::And, "&&"});
            ++i;
        } else if (ch == '|' && i + 1 < expression.size() && expression[i + 1] == '|') {
            flushCondition();
            tokens.push_back({TokenType::Or, "||"});
            ++i;
        } else if (ch == '!' && (i + 1 >= expression.size() || expression[i + 1] != '=') && current.trimmed().isEmpty()) {
            flushCondition();
            tokens.push_back({TokenType::Not, "!"});
        } else {
            current += ch;
            if (ch == '!' && i + 1 < expression.size() && expression[i + 1] == '=') {
                current += '=';
                ++i;
            } else if (ch == '&' || ch == '|') {
                if (errorMessage) {
                    *errorMessage = QString("Unsupported token near '%1'.").arg(ch);
                }
                return {};
            }
        }
    }

    if (inQuotes) {
        if (errorMessage) {
            *errorMessage = "Unterminated quoted string in filter expression.";
        }
        return {};
    }

    flushCondition();
    return tokens;
}

int precedence(TokenType type) {
    switch (type) {
        case TokenType::Not:
            return 3;
        case TokenType::And:
            return 2;
        case TokenType::Or:
            return 1;
        default:
            return 0;
    }
}

QVector<Token> toPostfix(const QVector<Token> &tokens, QString *errorMessage) {
    QVector<Token> output;
    QVector<Token> operators;

    for (const Token &token : tokens) {
        if (token.type == TokenType::Condition) {
            output.push_back(token);
            continue;
        }

        if (token.type == TokenType::LeftParen) {
            operators.push_back(token);
            continue;
        }

        if (token.type == TokenType::RightParen) {
            bool matched = false;
            while (!operators.isEmpty()) {
                Token op = operators.takeLast();
                if (op.type == TokenType::LeftParen) {
                    matched = true;
                    break;
                }
                output.push_back(op);
            }
            if (!matched) {
                if (errorMessage) {
                    *errorMessage = "Mismatched parentheses in filter expression.";
                }
                return {};
            }
            continue;
        }

        while (!operators.isEmpty() &&
               operators.last().type != TokenType::LeftParen &&
               precedence(operators.last().type) >= precedence(token.type)) {
            output.push_back(operators.takeLast());
        }
        operators.push_back(token);
    }

    while (!operators.isEmpty()) {
        if (operators.last().type == TokenType::LeftParen) {
            if (errorMessage) {
                *errorMessage = "Mismatched parentheses in filter expression.";
            }
            return {};
        }
        output.push_back(operators.takeLast());
    }

    return output;
}

QString stripQuotes(QString value) {
    value = value.trimmed();
    if (value.size() >= 2 && value.startsWith('"') && value.endsWith('"')) {
        value = value.mid(1, value.size() - 2);
    }
    return value;
}

bool compareNumber(qint64 lhs, qint64 rhs, const QString &op) {
    if (op == "=" || op == ":") {
        return lhs == rhs;
    }
    if (op == "!=") {
        return lhs != rhs;
    }
    if (op == ">") {
        return lhs > rhs;
    }
    if (op == "<") {
        return lhs < rhs;
    }
    if (op == ">=") {
        return lhs >= rhs;
    }
    if (op == "<=") {
        return lhs <= rhs;
    }
    return false;
}

bool compareText(const QString &lhs, const QString &rhs, const QString &op) {
    const Qt::CaseSensitivity cs = Qt::CaseInsensitive;

    if (op == ":" || op == "=") {
        return lhs.compare(rhs, cs) == 0 || lhs.contains(rhs, cs);
    }
    if (op == "!=") {
        return lhs.compare(rhs, cs) != 0 && !lhs.contains(rhs, cs);
    }
    return false;
}

bool evaluateCondition(const PacketData &packet, const QString &condition, QString *errorMessage) {
    static const QStringList operators = {"!=", ">=", "<=", ":", "=", ">", "<"};

    QString key;
    QString value;
    QString op;

    for (const QString &candidate : operators) {
        const int index = condition.indexOf(candidate);
        if (index != -1) {
            key = condition.left(index).trimmed();
            value = stripQuotes(condition.mid(index + candidate.size()));
            op = candidate;
            break;
        }
    }

    if (op.isEmpty()) {
        const QString needle = stripQuotes(condition);
        const QString haystack = QStringLiteral("%1 %2 %3 %4 %5")
            .arg(packet.timestamp, packet.source, packet.destination, packet.protocol,
                 QString::fromLatin1(packet.rawData.toHex(' ')));
        return haystack.contains(needle, Qt::CaseInsensitive);
    }

    if (key.isEmpty() || value.isEmpty()) {
        if (errorMessage) {
            *errorMessage = QString("Invalid condition '%1'.").arg(condition);
        }
        return false;
    }

    const QString normalizedKey = key.toLower();
    bool ok = false;
    const qint64 numericValue = value.toLongLong(&ok);

    if (normalizedKey == "number" || normalizedKey == "no") {
        if (!ok) {
            if (errorMessage) {
                *errorMessage = QString("Expected numeric value for '%1'.").arg(key);
            }
            return false;
        }
        return compareNumber(packet.number, numericValue, op);
    }

    if (normalizedKey == "length" || normalizedKey == "len") {
        if (!ok) {
            if (errorMessage) {
                *errorMessage = QString("Expected numeric value for '%1'.").arg(key);
            }
            return false;
        }
        return compareNumber(packet.length, numericValue, op);
    }

    if (normalizedKey == "time" || normalizedKey == "timestamp") {
        return compareText(packet.timestamp, value, op);
    }

    if (normalizedKey == "src" || normalizedKey == "source") {
        return compareText(packet.source, value, op);
    }

    if (normalizedKey == "dst" || normalizedKey == "destination") {
        return compareText(packet.destination, value, op);
    }

    if (normalizedKey == "proto" || normalizedKey == "protocol") {
        return compareText(packet.protocol, value, op);
    }

    if (normalizedKey == "data" || normalizedKey == "raw") {
        return compareText(QString::fromLatin1(packet.rawData.toHex(' ')), value, op);
    }

    if (errorMessage) {
        *errorMessage = QString("Unknown filter field '%1'.").arg(key);
    }
    return false;
}

bool evaluatePostfix(const PacketData &packet, const QVector<Token> &postfix, QString *errorMessage) {
    QVector<bool> stack;

    for (const Token &token : postfix) {
        if (token.type == TokenType::Condition) {
            stack.push_back(evaluateCondition(packet, token.text, errorMessage));
            continue;
        }

        if (token.type == TokenType::Not) {
            if (stack.isEmpty()) {
                if (errorMessage) {
                    *errorMessage = "Invalid unary operator placement in filter expression.";
                }
                return false;
            }
            stack.last() = !stack.last();
            continue;
        }

        if (stack.size() < 2) {
            if (errorMessage) {
                *errorMessage = "Invalid binary operator placement in filter expression.";
            }
            return false;
        }

        const bool rhs = stack.takeLast();
        const bool lhs = stack.takeLast();

        if (token.type == TokenType::And) {
            stack.push_back(lhs && rhs);
        } else if (token.type == TokenType::Or) {
            stack.push_back(lhs || rhs);
        }
    }

    if (stack.size() != 1) {
        if (errorMessage) {
            *errorMessage = "Malformed filter expression.";
        }
        return false;
    }

    return stack.back();
}
}  // namespace

bool FilterService::matches(const PacketData &packet, const QString &expression, QString *errorMessage) const {
    if (errorMessage) {
        errorMessage->clear();
    }

    const QString trimmed = expression.trimmed();
    if (trimmed.isEmpty()) {
        return true;
    }

    QVector<Token> tokens = tokenize(trimmed, errorMessage);
    if (tokens.isEmpty()) {
        return false;
    }

    QVector<Token> postfix = toPostfix(tokens, errorMessage);
    if (postfix.isEmpty()) {
        return false;
    }

    return evaluatePostfix(packet, postfix, errorMessage);
}

QVector<PacketData> FilterService::apply(const QVector<PacketData> &packets, const QString &expression, QString *errorMessage) const {
    QVector<PacketData> filteredPackets;
    filteredPackets.reserve(packets.size());

    for (const PacketData &packet : packets) {
        QString localError;
        if (matches(packet, expression, &localError)) {
            filteredPackets.push_back(packet);
        } else if (!localError.isEmpty()) {
            if (errorMessage) {
                *errorMessage = localError;
            }
            return {};
        }
    }

    if (errorMessage) {
        errorMessage->clear();
    }
    return filteredPackets;
}

bool FilterService::isValid(const QString &expression, QString *errorMessage) const {
    PacketData probePacket;
    probePacket.number = 1;
    probePacket.timestamp = "00:00:00.000";
    probePacket.source = "127.0.0.1";
    probePacket.destination = "127.0.0.1";
    probePacket.protocol = "TCP";
    probePacket.length = 64;
    QString localError;
    matches(probePacket, expression, &localError);

    if (errorMessage) {
        *errorMessage = localError;
    }
    return localError.isEmpty();
}
