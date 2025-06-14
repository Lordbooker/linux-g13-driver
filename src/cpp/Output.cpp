#include <iostream>
#include <cstring>
#include <mutex>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/uinput.h>
#include <sys/time.h>

#include "Output.h"
#include "Constants.h"

namespace {
    int uinput_fd = -1;
    std::mutex uinput_mutex;
}

void send_event(int type, int code, int val) {
    if (uinput_fd < 0) {
        return;
    }

    struct input_event event;
    std::memset(&event, 0, sizeof(event));
    gettimeofday(&event.time, nullptr);
    event.type = static_cast<__u16>(type);
    event.code = static_cast<__u16>(code);
    event.value = val;

    std::lock_guard<std::mutex> lock(uinput_mutex);
    if (write(uinput_fd, &event, sizeof(event)) < 0) {
        std::cerr << "Warning: Failed to write event to uinput device." << std::endl;
    }
}

void flush() {
    if (uinput_fd < 0) return;
    std::lock_guard<std::mutex> lock(uinput_mutex);
    fsync(uinput_fd);
}

void close_uinput() {
    if (uinput_fd >= 0) {
        ioctl(uinput_fd, UI_DEV_DESTROY);
        close(uinput_fd);
        uinput_fd = -1;
    }
}

bool create_uinput() {
    const char* dev_uinput_fname =
            access("/dev/uinput", W_OK) == 0? "/dev/uinput" :
            access("/dev/input/uinput", W_OK) == 0? "/dev/input/uinput" : nullptr;

    if (!dev_uinput_fname) {
        std::cerr << "Could not find a writable uinput device. Check permissions." << std::endl;
        return false;
    }

    uinput_fd = open(dev_uinput_fname, O_WRONLY | O_NONBLOCK);
    if (uinput_fd < 0) {
        std::cerr << "Could not open uinput device: " << dev_uinput_fname << std::endl;
        return false;
    }

    struct uinput_user_dev uinp;
    std::memset(&uinp, 0, sizeof(uinp));
    strncpy(uinp.name, "Logitech G13", UINPUT_MAX_NAME_SIZE - 1);
    uinp.id.bustype = BUS_USB;
    uinp.id.vendor  = G13_VENDOR_ID;
    uinp.id.product = G13_PRODUCT_ID;
    uinp.id.version = 1;

    ioctl(uinput_fd, UI_SET_EVBIT, EV_KEY);
    ioctl(uinput_fd, UI_SET_EVBIT, EV_SYN);
    ioctl(uinput_fd, UI_SET_EVBIT, EV_ABS);
    
    for (int i = 0; i < KEY_MAX; ++i) {
        ioctl(uinput_fd, UI_SET_KEYBIT, i);
    }

    ioctl(uinput_fd, UI_SET_ABSBIT, ABS_X);
    ioctl(uinput_fd, UI_SET_ABSBIT, ABS_Y);
    uinp.absmin[ABS_X] = 0;
    uinp.absmax[ABS_X] = 255;
    uinp.absmin[ABS_Y] = 0;
    uinp.absmax[ABS_Y] = 255;

    if (write(uinput_fd, &uinp, sizeof(uinp)) < 0) {
        std::cerr << "Could not write uinput device setup." << std::endl;
        close(uinput_fd);
        uinput_fd = -1;
        return false;
    }

    if (ioctl(uinput_fd, UI_DEV_CREATE) < 0) {
        std::cerr << "Could not create uinput device." << std::endl;
        close(uinput_fd);
        uinput_fd = -1;
        return false;
    }

    return true;
}