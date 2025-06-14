#include <iostream>
#include <vector>
#include <string>
#include <csignal>
#include <thread>
#include <atomic>
#include <memory>
#include <chrono>

#include <libusb-1.0/libusb.h>

#include "G13.h"
#include "Output.h"

// Use a vector of unique_ptr for automatic memory management of G13 objects
std::vector<std::unique_ptr<G13>> g13s;
std::vector<std::thread> g13_threads;
std::atomic<bool> keep_running = true;

// Signal handler is now async-signal-safe
void signal_handler(int signal) {
    keep_running.store(false, std::memory_order_relaxed);
}

void discover_devices() {
    libusb_context *ctx = nullptr;
    if (libusb_init(&ctx) < 0) {
        std::cerr << "libusb_init error" << std::endl;
        return;
    }

    libusb_device **devs;
    ssize_t count = libusb_get_device_list(ctx, &devs);
    if (count < 0) {
        std::cerr << "libusb_get_device_list error" << std::endl;
        libusb_exit(ctx);
        return;
    }

    for (ssize_t i = 0; i < count; ++i) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(devs[i], &desc) == 0) {
            if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
                auto g13 = std::make_unique<G13>(devs[i]);
                if (g13->isLoaded()) {
                    g13s.push_back(std::move(g13));
                }
            }
        }
    }

    libusb_free_device_list(devs, 1);
    libusb_exit(ctx);
}

// The function each thread will execute
void run_g13(G13* g13_ptr) {
    if (g13_ptr) {
        g13_ptr->start();
    }
}

int main(int argc, char *argv) {
    if (!create_uinput()) {
        std::cerr << "Failed to initialize uinput. Are permissions correct? Exiting." << std::endl;
        return 1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    discover_devices();
    std::cout << "Found " << g13s.size() << " G13s" << "\n";

    if (g13s.empty()) {
        close_uinput();
        return 0;
    }

    // Start a thread for each discovered G13 device
    for (const auto& g13 : g13s) {
        g13_threads.emplace_back(run_g13, g13.get());
    }

    // Main loop waits for shutdown signal
    while (keep_running.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nShutdown signal received. Stopping G13 threads..." << std::endl;

    // Signal all G13 instances to stop their loops
    for (const auto& g13 : g13s) {
        if (g13) {
            g13->stop();
        }
    }

    // Wait for all threads to complete
    for (auto& t : g13_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    std::cout << "All threads stopped. Cleaning up." << std::endl;

    // Cleanup resources
    g13s.clear(); // unique_ptr will call destructors
    close_uinput();

    return 0;
}