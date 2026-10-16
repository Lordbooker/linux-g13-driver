#pragma once
#include <QString>

class QKeyEvent;

// Lesbare Namen fuer Linux-Input-Keycodes (linux/input-event-codes.h).
//
// Grundlage ist eine Tabelle mit Namen nach US-Tastenposition. Zeichen-Tasten,
// die schon einmal erfasst wurden (Eingabefeld oder Makro-Aufnahme), werden mit
// der Beschriftung des eigenen Tastaturlayouts angezeigt (z. B. "Z" statt "Y"
// auf einer deutschen Tastatur). Diese Beschriftungen werden in den Einstellungen
// von g13-gui gespeichert.
namespace KeyNames {

// Name zu einem Keycode; unbekannte Codes werden als "Key <code>" angezeigt.
QString nameFor(int code);

// Merkt sich die Beschriftung der Taste aus einem Tastendruck, sofern sie ein
// Zeichen erzeugt (ohne Modifier, nicht auf dem Ziffernblock).
void learnLabel(int code, const QKeyEvent *event);

}
