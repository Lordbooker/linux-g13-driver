#ifndef __G13_H__
#define __G13_H__

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <array>
#include <atomic>
#include <libusb-1.0/libusb.h>

#include "Constants.h"
#include "G13Action.h"
#include "Macro.h"

// Forward declaration to avoid including libusb.h in all translation units
// struct libusb_device;
// struct libusb_device_handle;

class G13 {
private:
    // Use std::array for fixed-size container of actions.
    std::array<std::unique_ptr<G13Action>, G13_NUM_KEYS> _actions;
    
    libusb_device*        _device;
    libusb_device_handle* _handle;

    bool                  _is_loaded = false;
    std::atomic<bool>     _keep_running{false}; // Atomic flag for thread-safe shutdown

    stick_mode_t          _stick_mode = stick_mode_t::STICK_KEYS;
    int                   _stick_keys{};

    int                   _current_bindings_profile = 0;

    // Private helper methods for parsing and reading
    std::unique_ptr<Macro> loadMacro(int id);
    void read_loop();
    void parse_joystick(const unsigned char *buf);
    void parse_key(G13_KEYS key, const unsigned char *byte);
    void parse_keys(const unsigned char *buf);

public:
    explicit G13(libusb_device *device);
    ~G13();

    // Deleted copy constructor and assignment operator to prevent copying
    G13(const G13&) = delete;
    G13& operator=(const G13&) = delete;

    void start();
    void stop();
    void loadBindings();
    void setColor(int r, int g, int b);

    bool isLoaded() const { return _is_loaded; }
};

#endif // __G13_H__