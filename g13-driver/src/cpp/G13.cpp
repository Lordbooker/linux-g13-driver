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
#include <algorithm>
#include <sstream>
#include <istream>
#include <chrono> // For cooldown on checking file

#include "Constants.h"
#include "G13.h"
#include "G13Action.h"
#include "PassThroughAction.h"
#include "MacroAction.h"
#include "Output.h"

extern volatile sig_atomic_t daemon_keep_running;
const int G13_MAX_MACROS = 200;

std::string trim_string(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos) return "";
    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

G13::G13(libusb_device *device) {
	this->device = device;
	this->loaded = 0;
	this->bindings = 0;
	this->stick_mode = STICK_KEYS;
    this->last_config_mtime = 0;

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

    std::cout << "Initializing G13 display..." << std::endl;
    unsigned char lcd_init_payload[] = { 0x01 };
    libusb_control_transfer(handle,
        (LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE), 0x09, 0x0300, 0x0000,
        lcd_init_payload, sizeof(lcd_init_payload), 1000);

	setColor(128, 128, 128);
	clear_lcd_buffer();
	this->loaded = 1;
}

G13::~G13() {
	if (!this->loaded) return;
	libusb_release_interface(this->handle, 0);
	libusb_close(this->handle);
}

void G13::start() {
	if (!this->loaded) return;
	loadBindings();
	keepGoing = 1;

	while (keepGoing && daemon_keep_running) {
        // Feature: Auto-Reload Config
        check_for_config_update();

		if (read() == -4) {
            break; 
        }
	}
}

void G13::stop() {
	if (!this->loaded) return;
	keepGoing = 0;
}

// --- Live-Reload Implementation ---
void G13::check_for_config_update() {
    char filename[1024];
	const char* home_dir = getenv("HOME");
	if (!home_dir) return;

	snprintf(filename, sizeof(filename), "%s/.g13/bindings-%d.properties", home_dir, bindings);
    
    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {
        if (last_config_mtime != 0 && file_stat.st_mtime > last_config_mtime) {
            std::cout << "Config file change detected. Reloading..." << std::endl;
            loadBindings();
        }
    }
}

std::unique_ptr<Macro> G13::loadMacro(int num) {
	char filename[1024];
    const char* home_dir = getenv("HOME");
    if (!home_dir) return nullptr;

	snprintf(filename, sizeof(filename), "%s/.g13/macro-%d.properties", home_dir, num);
	std::ifstream file (filename);

	if (!file.is_open()) return nullptr;

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
			    if (key == "name") macro_name = value;
			    else if (key == "sequence") macro_sequence = value;
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

        if (trimmed_line.empty() || trimmed_line[0] == '#') continue;

        size_t eq_pos = trimmed_line.find('=');
        if (eq_pos == std::string::npos) continue;

        std::string key = trim_string(trimmed_line.substr(0, eq_pos));
        std::string value = trim_string(trimmed_line.substr(eq_pos + 1));

        if (key == "color") {
            std::stringstream ss(value);
            std::string segment;
            int r, g, b;
            if (std::getline(ss, segment, ',') && (r = std::stoi(segment)) >= 0 &&
                std::getline(ss, segment, ',') && (g = std::stoi(segment)) >= 0 &&
                std::getline(ss, segment, ',') && (b = std::stoi(segment)) >= 0) {
                if (r <= 255 && g <= 255 && b <= 255) setColor(r, g, b);
            }
        }
        else if (!key.empty() && key.rfind("G", 0) == 0) {
            try {
                int gKey = std::stoi(key.substr(1));
                std::stringstream ss(value);
                std::string type;
                if (!std::getline(ss, type, ',')) continue;
                type = trim_string(type);

                if (type == "p") { 
                    std::string keytype_str;
                    if (!std::getline(ss, keytype_str, ',')) continue;
                    keytype_str = trim_string(keytype_str);
                    if (keytype_str.rfind("k.", 0) == 0) {
                        int keycode = std::stoi(keytype_str.substr(2));
                        if (gKey >= 0 && gKey < G13_NUM_KEYS) {
                             actions[gKey] = std::make_unique<PassThroughAction>(keycode);
                        }
                    }
                }
                else if (type == "m") { 
                    std::string macroId_str, repeats_str;
                    if (!std::getline(ss, macroId_str, ',') || !std::getline(ss, repeats_str, ',')) continue;
                    int macroId = std::stoi(trim_string(macroId_str));
                    int repeats = std::stoi(trim_string(repeats_str));

                    if (macroId >= 0 && macroId < G13_MAX_MACROS) {
                        auto macro = loadMacro(macroId);
                        if (macro && gKey >= 0 && gKey < G13_NUM_KEYS) {
                            actions[gKey] = std::make_unique<MacroAction>(macro->getSequence());
                            static_cast<MacroAction*>(actions[gKey].get())->setRepeats(repeats);
                        }
                    }
                }
            } catch (...) {}
        }
    }
}

void G13::loadBindings() {
	char filename[1024];
	const char* home_dir = getenv("HOME");
	if (!home_dir) return;
    
	snprintf(filename, sizeof(filename), "%s/.g13/bindings-%d.properties", home_dir, bindings);

    // Update timestamp for Live-Reload
    struct stat file_stat;
    if (stat(filename, &file_stat) == 0) {
        last_config_mtime = file_stat.st_mtime;
    }

	std::ifstream file(filename);
	if (!file.is_open()) {
		std::cout << "Config file not found: " << filename << ". Loading defaults." << std::endl;
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
	unsigned char usb_data[] = { 5, (unsigned char)red, (unsigned char)green, (unsigned char)blue, 0 };
	libusb_control_transfer(handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 9, 0x307, 0, usb_data, 5, 1000);
}

int G13::read() {
	unsigned char buffer[G13_REPORT_SIZE];
	int size;
    // Timeout set to 100ms (0.1s) to allow frequent config checks without high CPU load
	int error = libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, buffer, G13_REPORT_SIZE, &size, 100);

    if (error == LIBUSB_ERROR_NO_DEVICE) {
        std::cerr << "G13 device disconnected." << std::endl;
        return -4; 
    }
    
	if (error && error != LIBUSB_ERROR_TIMEOUT) {
        // Log error only if it's not a timeout (timeout is normal now due to polling)
		std::cerr << "Error while reading keys: " << libusb_error_name(error) << std::endl;
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
		if (stick_y <= 96) { pressed[0] = 1; pressed[3] = 0; }
		else if (stick_y >= 160) { pressed[0] = 0; pressed[3] = 1; }
		else { pressed[0] = 0; pressed[3] = 0; }

		if (stick_x <= 96) { pressed[1] = 1; pressed[2] = 0; }
		else if (stick_x >= 160) { pressed[1] = 0; pressed[2] = 1; }
		else { pressed[1] = 0; pressed[2] = 0; }

		int codes[4] = {36, 37, 38, 39};
		for (int i = 0; i < 4; i++) {
			if (actions[codes[i]]) actions[codes[i]]->set(pressed[i]);
		}
	}
}

void G13::parse_key(int key, unsigned char *byte) {
    if (key < 0 || key >= G13_NUM_KEYS) return;

	unsigned char actual_byte = byte[key / 8];
	unsigned char mask = 1 << (key % 8);
	int pressed = actual_byte & mask;

	switch (key) {
	case 25: case 26: case 27: case 28:
		if (pressed) {
			bindings = key - 25; 
			loadBindings();
		}
		return;
	case 36: case 37: case 38: case 39:
		return;
	}

    if (actions[key]) {
	    actions[key]->set(pressed);
    }
}

void G13::parse_keys(unsigned char *buf) {
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

void G13::clear_lcd_buffer() {
    memset(this->lcd_buffer, 0, G13_LCD_BUFFER_SIZE);
}

void G13::set_pixel(int x, int y, bool on) {
    if (x < 0 || x >= 160 || y < 0 || y >= 48) return;
    int index = x + (y / 8) * 160;
    int bit = y % 8;
    if (on) this->lcd_buffer[index] |= (1 << bit);
    else this->lcd_buffer[index] &= ~(1 << bit);
}

void G13::write_lcd() {
    if (!this->loaded) return;
    unsigned char transfer_buffer[992];
    memset(transfer_buffer, 0, sizeof(transfer_buffer));
    transfer_buffer[0] = 0x03;
    memcpy(transfer_buffer + 32, this->lcd_buffer, G13_LCD_BUFFER_SIZE);
    int actual_length;
    libusb_bulk_transfer(this->handle, (G13_LCD_ENDPOINT | LIBUSB_ENDPOINT_OUT),
        transfer_buffer, sizeof(transfer_buffer), &actual_length, 1000);
}