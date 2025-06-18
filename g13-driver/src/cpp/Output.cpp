#include <iostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <iomanip>
#include <linux/uinput.h>
#include <fcntl.h>
#include <pthread.h>

#include "Output.h"
#include "Constants.h"

using namespace std;

// Initialization of static class members.
int UInput::file = -1;
pthread_mutex_t UInput::plock = PTHREAD_MUTEX_INITIALIZER;


/**
 * @brief Sends a single input event to the virtual uinput device.
 * @param type The event type (e.g., EV_KEY, EV_ABS).
 * @param code The event code (e.g., KEY_A, ABS_X).
 * @param val The event value (e.g., 1 for press, 0 for release, or axis position).
 *
 * This function is thread-safe, using a mutex to prevent concurrent writes
 * to the uinput file descriptor.
 */
void UInput::send_event(int type, int code, int val) {
	if (file < 0) {
		return;
	}

	pthread_mutex_lock(&plock);

	struct input_event event;
	memset(&event, 0, sizeof(event));
	gettimeofday(&event.time, nullptr); // Set the event timestamp.
	event.type = type;
	event.code = code;
	event.value = val;

	// Write the event structure to the uinput file descriptor.
	write(file, &event, sizeof(event));

	pthread_mutex_unlock(&plock);
}

/**
 * @brief Flushes any buffered data to the uinput device file descriptor.
 * This is generally not needed as events are written immediately.
 */
void UInput::flush() {
    if (file < 0) return;
	pthread_mutex_lock(&plock);
	fsync(file);
	pthread_mutex_unlock(&plock);
}

/**
 * @brief Closes and destroys the virtual uinput device.
 * This should be called on application shutdown to clean up resources.
 */
void UInput::close_uinput() {
    if (file >= 0) {
        // Destroy the uinput device via ioctl before closing the file.
        ioctl(file, UI_DEV_DESTROY);
        close(file);
        file = -1; // Mark as closed.
    }
}

/**
 * @brief Creates and configures the virtual uinput device.
 * @return true on success, false on failure.
 *
 * This function finds the uinput device file, opens it, and configures
 * a new virtual device named "G13" that can send key presses and
 * absolute joystick movements.
 */
bool UInput::create_uinput() {
	struct uinput_user_dev uinp;
	const char* dev_uinput_fname =
			access("/dev/input/uinput", F_OK) == 0 ? "/dev/input/uinput" :
			access("/dev/uinput", F_OK) == 0 ? "/dev/uinput" : 0;

	if (!dev_uinput_fname) {
		cerr << "Could not find an uinput device" << endl;
		return false;
	}

	if (access(dev_uinput_fname, W_OK) != 0) {
		cerr << dev_uinput_fname << " doesn't grant write permissions" << endl;
		return false;
	}

	file = open(dev_uinput_fname, O_WRONLY | O_NDELAY);
	if (file < 0) {
		cerr << "Could not open uinput" << endl;
		return false;
	}

	// Configure the virtual device.
	memset(&uinp, 0, sizeof(uinp));
	char name[] = "G13";
	strncpy(uinp.name, name, sizeof(name));
	uinp.id.version = 1;
	uinp.id.bustype = BUS_USB;
	uinp.id.product = G13_PRODUCT_ID;
	uinp.id.vendor = G13_VENDOR_ID;
	uinp.absmin[ABS_X] = 0;   // Set joystick X-axis range.
	uinp.absmin[ABS_Y] = 0;   // Set joystick Y-axis range.
	uinp.absmax[ABS_X] = 0xff;
	uinp.absmax[ABS_Y] = 0xff;

	// Enable event types for keys and absolute positioning.
	ioctl(file, UI_SET_EVBIT, EV_KEY);
	ioctl(file, UI_SET_EVBIT, EV_ABS);
	ioctl(file, UI_SET_MSCBIT, MSC_SCAN);
	ioctl(file, UI_SET_ABSBIT, ABS_X);
	ioctl(file, UI_SET_ABSBIT, ABS_Y);

	// Enable all possible key codes for the virtual device.
	for (int i = 0; i < 256; i++)
		ioctl(file, UI_SET_KEYBIT, i);
	ioctl(file, UI_SET_KEYBIT, BTN_THUMB);

	// Write the configuration to the uinput device.
	int retcode = write(file, &uinp, sizeof(uinp));
	if (retcode < 0) {
		cerr << "Could not write to uinput device (" << retcode << ")" << endl;
        close(file); file = -1;
		return false;
	}

	// Create the device.
	retcode = ioctl(file, UI_DEV_CREATE);
	if (retcode) {
		cerr << "Error creating uinput device for G13" << endl;
        close(file); file = -1;
		return false;
	}
	return true;
}