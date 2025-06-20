package com.booker.g13;

import java.awt.BorderLayout;
import java.io.IOException;
import java.util.Properties;
import java.util.Set;

import javax.swing.BorderFactory;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;
import javax.swing.UIManager;

/**
 * The main class for the G13 Configuration application.
 * It builds the main UI, orchestrates the different panels, and handles loading
 * and mapping of key bindings.
 */
public class G13 extends JPanel {

	private static final long serialVersionUID = 1L;
	
	/**
	 * The application version, retrieved from the JAR's manifest file. Defaults to "Development".
	 */
	public static final String VERSION = G13.class.getPackage().getImplementationVersion() != null 
			? G13.class.getPackage().getImplementationVersion() 
			: "Development";
	
	/**
	 * The maximum number of macros that can be configured.
	 */
	private static final int MAX_MACROS = 200;

	// Named constants for the G13 keycodes of the M1, M2, M3, and MR buttons.
    private static final int BINDING_KEY_M1 = 25;
    private static final int BINDING_KEY_M2 = 26;
    private static final int BINDING_KEY_M3 = 27;
    private static final int BINDING_KEY_MR = 28;
    
    /**
     * A set containing the keycodes for the binding switch keys (M1, M2, M3, MR).
     * Used for quick lookups to check if a pressed key should switch the current binding profile.
     */
    private static final Set<Integer> BINDING_SWITCH_KEYS = Set.of(
            BINDING_KEY_M1, BINDING_KEY_M2, BINDING_KEY_M3, BINDING_KEY_MR
    );
	
	// UI Components
	private final ImageMap g13Label = new ImageMap(); // The interactive G13 keypad image.
	private final KeybindPanel keybindPanel = new KeybindPanel(); // Panel for editing key bindings.
	private final MacroEditorPanel macroEditorPanel = new MacroEditorPanel(); // Panel for editing macros.
	
	// Data storage
	private final Properties[] keyBindings = new Properties[4]; // Holds the 4 binding profiles (M1, M2, M3, MR).
	private final Properties[] macros = new Properties[MAX_MACROS]; // Holds all configured macros.
	
	/**
	 * Constructor for the main G13 panel.
	 * Initializes layout, loads configuration, and sets up UI components and listeners.
	 */
	public G13() {
		setLayout(new BorderLayout());
		
		// Load all configurations and initialize the UI.
		loadConfiguration();
		
		// Set the initial bindings to the first profile (M1).
		keybindPanel.setBindings(0, keyBindings[0]);
		
		g13Label.addListener(new ImageMapListener() {
			@Override
			public void selected(Key key) {
				if (key == null) {
					keybindPanel.setSelectedKey(null);
					return;
				}
				
				// Check if the selected key is one of the binding switch keys (M1-M3, MR).
				if (BINDING_SWITCH_KEYS.contains(key.getG13KeyCode())) {
					// Switch the active binding profile.
					mapBindings(key.getG13KeyCode() - BINDING_KEY_M1);
				} else {
					// A regular key was selected, pass it to the keybind panel for editing.
					keybindPanel.setSelectedKey(key);

					// --- NEUE SYNCHRONISATIONS-LOGIK ---
					// Synchronize the MacroEditorPanel with the selected key's macro.
					int currentBindingNum = keybindPanel.getBindingsId();
					if (currentBindingNum != -1) {
						String property = "G" + key.getG13KeyCode();
						String val = keyBindings[currentBindingNum].getProperty(property);

						if (val != null && val.startsWith("m,")) {
							try {
								String[] parts = val.split("[,.]");
								if (parts.length >= 2) {
									int macroId = Integer.parseInt(parts[1]);
									macroEditorPanel.selectMacroById(macroId);
								}
							} catch (NumberFormatException ex) {
								System.err.println("Could not parse macro ID from binding: " + val);
							}
						}
					}
					// --- ENDE DER SYNCHRONISATIONS-LOGIK ---
				}
			}

			@Override
			public void mouseover(Key key) {
				// Currently unused, but preserved for future functionality.
			}			
		});
		
		// --- UI Assembly ---
		final JPanel p = new JPanel(new BorderLayout());
		p.setBorder(BorderFactory.createTitledBorder("G13 Keypad"));
		p.add(g13Label, BorderLayout.CENTER);
		add(p, BorderLayout.CENTER);
		
		final JPanel rightPanel = new JPanel(new BorderLayout());
		rightPanel.add(keybindPanel, BorderLayout.NORTH);
		rightPanel.add(macroEditorPanel, BorderLayout.CENTER);
		add(rightPanel, BorderLayout.EAST);
		
		// Provide the macro data to the panels that need it.
		keybindPanel.setMacros(macros);
		macroEditorPanel.setMacros(macros);
	}

	/**
	 * Loads all key binding profiles and macros from configuration files.
	 * In case of an error, it displays a dialog to the user.
	 */
	private void loadConfiguration() {
		try {
			// Load the 4 binding profiles.
			for (int i = 0; i < keyBindings.length; i++) {
				keyBindings[i] = Configs.loadBindings(i);
			}
			
			// Load all possible macros.
			for (int i = 0; i < macros.length; i++) {
				macros[i] = Configs.loadMacro(i);
			}
			
			// Apply the first binding profile (M1) by default.
			mapBindings(0);
		}
		catch (IOException e) {
			e.printStackTrace();
			JOptionPane.showMessageDialog(this, "Failed to load configuration:\n" + e.getMessage(), "Configuration Error", JOptionPane.ERROR_MESSAGE);
            // The application could exit here as it's not usable without configuration.
            // System.exit(1);
		}
	}
		
	/**
	 * Applies a specific binding profile to the keypad UI.
	 * This method updates the visual representation of each key on the ImageMap
	 * to show what it is currently mapped to.
	 * @param bindingNum The index of the binding profile to apply (0-3).
	 */
	private void mapBindings(int bindingNum) {
		keybindPanel.setSelectedKey(null); // Deselect any key.
		keybindPanel.setBindings(bindingNum, keyBindings[bindingNum]);
		
		// Iterate through all possible G-keys to update their display text.
		for (int i = 0; i < 40; i++) { 
			final Key k = Key.getKeyFor(i);
			if (k == null) continue;

			String property = "G" + i;
			String val = keyBindings[bindingNum].getProperty(property);
			
			// Set default display values.
			k.setMappedValue("Unassigned");
			k.setRepeats("N/A");
			
			if (val != null && !val.isBlank()) {
				// The value string is parsed to determine the binding type and value.
				// Format: "p,k.keycode" for passthrough, "m,macroNum,repeats" for macro.
				String[] parts = val.split("[,.]");
				if (parts.length < 2) continue; // Ignore invalid format.

				final String type = parts[0];
				
				try {
					if ("p".equals(type)) { // Passthrough key
						if (parts.length >= 3) {
							int keycode = Integer.parseInt(parts[2]);
							k.setMappedValue(JavaToLinuxKeymapping.cKeyCodeToString(keycode));
						}
					} else if ("m".equals(type)) { // Macro
						if (parts.length >= 3) {
							int macroNum = Integer.parseInt(parts[1]);
							if (macroNum >= 0 && macroNum < macros.length) {
								final String macroName = macros[macroNum].getProperty("name", "Unnamed Macro");
								boolean repeats = Integer.parseInt(parts[2]) != 0;
								k.setMappedValue("Macro: " + macroName);
								k.setRepeats(repeats ? "Yes" : "No");
							}
						}
					}
				} catch (NumberFormatException e) {
					// Handle cases where the number in the property is malformed.
					System.err.println("Could not parse binding: " + val);
					k.setMappedValue("Parse Error");
				}
			}
		}
	}
	
	/**
	 * The main entry point for the application.
	 * @param args Command line arguments (not used).
	 */
	public static void main(String[] args) {
		// Ensure all UI operations are performed on the Event Dispatch Thread (EDT).
        SwingUtilities.invokeLater(() -> {
            try {
                // Set a modern look and feel for the UI.
                UIManager.setLookAndFeel(UIManager.getSystemLookAndFeelClassName());
            } catch (Exception e) {
                e.printStackTrace();
            }

            final JFrame frame = new JFrame("G13 Configuration Tool, Version " + VERSION);
            frame.setIconImage(ImageMap.G13_KEYPAD.getImage());
            frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

            final G13 g13 = new G13();	
            frame.getContentPane().add(g13, BorderLayout.CENTER);
            
            frame.pack(); // Size the frame to fit its contents.
            frame.setLocationRelativeTo(null); // Center the frame on the screen.
            frame.setVisible(true);
        });
	}
}