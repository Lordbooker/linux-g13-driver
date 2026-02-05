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
 * Updated to be XDG-compliant (uses ~/.config/g13).
 */
public class Configs {

	/**
	 * Default key bindings mapping G-keys (G0, G1, etc.) to Linux keycodes.
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
	 */
	public static final String [][] defaultMacros = {
		{"CTRL-ALT-DEL", "kd.29,kd.56,kd.111,d.20,ku.111,ku.56,ku.29,d.100"},
		{"ALT-TAB", "kd.56,kd.15,d.20,ku.15,ku.56,d.100"},
		// ... (Same defaults as before)
		{"SHIFT-ALT-TAB", "kd.42,kd.56,kd.15,d.20,ku.15,ku.56,ku.42,d.100"},
        {"ALT-F4", "kd.56,kd.62,d.20,ku.62,ku.56,d.100"},
        {"Pause", "kd.119,d.20,ku.119,d.100"},
	};

    public static final int DEFAULT_MACROS_COUNT = defaultMacros.length;

    /**
     * Returns the platform-independent path to the configuration directory.
     * Implements XDG standard: ~/.config/g13
     * @return The Path object for the configuration directory.
     */
	private static Path getConfigDir() {
        String xdgConfig = System.getenv("XDG_CONFIG_HOME");
        Path baseDir;
        if (xdgConfig != null && !xdgConfig.isBlank()) {
            baseDir = Path.of(xdgConfig);
        } else {
            baseDir = Path.of(System.getProperty("user.home"), ".config");
        }
        return baseDir.resolve("g13");
    }

	public static Properties loadBindings(int item) throws IOException {
		Path file = getConfigDir().resolve("bindings-" + item + ".properties");

		if (Files.notExists(file)) {
			Files.createDirectories(file.getParent());
			final Properties props = new Properties();
			props.put("color", "255,255,255"); 

			for (final String [] binding: defaultBindings) {
				props.put(binding[0], "p,k." + binding[1]);
			}
			saveBindings(item, props);
			return props;
		}

		final Properties props = new Properties();
		try (FileInputStream fis = new FileInputStream(file.toFile())) {
			props.load(fis);
		}
		return props;
	}

	public static void saveBindings(int item, Properties props) throws IOException {
		Path file = getConfigDir().resolve("bindings-" + item + ".properties");
        Files.createDirectories(file.getParent()); 
		try (FileOutputStream fos = new FileOutputStream(file.toFile())) {
			props.store(fos, new Date().toString());
		}
	}

	public static Properties loadMacro(int macroNum) throws IOException {
		Path file = getConfigDir().resolve("macro-" + macroNum + ".properties");

		if (Files.notExists(file)) {
			Files.createDirectories(file.getParent());
			final Properties props = new Properties();

			if (macroNum < DEFAULT_MACROS_COUNT) {
				props.put("name", defaultMacros[macroNum][0]);
				props.put("sequence", defaultMacros[macroNum][1]);
			} else {
				props.put("name", "");
				props.put("sequence", "");
			}
			props.put("id", Integer.toString(macroNum));
			saveMacro(macroNum, props);
			return props;
		}

		final Properties props = new Properties();
		try (FileInputStream fis = new FileInputStream(file.toFile())) {
			props.load(fis);
		}
		props.put("id", Integer.toString(macroNum));
		return props;
	}

	public static void saveMacro(int macroNum, Properties props) throws IOException {
		Path file = getConfigDir().resolve("macro-" + macroNum + ".properties");
        Files.createDirectories(file.getParent());
		try (FileOutputStream fos = new FileOutputStream(file.toFile())) {
			props.store(fos, new Date().toString());
		}
	}
}