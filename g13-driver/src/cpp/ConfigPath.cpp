#include "ConfigPath.h"
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <pwd.h>
#include <iostream>
#include <limits.h>

// Helper to check if a directory exists
static bool dirExists(const std::string& path) {
    struct stat info;
    if (stat(path.c_str(), &info) != 0) {
        return false;
    }
    return (info.st_mode & S_IFDIR) != 0;
}

std::string ConfigPath::getConfigDir() {
    // 1. Try XDG_CONFIG_HOME
    const char* xdgConfig = getenv("XDG_CONFIG_HOME");
    std::string baseDir;

    if (xdgConfig && *xdgConfig) {
        baseDir = std::string(xdgConfig);
    } else {
        // 2. Fallback to HOME/.config
        const char* home = getenv("HOME");
        if (!home) {
            // Fallback for safety if HOME is unset (unlikely for user apps)
            struct passwd* pw = getpwuid(getuid());
            if (pw) {
                home = pw->pw_dir;
            } else {
                return "/tmp/g13-fallback"; 
            }
        }
        baseDir = std::string(home) + "/.config";
    }

    return baseDir + "/g13";
}

void ConfigPath::ensureConfigDirExists() {
    std::string path = getConfigDir();
    if (!dirExists(path)) {
        // Create directory with 0755 permissions
        // Note: mkdir only creates the last level, implies ~/.config exists. 
        // For robustness, a recursive mkdir would be better, but this suffices for standard systems.
        mkdir(path.c_str(), 0755);
    }
}

std::string ConfigPath::getBindingPath(int bindingId) {
    ensureConfigDirExists();
    return getConfigDir() + "/bindings-" + std::to_string(bindingId) + ".properties";
}

std::string ConfigPath::getMacroPath(int macroId) {
    ensureConfigDirExists();
    return getConfigDir() + "/macro-" + std::to_string(macroId) + ".properties";
}

std::string ConfigPath::getFifoPath() {
    // Ideally use XDG_RUNTIME_DIR for pipes (/run/user/1000/)
    const char* xdgRuntime = getenv("XDG_RUNTIME_DIR");
    if (xdgRuntime && *xdgRuntime) {
        return std::string(xdgRuntime) + "/g13-lcd";
    }
    // Fallback to tmp
    return "/tmp/g13-lcd";
}