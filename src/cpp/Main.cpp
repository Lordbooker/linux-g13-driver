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

std::vector<std::unique_ptr<G13>> g13s;
std::vector<std::thread> g13_threads;
std::atomic<bool> keep_running = true;

void signal_handler(int /*signal*/) {
    keep_running.store(false, std::memory_order_relaxed);
}

// discover_devices now takes an active libusb_context
void discover_devices(libusb_context *ctx) {
    libusb_device **devs;
    ssize_t count = libusb_get_device_list(ctx, &devs);
    if (count < 0) {
        std::cerr << "libusb_get_device_list error" << std::endl;
        // Do not exit libusb here, it's managed by main()
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
    // Do not exit libusb here, it's managed by main()
}

void run_g13(G13* g13_ptr) {
    if (g13_ptr) {
        g13_ptr->start();
    }
}

int main(int /*argc*/, char* /*argv*/) {
    libusb_context *usb_ctx = nullptr;

    if (!create_uinput()) {
        std::cerr << "Failed to initialize uinput. Are permissions correct? Exiting." << std::endl;
        return 1;
    }

    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // Initialize libusb
    if (libusb_init(&usb_ctx) < 0) {
        std::cerr << "libusb_init error. Exiting." << std::endl;
        close_uinput(); // Clean up uinput if it was created
        return 1;
    }

    discover_devices(usb_ctx); // Pass the initialized context
    std::cout << "Found " << g13s.size() << " G13s" << "\n";

    if (g13s.empty()) {
        close_uinput();
        libusb_exit(usb_ctx); // Clean up libusb
        return 0;
    }

    for (const auto& g13 : g13s) {
        g13_threads.emplace_back(run_g13, g13.get());
    }

    while (keep_running.load(std::memory_order_relaxed)) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nShutdown signal received. Stopping G13 threads..." << std::endl;

    for (const auto& g13 : g13s) {
        if (g13) {
            g13->stop();
        }
    }

    for (auto& t : g13_threads) {
        if (t.joinable()) {
            t.join();
        }
    }
    
    std::cout << "All threads stopped. Cleaning up." << std::endl;

    g13s.clear();
    close_uinput();
    libusb_exit(usb_ctx); // Deinitialize libusb at the very end

    return 0;
}