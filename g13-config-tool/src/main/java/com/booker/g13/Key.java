package com.booker.g13;

import java.awt.Polygon;
import java.awt.Shape;
import java.util.List;
import java.util.stream.Collectors;
import java.util.stream.Stream;

/**
 * Represents a single physical key on the G13 keypad.
 * This class acts as a data container and a factory, holding the key's shape (for UI interaction),
 * its unique G13 keycode, and its currently assigned function.
 */
public class Key {

    /**
     * Raw data defining the polygon shapes for each key on the G13.
     * Each entry represents one key. The first sub-array is the G13 keycode.
     * Subsequent sub-arrays are the [x, y] coordinates of the polygon's vertices.
     */
	private static final int [][][] items = {
        {{25}, {162, 116}, {195, 116}, {195, 124}, {162, 124}, },
		{{26}, {205, 116}, {240, 116}, {240, 124}, {205, 124}, },
		{{27}, {250, 116}, {283, 116}, {283, 124}, {250, 124}, },
		{{28}, {293, 116}, {327, 116}, {327, 124}, {293, 124}, },
		{{24}, {110, 110}, {131, 110}, {131, 133}, {110, 133}, },
		{{29}, {97,  145}, {168, 145}, {168, 162}, {118, 162}, },
		{{30}, {173, 145}, {242, 145}, {242, 162}, {173, 162}, },
		{{31}, {247, 145}, {316, 145}, {316, 162}, {247, 162}, },
		{{32}, {321, 145}, {392, 145}, {374, 162}, {321, 162}, },
		{{0}, {55,  188}, {107, 188}, {104, 219}, {52, 212}, },
		{{1}, {117, 188}, {158, 188}, {158, 222}, {115, 220}, },
		{{2}, {172, 188}, {211, 188}, {211, 223}, {170, 220}, },
		{{3}, {225, 188}, {262, 188}, {262, 222}, {225, 222}, },
		{{4}, {278, 188}, {318, 188}, {318, 220}, {278, 220}, },
		{{5}, {331, 188}, {371, 188}, {371, 217}, {331, 219}, },
		{{6}, {383, 188}, {432, 188}, {435, 212}, {385, 215}, },
		{{7},  {48,  233}, {101, 236}, {100, 267}, {70, 267}, },
		{{8},  {114, 237}, {155, 239}, {155, 268}, {111, 267}, },
		{{9},  {168, 241}, {210, 241}, {210, 270}, {167, 270}, },
		{{10}, {223, 241}, {264, 241}, {264, 272}, {223, 272}, },
		{{11}, {278, 240}, {320, 240}, {320, 271}, {278, 271}, },
		{{12}, {333, 240}, {374, 237}, {376, 265}, {335, 268}, },
		{{13}, {387, 237}, {437, 232}, {420, 262}, {390, 265}, },
		{{14}, {84, 289}, {154, 293}, {152, 323}, {109, 323}, },
		{{15}, {165, 293}, {208, 294}, {209, 323}, {164, 323}, },
		{{16}, {223, 294}, {264, 293}, {265, 323}, {223, 323}, },
		{{17}, {279, 293}, {323, 292}, {323, 323}, {279, 324}, },
		{{18}, {335, 292}, {402, 288}, {380, 321}, {337, 322}, },
		{{19}, {122, 345}, {208, 345}, {208, 375}, {152, 375}, },
		{{20}, {222, 346}, {264, 346}, {266, 376}, {221, 376}, },
		{{21}, {279, 345}, {360, 343}, {341, 373}, {279, 373}, },
		{{33}, {365, 415}, {391, 389}, {388, 460}, {381, 467}, {365, 448}, },
		{{34}, {393, 493}, {415, 473}, {476, 470}, {413, 516}, },
		{{36}, {440, 390}, {454, 404}, {424, 404}, },
		{{37}, {410, 420}, {424, 406}, {424, 434}, },
		{{38}, {470, 420}, {456, 406}, {456, 433}, },
		{{39}, {440, 450}, {454, 436}, {424, 436}, },
    };
	
    /**
     * An immutable list of all Key objects, created once at class loading time from the 'items' data.
     */
	private static final List<Key> KEYS = Stream.of(items)
            .map(Key::new)
            .collect(Collectors.toUnmodifiableList());
	
	/**
	 * Returns the complete list of all defined G13 keys.
	 * @return An unmodifiable list of all Key objects.
	 */
	public static List<Key> getAllMasks() {
		return KEYS;
	}
	
	/**
	 * Finds the Key object located at a specific (x, y) coordinate.
	 * Used for hit-testing in the ImageMap UI.
	 * @param x The x-coordinate.
	 * @param y The y-coordinate.
	 * @return The Key at that position, or null if no key is found there.
	 */
	public static Key getKeyAt(int x, int y) {
		return KEYS.stream()
                .filter(key -> key.getShape().contains(x, y))
                .findFirst()
                .orElse(null);
	}
	
	/**
	 * Finds a Key object by its unique G13 keycode.
	 * @param g13KeyCode The keycode to search for (e.g., 0 for G0).
	 * @return The matching Key object, or null if not found.
	 */
	public static Key getKeyFor(int g13KeyCode) {
		return KEYS.stream()
                .filter(key -> key.getG13KeyCode() == g13KeyCode)
                .findFirst()
                .orElse(null);
	}
	
	// --- Instance Properties ---
	private final Shape shape; // The polygon shape of the key for UI interaction.
	private final int g13KeyCode; // The unique identifier for this key.
	private String mappedValue = "Unknown"; // The human-readable value this key is mapped to.
	private String repeats = "Unknown"; // A string indicating if the key's macro repeats ("Yes", "No", "N/A").
	
	/**
	 * Private constructor to create a Key instance.
	 * Called by the static initializer stream.
	 * @param buttonData The raw data array for a single key.
	 */
	private Key(int[][] buttonData) {
		this.g13KeyCode = buttonData[0][0];
		
		final Polygon polygon = new Polygon();
        for (int i = 1; i < buttonData.length; i++) {
            polygon.addPoint(buttonData[i][0], buttonData[i][1]);
        }
		this.shape = polygon;
	}

	// --- Getters and Setters ---
	public Shape getShape() { return shape; }
	public int getG13KeyCode() { return g13KeyCode; }
	public String getMappedValue() { return mappedValue; }
	public String getRepeats() { return repeats; }

	public void setMappedValue(String mappedValue) { this.mappedValue = mappedValue; }
	public void setRepeats(String repeats) { this.repeats = repeats; }
}