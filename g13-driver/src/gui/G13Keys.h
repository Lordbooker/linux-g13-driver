#pragma once
#include <QString>
#include <QStringList>

// Zuordnung der Tasten im G13-Bild zu den Schluesseln der bindings-*.properties.
// Die Datei-Schluessel sind die Bit-Positionen aus driver/Constants.h:
//   G0..G21  = physische Tasten G1..G22
//   G24      = BD, G32 = MR, G33/G34/G35 = Daumentasten LEFT/DOWN/TOP
//   G36..G39 = Joystick hoch/links/rechts/runter (siehe G13::parse_joystick)
// Wird von G13Visualizer und BindingEditor gemeinsam genutzt.
namespace G13Keys {

// Alle Tastennamen, die das Bild kennt.
const QStringList &all();

// Datei-Schluessel (z. B. "G4" fuer die Taste "G5"); leer, wenn unbekannt.
QString fileKey(const QString &keyName);

// Profil- und LCD-Tasten sind nicht frei belegbar.
bool isConfigurable(const QString &keyName);

// Anzeigename, z. B. "Joystick ↑" fuer "STICK_UP".
QString displayName(const QString &keyName);

}
