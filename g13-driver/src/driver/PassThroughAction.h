#ifndef G13_DRIVER_PASSTHROUGHACTION_H
#define G13_DRIVER_PASSTHROUGHACTION_H

#include "G13Action.h"

/**
 * @brief Sends a single key: pressed while the G13 key is pressed.
 */
class PassThroughAction : public G13Action {
public:
    /// @param keycode Linux input key code (linux/input-event-codes.h)
    explicit PassThroughAction(int keycode) : m_keycode(keycode) {}

    [[nodiscard]] int keyCode() const { return m_keycode; }

protected:
    void key_down() override;
    void key_up() override;

private:
    int m_keycode;
};

#endif // G13_DRIVER_PASSTHROUGHACTION_H
