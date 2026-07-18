#include <linux/uinput.h>
#include <sstream>
#include <chrono>
#include <syslog.h>

#include "MouseMoveAction.h"
#include "Output.h"

std::vector<int> parse_plus_list(const std::string& value) {
    std::vector<int> codes;
    std::stringstream ss(value);
    std::string part;
    while (std::getline(ss, part, '+')) {
        try {
            codes.push_back(std::stoi(part));
        } catch (...) {
            syslog(LOG_WARNING, "parse_plus_list: ignoring invalid code '%s'", part.c_str());
        }
    }
    return codes;
}

MouseMoveAction::MouseMoveAction(int dx, int dy, std::vector<int> hold)
    : dx(dx), dy(dy), hold(std::move(hold)), stop_flag(false) {
}

MouseMoveAction::~MouseMoveAction() {
    stop_flag = true;
    if (mover.joinable()) {
        mover.join();
    }
}

void MouseMoveAction::move_loop() {
    while (!stop_flag) {
        UInput::send_event(EV_REL, REL_X, dx);
        UInput::send_event(EV_REL, REL_Y, dy);
        UInput::send_event(EV_SYN, SYN_REPORT, 0);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

void MouseMoveAction::key_down() {
    if (mover.joinable()) {
        mover.join();
    }
    for (int code : hold) {
        UInput::send_event(EV_KEY, code, 1);
    }
    UInput::send_event(EV_SYN, SYN_REPORT, 0);
    stop_flag = false;
    mover = std::thread(&MouseMoveAction::move_loop, this);
}

void MouseMoveAction::key_up() {
    stop_flag = true;
    if (mover.joinable()) {
        mover.join();
    }
    for (auto it = hold.rbegin(); it != hold.rend(); ++it) {
        UInput::send_event(EV_KEY, *it, 0);
    }
    UInput::send_event(EV_SYN, SYN_REPORT, 0);
}
