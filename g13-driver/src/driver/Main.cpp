#include <iostream>
#include <vector>
#include <map>
#include <string>
#include <csignal>
#include <thread>
#include <mutex>
#include <chrono>
#include <libusb-1.0/libusb.h>
#include <syslog.h>

#include "G13.h"
#include "Output.h"

// --- Globale Zustandsvariablen ---
static std::mutex              g13_map_mutex;
static std::map<uint16_t, std::thread> g13_instances;
// Nicht static: wird in G13.cpp per extern abgefragt.
volatile sig_atomic_t         daemon_keep_running = 1;
static libusb_context        *ctx = nullptr;
static std::thread             device_thread;

// --- Hilfsfunktion ---
static uint16_t get_device_key(libusb_device *dev) {
    return static_cast<uint16_t>(
        (libusb_get_bus_number(dev) << 8) | libusb_get_device_address(dev)
    );
}

// --- G13-Geraet-Handler (laeuft in eigenem Thread) ---
static void executeG13(libusb_device *dev) {
    {
        G13 g13(dev);
        g13.start();
    } // RAII: G13-Destruktor gibt USB-Ressourcen frei

    std::lock_guard<std::mutex> lock(g13_map_mutex);
    libusb_unref_device(dev);
}

// --- Geraete-Erkennung ---
static void find_and_manage_devices() {
    libusb_device **devs = nullptr;
    ssize_t count = libusb_get_device_list(ctx, &devs);
    if (count < 0) return;

    for (ssize_t i = 0; i < count; ++i) {
        libusb_device_descriptor desc{};
        if (libusb_get_device_descriptor(devs[i], &desc) < 0) continue;

        if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
            uint16_t key = get_device_key(devs[i]);

            std::lock_guard<std::mutex> lock(g13_map_mutex);
            if (!g13_instances.contains(key)) {
                syslog(LOG_INFO, "Neues G13-Geraet gefunden (ID: %x). Starte Handler.", key);
                libusb_ref_device(devs[i]);
                g13_instances[key] = std::thread(executeG13, devs[i]);
            }
        }
    }
    libusb_free_device_list(devs, 1);
}

// --- Geraete-Management-Loop ---
static void device_management_thread_loop() {
    while (daemon_keep_running) {
        find_and_manage_devices();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// --- Sauberes Herunterfahren ---
// Laeuft im Haupt-Thread, nachdem der Geraete-Management-Thread beendet ist.
// Alle Geraete-Threads werden gejoint, bevor uinput und libusb freigegeben werden.
static void shutdown_driver() {
    syslog(LOG_INFO, "Fahre Treiber herunter...");

    // Threads unter Lock aus der Map nehmen, aber ausserhalb des Locks joinen:
    // executeG13() sperrt g13_map_mutex beim Beenden selbst (sonst Deadlock).
    std::map<uint16_t, std::thread> threads;
    {
        std::lock_guard<std::mutex> lock(g13_map_mutex);
        threads.swap(g13_instances);
    }
    for (auto &[key, th] : threads) {
        if (th.joinable())
            th.join();
    }

    UInput::close_uinput();
    libusb_exit(ctx);
    syslog(LOG_INFO, "Shutdown abgeschlossen.");
    closelog();
}

// Signal-Handler setzt nur das Flag (async-signal-safe).
// Das eigentliche Herunterfahren erfolgt in main().
static void signal_handler(int /*signum*/) {
    daemon_keep_running = 0;
}

// --- Einstiegspunkt ---
int main(int /*argc*/, char * /*argv*/[]) {
    openlog("linux-g13-driver", LOG_PID | LOG_CONS, LOG_USER);
    syslog(LOG_INFO, "G13-Treiber startet (Version 2.0)...");

    if (!UInput::create_uinput()) {
        syslog(LOG_ERR, "uinput konnte nicht initialisiert werden. Beende.");
        return 1;
    }

    if (libusb_init(&ctx) < 0) {
        syslog(LOG_ERR, "libusb konnte nicht initialisiert werden. Beende.");
        UInput::close_uinput();
        return 1;
    }

    std::signal(SIGINT,  signal_handler);
    std::signal(SIGTERM, signal_handler);

    syslog(LOG_INFO, "Treiber aktiv. Warte auf G13-Geraete...");

    try {
        device_thread = std::thread(device_management_thread_loop);
    } catch (const std::system_error &e) {
        syslog(LOG_ERR, "Thread konnte nicht erstellt werden: %s", e.what());
        UInput::close_uinput();
        libusb_exit(ctx);
        return 1;
    }

    if (device_thread.joinable())
        device_thread.join();

    shutdown_driver();
    return 0;
}
