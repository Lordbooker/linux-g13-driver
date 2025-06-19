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
/** @brief Mutex to protect the g13_instances map from concurrent access by multiple threads. */
pthread_mutex_t g13_map_mutex = PTHREAD_MUTEX_INITIALIZER;
/** @brief Map to store active device threads. The key is a unique device ID. */
map<uint16_t, pthread_t> g13_instances;
/** @brief Flag to control the main daemon loop. It is set to 0 by the signal handler to initiate a shutdown. */
volatile sig_atomic_t daemon_keep_running = 1;

/**
 * @brief Creates a unique 16-bit ID for a USB device from its bus and device address.
 * @param dev The libusb device.
 * @return A unique ID for the device.
 */
uint16_t get_device_key(libusb_device *dev) {
    // Combine the bus number and device address into a single unique short integer.
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
    // Its destructor will be called automatically when the thread exits, ensuring clean resource release.
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
                
                // MODIFICATION: Do NOT detach the thread. We need to join it on shutdown.
                // pthread_detach(thread);
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

    // --- MODIFIED SHUTDOWN SEQUENCE ---
    // 5. Wait for all threads to terminate before cleaning up global resources.
    cout << "\nShutting down driver... Waiting for handler threads to exit." << endl;

    // Create a copy of the current thread handles to join them safely.
    // This avoids issues if a thread removes itself from the map while we iterate.
    vector<pthread_t> threads_to_join;
    pthread_mutex_lock(&g13_map_mutex);
    for (auto const& [key, thread_id] : g13_instances) {
        threads_to_join.push_back(thread_id);
    }
    pthread_mutex_unlock(&g13_map_mutex);

    // Join all active threads. This call blocks until each thread has completely finished.
    // This is the crucial step to prevent the race condition.
    for (pthread_t th : threads_to_join) {
        pthread_join(th, nullptr);
    }

    cout << "All handler threads have finished." << endl;

    // 6. Clean up global resources now that threads are safely terminated.
    UInput::close_uinput();
    libusb_exit(ctx);
    // --- END OF MODIFICATION ---

	return 0;
}