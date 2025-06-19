#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include <iomanip>

#include "G13.h"
#include "Output.h"

using namespace std;

// --- Global Variables for Hot-Plug Management ---
// Mutex to protect the thread map from concurrent access.
pthread_mutex_t g13_map_mutex = PTHREAD_MUTEX_INITIALIZER;
// Map to store active devices and their threads. The key is a unique device ID.
map<uint16_t, pthread_t> g13_instances;
// Global flag modified by the signal handler to terminate the main loop.
volatile sig_atomic_t daemon_keep_running = 1;

/**
 * @brief Creates a unique 16-bit ID for a USB device from its bus and device address.
 * @param dev The libusb device.
 * @return A unique ID for the device.
 */
uint16_t get_device_key(libusb_device *dev) {
    return (libusb_get_bus_number(dev) << 8) | libusb_get_device_address(dev);
}

/**
 * @brief Thread function that is executed for each connected G13 device.
 * @param arg A void pointer to the libusb_device object.
 * @return Always returns nullptr.
 */
void *executeG13(void *arg) {
	libusb_device *dev = (libusb_device *)arg;
    uint16_t key = get_device_key(dev);

    // The G13 object is created on this thread's stack.
    // It will be automatically destroyed when the thread exits.
	G13 g13(dev);
	g13.start(); // This function blocks until the device is disconnected or the program exits.

    // --- Cleanup after the device has been disconnected ---
    cout << "Thread for device " << hex << key << " cleaning up." << endl;

    // The thread removes itself from the global map to allow for reconnection.
    pthread_mutex_lock(&g13_map_mutex);
    g13_instances.erase(key);
    pthread_mutex_unlock(&g13_map_mutex);

    // Release the libusb device reference that was created before the thread started.
    libusb_unref_device(dev);
	return nullptr;
}

/**
 * @brief Signal handler for CTRL+C (SIGINT) and SIGTERM to cleanly shut down the program.
 * @param signal The received signal number.
 */
void signal_handler(int signal) {
    daemon_keep_running = 0;
}

/**
 * @brief Scans for connected G13 devices and starts/manages their handler threads.
 * @param ctx The libusb context.
 */
void find_and_manage_devices(libusb_context *ctx) {
    libusb_device **devs;
    ssize_t count = libusb_get_device_list(ctx, &devs);
    if (count < 0) return;

    // Iterate through all currently connected USB devices.
    for (int i = 0; i < count; i++) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(devs[i], &desc) < 0) continue;

        if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
            uint16_t key = get_device_key(devs[i]);

            pthread_mutex_lock(&g13_map_mutex);
            // Check if a thread is already running for this device.
            if (g13_instances.find(key) == g13_instances.end()) {
                cout << "New G13 device connected (ID: " << hex << key << "). Starting handler thread." << endl;
                pthread_t thread;

                // Increment the device's reference count before passing it to the thread.
                libusb_ref_device(devs[i]);
                pthread_create(&thread, nullptr, executeG13, devs[i]);
                g13_instances[key] = thread;
                // Detach the thread, allowing it to clean up its own resources upon termination.
                // We no longer need to join it.
                pthread_detach(thread);
            }
            pthread_mutex_unlock(&g13_map_mutex);
        }
    }
    libusb_free_device_list(devs, 1);
}

/**
 * @brief The main entry point of the G13 driver daemon.
 */
int main(int argc, char *argv[]) {
    // 1. Initialize the virtual input device.
	if (!UInput::create_uinput()) {
        cerr << "Failed to initialize uinput. Exiting." << endl;
        return 1;
    }

    // 2. Register signal handler for clean shutdown.
	signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 3. Initialize libusb.
    libusb_context *ctx = nullptr;
    if (libusb_init(&ctx) < 0) {
        cerr << "Failed to initialize libusb. Exiting." << endl;
        UInput::close_uinput();
        return 1;
    }

    cout << "G13 driver started. Waiting for devices... (Press Ctrl+C to exit)" << endl;

    // 4. Start the main device management loop.
    while (daemon_keep_running) {
        find_and_manage_devices(ctx);
        sleep(2); // Poll for new/removed devices every 2 seconds.
    }

    // 5. Clean up resources upon exit.
    cout << "\nShutting down driver..." << endl;
    UInput::close_uinput();
    libusb_exit(ctx);
    // Since threads are detached, we don't need to join them.
    // They will terminate because daemon_keep_running is now 0.

	return 0;
}