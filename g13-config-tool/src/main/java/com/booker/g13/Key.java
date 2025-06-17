package com.booker.g13;

import java.awt.Polygon;
import java.awt.Shape;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;

public class Key {

	private static final List<Key> keys = new ArrayList<>();

	// Data structure represents: { {keyCode}, {x1,y1}, {x2,y2}, ... }
	private static final int[][][] KEY_DEFINITIONS = {
			// M-Keys (remapped to G-codes for internal consistency)
			{{25}, {162, 116}, {195, 116}, {195, 124}, {162, 124},}, // M1 -> G25
			{{26}, {205, 116}, {240, 116}, {240, 124}, {205, 124},}, // M2 -> G26
			{{27}, {250, 116}, {283, 116}, {283, 124}, {250, 124},}, // M3 -> G27
			{{28}, {293, 116}, {327, 116}, {327, 124}, {293, 124},}, // MR -> G28

			// G-Keys
			{{0}, {55, 188}, {107, 188}, {104, 219}, {52, 212},},   // G1
			{{1}, {117, 188}, {158, 188}, {158, 222}, {115, 220},},  // G2
			{{2}, {172, 188}, {211, 188}, {211, 223}, {170, 220},},  // G3
			{{3}, {225, 188}, {262, 188}, {262, 222}, {225, 222},},  // G4
			{{4}, {278, 188}, {318, 188}, {318, 220}, {278, 220},},  // G5
			{{5}, {331, 188}, {371, 188}, {371, 217}, {331, 219},},  // G6
			{{6}, {383, 188}, {432, 188}, {435, 212}, {385, 215},},  // G7
			{{7}, {48, 233}, {101, 236}, {100, 267}, {70, 267},},    // G8
			{{8}, {114, 237}, {155, 239}, {155, 268}, {111, 267},},  // G9
			{{9}, {168, 241}, {210, 241}, {210, 270}, {167, 270},},  // G10
			{{10}, {223, 241}, {264, 241}, {264, 272}, {223, 272},}, // G11
			{{11}, {278, 240}, {320, 240}, {320, 271}, {278, 271},}, // G12
			{{12}, {333, 240}, {374, 237}, {376, 265}, {335, 268},}, // G13
			{{13}, {387, 237}, {437, 232}, {420, 262}, {390, 265},}, // G14
			{{14}, {84, 289}, {154, 293}, {152, 323}, {109, 323},},  // G15
			{{15}, {165, 293}, {208, 294}, {209, 323}, {164, 323},}, // G16
			{{16}, {223, 294}, {264, 293}, {265, 323}, {223, 323},}, // G17
			{{17}, {279, 293}, {323, 292}, {323, 323}, {279, 324},}, // G18
			{{18}, {335, 292}, {402, 288}, {380, 321}, {337, 322},}, // G19
			{{19}, {122, 345}, {208, 345}, {208, 375}, {152, 375},}, // G20
			{{20}, {222, 346}, {264, 346}, {266, 376}, {221, 376},}, // G21
			{{21}, {279, 345}, {360, 343}, {341, 373}, {279, 373},}, // G22

			// Thumbstick and buttons
			{{33}, {365, 415}, {391, 389}, {388, 460}, {381, 467}, {365, 448},}, // G23 (Thumb button 1)
			{{34}, {393, 493}, {415, 473}, {476, 470}, {413, 516},}, // G24 (Thumb button 2)

			// Stick directions
			{{36}, {440, 390}, {454, 404}, {424, 404},}, // Stick Up -> G36
			{{37}, {410, 420}, {424, 406}, {424, 434},}, // Stick Left -> G37
			{{38}, {470, 420}, {456, 406}, {456, 433},}, // Stick Right -> G38
			{{39}, {440, 450}, {454, 436}, {424, 436},}, // Stick Down -> G39
	};

	static {
		for (final int[][] item : KEY_DEFINITIONS) {
			keys.add(new Key(item));
		}
	}

	public static List<Key> getAllKeys() {
		return keys;
	}

	public static Key getKeyAt(int x, int y) {
		for (final Key key : keys) {
			if (key.getShape().contains(x, y)) {
				return key;
			}
		}
		return null;
	}

	public static Key getKeyFor(int g13KeyCode) {
		for (final Key key : keys) {
			if (key.getG13KeyCode() == g13KeyCode) {
				return key;
			}
		}
		return null;
	}

	private final Shape shape;
	private final int g13KeyCode;
	private String mappedValue = "Unassigned";
	private String repeats = "N/A";

	private Key(int[][] buttonData) {
		if (buttonData == null || buttonData.length < 2) {
			throw new IllegalArgumentException("Button data must not be null and contain at least a key code and one point.");
		}
		this.g13KeyCode = buttonData[0][0];

		final Polygon polygon = new Polygon();
		for (int i = 1; i < buttonData.length; i++) {
			int[] point = buttonData[i];
			if (point.length == 2) {
				polygon.addPoint(point[0], point[1]);
			}
		}
		this.shape = polygon;
	}

	public Shape getShape() {
		return shape;
	}

	public int getG13KeyCode() {
		return g13KeyCode;
	}

	public String getMappedValue() {
		return mappedValue;
	}

	public void setMappedValue(String mappedValue) {
		this.mappedValue = mappedValue;
	}

	public String getRepeats() {
		return repeats;
	}

	public void setRepeats(String repeats) {
		this.repeats = repeats;
	}

	@Override
	public boolean equals(Object o) {
		if (this == o) return true;
		if (o == null || getClass() != o.getClass()) return false;
		Key key = (Key) o;
		return g13KeyCode == key.g13KeyCode;
	}

	@Override
	public int hashCode() {
		return Objects.hash(g13KeyCode);
	}

	@Override
	public String toString() {
		return "Key{" + "g13KeyCode=G" + g13KeyCode + '}';
	}
}