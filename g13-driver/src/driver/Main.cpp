#include "ConfigPath.h"
#include "Constants.h"
#include "G13.h"
#include "Output.h"

#include <libusb-1.0/libusb.h>
#include <syslog.h>

#include <atomic>
#include <chrono>
#include <csignal>
#include <cstdint>
#include <map>
#include <memory>
#include <system_error>
#include <thread>

namespace {

// Set to false by SIGINT/SIGTERM. A lock-free atomic may be used in a signal
// handler and is safe to read from all threads.
std::atomic<bool> g_keepRunning{true};   // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)
static_assert(std::atomic<bool>::is_always_lock_free);

extern "C" void handleSignal(int /*signal*/)
{
    g_keepRunning = false;
}

bool installSignalHandlers()
{
    struct sigaction action {};
    action.sa_handler = handleSignal;   // NOLINT(cppcoreguidelines-pro-type-union-access)
    sigemptyset(&action.sa_mask);
    return sigaction(SIGINT, &action, nullptr) == 0 && sigaction(SIGTERM, &action, nullptr) == 0;
}

/**
 * @brief Finds G13 devices and runs one G13 object per device in its own thread.
 *
 * Polls the USB device list once per second. A device whose handler has
 * finished (unplugged, read errors) is picked up again when it shows up; a
 * device that could not be opened is retried after a delay.
 */
class DeviceManager {
public:
    explicit DeviceManager(libusb_context *context) : m_context(context) {}
    ~DeviceManager() { joinAll(); }

    DeviceManager(const DeviceManager &) = delete;
    DeviceManager &operator=(const DeviceManager &) = delete;
    DeviceManager(DeviceManager &&) = delete;
    DeviceManager &operator=(DeviceManager &&) = delete;

    void run()
    {
        while (g_keepRunning) {
            scan();
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        joinAll();
    }

private:
    enum class State : std::uint8_t { Running, Finished, OpenFailed };

    struct Handler {
        std::thread thread;
        std::shared_ptr<std::atomic<State>> state;
    };

    // Delay before a device is retried after opening it failed
    // (e.g. because the udev permissions were not set yet when it was plugged in).
    static constexpr auto OpenRetryDelay = std::chrono::seconds(5);

    static std::uint16_t deviceKey(libusb_device *device)
    {
        return static_cast<std::uint16_t>((libusb_get_bus_number(device) << 8U)
                                          | libusb_get_device_address(device));
    }

    static void runDevice(libusb_device *device, const std::shared_ptr<std::atomic<State>> &state)
    {
        bool opened = false;
        {
            G13 g13(device, g_keepRunning);
            opened = g13.isLoaded();
            g13.start();
        }   // the G13 destructor releases the USB resources
        libusb_unref_device(device);
        // Last action: scan() cleans up the entry afterwards.
        state->store(opened ? State::Finished : State::OpenFailed);
    }

    // Join finished device threads and remove them, so that a replugged or
    // previously failed device is detected again.
    void reapFinished()
    {
        for (auto it = m_handlers.begin(); it != m_handlers.end();) {
            const State state = it->second.state->load();
            if (state == State::Running) {
                ++it;
                continue;
            }
            it->second.thread.join();
            if (state == State::OpenFailed) {
                m_retryAfter[it->first] = std::chrono::steady_clock::now() + OpenRetryDelay;
                syslog(LOG_WARNING, "Could not open G13 device (ID: %x). Retrying in %lld s.",
                       it->first, static_cast<long long>(OpenRetryDelay.count()));
            } else {
                syslog(LOG_INFO, "Handler for G13 device (ID: %x) finished.", it->first);
            }
            it = m_handlers.erase(it);
        }
    }

    void scan()
    {
        reapFinished();

        libusb_device **devices = nullptr;
        const ssize_t count = libusb_get_device_list(m_context, &devices);
        if (count < 0)
            return;
        const auto now = std::chrono::steady_clock::now();

        for (ssize_t i = 0; i < count; ++i) {
            libusb_device *device = devices[i];   // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            libusb_device_descriptor descriptor{};
            if (libusb_get_device_descriptor(device, &descriptor) != 0
                || descriptor.idVendor != G13_VENDOR_ID || descriptor.idProduct != G13_PRODUCT_ID)
                continue;

            const std::uint16_t key = deviceKey(device);
            if (m_handlers.contains(key))
                continue;
            if (const auto retry = m_retryAfter.find(key); retry != m_retryAfter.end()) {
                if (now < retry->second)
                    continue;   // retry delay after a failure still running
                m_retryAfter.erase(retry);
            }

            syslog(LOG_INFO, "New G13 device found (ID: %x). Starting handler.", key);
            auto state = std::make_shared<std::atomic<State>>(State::Running);
            libusb_ref_device(device);
            try {
                m_handlers[key] = Handler{std::thread(runDevice, device, state), state};
            } catch (const std::system_error &e) {
                libusb_unref_device(device);
                syslog(LOG_ERR, "Could not start the device thread: %s", e.what());
            }
        }
        libusb_free_device_list(devices, 1);
    }

    void joinAll()
    {
        for (auto &[key, handler] : m_handlers) {
            if (handler.thread.joinable())
                handler.thread.join();
        }
        m_handlers.clear();
    }

    libusb_context *m_context;
    std::map<std::uint16_t, Handler> m_handlers;
    std::map<std::uint16_t, std::chrono::steady_clock::time_point> m_retryAfter;
};

} // namespace

int main()
{
    openlog("linux-g13-driver", LOG_PID | LOG_CONS, LOG_USER);   // NOLINT(hicpp-signed-bitwise)
    syslog(LOG_INFO, "G13 driver starting (version %s)...", G13_VERSION);

    if (ConfigPath::getConfigDir().empty()) {
        syslog(LOG_ERR, "Could not determine the home directory. Exiting.");
        return 1;
    }
    if (!installSignalHandlers()) {
        syslog(LOG_ERR, "Could not install signal handlers. Exiting.");
        return 1;
    }
    if (!UInput::create_uinput()) {
        syslog(LOG_ERR, "Could not initialize uinput. Exiting.");
        return 1;
    }

    libusb_context *context = nullptr;
    const int initResult = libusb_init(&context);
    if (initResult < 0) {
        syslog(LOG_ERR, "Could not initialize libusb: %s. Exiting.", libusb_error_name(initResult));
        UInput::close_uinput();
        return 1;
    }

    syslog(LOG_INFO, "Driver active. Waiting for G13 devices...");
    {
        DeviceManager manager(context);
        manager.run();   // returns after SIGINT/SIGTERM, all device threads joined
        syslog(LOG_INFO, "Shutting down driver...");
    }

    UInput::close_uinput();
    libusb_exit(context);
    syslog(LOG_INFO, "Shutdown complete.");
    closelog();
    return 0;
}
