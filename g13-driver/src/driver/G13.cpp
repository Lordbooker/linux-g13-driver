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
#include <chrono> 
#include <thread>
#include <syslog.h> 

#include "Constants.h"
#include "G13.h"
#include "G13Action.h"
#include "PassThroughAction.h"
#include "MacroAction.h"
#include "Output.h"
#include "Font.h"
#include "ConfigPath.h" // NEW: Include Helper
#include "IntroAnimation.h"

extern volatile sig_atomic_t daemon_keep_running;
const int G13_MAX_MACROS = 200;

// bmRequestType for HID class requests to the interface (LCD init, color).
// Built explicitly as uint8_t: combining two libusb enums of different types
// directly is deprecated since C++20 (-Wdeprecated-enum-enum-conversion).
constexpr uint8_t G13_HID_CLASS_REQUEST =
    static_cast<uint8_t>(LIBUSB_REQUEST_TYPE_CLASS) |
    static_cast<uint8_t>(LIBUSB_RECIPIENT_INTERFACE);

// After this many consecutive USB read errors (about 100 ms pause each) the
// device is released; the device thread opens it again afterwards.
const int G13_MAX_CONSECUTIVE_READ_ERRORS = 50;

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

    actions.resize(G13_NUM_KEYS);
    for (int i = 0; i < G13_NUM_KEYS; i++) {
        actions[i] = std::make_unique<G13Action>();
    }

    if (libusb_open(device, &handle) != 0) {
        syslog(LOG_ERR, "Error opening G13 device");
        return;
    }

    if (libusb_kernel_driver_active(handle, 0) == 1) {
        const int rc = libusb_detach_kernel_driver(handle, 0);
        if (rc == 0) {
            kernel_driver_detached = true;
            syslog(LOG_INFO, "Kernel driver detached");
        } else {
            syslog(LOG_WARNING, "Could not detach kernel driver: %s", libusb_error_name(rc));
        }
    }

    if (libusb_claim_interface(handle, 0) < 0) {
        syslog(LOG_ERR, "Cannot claim interface");
        reattach_kernel_driver();
        libusb_close(handle);
        handle = nullptr;
        return;
    }

    syslog(LOG_INFO, "Initializing G13 display...");
    unsigned char lcd_init_payload[] = { 0x01 };
    libusb_control_transfer(handle,
        G13_HID_CLASS_REQUEST, 0x09, 0x0300, 0x0000,
        lcd_init_payload, sizeof(lcd_init_payload), 1000);

    setColor(128, 128, 128);
    clear_lcd_buffer();
    this->loaded = 1;

    init_fifo();
}

G13::~G13() {
    stop_intro();
    cleanup_fifo(); 
    if (!this->loaded) return;
    libusb_release_interface(this->handle, 0);
    reattach_kernel_driver();
    libusb_close(this->handle);
}

// Reattach the kernel driver detached at startup (e.g. hid-lg-g15), so the G13
// keeps working after the driver stops, without replugging.
void G13::reattach_kernel_driver() {
    if (!kernel_driver_detached || !handle) return;
    kernel_driver_detached = false;
    const int rc = libusb_attach_kernel_driver(handle, 0);
    if (rc == 0)
        syslog(LOG_INFO, "Kernel driver reattached");
    else if (rc != LIBUSB_ERROR_NO_DEVICE)   // device unplugged: nothing to do
        syslog(LOG_WARNING, "Could not reattach kernel driver: %s", libusb_error_name(rc));
}

void G13::start() {
    if (!this->loaded) return;
    loadBindings();
    keepGoing = true;
    start_intro();

    while (keepGoing && daemon_keep_running) {
        check_for_config_update();
        check_fifo();

        if (read() == -4) {
            break; 
        }
    }
    stop_intro();
}

void G13::stop() {
    if (!this->loaded) return;
    keepGoing = false;
}

// --- Live-Reload Implementation ---
void G13::check_for_config_update() {
    // NEW: Use ConfigPath helper
    std::string filename = ConfigPath::getBindingPath(bindings);
    
    struct stat file_stat;
    if (stat(filename.c_str(), &file_stat) != 0)
        return;
    // Check for inequality instead of "newer": a clock set back or a file
    // created later also trigger a reload this way.
    const bool changed = !config_mtime_known
        || file_stat.st_mtim.tv_sec  != last_config_mtime.tv_sec
        || file_stat.st_mtim.tv_nsec != last_config_mtime.tv_nsec;
    if (changed) {
        syslog(LOG_INFO, "Config file change detected. Reloading...");
        loadBindings();
    }
}

void G13::remember_config_mtime(const std::string &filename) {
    struct stat file_stat;
    config_mtime_known = stat(filename.c_str(), &file_stat) == 0;
    if (config_mtime_known)
        last_config_mtime = file_stat.st_mtim;
}

std::unique_ptr<Macro> G13::loadMacro(int num) {
    // NEW: Use ConfigPath helper
    std::string filename = ConfigPath::getMacroPath(num);
    std::ifstream file(filename);

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
    // (Logic remains identical to previous version, omitted for brevity but preserved)
    std::string line;
    while (std::getline(stream, line)) {
        std::string trimmed_line = trim_string(line);
        if (trimmed_line.empty() || trimmed_line[0] == '#') continue;
        size_t eq_pos = trimmed_line.find('=');
        if (eq_pos == std::string::npos) continue;
        std::string key = trim_string(trimmed_line.substr(0, eq_pos));
        std::string value = trim_string(trimmed_line.substr(eq_pos + 1));

        if (key == "color") {
            // Invalid values are ignored (previously std::stoi threw an
            // exception outside the try block and terminated the daemon).
            try {
                std::stringstream ss(value);
                std::string segment;
                int rgb[3];
                int count = 0;
                while (count < 3 && std::getline(ss, segment, ',')) {
                    size_t used = 0;
                    const std::string s = trim_string(segment);
                    rgb[count] = std::stoi(s, &used);
                    if (used != s.size()) throw std::invalid_argument("color");
                    ++count;
                }
                if (count == 3 &&
                    rgb[0] >= 0 && rgb[0] <= 255 &&
                    rgb[1] >= 0 && rgb[1] <= 255 &&
                    rgb[2] >= 0 && rgb[2] <= 255) {
                    setColor(rgb[0], rgb[1], rgb[2]);
                } else {
                    syslog(LOG_WARNING, "Invalid color value ignored: %s", value.c_str());
                }
            } catch (...) {
                syslog(LOG_WARNING, "Invalid color value ignored: %s", value.c_str());
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

// Releases all pressed keys and removes all bindings.
// Called before every load:
//  - Keys that are not listed in the (new) profile have no function
//    (previously they kept the binding of the previous profile or of a line
//    removed from the file).
//  - Pressed keys are released so they do not get stuck in the system.
//  - Running macros are stopped when replaced (MacroAction destructor).
void G13::reset_actions() {
    for (auto &action : actions) {
        if (action && action->isPressed())
            action->set(0);
        action = std::make_unique<G13Action>();
    }
}

void G13::loadBindings() {
    reset_actions();

    // NEW: Use ConfigPath helper
    std::string filename = ConfigPath::getBindingPath(bindings);

    // Remember the timestamp BEFORE reading: if the file changes meanwhile,
    // check_for_config_update() notices it and loads again.
    remember_config_mtime(filename);

    std::ifstream file(filename);
    if (!file.is_open()) {
        syslog(LOG_WARNING, "Config file not found: %s. Creating defaults.", filename.c_str());
        
        // --- Create Default File ---
        // If the file doesn't exist, we write the default settings to it
        // so the user has something to edit in ~/.config/g13/
        std::ofstream outfile(filename);
        if (outfile.is_open()) {
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
G21=p,k.57
G20=p,k.50
)RAW";
            outfile << default_bindings;
            outfile.close();
            remember_config_mtime(filename);   // otherwise later changes have no effect
            
            // Now parse what we just wrote
            std::stringstream ss(default_bindings);
            parse_bindings_from_stream(ss);
        } else {
            syslog(LOG_ERR, "Could not create config file: %s", filename.c_str());
        }
    }
    else {
        syslog(LOG_INFO, "Loading config file: %s", filename.c_str());
        parse_bindings_from_stream(file);
        file.close();
    }
}

void G13::setColor(int red, int green, int blue) {
    unsigned char usb_data[] = { 5, (unsigned char)red, (unsigned char)green, (unsigned char)blue, 0 };
    libusb_control_transfer(handle, G13_HID_CLASS_REQUEST, 9, 0x307, 0, usb_data, 5, 1000);
}

int G13::read() {
    // (Existing read implementation)
    unsigned char buffer[G13_REPORT_SIZE];
    int size = 0;
    int error = libusb_interrupt_transfer(handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, buffer, G13_REPORT_SIZE, &size, 100);

    if (error == LIBUSB_ERROR_NO_DEVICE) {
        syslog(LOG_ERR, "G13 device disconnected.");
        return -4; 
    }
    
    if (error && error != LIBUSB_ERROR_TIMEOUT) {
        ++read_error_count;
        // Log only the first error of a series or a new error type.
        if (read_error_count == 1 || error != last_read_error) {
            syslog(LOG_ERR, "Error while reading keys: %s", libusb_error_name(error));
        }
        last_read_error = error;

        if (read_error_count >= G13_MAX_CONSECUTIVE_READ_ERRORS) {
            syslog(LOG_ERR, "%d consecutive read errors, releasing device (will be reopened).",
                   read_error_count);
            read_error_count = 0;
            last_read_error = 0;
            return -4;
        }
        // Short pause instead of an immediate retry (otherwise full CPU load and log flood).
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        return -1;
    }

    if (read_error_count > 0) {
        syslog(LOG_INFO, "Reading keys works again after %d error(s).", read_error_count);
        read_error_count = 0;
        last_read_error = 0;
    }

    if (size == G13_REPORT_SIZE) {
        parse_joystick(buffer);
        parse_keys(buffer);
        UInput::send_event(EV_SYN, SYN_REPORT, 0);
    }
    return 0;
}

void G13::parse_joystick(unsigned char *buf) {
    // (Existing implementation)
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
    // (Existing implementation)
    if (key < 0 || key >= G13_NUM_KEYS) return;

    unsigned char actual_byte = byte[key / 8];
    unsigned char mask = 1 << (key % 8);
    int pressed = actual_byte & mask;

    switch (key) {
    case 25: case 26: case 27: case 28: {
        // Switch only on press (edge). Previously the profile was reloaded with
        // every USB report while the key was held.
        const int index = key - 25;
        const bool down = pressed != 0;
        if (down && !profile_key_down[index]) {
            bindings = index;
            loadBindings();
        }
        profile_key_down[index] = down;
        return;
    }
    case 36: case 37: case 38: case 39:
        return;
    }

    if (actions[key]) {
        actions[key]->set(pressed);
    }
}

void G13::parse_keys(unsigned char *buf) {
    // (Existing implementation)
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

bool G13::write_lcd() {
    if (!this->loaded) return false;

    unsigned char transfer_buffer[992];
    memset(transfer_buffer, 0, sizeof(transfer_buffer));
    transfer_buffer[0] = 0x03; 

    memcpy(transfer_buffer + 32, this->lcd_buffer, G13_LCD_BUFFER_SIZE);

    int actual_length;
    int error = libusb_interrupt_transfer(
        this->handle, 
        G13_LCD_ENDPOINT | LIBUSB_ENDPOINT_OUT, 
        transfer_buffer, 
        sizeof(transfer_buffer), 
        &actual_length, 
        1000 
    );

    if (error) {
        syslog(LOG_ERR, "LCD write error: %s", libusb_error_name(error));
        return false;
    }
    return true;
}

// --- Start animation ---

// Global setting "intro" in settings.properties (default: on).
bool G13::intro_enabled() {
    std::ifstream file(ConfigPath::getSettingsPath());
    std::string line;
    bool enabled = true;
    while (std::getline(file, line)) {
        line = trim_string(line);
        if (line.empty() || line[0] == '#') continue;
        const size_t eq = line.find('=');
        if (eq == std::string::npos) continue;
        if (trim_string(line.substr(0, eq)) == "intro")
            enabled = trim_string(line.substr(eq + 1)) != "0";
    }
    return enabled;
}

void G13::draw_final_logo() {
    clear_lcd_buffer();
    for (int y = 0; y < IntroAnimation::Height; ++y)
        for (int x = 0; x < IntroAnimation::Width; ++x)
            set_pixel(x, y, IntroAnimation::finalPixel(x, y));
}

void G13::start_intro() {
    if (!intro_enabled()) {
        std::lock_guard<std::mutex> lock(lcd_mutex);
        draw_final_logo();
        write_lcd();
        return;
    }
    intro_stop = false;
    try {
        intro_thread = std::thread(&G13::run_intro, this);
    } catch (const std::system_error &e) {
        syslog(LOG_WARNING, "Could not start intro animation: %s", e.what());
        std::lock_guard<std::mutex> lock(lcd_mutex);
        draw_final_logo();
        write_lcd();
    }
}

void G13::stop_intro() {
    intro_stop = true;
    if (intro_thread.joinable())
        intro_thread.join();
}

void G13::run_intro() {
    IntroAnimation intro;
    uint8_t bits[IntroAnimation::Width * IntroAnimation::Height];
    const auto frame_time = std::chrono::milliseconds(1000 / IntroAnimation::Fps);
    auto next = std::chrono::steady_clock::now();

    for (int n = 0; n <= IntroAnimation::frameCount(); ++n) {
        if (intro_stop || !daemon_keep_running) return;
        if (n > 0) intro.step();
        intro.compose(bits);
        {
            std::lock_guard<std::mutex> lock(lcd_mutex);
            if (intro_stop) return;   // e.g. text from the LCD pipe came first
            for (int y = 0; y < IntroAnimation::Height; ++y)
                for (int x = 0; x < IntroAnimation::Width; ++x)
                    set_pixel(x, y, bits[y * IntroAnimation::Width + x] != 0);
            if (!write_lcd()) return;   // device gone or similar; error is already logged
        }
        next += frame_time;
        std::this_thread::sleep_until(next);
    }

    std::lock_guard<std::mutex> lock(lcd_mutex);
    if (!intro_stop) {
        draw_final_logo();
        write_lcd();
    }
}

void G13::write_char(int x, int y, char c) {
    // (Existing implementation)
    if (c < 32 || c > 126) c = 32; // font_5x7 contains the characters 32..126
    int font_index = (c - 32) * 5;
    for (int col = 0; col < 5; col++) {
        uint8_t line = font_5x7[font_index + col];
        for (int row = 0; row < 7; row++) {
            if (line & (1 << row)) {
                set_pixel(x + col, y + row, true);
            }
        }
    }
}

void G13::write_text(int x, int y, const std::string& text) {
    int cursor_x = x;
    for (char c : text) {
        write_char(cursor_x, y, c);
        cursor_x += 6; 
    }
}

void G13::init_fifo() {
    // NEW: Use ConfigPath helper
    fifo_path = ConfigPath::getFifoPath();
    fifo_fd = -1;

    unlink(fifo_path.c_str());

    // Only for the own user (usually located in /run/user/<uid>)
    if (mkfifo(fifo_path.c_str(), 0600) != 0) {
        syslog(LOG_ERR, "Failed to create FIFO at %s: %s", fifo_path.c_str(), strerror(errno));
        return;
    }
    
    chmod(fifo_path.c_str(), 0600);

    fifo_fd = ::open(fifo_path.c_str(), O_RDWR | O_NONBLOCK);
    
    if (fifo_fd < 0) {
        syslog(LOG_ERR, "Failed to open FIFO: %s", strerror(errno));
    } else {
        syslog(LOG_INFO, "LCD pipe created at %s", fifo_path.c_str());
    }
}

void G13::cleanup_fifo() {
    if (fifo_fd >= 0) {
        ::close(fifo_fd);
        fifo_fd = -1;
    }
    // fifo_path is empty if the constructor aborted before init_fifo().
    if (!fifo_path.empty()) {
        unlink(fifo_path.c_str());
    }
}

void G13::check_fifo() {
    if (fifo_fd < 0) return;

    char buffer[4096];
    ssize_t bytesRead = ::read(fifo_fd, buffer, sizeof(buffer) - 1);

    if (bytesRead > 0) {
        buffer[bytesRead] = '\0'; 
        std::string input(buffer);
        if (!input.empty() && input.back() == '\n') {
            input.pop_back();
        }

        stop_intro();   // text from the pipe takes precedence over the start animation
        std::lock_guard<std::mutex> lock(lcd_mutex);
        clear_lcd_buffer();
        
        std::stringstream ss(input);
        std::string line;
        int y = 0;
        int line_height = 8; 

        while (std::getline(ss, line)) {
            if (y + 7 > 48) break; 
            write_text(2, y, line); 
            y += line_height;
        }

        write_lcd();
    }
}