#ifndef __G13_H__
#define __G13_H__

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <istream>
#include <libusb-1.0/libusb.h>
#include <time.h> // For time_t
#include <atomic>
#include <mutex>
#include <thread>

#include "Constants.h"
#include "G13Action.h"
#include "Macro.h"

class G13 {
private:
    std::vector<std::unique_ptr<G13Action>> actions;

    libusb_device        *device = nullptr;
    libusb_device_handle *handle = nullptr;
    int                   uinput_file;   

    int                   loaded = 0;
    std::atomic<bool>     keepGoing{false};   // stop() kann aus einem anderen Thread kommen

    stick_mode_t          stick_mode;    
    int                   stick_keys[4];   
    int                   bindings;      

    // Profiltasten L1-L4: vorheriger Zustand, damit nur beim Druecken
    // (Flanke) umgeschaltet wird und nicht bei jedem USB-Report.
    bool profile_key_down[4] = {false, false, false, false};

    // Aufeinanderfolgende USB-Lesefehler (ohne Timeout)
    int read_error_count = 0;
    int last_read_error  = 0;

    unsigned char lcd_buffer[G13_LCD_BUFFER_SIZE];
    // Schuetzt lcd_buffer und das Senden ans LCD (Startanimation laeuft in eigenem Thread)
    std::mutex lcd_mutex;

    // Startanimation (Flammen-Intro), siehe IntroAnimation
    std::thread       intro_thread;
    std::atomic<bool> intro_stop{false};
    static bool intro_enabled();
    void start_intro();
    void stop_intro();
    void run_intro();
    void draw_final_logo();   // erwartet gesperrten lcd_mutex

    // Feature: Live-Reload
    time_t last_config_mtime;
    void check_for_config_update();

    // --- Private Methods ---
    std::unique_ptr<Macro> loadMacro(int id);
    void parse_bindings_from_stream(std::istream& stream);
    void reset_actions();
    int  read();
    void parse_joystick(unsigned char *buf);
    void parse_key(int key, unsigned char *byte);
    void parse_keys(unsigned char *buf);

    // FIFO / Pipe for external input
    int fifo_fd = -1;        // File Descriptor for the pipe
    std::string fifo_path;   // Path to pipe (default: /tmp/g13-lcd)
    
    void init_fifo();        // Create pipe
    void check_fifo();       // Read pipe data
    void cleanup_fifo();     // Remove pipe


public:
    G13(libusb_device *device);
    ~G13();

    void start();
    bool isLoaded() const { return loaded != 0; }
    void stop();
    void loadBindings();
    void setColor(int r, int g, int b);

    // --- LCD ---
    void clear_lcd_buffer();
    void set_pixel(int x, int y, bool on);
    bool write_lcd();         // erwartet gesperrten lcd_mutex
    void write_char(int x, int y, char c);
    void write_text(int x, int y, const std::string& text);
};

#endif