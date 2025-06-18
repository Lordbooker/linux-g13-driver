#include <iostream>
#include <fstream>
#include <vector>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>

#include "G13.h"
#include "Output.h"

using namespace std;

// --- Global Variables ---
vector<G13 *> g13s; // Holds pointers to all discovered G13 device instances.
vector<pthread_t> g13_threads; // Holds thread handles for each G13 device.
volatile sig_atomic_t keep_running = 1; // Flag to control the main loop, modified by the signal handler.

/**
 * @brief Scans for connected Logitech G13 devices using libusb.
 *
 * Initializes libusb, gets a list of all USB devices, and iterates through
 * them, creating a new G13 object for each device that matches the
 * G13's vendor and product ID.
 */
void discover() {
	libusb_context *ctx = nullptr;
	libusb_device **devs;

	int ret = libusb_init(&ctx);
	if (ret < 0) {
		cout << "Initialization error: " << ret << "\n";
		return;
	}

	ssize_t count = libusb_get_device_list(ctx, &devs);
	if (count < 0) {
		cout << "Error while getting device list" << "\n";
		return;
	}

	for (int i = 0; i < count; i++) {
		libusb_device_descriptor desc;
		int r = libusb_get_device_descriptor(devs[i], &desc);
		if (r < 0) {
			cout << "Failed to get device descriptor" << endl;
			return;
		}

		// Check for Logitech G13 Vendor/Product ID.
		if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
			G13 *g13 = new G13(devs[i]);
			g13s.push_back(g13);
		}
	}

	libusb_free_device_list(devs, 1);
}

/**
 * @brief The entry point function for each G13 device thread.
 * @param arg A void pointer to a G13 object.
 * @return Always returns nullptr.
 */
void *executeG13(void *arg) {
	G13 *g13 = (G13 *)arg;
    if (g13) {
        // Start the main event loop for this G13 instance.
	    g13->start();
    }
	return nullptr;
}

/**
 * @brief Creates and starts a processing thread for each discovered G13 device.
 *
 * After threads are created, this function waits for all of them to complete
 * by joining them.
 */
void start() {
	if (g13s.size() > 0) {
		pthread_attr_t attr;
		pthread_attr_init(&attr);

        g13_threads.resize(g13s.size());
		for (unsigned int i = 0; i < g13s.size(); i++) {
			pthread_create(&g13_threads[i], &attr, executeG13, g13s[i] );
		}
        pthread_attr_destroy(&attr);

		// Wait for all G13 threads to finish.
		for (unsigned int i = 0; i < g13s.size(); i++) {
			pthread_join(g13_threads[i], nullptr);
		}
	}
}

/**
 * @brief Signal handler for SIGINT and SIGTERM to ensure graceful shutdown.
 * @param signal The signal number received.
 *
 * Sets the global 'keep_running' flag to false and tells each G13 device
 * to stop its event loop.
 */
void signal_handler(int signal) {
    keep_running = 0;
	for (unsigned int i = 0; i < g13s.size(); i++) {
        if (g13s[i]) {
		    g13s[i]->stop();
        }
	}
}

/**
 * @brief Cleans up all allocated resources before the program exits.
 */
void cleanup() {
	// Delete all G13 objects.
	for (unsigned int i = 0; i < g13s.size(); i++) {
		delete g13s[i];
	}
    g13s.clear();
    // Close the virtual uinput device.
    UInput::close_uinput();
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

    // 2. Register signal handler for clean shutdown (e.g., on Ctrl+C).
	signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 3. Discover all connected G13 devices.
	discover();
	cout << "Found " << g13s.size() << " G13s" << "\n";

    // 4. Start processing threads if any G13s were found.
    if (!g13s.empty()) {
	    start();
    }

    // 5. Perform final cleanup after all threads have exited.
	cleanup();

	return 0;
}