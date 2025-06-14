#include "PassThroughAction.h"
#include "Output.h"
#include <linux/uinput.h>

PassThroughAction::PassThroughAction(int code) : _keycode(code) {}

void PassThroughAction::key_down() {
    send_event(EV_KEY, _keycode, 1);
    send_event(EV_SYN, SYN_REPORT, 0);
}

void PassThroughAction::key_up() {
    send_event(EV_KEY, _keycode, 0);
    send_event(EV_SYN, SYN_REPORT, 0);
}