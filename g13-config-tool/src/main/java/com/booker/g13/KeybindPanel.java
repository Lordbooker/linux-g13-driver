package com.booker.g13;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.FlowLayout;
import java.awt.GridLayout;
import java.io.IOException;
import java.util.Properties;
import javax.swing.*;

public class KeybindPanel extends JPanel {

	private static final long serialVersionUID = 1L;

	private final JCheckBox passthroughButton = new JCheckBox("Pass Through");
	private final JTextField passthroughText = new JTextField();
	private int passthroughCode = 0;

	private final JCheckBox macroButton = new JCheckBox("Macro");
	private final JComboBox<Properties> macroSelectionBox = new JComboBox<>();
	private final JCheckBox repeatsCheckBox = new JCheckBox("Auto Repeat");
	private final JButton colorChangeButton = new JButton("Click Here To Change");

	private int bindingsId = -1;
	private Properties bindings;
	private Properties[] macros;
	private Key key = null;
	private boolean loadingData = false;

	public KeybindPanel() {
		setLayout(new BorderLayout());
		setBorder(BorderFactory.createTitledBorder("Keybindings Panel"));
		add(createColorPanel(), BorderLayout.NORTH);

		final ButtonGroup buttonGroup = new ButtonGroup();
		buttonGroup.add(passthroughButton);
		buttonGroup.add(macroButton);

		passthroughText.setFocusTraversalKeysEnabled(false);

		JPanel grid = createBindingsGrid();
		add(grid, BorderLayout.CENTER);

		setupListeners();
		updateComponentStates();
	}

	private JPanel createBindingsGrid() {
		final JPanel grid = new JPanel(new GridLayout(0, 2, 5, 5));
		grid.setBorder(BorderFactory.createTitledBorder("Button Type"));
		grid.add(passthroughButton);
		grid.add(passthroughText);
		grid.add(new JLabel(" "));
		grid.add(new JLabel(" "));
		grid.add(macroButton);
		grid.add(macroSelectionBox);
		grid.add(new JLabel(" "));
		grid.add(repeatsCheckBox);
		macroSelectionBox.setRenderer(new MacroListCellRenderer());
		return grid;
	}

	private void setupListeners() {
		// Listener for any change that requires saving
		Runnable saveAction = () -> {
			try {
				saveBindings();
			} catch (IOException e) {
				e.printStackTrace();
				JOptionPane.showMessageDialog(KeybindPanel.this, "Error saving bindings: " + e.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
			}
		};

		passthroughButton.addActionListener(e -> {
			updateComponentStates();
			saveAction.run();
		});
		macroButton.addActionListener(e -> {
			updateComponentStates();
			saveAction.run();
		});
		macroSelectionBox.addActionListener(e -> saveAction.run());
		repeatsCheckBox.addActionListener(e -> saveAction.run());

		passthroughText.addKeyListener(new java.awt.event.KeyAdapter() {
			public void keyReleased(java.awt.event.KeyEvent evt) {
				if (loadingData) return;
				loadingData = true;
				passthroughCode = JavaToLinuxKeymapping.keyEventToCCode(evt);
				passthroughText.setText(JavaToLinuxKeymapping.cKeyCodeToString(passthroughCode));
				loadingData = false;
				saveAction.run();
			}
		});
	}

	private void updateComponentStates() {
		boolean isKeySelected = (key != null);
		boolean isPassthrough = passthroughButton.isSelected();
		boolean isMacro = macroButton.isSelected();

		colorChangeButton.setEnabled(bindings != null);
		passthroughButton.setEnabled(isKeySelected);
		macroButton.setEnabled(isKeySelected);

		passthroughText.setEnabled(isKeySelected && isPassthrough);
		macroSelectionBox.setEnabled(isKeySelected && isMacro);
		repeatsCheckBox.setEnabled(isKeySelected && isMacro);
	}

	public void setMacros(final Properties[] macros) {
		loadingData = true;
		this.macros = macros;
		macroSelectionBox.removeAllItems();
		for (final Properties properties : macros) {
			macroSelectionBox.addItem(properties);
		}
		loadingData = false;
	}

	public void setBindings(final int propertyNum, final Properties bindings) {
		loadingData = true;
		this.bindingsId = propertyNum;
		this.bindings = bindings;

		try {
			final String val = bindings.getProperty("color", "255,255,255");
			final String[] rgb = val.split(",");
			int r = Integer.parseInt(rgb[0]);
			int g = Integer.parseInt(rgb[1]);
			int b = Integer.parseInt(rgb[2]);
			final Color c = new Color(r, g, b);
			SwingUtilities.invokeLater(() -> colorChangeButton.setBackground(c));
		} catch (NumberFormatException | ArrayIndexOutOfBoundsException e) {
			System.err.println("Invalid color format in bindings: " + bindings.getProperty("color"));
		}

		setSelectedKey(null);
		loadingData = false;
	}

	public void setSelectedKey(final Key key) {
		this.key = key;
		loadingData = true;

		if (key != null) {
			final String propKey = "G" + key.getG13KeyCode();
			final String val = bindings.getProperty(propKey, "p,k.1"); // Default to "p,k.1" (ESC)
			
			try {
				final String[] parts = val.split("[,\\.]");
				String type = parts[0];

				if ("p".equals(type)) {
					passthroughButton.setSelected(true);
					passthroughCode = Integer.parseInt(parts[2]);
					passthroughText.setText(JavaToLinuxKeymapping.cKeyCodeToString(passthroughCode));
				} else { // "m" for macro
					macroButton.setSelected(true);
					int macroNum = Integer.parseInt(parts[1]);
					boolean repeats = Integer.parseInt(parts[2]) != 0;
					macroSelectionBox.setSelectedIndex(macroNum);
					repeatsCheckBox.setSelected(repeats);
				}
			} catch (NumberFormatException | ArrayIndexOutOfBoundsException e) {
				System.err.println("Could not parse binding for key " + propKey + ": " + val);
				passthroughButton.setSelected(true);
				passthroughCode = 1; // Default to ESC on error
				passthroughText.setText(JavaToLinuxKeymapping.cKeyCodeToString(passthroughCode));
			}

		}
		updateComponentStates();
		loadingData = false;
	}

	private void changeScreenColor() {
		final String val = bindings.getProperty("color");
		String[] rgb = val.split(",");
		Color initialColor = new Color(Integer.parseInt(rgb[0]), Integer.parseInt(rgb[1]), Integer.parseInt(rgb[2]));

		final Color newColor = JColorChooser.showDialog(this, "Choose Screen Color", initialColor);
		if (newColor != null) {
			bindings.setProperty("color", newColor.getRed() + "," + newColor.getGreen() + "," + newColor.getBlue());
			colorChangeButton.setBackground(newColor);
			try {
				Configs.saveBindings(bindingsId, bindings);
			} catch (Exception e) {
				e.printStackTrace();
				JOptionPane.showMessageDialog(this, e);
			}
		}
	}

	private JPanel createColorPanel() {
		final JPanel p = new JPanel(new FlowLayout(FlowLayout.LEFT));
		p.setBorder(BorderFactory.createTitledBorder("Screen Color"));
		p.add(colorChangeButton);
		colorChangeButton.addActionListener(e -> changeScreenColor());
		return p;
	}

	private void saveBindings() throws IOException {
		if (key == null || loadingData) {
			return;
		}

		String prop = "G" + key.getG13KeyCode();
		if (passthroughButton.isSelected()) {
			String val = "p,k." + passthroughCode;
			bindings.put(prop, val);
			key.setMappedValue(passthroughText.getText().trim());
			key.setRepeats("N/A");
		} else if (macroButton.isSelected()) {
			int macroNum = macroSelectionBox.getSelectedIndex();
			int repeats = repeatsCheckBox.isSelected() ? 1 : 0;
			String val = "m," + macroNum + "," + repeats;
			bindings.put(prop, val);
			
			if(macros != null && macroNum >= 0 && macroNum < macros.length) {
				final String macroName = macros[macroNum].getProperty("name");
				key.setMappedValue("Macro: " + macroName);
				key.setRepeats(repeats == 1 ? "Yes" : "No");
			}
		}
		Configs.saveBindings(bindingsId, bindings);
	}
}