#include <iostream> 
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <thread>
#include <mutex>
#include <libusb-1.0/libusb.h>
#include <iomanip>
#include <libgen.h> 
#include <sys/wait.h> 
#include <syslog.h> 

// Headers for the tray icon functionality
#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>

#include "G13.h"
#include "Output.h"

// --- Global Variables ---
std::mutex g13_map_mutex;
std::map<uint16_t, std::thread> g13_instances;
volatile sig_atomic_t daemon_keep_running = 1;

AppIndicator *indicator = NULL;
libusb_context *ctx = nullptr;
std::thread device_thread;

// --- Forward Declarations ---
static void quit_driver(GtkMenuItem *item, gpointer user_data);

// --- Helper Functions ---
uint16_t get_device_key(libusb_device *dev) {
    return (libusb_get_bus_number(dev) << 8) | libusb_get_device_address(dev);
}

// --- G13 Device Handling ---
void executeG13(libusb_device *dev) {
    uint16_t key = get_device_key(dev);
    
    // Create G13 instance on stack (RAII)
    {
        G13 g13(dev);
        g13.start(); 
    } // g13 destructor called here

    // Cleanup after device disconnects
    std::lock_guard<std::mutex> lock(g13_map_mutex);
    if (g13_instances.find(key) != g13_instances.end()) {
        // Thread is managed in the map
    }
    libusb_unref_device(dev);
}

void find_and_manage_devices() {
    libusb_device **devs;
    ssize_t count = libusb_get_device_list(ctx, &devs);
    if (count < 0) return;

    for (int i = 0; i < count; i++) {
        libusb_device_descriptor desc;
        if (libusb_get_device_descriptor(devs[i], &desc) < 0) continue;

        if (desc.idVendor == G13_VENDOR_ID && desc.idProduct == G13_PRODUCT_ID) {
            uint16_t key = get_device_key(devs[i]);
            
            std::lock_guard<std::mutex> lock(g13_map_mutex);
            if (g13_instances.find(key) == g13_instances.end()) {
                syslog(LOG_INFO, "New G13 device connected (ID: %x). Starting handler thread.", key);
                
                libusb_ref_device(devs[i]);
                // Move semantic for std::thread
                g13_instances[key] = std::thread(executeG13, devs[i]);
            }
        }
    }
    libusb_free_device_list(devs, 1);
}

void device_management_thread_loop() {
    while (daemon_keep_running) {
        find_and_manage_devices();
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
}

// --- Tray Icon and Main Application Logic ---
static void show_gui(GtkMenuItem *item, gpointer user_data) {
    syslog(LOG_INFO, "Attempting to start GUI via global command: g13-gui");

    pid_t pid = fork();

    if (pid == -1) {
        syslog(LOG_ERR, "Failed to fork process for GUI start.");
    } 
    else if (pid == 0) {
        // CHILD PROCESS
        setsid(); // Detach from parent

        // Redirect stdout/stderr to avoid cluttering driver logs
        // Using strict checking to silence compiler warnings
        if (freopen("/dev/null", "w", stdout) == NULL) {}
        if (freopen("/dev/null", "w", stderr) == NULL) {}

        // --- CHANGE START ---
        // We now use the wrapper script 'g13-gui' which is in the system PATH (/usr/bin)
        // This decouples the driver from knowing the JAR location.
        execlp("g13-gui", "g13-gui", (char *)NULL);
        // --- CHANGE END ---

        // If we reach here, execlp failed (e.g. g13-gui not in PATH)
        syslog(LOG_ERR, "Failed to execute 'g13-gui'. Is it installed in /usr/bin?");
        _exit(1); 
    } 
    else {
        // PARENT PROCESS
        syslog(LOG_INFO, "GUI process started with PID: %d", pid);
    }
}

static void create_tray_icon() {
    GtkWidget *menu = gtk_menu_new();
    GtkWidget *gui_item = gtk_menu_item_new_with_label("Configure G13");
    GtkWidget *quit_item = gtk_menu_item_new_with_label("Quit Driver");

    g_signal_connect(gui_item, "activate", G_CALLBACK(show_gui), NULL);
    g_signal_connect(quit_item, "activate", G_CALLBACK(quit_driver), NULL);

    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gui_item);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), quit_item);
    gtk_widget_show_all(menu);

    indicator = app_indicator_new("g13-driver", "input-gaming", APP_INDICATOR_CATEGORY_APPLICATION_STATUS);
    app_indicator_set_status(indicator, APP_INDICATOR_STATUS_ACTIVE);
    app_indicator_set_menu(indicator, GTK_MENU(menu));
    app_indicator_set_icon(indicator, "input-gaming");
}

void signal_handler(int signum) {
    quit_driver(nullptr, nullptr);
}

static void quit_driver(GtkMenuItem *item, gpointer user_data) {
    if (!daemon_keep_running) return;

    syslog(LOG_INFO, "Shutting down driver...");
    daemon_keep_running = 0;
    
    // Join the device management thread
    if (device_thread.joinable()) {
        device_thread.join();
    }
    syslog(LOG_INFO, "Device management thread finished.");

    // Join all active G13 handler threads
    std::lock_guard<std::mutex> lock(g13_map_mutex);
    for (auto& [key, th] : g13_instances) {
        if (th.joinable()) {
            th.join();
        }
    }
    g13_instances.clear();
    syslog(LOG_INFO, "All G13 handler threads have finished.");

    // Clean up global resources
    if(indicator) {
        app_indicator_set_status(indicator, APP_INDICATOR_STATUS_PASSIVE);
    }
    UInput::close_uinput();
    libusb_exit(ctx);
    syslog(LOG_INFO, "Shutdown complete.");
    closelog(); 

    gtk_main_quit();
}

extern "C" int main(int argc, char *argv[]) {
    // 0. Initialize Syslog
    openlog("linux-g13-driver", LOG_PID | LOG_CONS, LOG_USER);

    // 1. Initialize GTK
    gtk_init(&argc, &argv);

    // 2. Determine paths (Not strictly needed for GUI anymore, but kept for logging/future use if needed)
    // removed the complex JAR path logic since we use the wrapper script now.

    // 3. Initialize driver components
    if (!UInput::create_uinput()) {
        syslog(LOG_ERR, "Failed to initialize uinput. Exiting.");
        return 1;
    }
    if (libusb_init(&ctx) < 0) {
        syslog(LOG_ERR, "Failed to initialize libusb. Exiting.");
        UInput::close_uinput();
        return 1;
    }

    // 4. Register signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 5. Create UI and start background threads
    create_tray_icon();
    syslog(LOG_INFO, "G13 driver started. Tray icon is active.");
    
    try {
        device_thread = std::thread(device_management_thread_loop);
    } catch (const std::system_error& e) {
        syslog(LOG_ERR, "Failed to create device management thread: %s", e.what());
        UInput::close_uinput();
        libusb_exit(ctx);
        return 1;
    }

    // 6. Start the GTK main loop
    gtk_main();

    return 0;
}