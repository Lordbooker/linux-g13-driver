#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <string_view>
#include <system_error>
#include <charconv>
#include <filesystem>
#include <memory>
#include <algorithm>
#include <array>

#include <libusb-1.0/libusb.h>
#include <linux/uinput.h>

#include "G13.h"
#include "Constants.h"
#include "G13Action.h"
#include "PassThroughAction.h"
#include "MacroAction.h"
#include "Output.h"

namespace {
    std::string_view trim_string_view(std::string_view str) {
        const auto* whitespace = " \t\n\r\f\v";
        const auto start = str.find_first_not_of(whitespace);
        if (start == std::string_view::npos) {
            return {};
        }
        const auto end = str.find_last_not_of(whitespace);
        return str.substr(start, end - start + 1);
    }
}

G13::G13(libusb_device *device)
    : _device(device),
      _handle(nullptr),
      _is_loaded(false),
      _current_bindings_profile(0), // Default to profile 0
      _stick_mode(stick_mode_t::STICK_ABSOLUTE) // Default stick mode
{
    for (auto& action : _actions) {
        action = std::make_unique<G13Action>();
    }

    if (libusb_open(_device, &_handle)!= 0) {
        std::cerr << "Error opening G13 device" << std::endl;
        return;
    }

    if (libusb_kernel_driver_active(_handle, G13_INTERFACE) == 1) {
        if (libusb_detach_kernel_driver(_handle, G13_INTERFACE) == 0) {
            std::cout << "Kernel driver detached" << std::endl;
        }
    }

    if (libusb_claim_interface(_handle, G13_INTERFACE) < 0) {
        std::cerr << "Cannot Claim Interface" << std::endl;
        libusb_close(_handle);
        _handle = nullptr;
        return;
    }

    setColor(128, 128, 128);
    _is_loaded = true;
}

G13::~G13() {
    if (!_is_loaded ||!_handle) {
        return;
    }
    setColor(128, 128, 128);
    libusb_release_interface(_handle, G13_INTERFACE);
    libusb_close(_handle);
}

void G13::start() {
    if (!_is_loaded) {
        return;
    }
    loadBindings();
    _keep_running.store(true, std::memory_order_relaxed);
    read_loop();
}

void G13::stop() {
    _keep_running.store(false, std::memory_order_relaxed);
}

std::unique_ptr<Macro> G13::loadMacro(int num) {
    const char* home_dir_cstr = getenv("HOME");
    if (!home_dir_cstr) {
        std::cerr << "G13::loadMacro(" << num << ") HOME environment variable not set.\n";
        return nullptr;
    }

    std::filesystem::path file_path(home_dir_cstr);
    file_path /= ".g13";
    file_path /= "macro-" + std::to_string(num) + ".properties";

    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cerr << "Could not open config file: " << file_path << "\n";
        return nullptr;
    }

    auto macro = std::make_unique<Macro>();
    macro->setId(num);
    std::string macro_name, macro_sequence;
    std::string line;

    while (std::getline(file, line)) {
        auto trimmed_line = trim_string_view(line);
        if (trimmed_line.empty() || trimmed_line.front() == '#') {
            continue;
        }

        auto eq_pos = trimmed_line.find('=');
        if (eq_pos!= std::string_view::npos) {
            auto key = trim_string_view(trimmed_line.substr(0, eq_pos));
            auto value = trim_string_view(trimmed_line.substr(eq_pos + 1));
            
            if (key == "name") {
                macro_name = value;
            } else if (key == "sequence") {
                macro_sequence = value;
            }
        }
    }
    macro->setName(macro_name);
    macro->setSequence(macro_sequence);
    return macro;
}

void G13::loadBindings() {
    const char* home_dir_cstr = getenv("HOME");
    if (!home_dir_cstr) {
        std::cerr << "G13::loadBindings() HOME environment variable not set.\n";
        return;
    }

    std::filesystem::path file_path(home_dir_cstr);
    file_path /= ".g13";
    file_path /= "bindings-" + std::to_string(_current_bindings_profile) + ".properties";

    std::ifstream file(file_path);
    if (!file.is_open()) {
        std::cout << "Could not open config file: " << file_path << "\n";
        setColor(128, 128, 128);
        return;
    }

    std::string line;
    while (std::getline(file, line)) {
        auto trimmed_line = trim_string_view(line);
        if (trimmed_line.empty() || trimmed_line.front() == '#') {
            continue;
        }

        auto eq_pos = trimmed_line.find('=');
        if (eq_pos == std::string_view::npos) continue;

        auto key_sv = trim_string_view(trimmed_line.substr(0, eq_pos));
        auto value_sv = trim_string_view(trimmed_line.substr(eq_pos + 1));
        
        if (key_sv == "color") {
            uint8_t r = 0, g = 0, b = 0;
            size_t first_comma = value_sv.find(',');
            size_t second_comma = (first_comma == std::string_view::npos) ? std::string_view::npos : value_sv.find(',', first_comma + 1);

            if (first_comma != std::string_view::npos && second_comma != std::string_view::npos) {
                std::string_view r_sv = trim_string_view(value_sv.substr(0, first_comma));
                std::string_view g_sv = trim_string_view(value_sv.substr(first_comma + 1, second_comma - (first_comma + 1)));
                std::string_view b_sv = trim_string_view(value_sv.substr(second_comma + 1));

                bool r_ok = std::from_chars(r_sv.data(), r_sv.data() + r_sv.size(), r).ec == std::errc();
                bool g_ok = std::from_chars(g_sv.data(), g_sv.data() + g_sv.size(), g).ec == std::errc();
                bool b_ok = std::from_chars(b_sv.data(), b_sv.data() + b_sv.size(), b).ec == std::errc();

                if (r_ok && g_ok && b_ok) {
                    setColor(r, g, b);
                } else {
                    std::cerr << "G13::loadBindings() failed to parse color components: " << value_sv << std::endl;
                }
            } else {
                std::cerr << "G13::loadBindings() invalid color format (expected r,g,b): " << value_sv << std::endl;
            }
        } else if (key_sv == "stick_mode") {
            if (value_sv == "keys") _stick_mode = stick_mode_t::STICK_KEYS;
            else if (value_sv == "absolute") _stick_mode = stick_mode_t::STICK_ABSOLUTE;
            else std::cerr << "G13::loadBindings() unknown stick_mode: " << value_sv << std::endl;
        } else if (!key_sv.empty() && key_sv.front() == 'G') {
            int gKey_num = 0;
            auto [ptr, ec] = std::from_chars(key_sv.data() + 1, key_sv.data() + key_sv.size(), gKey_num);
            if (ec!= std::errc()) continue;

            size_t gKey_idx = gKey_num - 1;
            if (gKey_idx >= G13_NUM_KEYS) continue;

            std::string_view value_parser = value_sv;
            auto comma_pos = value_parser.find(',');
            std::string_view type = value_parser.substr(0, comma_pos);
            value_parser.remove_prefix(comma_pos!= std::string_view::npos? comma_pos + 1 : value_parser.size());

            if (type == "p") {
                if (value_parser.rfind("kc.", 0) == 0) {
                    std::string_view keycode_sv = value_parser.substr(3);
                    int keycode = 0;
                    if (std::from_chars(keycode_sv.data(), keycode_sv.data() + keycode_sv.size(), keycode).ec == std::errc()) {
                        _actions[gKey_idx] = std::make_unique<PassThroughAction>(keycode);
                    } else {
                        std::cerr << "G13::loadBindings() failed to parse keycode for G" << gKey_num << ": " << value_parser << std::endl;
                        _actions[gKey_idx] = std::make_unique<G13Action>(); // Default action
                    }
                }
            } else if (type == "m") {
                comma_pos = value_parser.find(',');
                auto macroId_sv = value_parser.substr(0, comma_pos);
                value_parser.remove_prefix(comma_pos!= std::string_view::npos? comma_pos + 1 : value_parser.size());
                auto repeats_sv = value_parser;

                int macroId = 0, repeats = 0;
                bool macroId_ok = std::from_chars(macroId_sv.data(), macroId_sv.data() + macroId_sv.size(), macroId).ec == std::errc();
                bool repeats_ok = std::from_chars(repeats_sv.data(), repeats_sv.data() + repeats_sv.size(), repeats).ec == std::errc();

                if (macroId_ok && repeats_ok) {
                    auto macro = loadMacro(macroId);
                    if (macro) {
                        _actions[gKey_idx] = std::make_unique<MacroAction>(macro->getSequence());
                        static_cast<MacroAction*>(_actions[gKey_idx].get())->setRepeats(repeats);
                    } else {
                        // loadMacro prints its own error
                        _actions[gKey_idx] = std::make_unique<G13Action>(); // Default action
                    }
                } else {
                    std::cerr << "G13::loadBindings() failed to parse macroId or repeats for G" << gKey_num << ": "
                              << "id_sv='" << macroId_sv << "', repeats_sv='" << repeats_sv << "'" << std::endl;
                    _actions[gKey_idx] = std::make_unique<G13Action>(); // Default action
                }
            }
        }
    }
}

// Sets the G13's LED color.
// Uses USB Feature Report ID 7 (0x307 in wValue).
// The data payload is 4 bytes: {Red, Green, Blue, Padding/Unused}.
void G13::setColor(uint8_t red, uint8_t green, uint8_t blue) {
    std::array<unsigned char, 4> usb_data = { red, green, blue, 0x00 /* Padding or unused */ };
    int bytes_transferred = libusb_control_transfer(_handle, LIBUSB_REQUEST_TYPE_CLASS | LIBUSB_RECIPIENT_INTERFACE, 0x09, 0x0307, 0, usb_data.data(), usb_data.size(), 1000);

    if (bytes_transferred!= static_cast<int>(usb_data.size())) {
        std::cerr << "Problem sending color data" << std::endl;
    }
}

void G13::read_loop() {
    std::array<unsigned char, G13_REPORT_SIZE> buffer;
    int size;

    while (_keep_running.load(std::memory_order_relaxed)) {
        int error = libusb_interrupt_transfer(_handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, buffer.data(), buffer.size(), &size, G13_KEY_READ_TIMEOUT_MS);
        
        if (error == LIBUSB_ERROR_TIMEOUT) {
            continue;
        }
        if (error!= LIBUSB_SUCCESS) {
            std::cerr << "Error while reading keys: " << libusb_error_name(error) << std::endl;
            stop();
            return;
        }

        if (size == G13_REPORT_SIZE) {
            parse_joystick(buffer.data());
            parse_keys(buffer.data());
            send_event(EV_SYN, SYN_REPORT, 0);
        }
    }
}

void G13::parse_joystick(const unsigned char *buf) {
    // buf[0] is likely a report ID
    // buf[1] is joystick X-axis value (0-255)
    // buf[2] is joystick Y-axis value (0-255)
    int stick_x = buf[1];
    int stick_y = buf[2];

    if (_stick_mode == stick_mode_t::STICK_ABSOLUTE) {
        send_event(EV_ABS, ABS_X, stick_x);
        send_event(EV_ABS, ABS_Y, stick_y);
    } else if (_stick_mode == stick_mode_t::STICK_KEYS) {
        // This logic can be refined, but the core idea is kept
        // If STICK_KEYS mode is implemented, stick_x and stick_y (or raw buf values)
        // would be used here to determine directional key presses.
    }
}

void G13::parse_key(G13_KEYS key, const unsigned char *byte) {
    size_t key_index = static_cast<size_t>(key);
    if (key_index >= G13_NUM_KEYS) {
        return;
    }

    unsigned char actual_byte = byte[key_index / 8];
    unsigned char mask = 1 << (key_index % 8);
    bool pressed = (actual_byte & mask)!= 0;

    if (key >= G13_KEYS::L1 && key <= G13_KEYS::L4) {
        if (pressed) {
            int new_profile = static_cast<int>(key) - static_cast<int>(G13_KEYS::L1);
            if (new_profile!= _current_bindings_profile) {
                _current_bindings_profile = new_profile;
                loadBindings();
            }
        }
        return;
    }

    if (_actions[key_index]) {
        _actions[key_index]->set(pressed);
    }
}

void G13::parse_keys(const unsigned char *buf) {
    for (int i = 0; i <= static_cast<int>(G13_KEYS::MISC_TOGGLE); ++i) {
        parse_key(static_cast<G13_KEYS>(i), buf + 3);
    }
}