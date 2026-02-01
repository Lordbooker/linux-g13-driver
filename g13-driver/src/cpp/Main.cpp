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
#include <libgen.h> // For dirname()
#include <sys/wait.h> // For waitpid

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
std::string gui_jar_path;
libusb_context *ctx = nullptr;
std::thread device_thread;

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
                std::cout << "New G13 device connected (ID: " << std::hex << key << "). Starting handler thread." << std::endl;
                
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
    std::cout << "Attempting to start GUI from path: " << gui_jar_path << std::endl;

    pid_t pid = fork();

    if (pid == -1) {
        std::cerr << "Failed to fork process for GUI start." << std::endl;
    } 
    else if (pid == 0) {
        // CHILD PROCESS
        setsid(); // Detach from parent

        // Redirect stdout/stderr to avoid cluttering driver logs
        freopen("/dev/null", "w", stdout);
        freopen("/dev/null", "w", stderr);

        // Secure execution without shell
        execlp("java", "java", "-jar", gui_jar_path.c_str(), (char *)NULL);

        // If we reach here, execlp failed
        _exit(1); 
    } 
    else {
        // PARENT PROCESS
        std::cout << "GUI process started with PID: " << pid << std::endl;
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

    std::cout << "\nShutting down driver..." << std::endl;
    daemon_keep_running = 0;
    
    // Join the device management thread
    if (device_thread.joinable()) {
        device_thread.join();
    }
    std::cout << "Device management thread finished." << std::endl;

    // Join all active G13 handler threads
    std::lock_guard<std::mutex> lock(g13_map_mutex);
    for (auto& [key, th] : g13_instances) {
        if (th.joinable()) {
            th.join();
        }
    }
    g13_instances.clear();
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
    
    try {
        device_thread = std::thread(device_management_thread_loop);
    } catch (const std::system_error& e) {
        std::cerr << "Failed to create device management thread: " << e.what() << std::endl;
        UInput::close_uinput();
        libusb_exit(ctx);
        return 1;
    }

    // 6. Start the GTK main loop
    gtk_main();

    return 0;
}