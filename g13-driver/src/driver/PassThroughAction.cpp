#include "PassThroughAction.h"
#include "Output.h"

#include <linux/input.h>

void PassThroughAction::key_down()
{
    UInput::send_event(EV_KEY, m_keycode, 1);
    UInput::send_event(EV_SYN, SYN_REPORT, 0);
}

void PassThroughAction::key_up()
{
    UInput::send_event(EV_KEY, m_keycode, 0);
    UInput::send_event(EV_SYN, SYN_REPORT, 0);
}
