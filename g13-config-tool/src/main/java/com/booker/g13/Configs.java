package com.booker.g13;

import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Date;
import java.util.Properties;

/**
 * Utility class for managing configuration files.
 * This class handles loading and saving key bindings and macros from .properties files.
 */
public class Configs {

	/**
	 * Default key bindings mapping G-keys (G0, G1, etc.) to Linux keycodes.
	 * Format: { "G-Key Name", "Linux Keycode" }
	 */
	public static final String [][] defaultBindings = {
			{"G34", "10"},         {"G29", "59"}, {"G30", "60"}, {"G31", "61"}, {"G32", "62"},        {"G35", "11"},
			{"G0" , "3" }, {"G1" , "4" }, {"G2",  "5" }, {"G3",  "6" }, {"G4",  "7" }, {"G5",  "8" }, {"G6",  "9" },
			{"G7" , "16"}, {"G8" , "17"}, {"G9",  "18"}, {"G10", "19"}, {"G11", "20"}, {"G12", "21"}, {"G13", "22"},
			{"G14", "30"}, {"G15", "31"}, {"G16", "32"}, {"G17", "33"}, {"G18", "34"},
			{"G19", "44"}, {"G20", "45"}, {"G21", "46"},
            /* Stick Buttons */ {"G22", "57"}, {"G23", "58"},
            /* Stick */ {"G36" , "103"}, {"G37" , "105"}, {"G38" , "106"}, {"G39", "108"},
	};

	/**
	 * Default macro definitions.
	 * Format: { "Macro Name", "Macro Sequence String" }
	 * The sequence consists of key down (kd), key up (ku), and delay (d) commands.
	 */
	public static final String [][] defaultMacros = {
		{"CTRL-ALT-DEL", "kd.29,kd.56,kd.111,d.20,ku.111,ku.56,ku.29,d.100"},
		{"ALT-TAB", "kd.56,kd.15,d.20,ku.15,ku.56,d.100"},
		{"SHIFT-ALT-TAB", "kd.42,kd.56,kd.15,d.20,ku.15,ku.56,ku.42,d.100"},
		// ... more default macros
		{"ALT-F4", "kd.56,kd.62,d.20,ku.62,ku.56,d.100"},
		{"ALT-F5", "kd.56,kd.63,d.20,ku.63,ku.56,d.100"},
		{"ALT-F6", "kd.56,kd.64,d.20,ku.64,ku.56,d.100"},
		{"ALT-F7", "kd.56,kd.65,d.20,ku.65,ku.56,d.100"},
		{"ALT-F8", "kd.56,kd.66,d.20,ku.66,ku.56,d.100"},
		{"ALT-F9", "kd.56,kd.67,d.20,ku.67,ku.56,d.100"},
		{"ALT-F10", "kd.56,kd.68,d.20,ku.68,ku.56,d.100"},
		{"ALT-F11", "kd.56,kd.87,d.20,ku.87,ku.56,d.100"},
		{"ALT-F12", "kd.56,kd.88,d.20,ku.88,ku.56,d.100"},
		{"Print Screen", "kd.99,d.20,ku.99,d.100"},
		{"ALT-Print Screen", "kd.56,kd.99,d.20,ku.99,ku.56,d.100"},
		{"CTRL-Print Screen", "kd.29,kd.99,d.20,ku.99,ku.29,d.100"},
		{"Pause", "kd.119,d.20,ku.119,d.100"},
	};

	/**
	 * A constant holding the count of default macros, to prevent their modification.
	 */
    public static final int DEFAULT_MACROS_COUNT = defaultMacros.length;

    /**
     * Returns the platform-independent path to the configuration directory (.g13 in the user's home).
     * @return The Path object for the configuration directory.
     */
	private static Path getConfigDir() {
        return Path.of(System.getProperty("user.home"), ".g13");
    }

	/**
	 * Loads a specific key binding profile from a .properties file.
	 * If the file does not exist, it creates a new one with default bindings.
	 * @param item The index of the binding profile (e.g., 0 for M1, 1 for M2).
	 * @return A Properties object containing the key bindings.
	 * @throws IOException If an I/O error occurs while reading or creating the file.
	 */
	public static Properties loadBindings(int item) throws IOException {
		Path file = getConfigDir().resolve("bindings-" + item + ".properties");

		if (Files.notExists(file)) {
			// Create a new file with default values if it doesn't exist.
			Files.createDirectories(file.getParent());

			final Properties props = new Properties();
			props.put("color", "255,255,255"); // Default screen color is white

			for (final String [] binding: defaultBindings) {
				// Default binding type is 'passthrough' (p) with a key (k).
				props.put(binding[0], "p,k." + binding[1]);
			}

			saveBindings(item, props);
			return props;
		}

		final Properties props = new Properties();
		// Use try-with-resources for safe and automatic stream handling.
		try (FileInputStream fis = new FileInputStream(file.toFile())) {
			props.load(fis);
		}

		return props;
	}


	/**
	 * Saves a key binding profile to a .properties file.
	 * @param item The index of the binding profile to save.
	 * @param props The Properties object containing the key bindings to be saved.
	 * @throws IOException If an I/O error occurs while writing the file.
	 */
	public static void saveBindings(int item, Properties props) throws IOException {
		Path file = getConfigDir().resolve("bindings-" + item + ".properties");
        Files.createDirectories(file.getParent()); // Ensure the parent directory exists.

		// Use try-with-resources to safely write to the file.
		try (FileOutputStream fos = new FileOutputStream(file.toFile())) {
			props.store(fos, new Date().toString());
		}
	}

	/**
	 * Loads a specific macro from a .properties file.
	 * If the file doesn't exist, it creates a new one, either with a default macro sequence or empty.
	 * @param macroNum The index number of the macro.
	 * @return A Properties object containing the macro data (name, sequence, id).
	 * @throws IOException If an I/O error occurs.
	 */
	public static Properties loadMacro(int macroNum) throws IOException {
		Path file = getConfigDir().resolve("macro-" + macroNum + ".properties");

		if (Files.notExists(file)) {
			Files.createDirectories(file.getParent());

			final Properties props = new Properties();

			if (macroNum < DEFAULT_MACROS_COUNT) {
				// Load from the hardcoded default macros.
				props.put("name", defaultMacros[macroNum][0]);
				props.put("sequence", defaultMacros[macroNum][1]);
			} else {
				// Create a new, empty macro for user definition.
				props.put("name", "");
				props.put("sequence", "");
			}
			props.put("id", Integer.toString(macroNum));
			saveMacro(macroNum, props);

			return props;
		}

		final Properties props = new Properties();
		// Use try-with-resources for safe file reading.
		try (FileInputStream fis = new FileInputStream(file.toFile())) {
			props.load(fis);
		}

		// Ensure the ID is present in the loaded properties.
		props.put("id", Integer.toString(macroNum));

		return props;
	}

	/**
	 * Saves a macro to its corresponding .properties file.
	 * @param macroNum The index number of the macro to save.
	 * @param props The Properties object containing the macro data.
	 * @throws IOException If an I/O error occurs during file writing.
	 */
	public static void saveMacro(int macroNum, Properties props) throws IOException {
		Path file = getConfigDir().resolve("macro-" + macroNum + ".properties");
        Files.createDirectories(file.getParent());

		// Use try-with-resources for safe stream handling.
		try (FileOutputStream fos = new FileOutputStream(file.toFile())) {
			props.store(fos, new Date().toString());
		}
	}
}