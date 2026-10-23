#ifndef G13_DRIVER_CONFIGPATH_H
#define G13_DRIVER_CONFIGPATH_H

#include <string>

/**
 * @brief Paths of the configuration files and of the LCD pipe.
 *
 * Configuration: $XDG_CONFIG_HOME/g13, else ~/.config/g13 (XDG Base Directory
 * Specification). LCD pipe: $XDG_RUNTIME_DIR/g13-lcd, else /tmp/g13-lcd.
 */
class ConfigPath {
public:
    ConfigPath() = delete;

    /// Configuration directory; empty if no home directory can be determined.
    static std::string getConfigDir();

    /// bindings-<id>.properties (profile 0..3); creates the directory if needed.
    static std::string getBindingPath(int bindingId);

    /// macro-<id>.properties; creates the directory if needed.
    static std::string getMacroPath(int macroId);

    /// settings.properties (settings for all profiles, e.g. intro=0|1).
    static std::string getSettingsPath();

    /// Path of the LCD pipe.
    static std::string getFifoPath();

private:
    static void ensureConfigDirExists();
};

#endif // G13_DRIVER_CONFIGPATH_H
