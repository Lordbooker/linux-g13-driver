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

// --- Globale Variablen für das Hot-Plug-Management ---
// Mutex zum Schutz der Thread-Map vor konkurrierendem Zugriff
pthread_mutex_t g13_map_mutex = PTHREAD_MUTEX_INITIALIZER;
// Map, um aktive Geräte und ihre Threads zu speichern. Der Key ist eine eindeutige Geräte-ID.
map<uint16_t, pthread_t> g13_instances;
// Globale Variable, die durch den Signal-Handler geändert wird, um das Programm zu beenden.
volatile sig_atomic_t daemon_keep_running = 1;

/**
 * @brief Erstellt eine eindeutige 16-Bit ID für ein USB-Gerät aus Bus- und Geräteadresse.
 * @param dev Das libusb-Gerät.
 * @return Eine eindeutige ID für das Gerät.
 */
uint16_t get_device_key(libusb_device *dev) {
    return (libusb_get_bus_number(dev) << 8) | libusb_get_device_address(dev);
}

/**
 * @brief Thread-Funktion, die für jedes angeschlossene G13-Gerät ausgeführt wird.
 * @param arg Ein void-Zeiger auf das libusb_device-Objekt.
 * @return Immer nullptr.
 */
void *executeG13(void *arg) {
	libusb_device *dev = (libusb_device *)arg;
    uint16_t key = get_device_key(dev);

    // Das G13-Objekt wird auf dem Stack dieses Threads erstellt.
    // Es wird automatisch zerstört, wenn der Thread endet.
	G13 g13(dev);
	g13.start(); // Diese Funktion blockiert, bis das Gerät getrennt wird oder das Programm endet.

    // --- Aufräumen, nachdem das Gerät getrennt wurde ---
    cout << "Thread for device " << hex << key << " cleaning up." << endl;

    // Thread entfernt sich selbst aus der globalen Map, um eine Wiederverbindung zu ermöglichen.
    pthread_mutex_lock(&g13_map_mutex);
    g13_instances.erase(key);
    pthread_mutex_unlock(&g13_map_mutex);

    // Referenz auf das libusb-Gerät freigeben, die vor dem Thread-Start erstellt wurde.
    libusb_unref_device(dev);
	return nullptr;
}

/**
 * @brief Signal-Handler für STRG+C (SIGINT) und SIGTERM zum sauberen Beenden des Programms.
 * @param signal Das empfangene Signal.
 */
void signal_handler(int signal) {
    daemon_keep_running = 0;
}

/**
 * @brief Sucht nach angeschlossenen G13-Geräten und startet/verwaltet deren Threads.
 * @param ctx Der libusb-Kontext.
 */
void find_and_manage_devices(libusb_context *ctx) {
    libusb_device **devs;
    ssize_t count = libusb_get_device_list(ctx, &devs);
    if (count < 0) return;

    // Iteriert durch alle aktuell angeschlossenen USB-Geräte
    for (int i = 0; i < count; i++) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(devs[i], &desc) < 0) continue;

        if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
            uint16_t key = get_device_key(devs[i]);

            pthread_mutex_lock(&g13_map_mutex);
            // Prüfen, ob für dieses Gerät bereits ein Thread läuft.
            if (g13_instances.find(key) == g13_instances.end()) {
                cout << "New G13 device connected (ID: " << hex << key << "). Starting handler thread." << endl;
                pthread_t thread;

                // Erhöht den Referenzzähler, da das Gerät an einen anderen Thread übergeben wird.
                libusb_ref_device(devs[i]);
                pthread_create(&thread, nullptr, executeG13, devs[i]);
                g13_instances[key] = thread;
                // Der Thread wird "detached", d.h. er gibt seine Ressourcen nach Beendigung selbst frei.
                // Wir müssen nicht mehr mit pthread_join auf ihn warten.
                pthread_detach(thread);
            }
            pthread_mutex_unlock(&g13_map_mutex);
        }
    }
    libusb_free_device_list(devs, 1);
}

/**
 * @brief Der Haupteinstiegspunkt des G13-Treiber-Daemons.
 */
int main(int argc, char *argv[]) {
    // 1. Virtuelles Eingabegerät initialisieren
	if (!UInput::create_uinput()) {
        cerr << "Failed to initialize uinput. Exiting." << endl;
        return 1;
    }

    // 2. Signal-Handler für sauberes Beenden registrieren
	signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 3. libusb initialisieren
    libusb_context *ctx = nullptr;
    if (libusb_init(&ctx) < 0) {
        cerr << "Failed to initialize libusb. Exiting." << endl;
        UInput::close_uinput();
        return 1;
    }

    cout << "G13 driver started. Waiting for devices... (Press Ctrl+C to exit)" << endl;

    // 4. Hauptschleife zur Geräteverwaltung
    while (daemon_keep_running) {
        find_and_manage_devices(ctx);
        sleep(2); // Alle 2 Sekunden nach neuen/entfernten Geräten suchen
    }

    // 5. Aufräumen nach dem Beenden
    cout << "\nShutting down driver..." << endl;
    UInput::close_uinput();
    libusb_exit(ctx);

	return 0;
}