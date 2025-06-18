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

using namespace std;

/**
 * @brief Helper function to trim whitespace from the beginning and end of a std::string.
 * @param str The string to trim.
 * @return The trimmed string.
 */
std::string trim_string(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return ""; // Return empty string if it contains only whitespace.

    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

/**
 * @brief Constructs a G13 object for a given USB device.
 * @param device Pointer to the libusb_device to manage.
 *
 * This constructor initializes member variables, opens the device handle, detaches
 * the kernel driver if necessary, and claims the interface. It sets a default
 * color for the LCD backlight.
 */
G13::G13(libusb_device *device) {
	this->device = device;
	this->loaded = 0;
	this->bindings = 0; // Default to the first binding profile (M1)
	this->stick_mode = STICK_KEYS;

    // Initialize action pointers for all keys.
    actions.resize(G13_NUM_KEYS);
	for (int i = 0; i < G13_NUM_KEYS; i++) {
		actions[i] = std::make_unique<G13Action>(); // Default to no-op action.
	}

	if (libusb_open(device, &handle) != 0) {
		cerr << "Error opening G13 device" << endl;
		return;
	}

	// Detach kernel driver if it's active.
	if (libusb_kernel_driver_active(handle, 0) == 1) {
		if (libusb_detach_kernel_driver(handle, 0) == 0) {
			cout << "Kernel driver detached" << endl;
		}
	}

	if (libusb_claim_interface(handle, 0) < 0) {
		cerr << "Cannot Claim Interface" << endl;
		return;
	}

	setColor(128, 128, 128); // Set a default backlight color.
	this->loaded = 1; // Mark as successfully loaded.
}

/**
 * @brief Destructor for the G13 class.
 *
 * Sets the backlight to a neutral color, releases the USB interface,
 * and closes the device handle.
 */
G13::~G13() {
	if (!this->loaded) {
		return;
	}
	setColor(128, 128, 128); // Reset color on exit.
	libusb_release_interface(this->handle, 0);
	libusb_close(this->handle);
}

/**
 * @brief Starts the main event reading loop for the G13 device.
 *
 * This method loads the initial key bindings and enters a loop that
 * continuously reads and processes key and joystick events from the device.
 */
void G13::start() {
	if (!this->loaded) {
		return;
	}
	loadBindings();
	keepGoing = 1;
	while (keepGoing) {
		read();
	}
}

/**
 * @brief Stops the main event reading loop.
 */
void G13::stop() {
	if (!this->loaded) {
		return;
	}
	keepGoing = 0;
}

/**
 * @brief Loads a macro definition from a properties file.
 * @param num The ID number of the macro to load.
 * @return A std::unique_ptr to the loaded Macro object, or nullptr on failure.
 */
std::unique_ptr<Macro> G13::loadMacro(int num) {
	char filename[1024];
    const char* home_dir = getenv("HOME");
    if (!home_dir) {
        cerr << "G13::loadMacro(" << num << ") HOME environment variable not set.\n";
        return nullptr;
    }

	snprintf(filename, sizeof(filename), "%s/.g13/macro-%d.properties", home_dir, num);
	ifstream file (filename);

	if (!file.is_open()) {
		cout << "Could not open config file: " << filename << "\n";
		return nullptr;
	}

	auto macro = std::make_unique<Macro>();
	macro->setId(num);
    std::string macro_name, macro_sequence;

	string line;
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


/**
 * @brief Parses key bindings from a given input stream.
 * @param stream The input stream (e.g., a file stream or string stream) to read from.
 *
 * This method reads a .properties-style format, parsing keys like "G12", "color", etc.,
 * and configures the corresponding actions (Passthrough, Macro) or device settings.
 */
void G13::parse_bindings_from_stream(std::istream& stream) {
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed_line = trim_string(line);

        if (trimmed_line.empty() || trimmed_line[0] == '#') {
            continue; // Skip empty lines and comments
        }

        size_t eq_pos = trimmed_line.find('=');
        if (eq_pos == std::string::npos) {
            continue; // Ignore lines without an '='
        }

        std::string key = trim_string(trimmed_line.substr(0, eq_pos));
        std::string value = trim_string(trimmed_line.substr(eq_pos + 1));

        if (key == "color") {
            // Parse "r,g,b" color string
            std::stringstream ss(value);
            std::string segment;
            int r, g, b;

            try {
                if (std::getline(ss, segment, ',') && (r = std::stoi(segment)) >= 0 &&
                    std::getline(ss, segment, ',') && (g = std::stoi(segment)) >= 0 &&
                    std::getline(ss, segment, ',') && (b = std::stoi(segment)) >= 0) {
                    setColor(r, g, b);
                }
            } catch (const std::exception& e) {
                 cerr << "G13::parse_bindings_from_stream() Invalid color format: " << value << endl;
            }
        }
        else if (key == "stick_mode") {
            // Future logic for stick mode can be placed here.
        }
        else if (!key.empty() && key[0] == 'G') {
            // Parse G-key assignments
            try {
                int gKey = std::stoi(key.substr(1));

                std::stringstream ss(value);
                std::string type;
                if (!std::getline(ss, type, ',')) continue;
                type = trim_string(type);

                if (type == "p") { /* passthrough */
                    std::string keytype_str;
                    if (!std::getline(ss, keytype_str, ',')) continue;
                    keytype_str = trim_string(keytype_str);

                    if (keytype_str.rfind("k.", 0) == 0) { // check if it starts with "k."
                        int keycode = std::stoi(keytype_str.substr(2));
                        if (gKey >= 0 && gKey < G13_NUM_KEYS) {
                            actions[gKey] = std::make_unique<PassThroughAction>(keycode);
                        }
                    }
                }
                else if (type == "m") { /* macro */
                    std::string macroId_str, repeats_str;
                    if (!std::getline(ss, macroId_str, ',') || !std::getline(ss, repeats_str, ',')) continue;
                    
                    int macroId = std::stoi(trim_string(macroId_str));
                    int repeats = std::stoi(trim_string(repeats_str));
                    auto macro = loadMacro(macroId);
                    if (macro && gKey >= 0 && gKey < G13_NUM_KEYS) {
                        actions[gKey] = std::make_unique<MacroAction>(macro->getSequence());
                        static_cast<MacroAction*>(actions[gKey].get())->setRepeats(repeats);
                    }
                }
                else {
                    cout << "G13::parse_bindings_from_stream() unknown type '" << type << "' for key " << key << "\n";
                }
            } catch (const std::invalid_argument& ia) {
                cerr << "G13::parse_bindings_from_stream() Invalid number format in line: " << trimmed_line << endl;
            } catch (const std::out_of_range& oor) {
                cerr << "G13::parse_bindings_from_stream() Number out of range in line: " << trimmed_line << endl;
            }
        }
        else {
            cout << "G13::parse_bindings_from_stream() Unknown token in key: " << key << "\n";
        }
    }
}

/**
 * @brief Loads the key binding configuration file for the current profile.
 *
 * If the user's configuration file is not found, it falls back to a hardcoded
 * default key mapping.
 */
void G13::loadBindings() {
	char filename[1024];
	const char* home_dir = getenv("HOME");
	if (!home_dir) {
		cerr << "G13::loadBindings() HOME environment variable not set.\n";
		return;
	}
	snprintf(filename, sizeof(filename), "%s/.g13/bindings-%d.properties", home_dir, bindings);

	ifstream file(filename);
	if (!file.is_open()) {
		cout << "Config file not found: " << filename << endl;
		cout << "Loading default key bindings." << endl;

        // Hardcoded default bindings as a fallback.
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
        cout << "Loading config file: " << filename << endl;
        parse_bindings_from_stream(file);
        file.close();
    }
}

/**
 * @brief Sets the RGB color of the G13's LCD backlight.
 * @param red Red component (0-255).
 * @param green Green component (0-255).
 * @param blue Blue component (0-255).
 */
void G13::setColor(int red, int green, int blue) {
	int error;
	unsigned char usb_data[] = { 5, 0, 0, 0, 0 };
	usb_data[1] = red;
	usb_data[2] = green;
	usb_data[3] = blue;

	error = libusb_control_transfer(handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x307, 0,
			usb_data, 5, 1000);

	if (error != 5) {
		cerr << "Problem sending data" << endl;
	}
}

/**
 * @brief Reads one report from the G13's interrupt endpoint.
 * @return 0 on success, -1 on a fatal error.
 *
 * This function performs a blocking read on the USB endpoint. If a report is
 * received, it dispatches it to the joystick and key parsing functions.
 */
int G13::read() {
	unsigned char buffer[G13_REPORT_SIZE];
	int size;
	int error = libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, buffer, G13_REPORT_SIZE, &size, 1000);
	if (error && error != LIBUSB_ERROR_TIMEOUT) {
		// Error handling and reporting.
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
		cerr << "Error while reading keys: " << error << " (" << errors[error]
				<< ")" << endl;
		cerr << "Stopping daemon" << endl;
		return -1;
	}

	if (size == G13_REPORT_SIZE) {
		parse_joystick(buffer);
		parse_keys(buffer);
		UInput::send_event(EV_SYN, SYN_REPORT, 0); // Send sync event after processing.
	}
	return 0;
}

/**
 * @brief Parses the joystick data from the G13 report.
 * @param buf The 8-byte report buffer received from the device.
 *
 * Depending on the stick_mode, this function either sends absolute joystick
 * coordinates or simulates key presses for the four stick directions.
 */
void G13::parse_joystick(unsigned char *buf) {
	int stick_x = buf[1];
	int stick_y = buf[2];

	if (stick_mode == STICK_ABSOLUTE) {
		// Send absolute X and Y events to uinput.
		UInput::send_event(EV_ABS, ABS_X, stick_x);
		UInput::send_event(EV_ABS, ABS_Y, stick_y);
	} else if (stick_mode == STICK_KEYS) {
		// Emulate key presses for stick directions.
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

		// G-key codes for the stick directions.
		int codes[4] = {36, 37, 38, 39};
		for (int i = 0; i < 4; i++) {
			int key = codes[i];
			int p = pressed[i];
			if (actions[key]->set(p)) {
				// State changed, action was triggered.
			}
		}
	} else {
		// Future mode, e.g., STICK_RELATIVE
	}
}

/**
 * @brief Parses a single key's state from the report buffer.
 * @param key The G-key index (0-39) to parse.
 * @param byte The pointer to the start of the key data in the report (byte 3).
 *
 * This function determines if a key is pressed by checking its corresponding bit
 * in the report. It handles special keys (M1-M3, MR) for binding switching and
 * dispatches regular key events to their configured G13Action object.
 */
void G13::parse_key(int key, unsigned char *byte) {
    if (key < 0 || key >= G13_NUM_KEYS) {
        cerr << "G13::parse_key: Invalid key index " << key << endl;
        return;
    }

	// Determine the state of the key's bit in the report.
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

/**
 * @brief Parses all keys from the G13 report buffer.
 * @param buf The 8-byte report buffer.
 *
 * This is a helper function that calls parse_key for every non-stick key on the device.
 */
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