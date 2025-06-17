package lordbooker.g13;

import java.io.IOException;
import java.io.Reader;
import java.io.Writer;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.Paths;
import java.nio.file.StandardCopyOption;
import java.util.Date;
import java.util.Properties;

public class Configs {

	public static final String [][] defaultBindings = {
			{"G34", "10"},         {"G29", "59"}, {"G30", "60"}, {"G31", "61"}, {"G32", "62"},        {"G35", "11"},
			{"G0" , "3" }, {"G1" , "4" }, {"G2",  "5" }, {"G3",  "6" }, {"G4",  "7" }, {"G5",  "8" }, {"G6",  "9" },
			{"G7" , "16"}, {"G8" , "17"}, {"G9",  "18"}, {"G10", "19"}, {"G11", "20"}, {"G12", "21"}, {"G13", "22"},
                           {"G14", "30"}, {"G15", "31"}, {"G16", "32"}, {"G17", "33"}, {"G18", "34"},
                                      {"G19", "44"}, {"G20", "45"}, {"G21", "46"},
            /* Stick Buttons */ {"G22", "57"}, {"G23", "58"},
            /* Stick */ {"G36" , "103"}, {"G37" , "105"}, {"G38",  "106"}, {"G39", "108"},

	};

	public static final String [][] defaultMacros = {
		{"CTRL-ALT-DEL", "kd.29,kd.56,kd.111,d.20,ku.111,ku.56,ku.29,d.100"},
		{"ALT-TAB", "kd.56,kd.15,d.20,ku.15,ku.56,d.100"},
		{"SHIFT-ALT-TAB", "kd.42,kd.56,kd.15,d.20,ku.15,ku.56,ku.42,d.100"},
		{"ALT-F1", "kd.56,kd.59,d.20,ku.59,ku.56,d.100"},
		{"ALT-F2", "kd.56,kd.60,d.20,ku.60,ku.56,d.100"},
		{"ALT-F3", "kd.56,kd.61,d.20,ku.61,ku.56,d.100"},
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

    private static final Path G13_CONFIG_DIR = Paths.get(System.getProperty("user.home"), ".g13");

	private static Path getBindingsPath(int item) {
		return G13_CONFIG_DIR.resolve("bindings-" + item + ".properties");
	}

	private static Path getMacroPath(int macroNum) {
		return G13_CONFIG_DIR.resolve("macro-" + macroNum + ".properties");
	}

	public static Properties loadBindings(int item) throws IOException {
		Path file = getBindingsPath(item);
		Properties props = new Properties();

		if (!Files.exists(file)) {
			Files.createDirectories(file.getParent());
			props.put("color", "255,255,255");
			for (final String [] binding: defaultBindings) {
				props.put(binding[0], "p,k." + binding[1]);
			}
			saveProperties(file, props);
		} else {
			try (Reader reader = Files.newBufferedReader(file)) {
				props.load(reader);
			}
		}
		return props;
	}


	public static void saveBindings(int item, Properties props) throws IOException {
		saveProperties(getBindingsPath(item), props);
	}

	public static Properties loadMacro(int macroNum) throws IOException {
		Path file = getMacroPath(macroNum);
		Properties props = new Properties();

		if (!Files.exists(file)) {
			Files.createDirectories(file.getParent());
			if (macroNum < defaultMacros.length) {
				props.put("name", defaultMacros[macroNum][0]);
				props.put("sequence", defaultMacros[macroNum][1]);
			} else {
				props.put("name", "");
				props.put("sequence", "");
			}
			props.put("id", Integer.toString(macroNum));
			saveProperties(file, props);
		} else {
			try (Reader reader = Files.newBufferedReader(file)) {
				props.load(reader);
			}
		}
		props.put("id", Integer.toString(macroNum));
		return props;
	}

	public static void saveMacro(int macroNum, Properties props) throws IOException {
		saveProperties(getMacroPath(macroNum), props);
	}

	private static void saveProperties(Path targetFile, Properties props) throws IOException {
		Path tempFile = Files.createTempFile(targetFile.getParent(), "g13-", ".tmp");
		try (Writer writer = Files.newBufferedWriter(tempFile)) {
			props.store(writer, new Date().toString());
		} catch (IOException e) {
			Files.deleteIfExists(tempFile); // Clean up temp file on failure
			throw e;
		}
		Files.move(tempFile, targetFile, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE);
	}
}