#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <csignal>
#include <thread>
#include <mutex>
#include <chrono>
#include <atomic>
#include <memory>
#include <libusb-1.0/libusb.h>
#include <syslog.h>

#include "G13.h"
#include "Output.h"

// --- Global state ---

// State of a device thread.
enum class HandlerState { Running, Finished, OpenFailed };

struct DeviceHandler {
    std::thread thread;
    std::shared_ptr<std::atomic<HandlerState>> state;
};

// Delay before a device is retried after opening it failed
// (e.g. because the udev permissions were not set yet when it was plugged in).
static constexpr auto OpenRetryDelay = std::chrono::seconds(5);

static std::mutex              g13_map_mutex;
static std::map<uint16_t, DeviceHandler> g13_instances;
static std::map<uint16_t, std::chrono::steady_clock::time_point> retry_after;
// Not static: read in G13.cpp via extern.
volatile sig_atomic_t         daemon_keep_running = 1;
static libusb_context        *ctx = nullptr;
static std::thread             device_thread;

// --- Helper ---
static uint16_t get_device_key(libusb_device *dev) {
    return static_cast<uint16_t>(
        (libusb_get_bus_number(dev) << 8) | libusb_get_device_address(dev)
    );
}

// --- G13 device handler (runs in its own thread) ---
static void executeG13(libusb_device *dev,
                       std::shared_ptr<std::atomic<HandlerState>> state) {
    bool opened = false;
    {
        G13 g13(dev);
        opened = g13.isLoaded();
        g13.start();
    } // RAII: the G13 destructor releases the USB resources

    libusb_unref_device(dev);
    // Last action: the management thread cleans up the entry afterwards.
    state->store(opened ? HandlerState::Finished : HandlerState::OpenFailed);
}

// Join finished device threads and remove them from the list, so that a
// replugged or previously failed device is detected again.
// Must be called with g13_map_mutex held.
static void reap_finished_handlers() {
    for (auto it = g13_instances.begin(); it != g13_instances.end(); ) {
        const HandlerState s = it->second.state->load();
        if (s == HandlerState::Running) {
            ++it;
            continue;
        }
        if (it->second.thread.joinable())
            it->second.thread.join();
        if (s == HandlerState::OpenFailed) {
            retry_after[it->first] = std::chrono::steady_clock::now() + OpenRetryDelay;
            syslog(LOG_WARNING, "Could not open G13 device (ID: %x). "
                                "Retrying in %lld s.", it->first,
                   static_cast<long long>(OpenRetryDelay.count()));
        } else {
            syslog(LOG_INFO, "Handler for G13 device (ID: %x) finished.", it->first);
        }
        it = g13_instances.erase(it);
    }
}

// --- Device detection ---
static void find_and_manage_devices() {
    libusb_device **devs = nullptr;
    ssize_t count = libusb_get_device_list(ctx, &devs);
    if (count < 0) return;

    std::lock_guard<std::mutex> lock(g13_map_mutex);
    reap_finished_handlers();
    const auto now = std::chrono::steady_clock::now();

    for (ssize_t i = 0; i < count; ++i) {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(devs[i], &desc) < 0) continue;

        if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
            uint16_t key = get_device_key(devs[i]);

            if (g13_instances.contains(key))
                continue;
            if (auto r = retry_after.find(key); r != retry_after.end()) {
                if (now < r->second)
                    continue;                  // retry delay after a failure still running
                retry_after.erase(r);
            }

            syslog(LOG_INFO, "New G13 device found (ID: %x). Starting handler.", key);
            libusb_ref_device(devs[i]);
            auto state = std::make_shared<std::atomic<HandlerState>>(HandlerState::Running);
            g13_instances[key] = DeviceHandler{std::thread(executeG13, devs[i], state), state};
        }
    }
    libusb_free_device_list(devs, 1);
}

// --- Device management loop ---
static void device_management_thread_loop() {
    while (daemon_keep_running) {
        find_and_manage_devices();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// --- Clean shutdown ---
// Runs in the main thread after the device management thread has finished.
// All device threads are joined before uinput and libusb are released.
static void shutdown_driver() {
    syslog(LOG_INFO, "Shutting down driver...");

    // Take the threads out of the map under the lock, but join them outside of it.
    std::map<uint16_t, DeviceHandler> handlers;
    {
        std::lock_guard<std::mutex> lock(g13_map_mutex);
        handlers.swap(g13_instances);
    }
    for (auto &[key, handler] : handlers) {
        if (handler.thread.joinable())
            handler.thread.join();
    }

    UInput::close_uinput();
    libusb_exit(ctx);
    syslog(LOG_INFO, "Shutdown complete.");
    closelog();
}

// The signal handler only sets the flag (async-signal-safe).
// The actual shutdown happens in main().
static void signal_handler(int /*signum*/) {
    daemon_keep_running = 0;
}

// --- Entry point ---
int main(int /*argc*/, char * /*argv*/[]) {
    openlog("linux-g13-driver", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "G13 driver starting (version %s)...", G13_VERSION);

    if (!UInput::create_uinput()) {
        syslog(LOG_ERR, "Could not initialize uinput. Exiting.");
        return 1;
    }

    if (libusb_init(&ctx) < 0) {
        syslog(LOG_ERR, "Could not initialize libusb. Exiting.");
        UInput::close_uinput();
        return 1;
    }

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    syslog(LOG_INFO, "Driver active. Waiting for G13 devices...");

    try {
        device_thread = std::thread(device_management_thread_loop);
    } catch (const std::system_error &e) {
        syslog(LOG_ERR, "Could not create thread: %s", e.what());
        UInput::close_uinput();
        libusb_exit(ctx);
        return 1;
    }

    if (device_thread.joinable())
        device_thread.join();

    shutdown_driver();
    return 0;
}
