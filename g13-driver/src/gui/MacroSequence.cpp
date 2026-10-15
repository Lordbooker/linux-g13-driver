#include "MacroSequence.h"
#include "KeyNames.h"

namespace MacroSequence {

QString Event::token() const
{
    switch (type) {
    case Type::KeyDown: return QString("kd.%1").arg(value);
    case Type::KeyUp:   return QString("ku.%1").arg(value);
    case Type::Delay:   return QString("d.%1").arg(value);
    }
    return {};
}

QString Event::displayText() const
{
    switch (type) {
    case Type::KeyDown: return QString("↓  %1 drücken").arg(KeyNames::nameFor(value));
    case Type::KeyUp:   return QString("↑  %1 loslassen").arg(KeyNames::nameFor(value));
    case Type::Delay:   return QString("⏱  %1 ms warten").arg(value);
    }
    return {};
}

// Liest "<prefix><zahl>" mit nicht-negativer Ganzzahl; sonst false.
static bool readValue(const QString &token, const QString &prefix, int &value)
{
    if (!token.startsWith(prefix))
        return false;
    bool ok = false;
    const int v = token.mid(prefix.size()).toInt(&ok);
    if (!ok || v < 0)
        return false;
    value = v;
    return true;
}

ParseResult parse(const QString &sequence)
{
    ParseResult result;
    const QStringList tokens = sequence.split(',', Qt::SkipEmptyParts);

    for (const QString &raw : tokens) {
        const QString token = raw.trimmed();
        if (token.isEmpty())
            continue;

        Event ev;
        int value = 0;
        if (readValue(token, "kd.", value)) {
            ev.type = Event::Type::KeyDown;
        } else if (readValue(token, "ku.", value)) {
            ev.type = Event::Type::KeyUp;
        } else if (readValue(token, "d.", value)) {
            ev.type = Event::Type::Delay;
        } else {
            result.invalidTokens << token;
            continue;
        }
        ev.value = value;
        result.events << ev;
    }
    return result;
}

QString serialize(const QList<Event> &events)
{
    QStringList tokens;
    tokens.reserve(events.size());
    for (const Event &ev : events)
        tokens << ev.token();
    return tokens.join(',');
}

QList<int> unreleasedKeys(const QList<Event> &events)
{
    QList<int> pressed;
    for (const Event &ev : events) {
        if (ev.type == Event::Type::KeyDown && !pressed.contains(ev.value))
            pressed << ev.value;
        else if (ev.type == Event::Type::KeyUp)
            pressed.removeAll(ev.value);
    }
    return pressed;
}

}
