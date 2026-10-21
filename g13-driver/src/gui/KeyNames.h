#pragma once
#include <QString>

class QKeyEvent;

// Readable names for Linux input keycodes (linux/input-event-codes.h).
//
// Based on a table of names by US key position. Character keys that have been
// captured before (input field or macro recording) are shown with the label of
// the user's own keyboard layout (e.g. "Z" instead of "Y" on a German keyboard).
// These labels are stored in the g13-gui settings.
namespace KeyNames {

// Name for a keycode; unknown codes are shown as "Key <code>".
QString nameFor(int code);

// Remembers the label of the key from a key press, if it produces a
// character (without modifiers, not on the keypad).
void learnLabel(int code, const QKeyEvent *event);

}
