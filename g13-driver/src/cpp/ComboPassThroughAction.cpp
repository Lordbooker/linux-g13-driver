#include <linux/uinput.h>

#include "ComboPassThroughAction.h"
#include "Output.h"

ComboPassThroughAction::ComboPassThroughAction(const std::vector<int>& codes) {
	this->keycodes = codes;
}

ComboPassThroughAction::~ComboPassThroughAction() {
}

void ComboPassThroughAction::key_down() {
	// Press in listed order (modifiers are written first in the binding).
	for (int code : this->keycodes) {
		UInput::send_event(EV_KEY, code, 1);
		UInput::send_event(0, 0, 0); // SYN_REPORT
	}
}

void ComboPassThroughAction::key_up() {
	// Release in reverse order so modifiers come up last.
	for (auto it = this->keycodes.rbegin(); it != this->keycodes.rend(); ++it) {
		UInput::send_event(EV_KEY, *it, 0);
		UInput::send_event(0, 0, 0); // SYN_REPORT
	}
}
