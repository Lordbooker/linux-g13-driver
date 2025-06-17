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
#include <algorithm> // For std::remove


#include "Constants.h"
#include "G13.h"
#include "G13Action.h"
#include "PassThroughAction.h"
#include "MacroAction.h"
#include "Output.h"

using namespace std;

// Helper to trim whitespace from both ends of a std::string
std::string trim_string(const std::string& str) {
    const std::string whitespace = " \t\n\r\f\v";
    size_t start = str.find_first_not_of(whitespace);
    if (start == std::string::npos)
        return ""; // no content

    size_t end = str.find_last_not_of(whitespace);
    return str.substr(start, end - start + 1);
}

// Overload for C-style strings (modifies in place, for compatibility with existing strtok use)
void trim_c_string(char *s) {
    char *start = s;
    while (isspace((unsigned char)*start)) start++;

    char *end = s + strlen(s) - 1;
    while (end > start && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';

    if (start != s) memmove(s, start, strlen(start) + 1);
}


G13::G13(libusb_device *device) {

	this->device = device;

	this->loaded = 0;

	this->bindings = 0;

	this->stick_mode = STICK_KEYS;

    actions.resize(G13_NUM_KEYS);
	for (int i = 0; i < G13_NUM_KEYS; i++) {
		actions[i] = std::make_unique<G13Action>();
	}

	if (libusb_open(device, &handle) != 0) {
		cerr << "Error opening G13 device" << endl;
		return;
	}

	if (libusb_kernel_driver_active(handle, 0) == 1) {
		if (libusb_detach_kernel_driver(handle, 0) == 0) {
			cout << "Kernel driver detached" << endl;
		}
	}

	if (libusb_claim_interface(handle, 0) < 0) {
		cerr << "Cannot Claim Interface" << endl;
		return;
	}

	setColor(128, 128, 128);

	this->loaded = 1;

}

G13::~G13() {
	if (!this->loaded) {
		return;
	}

	setColor(128, 128, 128);

	libusb_release_interface(this->handle, 0);
	libusb_close(this->handle);
    // actions vector with unique_ptr will clean itself up.

}

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
        cerr << "G13::loadMacro(" << num << ") HOME environment variable not set.\n";
        return nullptr;
    }

	snprintf(filename, sizeof(filename), "%s/.g13/macro-%d.properties", home_dir, num);
	//cout << "G13::loadMacro(" << num << ") filename=" << filename << "\n";
	ifstream file (filename);

	if (!file.is_open()) {
		cout << "Could not open config file: " << filename << "\n";
		return nullptr;
	}

	auto macro = std::make_unique<Macro>();
	macro->setId(num);
    std::string macro_name, macro_sequence;

	while (file.good()) {
		string line;
		getline(file, line);
        std::string trimmed_line = trim_string(line);

		if (!trimmed_line.empty() && trimmed_line[0] != '#') {
            size_t eq_pos = trimmed_line.find('=');
            if (eq_pos != std::string::npos) {
                std::string key = trim_string(trimmed_line.substr(0, eq_pos));
                std::string value = trim_string(trimmed_line.substr(eq_pos + 1));
			    //cout << "G13::loadMacro(" << num << ") key=" << key << ", value=" << value << "\n";
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

void G13::loadBindings() {

	char filename[1024];

	sprintf(filename, "%s/.g13/bindings-%d.properties", getenv("HOME"), bindings);
    const char* home_dir = getenv("HOME");
    if (!home_dir) {
        cerr << "G13::loadBindings() HOME environment variable not set.\n";
        return;
    }
	snprintf(filename, sizeof(filename), "%s/.g13/bindings-%d.properties", home_dir, bindings);

	  ifstream file (filename);
	  if (!file.is_open()) {
		  cout << "Could not open config file: " << filename << "\n";
		  setColor(128, 128, 128);
		  return;
	  }

	  while (file.good()) {
		  string line;
	      getline(file, line);

          // Use C-style char array for strtok compatibility if needed, or parse with std::string methods
	      char l[1024];
          strncpy(l, line.c_str(), sizeof(l) - 1);
          l[sizeof(l)-1] = '\0';

		  trim_c_string(l);
		  if (strlen(l) > 0 && l[0] != '#') { // Also check for comment lines here
			  char *key = strtok(l, "=");
			  if (key[0] == '#') {
				  // ignore line
			  }
			  else if (strcmp(key, "color") == 0) {
				  char *num = strtok(NULL, ",");
				  int r = atoi(num);
				  num = strtok(NULL, ",");
				  int g = atoi(num);
				  num = strtok(NULL, ",");
				  int b = atoi(num);

				  setColor(r, g, b);
			  }
			  else if (strcmp(key, "stick_mode") == 0) {

			  }
			  else if (key[0] == 'G') {
				  int gKey = atoi(&key[1]);
				  //cout << "gKey = " << gKey << "\n";
				  char *type = strtok(NULL, ",");
				  trim_c_string(type);
				  //cout << "type = " << type << "\n";
				  if (strcmp(type, "p") == 0) { /* passthrough */
					  char *keytype = strtok(NULL, ",\n ");
					  trim_c_string(keytype);
					  int keycode = atoi(&keytype[2]);

                      if (gKey >= 0 && gKey < G13_NUM_KEYS) {
					  //cout << "assigning G" << gKey << " to keycode " << keycode << "\n";
					      actions[gKey] = std::make_unique<PassThroughAction>(keycode);
                      }
				  }
				  else if (strcmp(type, "m") == 0) { /* macro */
					  int macroId = atoi(strtok(NULL, ",\n "));
					  int repeats = atoi(strtok(NULL, ",\n "));
					  auto macro = loadMacro(macroId); // Returns std::unique_ptr<Macro>
                      if (macro && gKey >= 0 && gKey < G13_NUM_KEYS) {
					      actions[gKey] = std::make_unique<MacroAction>(macro->getSequence());
					      static_cast<MacroAction*>(actions[gKey].get())->setRepeats(repeats);
                      } // macro unique_ptr goes out of scope and is deleted if not moved
				  }
				  else {
					  cout << "G13::loadBindings() unknown type '" << type << "\n";
				  }

			  }
			  else {
				  cout << "G13::loadBindings() Unknown first token: " << key << "\n";
			  }
		  }
	  }

	  file.close();
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
		cerr << "Problem sending data" << endl;
	}

}

int G13::read() {
	unsigned char buffer[G13_REPORT_SIZE];
	int size;
	int error = libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, buffer, G13_REPORT_SIZE, &size, 1000);
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
		errors[LIBUSB_ERROR_OTHER] = "LIBUSB_ERROR_OTHER    ";
		cerr << "Error while reading keys: " << error << " (" << errors[error]
				<< ")" << endl;
		cerr << "Stopping daemon" << endl;
		return -1;
	}

	if (size == G13_REPORT_SIZE) {
		parse_joystick(buffer);
		parse_keys(buffer);
		send_event(EV_SYN, SYN_REPORT, 0);
	}
	return 0;
}

void G13::parse_joystick(unsigned char *buf) {
	int stick_x = buf[1];
	int stick_y = buf[2];

	//cout << "stick = (" << stick_x << ", " << stick_y << ")\n";


	if (stick_mode == STICK_ABSOLUTE) {
		send_event(EV_ABS, ABS_X, stick_x);
		send_event(EV_ABS, ABS_Y, stick_y);
	} else if (stick_mode == STICK_KEYS) {

		// 36=up, 37=left, 38=right, 39=down
		int pressed[4];

		if (stick_y <= 96) {
			pressed[0] = 1;
			pressed[3] = 0;
		}
		else if (stick_y >= 160) {
			pressed[0] = 0;
			pressed[3] = 1;
		}
		else {
			pressed[0] = 0;
			pressed[3] = 0;
		}

		if (stick_x <= 96) {
			pressed[1] = 1;
			pressed[2] = 0;
		}
		else if (stick_x >= 160) {
			pressed[1] = 0;
			pressed[2] = 1;
		}
		else {
			pressed[1] = 0;
			pressed[2] = 0;
		}


		int codes[4] = {36, 37, 38, 39};
		for (int i = 0; i < 4; i++) {
			int key = codes[i];
			int p = pressed[i]; // p is 0 or 1
			if (actions[key]->set(p)) {
				//cout << "key " << key << ", pressed=" << p << ", actions[key]->isPressed()="
				//		<< actions[key]->isPressed() <<  ", x=" << stick_x << "\n";
			}
		}
	} else {
		/*    send_event(g13->uinput_file, EV_REL, REL_X, stick_x/16 - 8);
		 send_event(g13->uinput_file, EV_REL, REL_Y, stick_y/16 - 8);*/
	}

}
void G13::parse_key(int key, unsigned char *byte) {
    if (key < 0 || key >= G13_NUM_KEYS) {
        cerr << "G13::parse_key: Invalid key index " << key << endl;
        return;
    }

	unsigned char actual_byte = byte[key / 8];
	unsigned char mask = 1 << (key % 8);

	int pressed = actual_byte & mask;

	switch (key) {
	case 25: // key 25-28 are mapped to change bindings
	case 26:
	case 27:
	case 28:
		if (pressed) {
			//cout << "key " << key << "\n";
			bindings = key - 25;
			loadBindings();
		}
		return;

	case 36: // key 36-39 are mapped as joystick keys
	case 37:
	case 38:
	case 39:
		return;
	}

    if (actions[key]) { // Check if action exists
	    int changed = actions[key]->set(pressed);

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
	//  parse_key(G13_KEY_LIGHT_STATE, buf+3);

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
	//  parse_key(G13_KEY_LIGHT2, buf+3, file);
	/*  cout << hex << setw(2) << setfill('0') << (int)buf[7];
	 cout << hex << setw(2) << setfill('0') << (int)buf[6];
	 cout << hex << setw(2) << setfill('0') << (int)buf[5];
	 cout << hex << setw(2) << setfill('0') << (int)buf[4];
	 cout << hex << setw(2) << setfill('0') << (int)buf[3] << endl;*/
}
