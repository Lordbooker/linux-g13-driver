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

	int                   loaded;        // Flag indicating if the device was loaded successfully.
	volatile int          keepGoing;     // Flag to control the main event loop of this thread.

	stick_mode_t          stick_mode;    // Current mode of the joystick (keys or absolute).
	int                   bindings;      // Index of the current key binding profile (0-3).

    // ++ ADDED: Internal buffer to store the 160x48 pixel image for the LCD. ++
    unsigned char lcd_buffer[G13_LCD_BUFFER_SIZE];

    // --- Private Methods ---
	std::unique_ptr<Macro> loadMacro(int id);
	void parse_bindings_from_stream(std::istream& stream);
	int  read();
	void parse_joystick(unsigned char *buf);
	void parse_key(int key, unsigned char *byte);
	void parse_keys(unsigned char *buf);

public:
	G13(libusb_device *device);
	~G13();

	void start();
	void stop();
	void loadBindings();
	void setColor(int r, int g, int b);

    // --- ADDED: New methods for display control ---

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