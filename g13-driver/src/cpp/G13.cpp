#include <iostream>
#include <fstream>
#include <vector>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <libusb-1.0/libusb.h>
#include <iomanip>
#include <linux/uinput.h>
#include <fcntl.h>
#include <pthread.h>
#include <algorithm>
#include <sstream>
#include <istream>

#include "Constants.h"
#include "G13.h"
#include "G13Action.h"
#include "PassThroughAction.h"
#include "MacroAction.h"
#include "Output.h"

// This global flag is defined in Main.cpp and used here to gracefully
// terminate the device's event loop when the whole application is shutting down.
extern volatile sig_atomic_t daemon_keep_running;

// HARDENING: Define a constant for the maximum number of macros, matching the GUI.
const int G13_MAX_MACROS = 200;

// Helper to trim whitespace from both ends of a std::string.
std::string trim_string(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return ""; // no content

    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

G13::G13(libusb_device *device) {
	this->device = device;
	this->loaded = 0;
	this->bindings = 0;
	this->stick_mode = STICK_KEYS;

    // Initialize a default (no-op) action for every possible key.
    actions.resize(G13_NUM_KEYS);
	for (int i = 0; i < G13_NUM_KEYS; i++) {
		actions[i] = std::make_unique<G13Action>();
	}

	if (libusb_open(device, &handle) != 0) {
		std::cerr << "Error opening G13 device" << std::endl;
		return;
	}

	if (libusb_kernel_driver_active(handle, 0) == 1) {
		if (libusb_detach_kernel_driver(handle, 0) == 0) {
			std::cout << "Kernel driver detached" << std::endl;
		}
	}

	if (libusb_claim_interface(handle, 0) < 0) {
		std::cerr << "Cannot Claim Interface" << std::endl;
		return;
	}

    // Explicitly initialize the display.
    // This control transfer tells the G13 that a host driver wants to take
    // control of the LCD, making it ready to accept data on the bulk endpoint.
    std::cout << "Initializing G13 display..." << std::endl;
    unsigned char lcd_init_payload[] = { 0x01 };
    int init_error = libusb_control_transfer(handle,
        (LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE), // bmRequestType
        0x09,           // bRequest
        0x0300,         // wValue
        0x0000,         // wIndex
        lcd_init_payload, // data
        sizeof(lcd_init_payload), // wLength
        1000            // timeout
    );

    if (init_error < 0) {
        std::cerr << "Error initializing G13 LCD: " << libusb_error_name(init_error) << std::endl;
    }

	setColor(128, 128, 128);

	// Initialize the LCD buffer to all black.
	clear_lcd_buffer();

	this->loaded = 1;
}

G13::~G13() {
	if (!this->loaded) {
		return;
	}
	// setColor call was removed to prevent errors on hot-unplug.
	libusb_release_interface(this->handle, 0);
	libusb_close(this->handle);
}

void G13::start() {
	if (!this->loaded) {
		return;
	}
	loadBindings();
	keepGoing = 1;
    // The loop runs as long as the thread should run AND the daemon is not shutting down.
	while (keepGoing && daemon_keep_running) {
		// Check if the device was disconnected during the read operation.
        if (read() == -4) {
            break; // Exit the loop to allow the thread to terminate cleanly.
        }
	}
}

void G13::stop() {
	if (!this->loaded) {
		return;
	}
	keepGoing = 0;
}

std::unique_ptr<Macro> G13::loadMacro(int num) {
	char filename[1024];
    const char* home_dir = getenv("HOME");
    if (!home_dir) {
        std::cerr << "G13::loadMacro(" << num << ") HOME environment variable not set.\n";
        return nullptr;
    }

	snprintf(filename, sizeof(filename), "%s/.g13/macro-%d.properties", home_dir, num);
	std::ifstream file (filename);

	if (!file.is_open()) {
		std::cout << "Could not open config file: " << filename << "\n";
		return nullptr;
	}

	auto macro = std::make_unique<Macro>();
	macro->setId(num);
    std::string macro_name, macro_sequence;

	std::string line;
	while (getline(file, line)) {
        std::string trimmed_line = trim_string(line);

		if (!trimmed_line.empty() && trimmed_line[0] != '#') {
            size_t eq_pos = trimmed_line.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = trim_string(trimmed_line.substr(0, eq_pos));
                std::string value = trim_string(trimmed_line.substr(eq_pos + 1));
			    if (key == "name") {
				    macro_name = value;
			    } else if (key == "sequence") {
				    macro_sequence = value;
			    }
			}
		}
	}
    macro->setName(macro_name);
    macro->setSequence(macro_sequence);
	return macro;
}


void G13::parse_bindings_from_stream(std::istream& stream) {
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed_line = trim_string(line);

        if (trimmed_line.empty() || trimmed_line[0] == '#') {
            continue;
        }

        size_t eq_pos = trimmed_line.find('=');
        if (eq_pos == std::string::npos) {
            continue;
        }

        std::string key = trim_string(trimmed_line.substr(0, eq_pos));
        std::string value = trim_string(trimmed_line.substr(eq_pos + 1));

        if (key == "color") {
            std::stringstream ss(value);
            std::string segment;
            int r, g, b;

            try {
                if (std::getline(ss, segment, ',') && (r = std::stoi(segment)) >= 0 &&
                    std::getline(ss, segment, ',') && (g = std::stoi(segment)) >= 0 &&
                    std::getline(ss, segment, ',') && (b = std::stoi(segment)) >= 0) {
                    
                    // HARDENING: Validate color values are within the 0-255 range.
                    if (r <= 255 && g <= 255 && b <= 255) {
                        setColor(r, g, b);
                    } else {
                        std::cerr << "G13::parse_bindings_from_stream() Invalid color value: " << value 
                                  << ". All values must be between 0 and 255." << std::endl;
                    }
                }
            } catch (const std::exception& e) {
                 std::cerr << "G13::parse_bindings_from_stream() Invalid color format: " << value << std::endl;
            }
        }
        else if (key == "stick_mode") {
            // Stick mode logic here
        }
        else if (!key.empty() && key.rfind("G", 0) == 0) { // Check if key starts with "G"
            try {
                int gKey = std::stoi(key.substr(1));

                std::stringstream ss(value);
                std::string type;
                if (!std::getline(ss, type, ',')) continue;
                type = trim_string(type);

                if (type == "p") { // Passthrough type
                    std::string keytype_str;
                    if (!std::getline(ss, keytype_str, ',')) continue;
                    keytype_str = trim_string(keytype_str);

                    if (keytype_str.rfind("k.", 0) == 0) {
                        int keycode = std::stoi(keytype_str.substr(2));

                        // HARDENING: Validate key and keycode ranges before creating the action.
                        if (gKey >= 0 && gKey < G13_NUM_KEYS) {
                            if (keycode >= 0 && keycode < 256) {
                                actions[gKey] = std::make_unique<PassThroughAction>(keycode);
                            } else {
                                std::cerr << "G13::parse_bindings_from_stream() Invalid keycode " << keycode << " for key " << key 
                                          << ". Keycode must be between 0 and 255." << std::endl;
                            }
                        }
                    }
                }
                else if (type == "m") { // Macro type
                    std::string macroId_str, repeats_str;
                    if (!std::getline(ss, macroId_str, ',') || !std::getline(ss, repeats_str, ',')) continue;
                    
                    int macroId = std::stoi(trim_string(macroId_str));
                    int repeats = std::stoi(trim_string(repeats_str));

                    // HARDENING: Validate macroId and repeats values.
                    if (macroId >= 0 && macroId < G13_MAX_MACROS && repeats >= 0) {
                        auto macro = loadMacro(macroId);
                        if (macro && gKey >= 0 && gKey < G13_NUM_KEYS) {
                            actions[gKey] = std::make_unique<MacroAction>(macro->getSequence());
                            static_cast<MacroAction*>(actions[gKey].get())->setRepeats(repeats);
                        }
                    } else {
                        std::cerr << "G13::parse_bindings_from_stream() Invalid macro parameters for key " << key 
                                  << ". macroId must be 0-" << (G13_MAX_MACROS - 1) 
                                  << " and repeats must be non-negative." << std::endl;
                    }
                }
                else {
                    std::cout << "G13::parse_bindings_from_stream() unknown type '" << type << "' for key " << key << "\n";
                }
            } catch (const std::invalid_argument& ia) {
                std::cerr << "G13::parse_bindings_from_stream() Invalid number format in line: " << trimmed_line << std::endl;
            } catch (const std::out_of_range& oor) {
                std::cerr << "G13::parse_bindings_from_stream() Number out of range in line: " << trimmed_line << std::endl;
            }
        }
        else {
            std::cout << "G13::parse_bindings_from_stream() Unknown token in key: " << key << "\n";
        }
    }
}


void G13::loadBindings() {
	char filename[1024];
	const char* home_dir = getenv("HOME");
	if (!home_dir) {
		std::cerr << "G13::loadBindings() HOME environment variable not set.\n";
		return;
	}
	snprintf(filename, sizeof(filename), "%s/.g13/bindings-%d.properties", home_dir, bindings);

	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cout << "Config file not found: " << filename << std::endl;
		std::cout << "Loading default key bindings." << std::endl;

        const std::string default_bindings = R"RAW(
# Default G13 Key Bindings
G19=p,k.42
G18=p,k.18
G17=p,k.16
G16=p,k.10
G9=p,k.3
G15=p,k.9
G8=p,k.2
G14=p,k.8
G7=p,k.15
G13=p,k.7
G12=p,k.6
G6=p,k.46
G11=p,k.5
G5=p,k.76
G10=p,k.4
G4=p,k.75
G3=p,k.81
G2=p,k.80
G1=p,k.79
G0=p,k.1
G39=p,k.31
color=0,0,255
G38=p,k.32
G37=p,k.30
G36=p,k.17
G35=p,k.11
G34=p,k.72
G33=p,k.71
G32=p,k.62
G31=p,k.61
G30=p,k.60
G29=p,k.59
G23=p,k.58
G22=p,k.57
G21=p,k.57
G20=p,k.50
)RAW";
        
        std::stringstream ss(default_bindings);
        parse_bindings_from_stream(ss);
	}
    else {
        std::cout << "Loading config file: " << filename << std::endl;
        parse_bindings_from_stream(file);
        file.close();
    }
}

void G13::setColor(int red, int green, int blue) {
	int error;
	unsigned char usb_data[] = { 5, 0, 0, 0, 0 };
	usb_data[1] = red;
	usb_data[2] = green;
	usb_data[3] = blue;

	error = libusb_control_transfer(handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x307, 0,
			usb_data, 5, 1000);

	if (error != 5) {
		std::cerr << "Problem sending color data" << std::endl;
	}
}

int G13::read() {
	unsigned char buffer[G13_REPORT_SIZE];
	int size;
	int error = libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, buffer, G13_REPORT_SIZE, &size, 1000);

    // Specifically handle the "NO_DEVICE" error for hot-plugging.
    if (error == LIBUSB_ERROR_NO_DEVICE) {
        std::cerr << "G13 device disconnected." << std::endl;
        return -4; // Return a special code for a disconnected device.
    }
    
	if (error && error != LIBUSB_ERROR_TIMEOUT) {
		std::map<int, std::string> errors;
		errors[LIBUSB_SUCCESS] = "LIBUSB_SUCCESS";
		errors[LIBUSB_ERROR_IO] = "LIBUSB_ERROR_IO";
		errors[LIBUSB_ERROR_INVALID_PARAM] = "LIBUSB_ERROR_INVALID_PARAM";
		errors[LIBUSB_ERROR_ACCESS] = "LIBUSB_ERROR_ACCESS";
		errors[LIBUSB_ERROR_NO_DEVICE] = "LIBUSB_ERROR_NO_DEVICE";
		errors[LIBUSB_ERROR_NOT_FOUND] = "LIBUSB_ERROR_NOT_FOUND";
		errors[LIBUSB_ERROR_BUSY] = "LIBUSB_ERROR_BUSY";
		errors[LIBUSB_ERROR_TIMEOUT] = "LIBUSB_ERROR_TIMEOUT";
		errors[LIBUSB_ERROR_OVERFLOW] = "LIBUSB_ERROR_OVERFLOW";
		errors[LIBUSB_ERROR_PIPE] = "LIBUSB_ERROR_PIPE";
		errors[LIBUSB_ERROR_INTERRUPTED] = "LIBUSB_ERROR_INTERRUPTED";
		errors[LIBUSB_ERROR_NO_MEM] = "LIBUSB_ERROR_NO_MEM";
		errors[LIBUSB_ERROR_NOT_SUPPORTED] = "LIBUSB_ERROR_NOT_SUPPORTED";
		errors[LIBUSB_ERROR_OTHER] = "LIBUSB_ERROR_OTHER";
		std::cerr << "Error while reading keys: " << error << " (" << errors[error]
				<< ")" << std::endl;
		return -1;
	}

	if (size == G13_REPORT_SIZE) {
		parse_joystick(buffer);
		parse_keys(buffer);
		UInput::send_event(EV_SYN, SYN_REPORT, 0);
	}
	return 0;
}

void G13::parse_joystick(unsigned char *buf) {
	int stick_x = buf[1];
	int stick_y = buf[2];

	if (stick_mode == STICK_ABSOLUTE) {
		UInput::send_event(EV_ABS, ABS_X, stick_x);
		UInput::send_event(EV_ABS, ABS_Y, stick_y);
	} else if (stick_mode == STICK_KEYS) {
		int pressed[4];
		if (stick_y <= 96) {
			pressed[0] = 1; // UP
			pressed[3] = 0;
		}
		else if (stick_y >= 160) {
			pressed[0] = 0;
			pressed[3] = 1; // DOWN
		}
		else {
			pressed[0] = 0;
			pressed[3] = 0;
		}

		if (stick_x <= 96) {
			pressed[1] = 1; // LEFT
			pressed[2] = 0;
		}
		else if (stick_x >= 160) {
			pressed[1] = 0;
			pressed[2] = 1; // RIGHT
		}
		else {
			pressed[1] = 0;
			pressed[2] = 0;
		}

		int codes[4] = {36, 37, 38, 39};
		for (int i = 0; i < 4; i++) {
			int key = codes[i];
			int p = pressed[i];
			if (actions[key]->set(p)) {
				// State changed, action was triggered.
			}
		}
	}
}

void G13::parse_key(int key, unsigned char *byte) {
    if (key < 0 || key >= G13_NUM_KEYS) {
        std::cerr << "G13::parse_key: Invalid key index " << key << std::endl;
        return;
    }

	unsigned char actual_byte = byte[key / 8];
	unsigned char mask = 1 << (key % 8);
	int pressed = actual_byte & mask;

	switch (key) {
	// M1, M2, M3, MR keys switch the binding profile.
	case 25: // M1
	case 26: // M2
	case 27: // M3
	case 28: // MR
		if (pressed) {
			bindings = key - 25; // Profile index is 0, 1, 2, 3
			loadBindings();
		}
		return;

	// Stick keys are handled by parse_joystick, so we ignore them here.
	case 36:
	case 37:
	case 38:
	case 39:
		return;
	}

	// For all other keys, delegate to the assigned action.
    if (actions[key]) {
	    actions[key]->set(pressed);
    }
}

void G13::parse_keys(unsigned char *buf) {
	// The key data starts at byte 3 of the report.
	parse_key(G13_KEY_G1, buf + 3);
	parse_key(G13_KEY_G2, buf + 3);
	parse_key(G13_KEY_G3, buf + 3);
	parse_key(G13_KEY_G4, buf + 3);
	parse_key(G13_KEY_G5, buf + 3);
	parse_key(G13_KEY_G6, buf + 3);
	parse_key(G13_KEY_G7, buf + 3);
	parse_key(G13_KEY_G8, buf + 3);
	parse_key(G13_KEY_G9, buf + 3);
	parse_key(G13_KEY_G10, buf + 3);
	parse_key(G13_KEY_G11, buf + 3);
	parse_key(G13_KEY_G12, buf + 3);
	parse_key(G13_KEY_G13, buf + 3);
	parse_key(G13_KEY_G14, buf + 3);
	parse_key(G13_KEY_G15, buf + 3);
	parse_key(G13_KEY_G16, buf + 3);
	parse_key(G13_KEY_G17, buf + 3);
	parse_key(G13_KEY_G18, buf + 3);
	parse_key(G13_KEY_G19, buf + 3);
	parse_key(G13_KEY_G20, buf + 3);
	parse_key(G13_KEY_G21, buf + 3);
	parse_key(G13_KEY_G22, buf + 3);
	parse_key(G13_KEY_BD, buf + 3);
	parse_key(G13_KEY_L1, buf + 3);
	parse_key(G13_KEY_L2, buf + 3);
	parse_key(G13_KEY_L3, buf + 3);
	parse_key(G13_KEY_L4, buf + 3);
	parse_key(G13_KEY_M1, buf + 3);
	parse_key(G13_KEY_M2, buf + 3);
	parse_key(G13_KEY_M3, buf + 3);
	parse_key(G13_KEY_MR, buf + 3);
	parse_key(G13_KEY_LEFT, buf + 3);
	parse_key(G13_KEY_DOWN, buf + 3);
	parse_key(G13_KEY_TOP, buf + 3);
	parse_key(G13_KEY_LIGHT, buf + 3);
}

// --- Implementation of LCD control methods ---

void G13::clear_lcd_buffer() {
    memset(this->lcd_buffer, 0, G13_LCD_BUFFER_SIZE);
}

void G13::set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= 160 || y < 0 || y >= 48) {
        return;
    }

    int index = x + (y / 8) * 160;
    int bit = y % 8;

    if (on) {
        this->lcd_buffer[index] |= (1 << bit);
    } else {
        this->lcd_buffer[index] &= ~(1 << bit);
    }
}

void G13::write_lcd() {
    if (!this->loaded) return;

    unsigned char transfer_buffer[992];
    memset(transfer_buffer, 0, sizeof(transfer_buffer));

    transfer_buffer[0] = 0x03;

    memcpy(transfer_buffer + 32, this->lcd_buffer, G13_LCD_BUFFER_SIZE);

    int actual_length;
    int ret = libusb_bulk_transfer(
        this->handle,
        (G13_LCD_ENDPOINT | LIBUSB_ENDPOINT_OUT),
        transfer_buffer,
        sizeof(transfer_buffer),
        &actual_length,
        1000
    );

    if (ret < 0) {
        std::cerr << "Error writing to G13 LCD: " << libusb_error_name(ret) << std::endl;
    }
}