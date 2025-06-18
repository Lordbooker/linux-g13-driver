#include <iostream>
#include <unistd.h>
#include <fcntl.h>

#include <sys/time.h>
#include <time.h>
#include <unistd.h>
#include <linux/uinput.h>
#include <stdlib.h>
#include <string.h>

#include "G13Action.h"

using namespace std;

/**
 * @brief Default constructor. Initializes the key state to not pressed.
 */
G13Action::G13Action() {
	pressed = 0;
}

/**
 * @brief Virtual destructor. Does nothing in the base class.
 */
G13Action::~G13Action() {
}

/**
 * @brief Virtual method called when the key is pressed down.
 * Subclasses should override this to implement their specific action.
 */
void G13Action::key_down() {
	// Base implementation is a no-op.
	// cout << "G13Action::key_down()\n";
}

/**
 * @brief Virtual method called when the key is released.
 * Subclasses should override this to implement their specific action.
 */
void G13Action::key_up() {
	// Base implementation is a no-op.
	// cout << "G13Action::key_up()\n";
}

/**
 * @brief Sets the new state of the key and triggers actions if the state changed.
 * @param state The new state (0 for released, non-zero for pressed).
 * @return 1 if the state changed and an action was triggered, 0 otherwise.
 *
 * This method acts as a template method. It manages the state transition
 * and calls the appropriate virtual methods (key_down or key_up).
 */
int G13Action::set(int state) {
	int s = 0;
	if (state != 0) {
		s = 1;
	}

	// Only trigger actions if the state has actually changed.
	if (s != pressed) {
		pressed = s;

		if (s) {
			key_down();
		}
		else {
			key_up();
		}

		return 1; // State changed.
	}

	return 0; // State did not change.
}

/**
 * @brief Returns the current pressed state of the key.
 * @return 1 if pressed, 0 if released.
 */
int G13Action::isPressed() const {
	return pressed;
}