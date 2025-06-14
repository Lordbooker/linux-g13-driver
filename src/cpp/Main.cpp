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

vector<G13 *> g13s;
vector<pthread_t> g13_threads; // Use std::vector for pthreads
volatile sig_atomic_t keep_running = 1;

void discover() {
	libusb_context *ctx = nullptr;
	libusb_device **devs;

	int ret = libusb_init(&ctx);
	if (ret < 0) {
		cout << "Initialization error: " << ret << "\n";
		return;
	}

	// libusb_set_debug(ctx, 3); // Optional: for debugging

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

		if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
			G13 *g13 = new G13(devs[i]);
			g13s.push_back(g13);
		}
	}

	libusb_free_device_list(devs, 1);
}

void *executeG13(void *arg) {
	G13 *g13 = (G13 *)arg;
    if (g13) {
	    g13->start(); // This loop will run as long as g13->keepGoing is true
    }
	return nullptr;
}

void start() {
	if (g13s.size() > 0) {
		pthread_attr_t attr;
		pthread_attr_init(&attr);
        // Consider pthread_attr_setdetachstate if they are not meant to be joined,
        // but joining is generally better for controlled shutdown.

        g13_threads.resize(g13s.size());
		for (unsigned int i = 0; i < g13s.size(); i++) {
			pthread_create(&g13_threads[i], &attr, executeG13, g13s[i] );
		}
        pthread_attr_destroy(&attr);

        // Wait for all G13 threads to complete.
        // They will complete when their respective g13->stop() is called and
        // their internal keepGoing flag becomes false.
		for (unsigned int i = 0; i < g13s.size(); i++) {
			pthread_join(g13_threads[i], nullptr);
		}
	}
}

void signal_handler(int signal) {
    keep_running = 0; // Signal main loop and G13 threads to stop
	for (unsigned int i = 0; i < g13s.size(); i++) {
        if (g13s[i]) {
		    g13s[i]->stop(); // Tell each G13 instance to stop its loop
        }
	}
}

void cleanup() {
	for (unsigned int i = 0; i < g13s.size(); i++) {
		delete g13s[i];
	}
    g13s.clear();
    close_uinput(); // Close the uinput device
}

int main(int argc, char *argv[]) {

	if (!create_uinput()) {
        cerr << "Failed to initialize uinput. Exiting." << endl;
        return 1;
    }

	signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

	discover();

	cout << "Found " << g13s.size() << " G13s" << "\n";

    if (!g13s.empty()) {
	    start(); // This will block until all G13 threads are joined
    }

	cleanup(); // Perform cleanup after threads have finished

	return 0;
}
