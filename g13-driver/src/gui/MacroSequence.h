#pragma once
#include <QList>
#include <QString>
#include <QStringList>

// Makro-Sequenz im Treiberformat, z. B. "kd.17,d.120,ku.17".
// Muss mit MacroAction::tokenToEvent() im Treiber uebereinstimmen:
//   kd.<code>  Taste druecken
//   ku.<code>  Taste loslassen
//   d.<ms>     Wartezeit in Millisekunden
// Der Treiber trennt nur an ',' und trimmt nicht, daher ohne Leerzeichen schreiben.
namespace MacroSequence {

struct Event {
    enum class Type { KeyDown, KeyUp, Delay };
    Type type  = Type::Delay;
    int  value = 0;   // Keycode bzw. Millisekunden

    QString token() const;          // z. B. "kd.17"
    QString displayText() const;    // z. B. "W drücken"
};

struct ParseResult {
    QList<Event> events;
    QStringList  invalidTokens;     // Tokens, die der Treiber ignorieren wuerde
};

ParseResult parse(const QString &sequence);
QString     serialize(const QList<Event> &events);

// Keycodes, die gedrueckt, aber bis zum Ende nicht losgelassen werden
// (fuehrt beim Abspielen zu haengenden Tasten).
QList<int>  unreleasedKeys(const QList<Event> &events);

}
