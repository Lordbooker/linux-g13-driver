#ifndef __G13_H__
#define __G13_H__

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <istream>
#include <libusb-1.0/libusb.h>
#include <time.h> // For time_t

#include "Constants.h"
#include "G13Action.h"
#include "Macro.h"

class G13 {
private:
	std::vector<std::unique_ptr<G13Action>> actions;

	libusb_device        *device;       
	libusb_device_handle *handle;        
	int                   uinput_file;   

	int                   loaded;        
	volatile int          keepGoing;     

	stick_mode_t          stick_mode;    
	int                   stick_keys[4];   
	int                   bindings;      

    unsigned char lcd_buffer[G13_LCD_BUFFER_SIZE];

    // Feature: Live-Reload
    time_t last_config_mtime;
    void check_for_config_update();

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

    // --- LCD ---
    void clear_lcd_buffer();
    void set_pixel(int x, int y, bool on);
    void write_lcd();
	void draw_test_pattern();
	void write_char(int x, int y, char c);
    void write_text(int x, int y, const std::string& text);
};

#endif