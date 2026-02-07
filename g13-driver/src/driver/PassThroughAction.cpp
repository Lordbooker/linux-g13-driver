#include <linux/uinput.h>

#include "PassThroughAction.h"
#include "Output.h"

/**
 * @brief Constructs a PassThroughAction with a specific keycode.
 * @param code The Linux keycode to be sent when this action is triggered.
 */
PassThroughAction::PassThroughAction(int code) {
	this->keycode = code;
}

/**
 * @brief Destructor.
 */
PassThroughAction::~PassThroughAction() {
}

/**
 * @brief Gets the keycode associated with this action.
 * @return The Linux keycode.
 */
int PassThroughAction::getKeyCode() const {
	return this->keycode;
}

/**
 * @brief Sets a new keycode for this action.
 * @param code The new Linux keycode.
 */
void PassThroughAction::setKeyCode(int code) {
	this->keycode = code;
}

/**
 * @brief Handles the key-down event by sending a key press to uinput.
 */
void PassThroughAction::key_down() {
	// Send a key press event (value 1) for the stored keycode.
	UInput::send_event(EV_KEY, this->keycode, 1);
	// Send a synchronization event to report that the event is complete.
	UInput::send_event(0, 0, 0); // SYN_REPORT
}

/**
 * @brief Handles the key-up event by sending a key release to uinput.
 */
void PassThroughAction::key_up() {
	// Send a key release event (value 0) for the stored keycode.
	UInput::send_event(EV_KEY, this->keycode, 0);
	// Send a synchronization event.
	UInput::send_event(0, 0, 0); // SYN_REPORT
}