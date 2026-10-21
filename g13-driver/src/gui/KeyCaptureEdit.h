#pragma once
#include <QLineEdit>

// Input field that captures a single key: click into it, press a key.
// The Linux input keycode is stored (as in the driver). Keys that otherwise
// control the user interface (Tab, Enter, Esc, Ctrl+S, ...) are captured too.
class KeyCaptureEdit : public QLineEdit {
    Q_OBJECT
public:
    explicit KeyCaptureEdit(QWidget *parent = nullptr);

    // Linux keycode of the captured key, 0 = no key.
    int keyCode() const { return m_keyCode; }
    void setKeyCode(int code);
    void clearKey() { setKeyCode(0); }

    // Linux keycode from QKeyEvent::nativeScanCode() (XKB keycode = Linux keycode + 8),
    // 0 if the system does not provide a scan code.
    static int linuxKeyCode(quint32 nativeScanCode);

signals:
    // withModifiers: Ctrl/Shift/Alt/Super were held during the press. Only the
    // single key is captured anyway (the driver sends exactly one key code).
    void keyCaptured(int code, bool withModifiers);

protected:
    bool event(QEvent *event) override;
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    void updateText();

    int m_keyCode = 0;
};
