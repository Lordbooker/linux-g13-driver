#include "G13Action.h"

bool G13Action::set(bool pressed)
{
    if (pressed == m_pressed)
        return false;
    m_pressed = pressed;
    if (pressed)
        key_down();
    else
        key_up();
    return true;
}
