#ifndef __G13_H__
#define __G13_H__

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <istream>
#include <libusb-1.0/libusb.h>

#include "Constants.h"
#include "G13Action.h"
#include "Macro.h"

class G13 {
private:
    // A vector of smart pointers to the actions assigned to each G-key.
	std::vector<std::unique_ptr<G13Action>> actions;

	libusb_device        *device;       // The USB device pointer.
	libusb_device_handle *handle;        // The handle for the opened USB device.
	int                   uinput_file;   // (Legacy) File descriptor for uinput device.

	int                   loaded;        // Flag indicating if the device was loaded successfully.
	volatile int          keepGoing;     // Flag to control the main event loop of this thread.

	stick_mode_t          stick_mode;    // Current mode of the joystick (keys or absolute).
	int                   stick_keys[4];   // (Legacy) State of the emulated stick keys.
	int                   bindings;      // Index of the current key binding profile (0-3).

    // Internal buffer to store the 160x48 pixel image for the LCD.
    unsigned char lcd_buffer[G13_LCD_BUFFER_SIZE];

    // --- Private Methods ---
	std::unique_ptr<Macro> loadMacro(int id);

    /**
     * @brief Parses key binding definitions from a stream.
     * @param stream The input stream containing binding definitions.
     */
	void parse_bindings_from_stream(std::istream& stream);
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

    // --- Methods for display control ---

    /**
     * @brief Clears the internal LCD buffer (sets all pixels to off/black).
     */
    void clear_lcd_buffer();

    /**
     * @brief Sets a single pixel in the internal LCD buffer.
     * @param x The x-coordinate (0-159).
     * @param y The y-coordinate (0-47).
     * @param on true to turn the pixel on (white), false to turn it off (black).
     */
    void set_pixel(int x, int y, bool on);

    /**
     * @brief Sends the content of the internal LCD buffer to the G13 display.
     */
    void write_lcd();
};


#endif