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

// --- Globale Zustandsvariablen ---

// Zustand eines Geraete-Threads.
enum class HandlerState { Running, Finished, OpenFailed };

struct DeviceHandler {
    std::thread thread;
    std::shared_ptr<std::atomic<HandlerState>> state;
};

// Wartezeit, bevor ein Geraet nach fehlgeschlagenem Oeffnen erneut versucht wird
// (z. B. weil die udev-Rechte beim Einstecken noch nicht gesetzt waren).
static constexpr auto OpenRetryDelay = std::chrono::seconds(5);

static std::mutex              g13_map_mutex;
static std::map<uint16_t, DeviceHandler> g13_instances;
static std::map<uint16_t, std::chrono::steady_clock::time_point> retry_after;
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
static void executeG13(libusb_device *dev,
                       std::shared_ptr<std::atomic<HandlerState>> state) {
    bool opened = false;
    {
        G13 g13(dev);
        opened = g13.isLoaded();
        g13.start();
    } // RAII: G13-Destruktor gibt USB-Ressourcen frei

    libusb_unref_device(dev);
    // Als letzte Aktion: der Management-Thread raeumt den Eintrag danach ab.
    state->store(opened ? HandlerState::Finished : HandlerState::OpenFailed);
}

// Beendete Geraete-Threads joinen und aus der Liste entfernen, damit ein
// wieder eingestecktes oder zuvor fehlgeschlagenes Geraet erneut erkannt wird.
// Muss mit gehaltenem g13_map_mutex aufgerufen werden.
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
            syslog(LOG_WARNING, "G13-Geraet (ID: %x) konnte nicht geoeffnet werden. "
                                "Neuer Versuch in %lld s.", it->first,
                   static_cast<long long>(OpenRetryDelay.count()));
        } else {
            syslog(LOG_INFO, "Handler fuer G13-Geraet (ID: %x) beendet.", it->first);
        }
        it = g13_instances.erase(it);
    }
}

// --- Geraete-Erkennung ---
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
                    continue;                  // Wartezeit nach Fehlschlag laeuft noch
                retry_after.erase(r);
            }

            syslog(LOG_INFO, "Neues G13-Geraet gefunden (ID: %x). Starte Handler.", key);
            libusb_ref_device(devs[i]);
            auto state = std::make_shared<std::atomic<HandlerState>>(HandlerState::Running);
            g13_instances[key] = DeviceHandler{std::thread(executeG13, devs[i], state), state};
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

    // Threads unter Lock aus der Map nehmen, aber ausserhalb des Locks joinen.
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
    syslog(LOG_INFO, "G13-Treiber startet (Version %s)...", G13_VERSION);

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
