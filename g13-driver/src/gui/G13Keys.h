#pragma once
#include <QString>
#include <QStringList>

// Mapping of the keys in the G13 image to the keys of bindings-*.properties.
// The file keys are the bit positions from driver/Constants.h:
//   G0..G21  = physical keys G1..G22
//   G24      = BD, G29..G32 = M1/M2/M3/MR, G33/G34/G35 = thumb keys LEFT/DOWN/TOP
//   L1..L4 (bits 25..28) switch the profiles and cannot be bound.
//   G36..G39 = joystick up/left/right/down (see G13::parse_joystick)
// Shared by G13Visualizer and BindingEditor.
namespace G13Keys {

// All key names known to the image.
const QStringList &all();

// File key (e.g. "G4" for the key "G5"); empty if unknown.
QString fileKey(const QString &keyName);

// The profile keys L1..L4 cannot be bound.
bool isConfigurable(const QString &keyName);

// Display name, e.g. "Joystick ↑" for "STICK_UP".
QString displayName(const QString &keyName);

}
