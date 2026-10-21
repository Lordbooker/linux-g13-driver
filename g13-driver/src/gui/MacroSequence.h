#pragma once
#include <QList>
#include <QString>
#include <QStringList>

// Macro sequence in the driver format, e.g. "kd.17,d.120,ku.17".
// Must match MacroAction::tokenToEvent() in the driver:
//   kd.<code>  press key
//   ku.<code>  release key
//   d.<ms>     delay in milliseconds
// The driver only splits at ',' and does not trim, so write without spaces.
namespace MacroSequence {

struct Event {
    enum class Type { KeyDown, KeyUp, Delay };
    Type type  = Type::Delay;
    int  value = 0;   // keycode or milliseconds

    QString token() const;          // e.g. "kd.17"
    QString displayText() const;    // e.g. "↓  press W"
};

struct ParseResult {
    QList<Event> events;
    QStringList  invalidTokens;     // tokens the driver would ignore
};

ParseResult parse(const QString &sequence);
QString     serialize(const QList<Event> &events);

// Keycodes that are pressed but not released until the end
// (leads to stuck keys during playback).
QList<int>  unreleasedKeys(const QList<Event> &events);

}
