#ifndef G13_DRIVER_G13_H
#define G13_DRIVER_G13_H

#include "Constants.h"
#include "G13Action.h"

#include <libusb-1.0/libusb.h>

#include <array>
#include <cstdint>
#include <atomic>
#include <ctime>
#include <istream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <thread>

/**
 * @brief One connected G13: reads its keys, applies the bindings of the active
 * profile and drives the LCD and the backlight.
 *
 * start() runs the read loop in the calling thread until the device is gone
 * or keepRunning becomes false. The start animation runs in its own thread.
 */
class G13 {
public:
    G13(libusb_device *device, const std::atomic<bool> &keepRunning);
    ~G13();

    G13(const G13 &) = delete;
    G13 &operator=(const G13 &) = delete;
    G13(G13 &&) = delete;
    G13 &operator=(G13 &&) = delete;

    /// true if the device could be opened and claimed.
    [[nodiscard]] bool isLoaded() const { return m_loaded; }

    /// Read loop; returns when the device is gone or the driver stops.
    void start();

private:
    enum class ReadResult : std::uint8_t { Ok, Retry, Stop };
    using LcdBuffer = std::array<unsigned char, G13_LCD_BUFFER_SIZE>;

    // --- Device ---
    void reattach_kernel_driver();
    bool setColor(int red, int green, int blue);
    ReadResult read();

    // --- Keys and bindings ---
    void loadBindings();
    void reset_actions();
    void parse_bindings_from_stream(std::istream &stream);
    bool parse_binding(const std::string &key, const std::string &value);
    static std::optional<std::string> loadMacroSequence(int macroId);
    void parse_joystick(const unsigned char *report);
    void parse_keys(const unsigned char *report);
    void set_key(int key, bool pressed);

    // --- Live reload of the bindings file of the active profile ---
    void remember_config_mtime(const std::string &filename);
    void check_for_config_update();

    // --- LCD ---
    void clear_lcd_buffer();
    void set_pixel(int x, int y, bool on);
    bool write_lcd();   // expects m_lcdMutex to be locked
    void write_char(int x, int y, char c);
    void write_text(int x, int y, const std::string &text);

    // --- Start animation (flame intro), see IntroAnimation ---
    static bool intro_enabled();
    void start_intro();
    void stop_intro();
    void run_intro();
    void draw_final_logo();   // expects m_lcdMutex to be locked

    // --- LCD pipe for external programs ---
    void init_fifo();
    void check_fifo();
    void cleanup_fifo();

    const std::atomic<bool> &m_keepRunning;

    libusb_device_handle *m_handle = nullptr;
    bool m_loaded = false;
    bool m_kernelDriverDetached = false;   // reattach on shutdown

    std::array<std::unique_ptr<G13Action>, G13_NUM_KEYS> m_actions;
    int m_profile = 0;   // 0..3 = bindings-<n>.properties, switched with L1..L4
    // Previous state of L1..L4, so the profile is switched only on press (edge).
    std::array<bool, 4> m_profileKeyDown{};

    timespec m_configMtime{};
    bool m_configMtimeKnown = false;

    // Consecutive USB read errors (timeouts excluded)
    int m_readErrorCount = 0;
    int m_lastReadError = 0;

    // m_lcdMutex protects m_lcdBuffer and sending to the LCD.
    std::mutex m_lcdMutex;
    LcdBuffer m_lcdBuffer{};

    std::thread m_introThread;
    std::atomic<bool> m_introStop{false};

    int m_fifoFd = -1;
    std::string m_fifoPath;
};

#endif // G13_DRIVER_G13_H
