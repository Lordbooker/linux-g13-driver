#pragma once
#include <QList>
#include <QPair>
#include <QString>

// Lesbare Namen fuer Linux-Input-Keycodes (linux/input-event-codes.h).
// Wird von BindingEditor (Auswahlliste) und MacroRecorder (Anzeige) genutzt.
namespace KeyNames {

// Alle bekannten Tasten in Anzeigereihenfolge: {Name, Linux-Keycode}
const QList<QPair<QString, int>> &all();

// Name zu einem Keycode; unbekannte Codes werden als "Taste <code>" angezeigt.
QString nameFor(int code);

}
