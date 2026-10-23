#include "Output.h"
#include "Constants.h"

#include <fcntl.h>
#include <linux/uinput.h>
#include <sys/ioctl.h>
#include <syslog.h>
#include <unistd.h>

#include <cerrno>
#include <cstring>
#include <system_error>

int UInput::s_fd = -1;
bool UInput::s_writeErrorLogged = false;
std::mutex UInput::s_mutex;

namespace {

bool enableKey(int fd, int code)
{
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-vararg): ioctl is variadic
    return ioctl(fd, UI_SET_KEYBIT, code) == 0;
}

} // namespace

bool UInput::create_uinput()
{
    const std::lock_guard<std::mutex> lock(s_mutex);
    if (s_fd >= 0)
        return true;

    const int fd = ::open("/dev/uinput", O_WRONLY | O_NONBLOCK | O_CLOEXEC);
    if (fd < 0) {
        syslog(LOG_ERR, "Could not open /dev/uinput: %s", std::generic_category().message(errno).c_str());
        return false;
    }

    // Keyboard keys only (KEY_ESC..KEY_MICMUTE and KEY_OK..KEY_MAX without the
    // button ranges): with buttons or axes the device would be classified as
    // a mouse or joystick.
    bool ok = ioctl(fd, UI_SET_EVBIT, EV_KEY) == 0;   // NOLINT(cppcoreguidelines-pro-type-vararg)
    for (int code = KEY_ESC; ok && code < BTN_MISC; ++code)
        ok = enableKey(fd, code);
    for (int code = KEY_OK; ok && code < BTN_TRIGGER_HAPPY; ++code)
        ok = enableKey(fd, code);

    uinput_setup setup{};
    setup.id.bustype = BUS_USB;
    setup.id.vendor = G13_VENDOR_ID;
    setup.id.product = G13_PRODUCT_ID;
    setup.id.version = 1;
    std::strncpy(setup.name, "G13", UINPUT_MAX_NAME_SIZE - 1);

    ok = ok && ioctl(fd, UI_DEV_SETUP, &setup) == 0     // NOLINT(cppcoreguidelines-pro-type-vararg)
            && ioctl(fd, UI_DEV_CREATE) == 0;           // NOLINT(cppcoreguidelines-pro-type-vararg)
    if (!ok) {
        syslog(LOG_ERR, "Could not create the uinput device: %s", std::generic_category().message(errno).c_str());
        ::close(fd);
        return false;
    }
    s_fd = fd;
    s_writeErrorLogged = false;
    return true;
}

void UInput::close_uinput()
{
    const std::lock_guard<std::mutex> lock(s_mutex);
    if (s_fd < 0)
        return;
    ioctl(s_fd, UI_DEV_DESTROY);   // NOLINT(cppcoreguidelines-pro-type-vararg)
    ::close(s_fd);
    s_fd = -1;
}

void UInput::send_event(int type, int code, int value)
{
    const std::lock_guard<std::mutex> lock(s_mutex);
    if (s_fd < 0)
        return;

    input_event event{};
    event.type = static_cast<__u16>(type);
    event.code = static_cast<__u16>(code);
    event.value = value;   // the kernel sets the timestamp

    if (::write(s_fd, &event, sizeof(event)) != static_cast<ssize_t>(sizeof(event))) {
        // Log once per series, otherwise every key press would flood the log.
        if (!s_writeErrorLogged)
            syslog(LOG_ERR, "Could not send input event: %s", std::generic_category().message(errno).c_str());
        s_writeErrorLogged = true;
        return;
    }
    s_writeErrorLogged = false;
}
