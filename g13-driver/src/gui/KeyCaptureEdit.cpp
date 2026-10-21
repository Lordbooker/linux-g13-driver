#include "KeyCaptureEdit.h"
#include "KeyNames.h"

#include <QContextMenuEvent>
#include <QKeyEvent>

KeyCaptureEdit::KeyCaptureEdit(QWidget *parent) : QLineEdit(parent)
{
    setReadOnly(true);                      // no text cursor, no pasting
    setFocusPolicy(Qt::StrongFocus);
    setPlaceholderText(tr("Click here and press a key"));
    setToolTip(tr("Click into the field and press a key on your keyboard."));
}

int KeyCaptureEdit::linuxKeyCode(quint32 nativeScanCode)
{
    constexpr quint32 XkbOffset = 8;
    return nativeScanCode > XkbOffset ? static_cast<int>(nativeScanCode - XkbOffset) : 0;
}

void KeyCaptureEdit::setKeyCode(int code)
{
    m_keyCode = code > 0 ? code : 0;
    updateText();
}

void KeyCaptureEdit::updateText()
{
    setText(m_keyCode > 0
                ? tr("%1  (code %2)").arg(KeyNames::nameFor(m_keyCode)).arg(m_keyCode)
                : QString());
}

bool KeyCaptureEdit::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::ShortcutOverride:
        // Accepting suppresses shortcuts (e.g. Ctrl+S); the KeyPress follows.
        event->accept();
        return true;

    case QEvent::KeyPress: {
        // Intercept before QWidget::event, so Tab/Backtab are captured as well
        // instead of moving the focus.
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (!keyEvent->isAutoRepeat()) {
            const int code = linuxKeyCode(keyEvent->nativeScanCode());
            if (code > 0) {
                KeyNames::learnLabel(code, keyEvent);
                setKeyCode(code);
                const Qt::KeyboardModifiers held = keyEvent->modifiers()
                    & (Qt::ControlModifier | Qt::ShiftModifier | Qt::AltModifier | Qt::MetaModifier);
                // When the modifier itself is pressed it is already set -> ignore it
                const bool isModifierKey = keyEvent->key() == Qt::Key_Control
                    || keyEvent->key() == Qt::Key_Shift || keyEvent->key() == Qt::Key_Alt
                    || keyEvent->key() == Qt::Key_Meta || keyEvent->key() == Qt::Key_Super_L
                    || keyEvent->key() == Qt::Key_Super_R || keyEvent->key() == Qt::Key_AltGr;
                emit keyCaptured(code, held != Qt::NoModifier && !isModifierKey);
            }
        }
        return true;
    }

    case QEvent::KeyRelease:
        return true;

    default:
        return QLineEdit::event(event);
    }
}

void KeyCaptureEdit::contextMenuEvent(QContextMenuEvent *event)
{
    event->accept();                        // no copy/paste menu
}
