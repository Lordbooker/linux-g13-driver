#ifndef __OUTPUT_H__
#define __OUTPUT_H__

#include <mutex>

/**
 * @class UInput
 * @brief A static utility class for managing a virtual input device via /dev/uinput.
 *
 * This class encapsulates the creation, destruction, and event sending for a
 * virtual keyboard/joystick. It is designed to be used statically.
 * All operations are thread-safe using std::mutex.
 */
class UInput {
private:
    /** The file descriptor for the opened /dev/uinput device. */
    static int file;
    /** A mutex to ensure thread-safe access to the file descriptor. */
    static std::mutex plock;

public:
    // Prevent instantiation of this static utility class.
    UInput() = delete;

    /**
     * @brief Sends an input event to the kernel.
     * @param type The event type (e.g., EV_KEY).
     * @param code The event code (e.g., KEY_A).
     * @param val The event value (e.g., 1 for press, 0 for release).
     */
    static void send_event(int type, int code, int val);

    /** @brief Flushes any buffered data to the device file. */
    static void flush();

    /**
     * @brief Creates and configures the virtual input device.
     * @return true on success, false on failure.
     */
    static bool create_uinput();

    /** @brief Destroys and closes the virtual input device. */
    static void close_uinput();
};

#endif