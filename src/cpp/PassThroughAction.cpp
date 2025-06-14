#include <linux/uinput.h>
// #include <iostream> // Unused

#include "PassThroughAction.h"
#include "Output.h"

// using namespace std; // Not strictly needed here

PassThroughAction::PassThroughAction(int code) {
	this->keycode = code;
}

PassThroughAction::~PassThroughAction() {
}

int PassThroughAction::getKeyCode() const {
	return this->keycode;
}

void PassThroughAction::setKeyCode(int code) {
	this->keycode = code;
}

void PassThroughAction::key_down() {
	send_event(EV_KEY, this->keycode, 1);
	send_event(0, 0, 0); // SYN_REPORT
}

void PassThroughAction::key_up() {
	send_event(EV_KEY, this->keycode, 0);
	send_event(0, 0, 0); // SYN_REPORT
}
