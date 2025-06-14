#include "G13Action.h"

bool G13Action::set(bool state) {
    if (state!= _is_pressed) {
        _is_pressed = state;
        if (_is_pressed) {
            key_down();
        } else {
            key_up();
        }
        return true;
    }
    return false;
}

bool G13Action::isPressed() const {
    return _is_pressed;
}