#pragma once
#include <QLineEdit>

// Eingabezeile, die eine einzelne Taste erfasst: hineinklicken, Taste druecken.
// Gespeichert wird der Linux-Input-Keycode (wie im Treiber). Erfasst werden auch
// Tasten, die sonst die Oberflaeche bedienen (Tab, Enter, Esc, Strg+S, ...).
class KeyCaptureEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit KeyCaptureEdit(QWidget *parent = nullptr);

    // Linux-Keycode der erfassten Taste, 0 = keine Taste.
    int keyCode() const { return m_keyCode; }
    void setKeyCode(int code);
    void clearKey() { setKeyCode(0); }

    // Linux-Keycode aus QKeyEvent::nativeScanCode() (XKB-Keycode = Linux-Keycode + 8),
    // 0 wenn das System keinen Scancode liefert.
    static int linuxKeyCode(quint32 nativeScanCode);

signals:
    // withModifiers: beim Druecken waren Strg/Shift/Alt/Super gehalten. Erfasst
    // wird trotzdem nur die eine Taste (der Treiber sendet genau einen Tastencode).
    void keyCaptured(int code, bool withModifiers);

protected:
    bool event(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void updateText();

    int m_keyCode = 0;
};
