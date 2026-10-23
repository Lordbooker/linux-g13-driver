#include "G13.h"

#include "ConfigPath.h"
#include "Font.h"
#include "IntroAnimation.h"
#include "MacroAction.h"
#include "Output.h"
#include "PassThroughAction.h"

#include <fcntl.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string_view>
#include <system_error>
#include <vector>

namespace {

constexpr int G13_MAX_MACROS = 200;   // macro-0 .. macro-199

// LCD geometry: 160 x 43 visible pixels; the buffer has 6 pages of 8 rows.
constexpr int LCD_WIDTH = 160;
constexpr int LCD_BUFFER_ROWS = 48;
constexpr int LCD_LINE_HEIGHT = 8;
constexpr int LCD_CHAR_WIDTH = FONT_COLUMNS + 1;

// LCD transfer: report 0x03, 31 unused header bytes, then the image.
constexpr unsigned char LCD_REPORT_ID = 0x03;
constexpr std::size_t LCD_HEADER_SIZE = 32;
constexpr std::size_t LCD_TRANSFER_SIZE = LCD_HEADER_SIZE + G13_LCD_BUFFER_SIZE;

// HID class requests to the interface (LCD init, backlight color).
// Built explicitly as uint8_t: combining two libusb enums of different types
// directly is deprecated since C++20 (-Wdeprecated-enum-enum-conversion).
constexpr std::uint8_t HID_CLASS_REQUEST =
    static_cast<std::uint8_t>(LIBUSB_REQUEST_TYPE_CLASS) |
    static_cast<std::uint8_t>(LIBUSB_RECIPIENT_INTERFACE);
constexpr std::uint8_t HID_SET_REPORT = 0x09;
constexpr std::uint16_t LCD_INIT_REPORT = 0x0300;
constexpr std::uint16_t BACKLIGHT_REPORT = 0x0307;
constexpr unsigned int USB_TIMEOUT_MS = 1000;
constexpr unsigned int KEY_READ_TIMEOUT_MS = 100;

// After this many consecutive USB read errors (about 100 ms pause each) the
// device is released; the device thread opens it again afterwards.
constexpr int MAX_CONSECUTIVE_READ_ERRORS = 50;

// Joystick: positions 0..255 per axis; outside of 97..159 a direction counts as pressed.
constexpr int STICK_LOW = 96;
constexpr int STICK_HIGH = 160;

// Input report: byte 1/2 = stick X/Y, bytes 3..7 = key bits 0..39 (see Constants.h).
constexpr std::size_t REPORT_STICK_X = 1;
constexpr std::size_t REPORT_STICK_Y = 2;
constexpr std::size_t REPORT_KEY_OFFSET = 3;

constexpr const char *DEFAULT_BINDINGS = R"(# Default G13 key bindings
G0=p,k.1
G1=p,k.79
G2=p,k.80
G3=p,k.81
G4=p,k.75
G5=p,k.76
G6=p,k.46
G7=p,k.15
G8=p,k.2
G9=p,k.3
G10=p,k.4
G11=p,k.5
G12=p,k.6
G13=p,k.7
G14=p,k.8
G15=p,k.9
G16=p,k.10
G17=p,k.16
G18=p,k.18
G19=p,k.42
G20=p,k.50
G21=p,k.57
G29=p,k.59
G30=p,k.60
G31=p,k.61
G32=p,k.62
G33=p,k.71
G34=p,k.72
G35=p,k.11
G36=p,k.17
G37=p,k.30
G38=p,k.32
G39=p,k.31
color=0,0,255
)";

std::string trim(std::string_view text)
{
    constexpr std::string_view whitespace = " \t\n\r\f\v";
    const auto first = text.find_first_not_of(whitespace);
    if (first == std::string_view::npos)
        return {};
    const auto last = text.find_last_not_of(whitespace);
    return std::string(text.substr(first, last - first + 1));
}

// Parses the whole text as a decimal integer.
bool parseInt(std::string_view text, int &value)
{
    const char *last = text.data() + text.size();
    const auto [end, error] = std::from_chars(text.data(), last, value);
    return error == std::errc() && end == last && !text.empty();
}

// Splits "key=value" (both trimmed); false for empty lines, comments and lines without '='.
bool splitProperty(const std::string &line, std::string &key, std::string &value)
{
    const std::string trimmed = trim(line);
    if (trimmed.empty() || trimmed.front() == '#')
        return false;
    const auto eq = trimmed.find('=');
    if (eq == std::string::npos)
        return false;
    key = trim(std::string_view(trimmed).substr(0, eq));
    value = trim(std::string_view(trimmed).substr(eq + 1));
    return true;
}

std::vector<std::string> splitList(const std::string &text)
{
    std::vector<std::string> parts;
    std::stringstream stream(text);
    std::string part;
    while (std::getline(stream, part, ','))
        parts.push_back(trim(part));
    return parts;
}

// Text for errno (thread-safe, unlike strerror()).
std::string errnoText()
{
    return std::generic_category().message(errno);
}

bool isProfileKey(int key)
{
    return key >= G13_KEY_L1 && key <= G13_KEY_L4;
}

bool isBindableKey(int key)
{
    return (key >= G13_KEY_G1 && key <= G13_KEY_G22) || key == G13_KEY_BD
        || (key >= G13_KEY_M1 && key <= G13_STICK_DOWN);
}

} // namespace

// ---------------------------------------------------------------------------
// Device
// ---------------------------------------------------------------------------

G13::G13(libusb_device *device, const std::atomic<bool> &keepRunning)
    : m_keepRunning(keepRunning)
{
    reset_actions();

    const int openResult = libusb_open(device, &m_handle);
    if (openResult != 0) {
        syslog(LOG_ERR, "Error opening G13 device: %s", libusb_error_name(openResult));
        m_handle = nullptr;
        return;
    }

    if (libusb_kernel_driver_active(m_handle, G13_INTERFACE) == 1) {
        const int rc = libusb_detach_kernel_driver(m_handle, G13_INTERFACE);
        if (rc == 0) {
            m_kernelDriverDetached = true;
            syslog(LOG_INFO, "Kernel driver detached");
        } else {
            syslog(LOG_WARNING, "Could not detach kernel driver: %s", libusb_error_name(rc));
        }
    }

    const int claimResult = libusb_claim_interface(m_handle, G13_INTERFACE);
    if (claimResult != 0) {
        syslog(LOG_ERR, "Cannot claim interface: %s", libusb_error_name(claimResult));
        reattach_kernel_driver();
        libusb_close(m_handle);
        m_handle = nullptr;
        return;
    }

    syslog(LOG_INFO, "Initializing G13 display...");
    std::array<unsigned char, 1> lcdInit = {0x01};
    const int initResult = libusb_control_transfer(
        m_handle, HID_CLASS_REQUEST, HID_SET_REPORT, LCD_INIT_REPORT, G13_INTERFACE,
        lcdInit.data(), static_cast<std::uint16_t>(lcdInit.size()), USB_TIMEOUT_MS);
    if (initResult < 0)
        syslog(LOG_WARNING, "LCD initialization failed: %s", libusb_error_name(initResult));

    m_loaded = true;
    setColor(128, 128, 128);
    init_fifo();
}

G13::~G13()
{
    stop_intro();
    cleanup_fifo();
    reset_actions();   // release held keys and stop macros (e.g. device unplugged while a key was down)
    if (!m_loaded)
        return;
    libusb_release_interface(m_handle, G13_INTERFACE);
    reattach_kernel_driver();
    libusb_close(m_handle);
}

// Reattach the kernel driver detached at startup (e.g. hid-lg-g15), so the G13
// keeps working after the driver stops, without replugging.
void G13::reattach_kernel_driver()
{
    if (!m_kernelDriverDetached || m_handle == nullptr)
        return;
    m_kernelDriverDetached = false;
    const int rc = libusb_attach_kernel_driver(m_handle, G13_INTERFACE);
    if (rc == 0)
        syslog(LOG_INFO, "Kernel driver reattached");
    else if (rc != LIBUSB_ERROR_NO_DEVICE)   // device unplugged: nothing to do
        syslog(LOG_WARNING, "Could not reattach kernel driver: %s", libusb_error_name(rc));
}

void G13::start()
{
    if (!m_loaded)
        return;
    loadBindings();
    start_intro();

    while (m_keepRunning) {
        check_for_config_update();
        check_fifo();
        if (read() == ReadResult::Stop)
            break;
    }
    stop_intro();
}

bool G13::setColor(int red, int green, int blue)
{
    std::array<unsigned char, 5> report = {5, static_cast<unsigned char>(red),
                                           static_cast<unsigned char>(green),
                                           static_cast<unsigned char>(blue), 0};
    const int result = libusb_control_transfer(
        m_handle, HID_CLASS_REQUEST, HID_SET_REPORT, BACKLIGHT_REPORT, G13_INTERFACE,
        report.data(), static_cast<std::uint16_t>(report.size()), USB_TIMEOUT_MS);
    if (result < 0) {
        syslog(LOG_WARNING, "Could not set the backlight color: %s", libusb_error_name(result));
        return false;
    }
    return true;
}

G13::ReadResult G13::read()
{
    std::array<unsigned char, G13_REPORT_SIZE> report{};
    int size = 0;
    const int error = libusb_interrupt_transfer(
        m_handle, LIBUSB_ENDPOINT_IN | G13_KEY_ENDPOINT, report.data(),
        static_cast<int>(report.size()), &size, KEY_READ_TIMEOUT_MS);

    if (error == LIBUSB_ERROR_NO_DEVICE) {
        syslog(LOG_ERR, "G13 device disconnected.");
        return ReadResult::Stop;
    }

    if (error != 0 && error != LIBUSB_ERROR_TIMEOUT) {
        ++m_readErrorCount;
        // Log only the first error of a series or a new error type.
        if (m_readErrorCount == 1 || error != m_lastReadError)
            syslog(LOG_ERR, "Error while reading keys: %s", libusb_error_name(error));
        m_lastReadError = error;

        if (m_readErrorCount >= MAX_CONSECUTIVE_READ_ERRORS) {
            syslog(LOG_ERR, "%d consecutive read errors, releasing device (will be reopened).",
                   m_readErrorCount);
            m_readErrorCount = 0;
            m_lastReadError = 0;
            return ReadResult::Stop;
        }
        // Short pause instead of an immediate retry (otherwise full CPU load and log flood).
        std::this_thread::sleep_for(std::chrono::milliseconds(KEY_READ_TIMEOUT_MS));
        return ReadResult::Retry;
    }

    if (m_readErrorCount > 0) {
        syslog(LOG_INFO, "Reading keys works again after %d error(s).", m_readErrorCount);
        m_readErrorCount = 0;
        m_lastReadError = 0;
    }

    if (static_cast<std::size_t>(size) == report.size()) {
        parse_joystick(report.data());
        parse_keys(report.data());
    }
    return ReadResult::Ok;
}

// ---------------------------------------------------------------------------
// Keys and bindings
// ---------------------------------------------------------------------------

void G13::parse_joystick(const unsigned char *report)
{
    const int x = report[REPORT_STICK_X];
    const int y = report[REPORT_STICK_Y];
    set_key(G13_STICK_UP, y <= STICK_LOW);
    set_key(G13_STICK_DOWN, y >= STICK_HIGH);
    set_key(G13_STICK_LEFT, x <= STICK_LOW);
    set_key(G13_STICK_RIGHT, x >= STICK_HIGH);
}

void G13::parse_keys(const unsigned char *report)
{
    for (int key = 0; key <= G13_KEY_TOP; ++key) {
        const unsigned byte = report[REPORT_KEY_OFFSET + static_cast<std::size_t>(key / 8)];
        const bool pressed = ((byte >> static_cast<unsigned>(key % 8)) & 1U) != 0;

        if (isProfileKey(key)) {
            // Switch only on press (edge), not with every report while the key is held.
            const auto index = static_cast<std::size_t>(key - G13_KEY_L1);
            if (pressed && !m_profileKeyDown.at(index)) {
                m_profile = static_cast<int>(index);
                loadBindings();
            }
            m_profileKeyDown.at(index) = pressed;
        } else if (isBindableKey(key)) {
            set_key(key, pressed);
        }
    }
}

void G13::set_key(int key, bool pressed)
{
    const auto &action = m_actions.at(static_cast<std::size_t>(key));
    if (action)
        action->set(pressed);
}

// Releases all pressed keys and removes all bindings. Called before every load:
//  - keys that are not listed in the (new) profile have no function,
//  - pressed keys are released so they do not get stuck in the system,
//  - running macros are stopped (MacroAction destructor).
void G13::reset_actions()
{
    for (auto &action : m_actions) {
        if (action && action->isPressed())
            action->set(false);
        action = std::make_unique<G13Action>();
    }
}

void G13::loadBindings()
{
    reset_actions();
    const std::string filename = ConfigPath::getBindingPath(m_profile);

    // Remember the timestamp BEFORE reading: if the file changes meanwhile,
    // check_for_config_update() notices it and loads again.
    remember_config_mtime(filename);

    std::ifstream file(filename);
    if (file.is_open()) {
        syslog(LOG_INFO, "Loading config file: %s", filename.c_str());
        parse_bindings_from_stream(file);
        return;
    }

    // No file yet: write the defaults so the user has something to edit.
    syslog(LOG_WARNING, "Config file not found: %s. Creating defaults.", filename.c_str());
    std::ofstream out(filename);
    if (out << DEFAULT_BINDINGS) {
        out.close();
        remember_config_mtime(filename);   // otherwise later changes have no effect
    } else {
        syslog(LOG_ERR, "Could not create config file: %s", filename.c_str());
    }
    std::stringstream defaults(DEFAULT_BINDINGS);
    parse_bindings_from_stream(defaults);
}

void G13::parse_bindings_from_stream(std::istream &stream)
{
    std::string line;
    std::string key;
    std::string value;
    while (std::getline(stream, line)) {
        if (!splitProperty(line, key, value))
            continue;
        if (!parse_binding(key, value))
            syslog(LOG_WARNING, "Invalid binding ignored: %s", trim(line).c_str());
    }
}

bool G13::parse_binding(const std::string &key, const std::string &value)
{
    if (key == "color") {
        const auto parts = splitList(value);
        std::array<int, 3> rgb{};
        if (parts.size() != rgb.size())
            return false;
        for (std::size_t i = 0; i < rgb.size(); ++i) {
            if (!parseInt(parts[i], rgb.at(i)) || rgb.at(i) < 0 || rgb.at(i) > 255)
                return false;
        }
        setColor(rgb[0], rgb[1], rgb[2]);
        return true;
    }

    int gKey = -1;
    if (!key.starts_with('G') || !parseInt(std::string_view(key).substr(1), gKey) || gKey < 0
        || gKey >= G13_NUM_KEYS)
        return false;
    if (!isBindableKey(gKey))
        return true;   // e.g. G22/G23 from old default files or L1..L4: silently ignored
    auto &action = m_actions.at(static_cast<std::size_t>(gKey));

    const auto parts = splitList(value);
    if (parts.size() == 2 && parts[0] == "p" && parts[1].starts_with("k.")) {
        int keycode = 0;
        if (!parseInt(std::string_view(parts[1]).substr(2), keycode) || keycode <= 0
            || keycode > KEY_MAX)
            return false;
        action = std::make_unique<PassThroughAction>(keycode);
        return true;
    }
    if (parts.size() == 3 && parts[0] == "m") {
        int macroId = 0;
        int repeats = 0;
        if (!parseInt(parts[1], macroId) || !parseInt(parts[2], repeats) || macroId < 0
            || macroId >= G13_MAX_MACROS || repeats < 0)
            return false;
        const auto sequence = loadMacroSequence(macroId);
        if (!sequence) {
            syslog(LOG_WARNING, "Macro file %s not found.",
                   ConfigPath::getMacroPath(macroId).c_str());
            return true;   // the binding itself is valid
        }
        action = std::make_unique<MacroAction>(*sequence, repeats);
        return true;
    }
    return false;
}

std::optional<std::string> G13::loadMacroSequence(int macroId)
{
    std::ifstream file(ConfigPath::getMacroPath(macroId));
    if (!file.is_open())
        return std::nullopt;

    std::string sequence;
    std::string line;
    std::string key;
    std::string value;
    while (std::getline(file, line)) {
        if (splitProperty(line, key, value) && key == "sequence")
            sequence = value;
    }
    return sequence;
}

// ---------------------------------------------------------------------------
// Live reload
// ---------------------------------------------------------------------------

void G13::check_for_config_update()
{
    const std::string filename = ConfigPath::getBindingPath(m_profile);
    struct stat fileStat {};
    if (stat(filename.c_str(), &fileStat) != 0)
        return;
    // Check for inequality instead of "newer": a clock set back or a file
    // created later also trigger a reload this way.
    const bool changed = !m_configMtimeKnown
        || fileStat.st_mtim.tv_sec != m_configMtime.tv_sec
        || fileStat.st_mtim.tv_nsec != m_configMtime.tv_nsec;
    if (changed) {
        syslog(LOG_INFO, "Config file change detected. Reloading...");
        loadBindings();
    }
}

void G13::remember_config_mtime(const std::string &filename)
{
    struct stat fileStat {};
    m_configMtimeKnown = stat(filename.c_str(), &fileStat) == 0;
    if (m_configMtimeKnown)
        m_configMtime = fileStat.st_mtim;
}

// ---------------------------------------------------------------------------
// LCD
// ---------------------------------------------------------------------------

void G13::clear_lcd_buffer()
{
    m_lcdBuffer.fill(0);
}

void G13::set_pixel(int x, int y, bool on)
{
    if (x < 0 || x >= LCD_WIDTH || y < 0 || y >= LCD_BUFFER_ROWS)
        return;
    const auto index = static_cast<std::size_t>(x) + (static_cast<std::size_t>(y / 8) * LCD_WIDTH);
    const auto mask = static_cast<unsigned char>(1U << static_cast<unsigned>(y % 8));
    if (on)
        m_lcdBuffer.at(index) |= mask;
    else
        m_lcdBuffer.at(index) &= static_cast<unsigned char>(~mask);
}

bool G13::write_lcd()
{
    if (!m_loaded)
        return false;

    std::array<unsigned char, LCD_TRANSFER_SIZE> transfer{};
    transfer[0] = LCD_REPORT_ID;
    std::ranges::copy(m_lcdBuffer, transfer.begin() + LCD_HEADER_SIZE);

    int transferred = 0;
    const int error = libusb_interrupt_transfer(m_handle, G13_LCD_ENDPOINT | LIBUSB_ENDPOINT_OUT,
                                                transfer.data(), static_cast<int>(transfer.size()),
                                                &transferred, USB_TIMEOUT_MS);
    if (error != 0) {
        syslog(LOG_ERR, "LCD write error: %s", libusb_error_name(error));
        return false;
    }
    return true;
}

void G13::write_char(int x, int y, char c)
{
    int code = static_cast<unsigned char>(c);
    if (code < FONT_FIRST_CHAR || code > FONT_LAST_CHAR)
        code = ' ';
    const auto first = static_cast<std::size_t>(code - FONT_FIRST_CHAR) * FONT_COLUMNS;
    for (int col = 0; col < FONT_COLUMNS; ++col) {
        const unsigned bits = font_5x7.at(first + static_cast<std::size_t>(col));
        for (int row = 0; row < FONT_ROWS; ++row) {
            if (((bits >> static_cast<unsigned>(row)) & 1U) != 0)
                set_pixel(x + col, y + row, true);
        }
    }
}

void G13::write_text(int x, int y, const std::string &text)
{
    for (const char c : text) {
        write_char(x, y, c);
        x += LCD_CHAR_WIDTH;
    }
}

// ---------------------------------------------------------------------------
// Start animation
// ---------------------------------------------------------------------------

// Global setting "intro" in settings.properties (default: on).
bool G13::intro_enabled()
{
    std::ifstream file(ConfigPath::getSettingsPath());
    bool enabled = true;
    std::string line;
    std::string key;
    std::string value;
    while (std::getline(file, line)) {
        if (splitProperty(line, key, value) && key == "intro")
            enabled = value != "0";
    }
    return enabled;
}

void G13::draw_final_logo()
{
    clear_lcd_buffer();
    for (int y = 0; y < IntroAnimation::Height; ++y) {
        for (int x = 0; x < IntroAnimation::Width; ++x)
            set_pixel(x, y, IntroAnimation::finalPixel(x, y));
    }
}

void G13::start_intro()
{
    if (intro_enabled()) {
        m_introStop = false;
        try {
            m_introThread = std::thread(&G13::run_intro, this);
            return;
        } catch (const std::system_error &e) {
            syslog(LOG_WARNING, "Could not start intro animation: %s", e.what());
        }
    }
    const std::lock_guard<std::mutex> lock(m_lcdMutex);
    draw_final_logo();
    write_lcd();
}

void G13::stop_intro()
{
    m_introStop = true;
    if (m_introThread.joinable())
        m_introThread.join();
}

void G13::run_intro()
{
    IntroAnimation intro;
    std::vector<std::uint8_t> bits(static_cast<std::size_t>(IntroAnimation::Width)
                                   * IntroAnimation::Height);
    const auto frameTime = std::chrono::milliseconds(1000 / IntroAnimation::Fps);
    auto next = std::chrono::steady_clock::now();

    for (int frame = 0; frame <= IntroAnimation::frameCount(); ++frame) {
        if (m_introStop || !m_keepRunning)
            return;
        if (frame > 0)
            intro.step();
        intro.compose(bits.data());
        {
            const std::lock_guard<std::mutex> lock(m_lcdMutex);
            if (m_introStop)
                return;   // e.g. text from the LCD pipe came first
            for (int y = 0; y < IntroAnimation::Height; ++y) {
                for (int x = 0; x < IntroAnimation::Width; ++x) {
                    const auto i = (static_cast<std::size_t>(y) * IntroAnimation::Width) + static_cast<std::size_t>(x);
                    set_pixel(x, y, bits[i] != 0);
                }
            }
            if (!write_lcd())
                return;   // device gone or similar; the error is already logged
        }
        next += frameTime;
        std::this_thread::sleep_until(next);
    }

    const std::lock_guard<std::mutex> lock(m_lcdMutex);
    if (!m_introStop) {
        draw_final_logo();
        write_lcd();
    }
}

// ---------------------------------------------------------------------------
// LCD pipe
// ---------------------------------------------------------------------------

void G13::init_fifo()
{
    const std::string path = ConfigPath::getFifoPath();

    // Remove a leftover pipe of an earlier run. In /tmp (no XDG_RUNTIME_DIR) a
    // file of another user cannot be removed; it is rejected below.
    ::unlink(path.c_str());
    if (mkfifo(path.c_str(), 0600) != 0 && errno != EEXIST) {
        syslog(LOG_ERR, "Failed to create FIFO at %s: %s", path.c_str(), errnoText().c_str());
        return;
    }

    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg, hicpp-signed-bitwise)
    const int fd = ::open(path.c_str(), O_RDWR | O_NONBLOCK | O_NOFOLLOW | O_CLOEXEC);
    if (fd < 0) {
        syslog(LOG_ERR, "Failed to open FIFO %s: %s", path.c_str(), errnoText().c_str());
        return;
    }
    // Only use a pipe that belongs to this user, and only for this user.
    struct stat info {};
    if (fstat(fd, &info) != 0 || !S_ISFIFO(info.st_mode) || info.st_uid != geteuid()
        || fchmod(fd, 0600) != 0) {
        syslog(LOG_ERR, "%s is not a pipe of this user, LCD pipe disabled.", path.c_str());
        ::close(fd);
        return;
    }
    m_fifoFd = fd;
    m_fifoPath = path;
    syslog(LOG_INFO, "LCD pipe created at %s", path.c_str());
}

void G13::cleanup_fifo()
{
    if (m_fifoFd >= 0) {
        ::close(m_fifoFd);
        m_fifoFd = -1;
    }
    if (!m_fifoPath.empty()) {
        ::unlink(m_fifoPath.c_str());
        m_fifoPath.clear();
    }
}

void G13::check_fifo()
{
    if (m_fifoFd < 0)
        return;

    std::array<char, 4096> buffer{};
    const ssize_t bytesRead = ::read(m_fifoFd, buffer.data(), buffer.size());
    if (bytesRead <= 0)
        return;

    std::string input(buffer.data(), static_cast<std::size_t>(bytesRead));
    if (input.ends_with('\n'))
        input.pop_back();

    stop_intro();   // text from the pipe takes precedence over the start animation
    const std::lock_guard<std::mutex> lock(m_lcdMutex);
    clear_lcd_buffer();

    std::stringstream lines(input);
    std::string line;
    for (int y = 0; y + FONT_ROWS <= LCD_BUFFER_ROWS && std::getline(lines, line);
         y += LCD_LINE_HEIGHT)
        write_text(2, y, line);

    write_lcd();
}
