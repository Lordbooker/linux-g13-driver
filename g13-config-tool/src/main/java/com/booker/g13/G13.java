package lordbooker.g13;

import java.awt.BorderLayout;
import java.awt.Dimension;
import java.awt.Toolkit;
import java.util.Properties;
import javax.swing.BorderFactory;
import javax.swing.JFrame;
import javax.swing.JOptionPane;
import javax.swing.JPanel;
import javax.swing.SwingUtilities;

public class G13 extends JPanel {

	private static final long serialVersionUID = 1L;
	public static final String VERSION = G13.class.getPackage().getImplementationVersion() != null ? G13.class.getPackage().getImplementationVersion() : "Unknown";
	private static final int MAX_MACROS = 200;

	private final ImageMap g13Label = new ImageMap();
	private final KeybindPanel keybindPanel = new KeybindPanel();
	private final MacroEditorPanel macroEditorPanel = new MacroEditorPanel();

	private final Properties[] keyBindings = new Properties[4];
	private final Properties[] macros = new Properties[MAX_MACROS];

	public G13() {
		setLayout(new BorderLayout());

		try {
			for (int i = 0; i < keyBindings.length; i++) {
				keyBindings[i] = Configs.loadBindings(i);
			}

			for (int i = 0; i < macros.length; i++) {
				macros[i] = Configs.loadMacro(i);
			}

			mapBindings(0);
		} catch (Exception e) {
			e.printStackTrace();
			JOptionPane.showMessageDialog(this, "Failed to load configuration:\n" + e.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
		}

		keybindPanel.setBindings(0, keyBindings[0]);

		g13Label.addListener(new ImageMapListener() {
			@Override
			public void selected(Key key) {
				if (key == null) {
					keybindPanel.setSelectedKey(null);
					return;
				}
				// M1-M4 keys reserved for switching bindings
				if (key.getG13KeyCode() >= 25 && key.getG13KeyCode() <= 28) {
					mapBindings(key.getG13KeyCode() - 25);
				} else {
					keybindPanel.setSelectedKey(key);
				}
			}

			@Override
			public void mouseover(Key key) {
				// Mouseover logic can be implemented here if needed
			}
		});

		final JPanel p = new JPanel(new BorderLayout());
		p.setBorder(BorderFactory.createTitledBorder("G13 Keypad"));
		p.add(g13Label, BorderLayout.CENTER);
		add(p, BorderLayout.CENTER);

		final JPanel rightPanel = new JPanel(new BorderLayout());
		rightPanel.add(keybindPanel, BorderLayout.NORTH);
		rightPanel.add(macroEditorPanel, BorderLayout.CENTER);
		add(rightPanel, BorderLayout.EAST);

		keybindPanel.setMacros(macros);
		macroEditorPanel.setMacros(macros);
	}

	private void mapBindings(int bindingnum) {
		keybindPanel.setSelectedKey(null);
		keybindPanel.setBindings(bindingnum, keyBindings[bindingnum]);

		for (int i = 0; i < 40; i++) {
			String property = "G" + i;
			String val = keyBindings[bindingnum].getProperty(property);
			final Key k = Key.getKeyFor(i);

			if (k == null) {
				continue;
			}

			k.setMappedValue("Unassigned");
			k.setRepeats("N/A");

			if (val != null && !val.trim().isEmpty()) {
				try {
					final String[] parts = val.split("[,\\.]");
					final String type = parts[0];

					if ("p".equals(type)) { // Passthrough
						int keycode = Integer.parseInt(parts[2]);
						k.setMappedValue(JavaToLinuxKeymapping.cKeyCodeToString(keycode));
						k.setRepeats("N/A");
					} else if ("m".equals(type)) { // Macro
						int macroNum = Integer.parseInt(parts[1]);
						if (macroNum >= 0 && macroNum < macros.length) {
							final String macroName = macros[macroNum].getProperty("name");
							boolean repeats = Integer.parseInt(parts[2]) != 0;
							k.setMappedValue("Macro: " + macroName);
							k.setRepeats(repeats ? "Yes" : "No");
						}
					}
				} catch (NumberFormatException | ArrayIndexOutOfBoundsException e) {
					System.err.println("Could not parse binding for key G" + i + ": " + val);
					k.setMappedValue("Error!");
					k.setRepeats("Error!");
				}
			}
		}
		g13Label.repaint(); // Redraw with new mappings
	}

	public static void main(String[] args) {
		SwingUtilities.invokeLater(() -> {
			final JFrame frame = new JFrame("G13 Configuration Tool, Version " + VERSION);
			frame.setIconImage(ImageMap.G13_KEYPAD.getImage());
			frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);

			final G13 g13 = new G13();
			frame.getContentPane().add(g13, BorderLayout.CENTER);

			frame.pack();
			frame.setMinimumSize(frame.getPreferredSize());
			frame.setLocationRelativeTo(null); // Center on screen
			frame.setVisible(true);
		});
	}
}