#ifndef G13_DRIVER_OUTPUT_H
#define G13_DRIVER_OUTPUT_H

#include <mutex>

/**
 * @brief The virtual keyboard (/dev/uinput) through which the driver sends keys.
 *
 * One device for all G13s. All functions are thread-safe; send_event() is
 * called from the device threads and from macro threads.
 */
class UInput {
public:
    UInput() = delete;

    /// Creates the virtual keyboard. Returns false on error (logged).
    static bool create_uinput();
    /// Destroys the virtual keyboard.
    static void close_uinput();
    /// Sends one input event (e.g. EV_KEY/KEY_A/1, then EV_SYN/SYN_REPORT/0).
    static void send_event(int type, int code, int value);

private:
    static int s_fd;
    static bool s_writeErrorLogged;
    static std::mutex s_mutex;
};

#endif // G13_DRIVER_OUTPUT_H
