#ifndef CONFIGPATH_H
#define CONFIGPATH_H

#include <string>

/**
 * @class ConfigPath
 * @brief Helper class to handle XDG-compliant file paths.
 * * This class determines the correct location for configuration files 
 * according to the XDG Base Directory Specification.
 * It defaults to ~/.config/g13/ for user configurations.
 */
class ConfigPath {
public:
    /**
     * @brief Gets the full path to a specific binding configuration file.
     * @param bindingId The ID of the binding profile (e.g., 0, 1, 2).
     * @return The absolute path string (e.g., "/home/user/.config/g13/bindings-0.properties").
     */
    static std::string getBindingPath(int bindingId);

    /**
     * @brief Gets the full path to a specific macro file.
     * @param macroId The ID of the macro.
     * @return The absolute path string.
     */
    static std::string getMacroPath(int macroId);

    /**
     * @brief Gets the full path to the FIFO pipe.
     * @return The absolute path (e.g., "/run/user/1000/g13-lcd" or fallback to "/tmp/g13-lcd").
     */
    static std::string getFifoPath();

    /**
     * @brief Ensures that the configuration directory exists.
     * Creates it if it is missing.
     */
    static void ensureConfigDirExists();

private:
    /**
     * @brief Internal helper to determine the user's config directory.
     * Checks XDG_CONFIG_HOME or defaults to HOME/.config/g13.
     */
    static std::string getConfigDir();
};

#endif // CONFIGPATH_H