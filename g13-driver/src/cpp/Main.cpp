#include <iostream>
#include <fstream>
#include <vector>
#include <map>
#include <string>
#include <csignal>
#include <cstdlib>
#include <unistd.h>
#include <pthread.h>
#include <libusb-1.0/libusb.h>
#include <iomanip>
#include <libgen.h> // For dirname()

// Headers for the tray icon functionality
#include <gtk/gtk.h>
#include <libappindicator/app-indicator.h>

#include "G13.h"
#include "Output.h"

// --- Global Variables ---
pthread_mutex_t g13_map_mutex = PTHREAD_MUTEX_INITIALIZER;
std::map<uint16_t, pthread_t> g13_instances;
volatile sig_atomic_t daemon_keep_running = 1;

AppIndicator *indicator = NULL;
std::string gui_jar_path;
libusb_context *ctx = nullptr;
pthread_t device_thread;

// --- Forward Declarations ---
static void quit_driver(GtkMenuItem *item, gpointer user_data);

// --- Helper Functions ---
std::string get_executable_directory() {
    char result[1024];
    ssize_t count = readlink("/proc/self/exe", result, sizeof(result) - 1);
    if (count != -1) {
        result[count] = '\0';
        return std::string(dirname(result));
    }
    return ".";
}

uint16_t get_device_key(libusb_device *dev) {
    return (libusb_get_bus_number(dev) << 8) | libusb_get_device_address(dev);
}

// --- G13 Device Handling ---
void *executeG13(void *arg) {
    libusb_device *dev = (libusb_device *)arg;
    uint16_t key = get_device_key(dev);
    G13 g13(dev);
    g13.start();

    pthread_mutex_lock(&g13_map_mutex);
    g13_instances.erase(key);
    pthread_mutex_unlock(&g13_map_mutex);
    libusb_unref_device(dev);
    return nullptr;
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
            pthread_mutex_lock(&g13_map_mutex);
            if (g13_instances.find(key) == g13_instances.end()) {
                std::cout << "New G13 device connected (ID: " << std::hex << key << "). Starting handler thread." << std::endl;
                pthread_t thread;
                libusb_ref_device(devs[i]);
                pthread_create(&thread, nullptr, executeG13, devs[i]);
                g13_instances[key] = thread;
            }
            pthread_mutex_unlock(&g13_map_mutex);
        }
    }
    libusb_free_device_list(devs, 1);
}

void* device_management_thread_loop(void* arg) {
    while (daemon_keep_running) {
        find_and_manage_devices();
        sleep(1);
    }
    return nullptr;
}

// --- Tray Icon and Main Application Logic ---
static void show_gui(GtkMenuItem *item, gpointer user_data) {
    std::string command = "java -jar \"" + gui_jar_path + "\" &";
    std::cout << "Executing: " << command << std::endl;
    system(command.c_str());
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

    std::cout << "\nShutting down driver..." << std::endl;
    daemon_keep_running = 0;
    
    // Join the device management thread
    pthread_join(device_thread, nullptr);
    std::cout << "Device management thread finished." << std::endl;

    // Join all active G13 handler threads
    std::vector<pthread_t> threads_to_join;
    pthread_mutex_lock(&g13_map_mutex);
    for (auto const& [key, thread_id] : g13_instances) {
        threads_to_join.push_back(thread_id);
    }
    pthread_mutex_unlock(&g13_map_mutex);
    for (pthread_t th : threads_to_join) {
        pthread_join(th, nullptr);
    }
    std::cout << "All G13 handler threads have finished." << std::endl;

    // Clean up global resources
    if(indicator) {
        app_indicator_set_status(indicator, APP_INDICATOR_STATUS_PASSIVE);
    }
    UInput::close_uinput();
    libusb_exit(ctx);
    std::cout << "Shutdown complete." << std::endl;

    gtk_main_quit();
}

extern "C" int main(int argc, char *argv[]) {
    // 1. Initialize GTK
    gtk_init(&argc, &argv);

    // 2. Determine paths and parse arguments
    gui_jar_path = get_executable_directory() + "/Linux-G13-GUI.jar";
    for (int i = 1; i < argc; ++i) {
        if (std::string(argv[i]) == "--gui-path" && i + 1 < argc) {
            gui_jar_path = argv[++i];
            std::cout << "GUI path set from command line: " << gui_jar_path << std::endl;
        }
    }

    // 3. Initialize driver components
    if (!UInput::create_uinput()) {
        std::cerr << "Failed to initialize uinput. Exiting." << std::endl;
        return 1;
    }
    if (libusb_init(&ctx) < 0) {
        std::cerr << "Failed to initialize libusb. Exiting." << std::endl;
        UInput::close_uinput();
        return 1;
    }

    // 4. Register signal handlers
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    // 5. Create UI and start background threads
    create_tray_icon();
    std::cout << "G13 driver started. Tray icon is active." << std::endl;
    if (pthread_create(&device_thread, nullptr, device_management_thread_loop, nullptr) != 0) {
        std::cerr << "Failed to create device management thread. Exiting." << std::endl;
        UInput::close_uinput();
        libusb_exit(ctx);
        return 1;
    }

    // 6. Start the GTK main loop
    gtk_main();

    return 0;
}
