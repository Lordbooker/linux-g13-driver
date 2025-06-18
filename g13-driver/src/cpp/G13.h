#ifndef __G13_H__
#define __G13_H__

// Removed 'using namespace std;' from header and qualified types with std::.
// This is good practice to prevent namespace pollution.
#include <string>
#include <vector>
#include <memory>
#include <map>
#include <istream>

#include <libusb-1.0/libusb.h> // Include libusb header for types

#include "Constants.h"
#include "G13Action.h"
#include "Macro.h"

class G13 {
private:
    /** A vector of smart pointers to the actions assigned to each G-key. */
	std::vector<std::unique_ptr<G13Action>> actions;

	libusb_device        *device; // The USB device pointer.
	libusb_device_handle *handle;  // The handle for the opened USB device.
	int                   uinput_file; // File descriptor for uinput device.

	int                   loaded; // Flag indicating if the device was loaded successfully.
	volatile int          keepGoing; // Flag to control the main event loop.

	stick_mode_t          stick_mode; // Current mode of the joystick (keys or absolute).
	int                   stick_keys[4]; // State of the emulated stick keys.

	int                   bindings; // Index of the current key binding profile (0-3).

    /**
     * @brief Loads a macro from a .properties file.
     * @param id The ID of the macro to load.
     * @return A unique_ptr to the Macro object.
     */
	std::unique_ptr<Macro> loadMacro(int id);

    /**
     * @brief Parses key binding definitions from a stream.
     * @param stream The input stream containing binding definitions.
     */
	void parse_bindings_from_stream(std::istream& stream);

    /**
     * @brief Reads a single event report from the G13.
     * @return 0 on success, -1 on error.
     */
	int  read();

    /**
     * @brief Parses joystick data from the report buffer.
     * @param buf The report buffer.
     */
	void parse_joystick(unsigned char *buf);

    /**
     * @brief Parses the state of a single key from the report buffer.
     * @param key The G-key index.
     * @param byte The pointer to the start of the key data in the buffer.
     */
	void parse_key(int key, unsigned char *byte);

    /**
     * @brief Parses all key states from the report buffer.
     * @param buf The report buffer.
     */
	void parse_keys(unsigned char *buf);

public:
    /**
     * @brief Constructor.
     * @param device Pointer to the libusb_device.
     */
	G13(libusb_device *device);

    /** @brief Destructor. */
	~G13();

    /** @brief Starts the main event reading loop. */
	void start();

    /** @brief Stops the main event reading loop. */
	void stop();

    /** @brief Loads the bindings for the current profile. */
	void loadBindings();

    /**
     * @brief Sets the color of the LCD backlight.
     * @param r Red component (0-255).
     * @param g Green component (0-255).
     * @param b Blue component (0-255).
     */
	void setColor(int r, int g, int b);
};


#endif