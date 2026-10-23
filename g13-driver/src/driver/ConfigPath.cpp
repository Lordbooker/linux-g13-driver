#include "ConfigPath.h"

#include <pwd.h>
#include <syslog.h>
#include <unistd.h>

#include <cstdlib>
#include <filesystem>
#include <system_error>

namespace {

// The driver never calls setenv(), so reading the environment is safe from
// several threads.
const char *environment(const char *name)
{
    const char *value = std::getenv(name);   // NOLINT(concurrency-mt-unsafe)
    return (value != nullptr && *value != '\0') ? value : nullptr;
}

std::string configDirFromEnvironment()
{
    if (const char *xdgConfig = environment("XDG_CONFIG_HOME"))
        return std::string(xdgConfig) + "/g13";

    const char *home = environment("HOME");
    if (home == nullptr) {
        // Only called once (see getConfigDir), so getpwuid() is not called concurrently.
        const passwd *pw = getpwuid(getuid());   // NOLINT(concurrency-mt-unsafe)
        if (pw == nullptr || pw->pw_dir == nullptr)
            return {};   // no safe location; main() refuses to start
        home = pw->pw_dir;
    }
    return std::string(home) + "/.config/g13";
}

} // namespace

std::string ConfigPath::getConfigDir()
{
    // Determined once; the environment of the driver does not change.
    static const std::string dir = configDirFromEnvironment();
    return dir;
}

void ConfigPath::ensureConfigDirExists()
{
    const std::string dir = getConfigDir();
    if (dir.empty())
        return;
    std::error_code error;
    std::filesystem::create_directories(dir, error);
    if (error)
        syslog(LOG_ERR, "Could not create %s: %s", dir.c_str(), error.message().c_str());
}

std::string ConfigPath::getBindingPath(int bindingId)
{
    ensureConfigDirExists();
    return getConfigDir() + "/bindings-" + std::to_string(bindingId) + ".properties";
}

std::string ConfigPath::getMacroPath(int macroId)
{
    ensureConfigDirExists();
    return getConfigDir() + "/macro-" + std::to_string(macroId) + ".properties";
}

std::string ConfigPath::getSettingsPath()
{
    ensureConfigDirExists();
    return getConfigDir() + "/settings.properties";
}

std::string ConfigPath::getFifoPath()
{
    if (const char *runtimeDir = environment("XDG_RUNTIME_DIR"))
        return std::string(runtimeDir) + "/g13-lcd";
    return "/tmp/g13-lcd";
}
