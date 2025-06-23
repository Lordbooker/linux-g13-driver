package com.booker.g13;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.GridLayout;
import java.awt.event.KeyEvent;
import java.awt.event.KeyListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.Properties;
import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;

/**
 * A JPanel for creating, editing, and managing macros.
 * It provides a UI to select a macro, view/edit its sequence of key presses,
 * and record new sequences.
 */
public class MacroEditorPanel extends JPanel {

	private static final long serialVersionUID = 1L;

	// --- Icons for the macro step list ---
	private static final ImageIcon UP_ICON = ImageIconHelper.loadEmbeddedImage("/com/booker/g13/images/up.png", 16, 16);
	private static final ImageIcon DOWN_ICON = ImageIconHelper.loadEmbeddedImage("/com/booker/g13/images/down.png", 16, 16);
	private static final ImageIcon DELAY_ICON = ImageIconHelper.loadEmbeddedImage("/com/booker/g13/images/pause.png", 16, 16);

	// --- UI Components ---
	private final JComboBox<Properties> macroSelectionBox = new JComboBox<>();
	private final DefaultListModel<String> listModel = new DefaultListModel<>();
	private final JList<String> macroList = new JList<>(listModel);
	private final JTextField nameText = new JTextField();
	private final JButton addDelayButton = new JButton("Add Delay");
	private final JCheckBox captureDelays = new JCheckBox("Rec Delays", true);
	private final JButton editButton = new JButton("Edit Delay");
	private final JButton deleteButton = new JButton("Delete Step");
	private final JButton recordButton = new JButton("Clear & Record");
	
	// --- State Variables ---
	private volatile boolean loadingData = false; // Flag to prevent listeners firing during data load.
	private volatile boolean captureMode = false; // Flag to indicate if we are currently recording a macro.
	private long lastCapture = 0; // Timestamp of the last key event for calculating delays.

	/**
	 * Constructs the MacroEditorPanel, setting up its UI and listeners.
	 */
	public MacroEditorPanel(){
		setLayout(new BorderLayout());
		setBorder(BorderFactory.createTitledBorder("Macro Editor Panel"));
		
		setupUI();
        attachListeners();

        // Initially disable components until a macro is selected.
        setComponentStates(false);
	}
    
    /**
     * Creates and arranges all UI components within the panel.
     */
    private void setupUI() {
        final JPanel northPanel = new JPanel(new BorderLayout());
		northPanel.add(macroSelectionBox, BorderLayout.NORTH);

		final JPanel namePanel = new JPanel(new BorderLayout());
		namePanel.add(new JLabel("Name : "), BorderLayout.WEST);
		namePanel.add(nameText, BorderLayout.CENTER);
		northPanel.add(namePanel, BorderLayout.SOUTH);

		add(northPanel, BorderLayout.NORTH);
		add(new JScrollPane(macroList), BorderLayout.CENTER);

		final JPanel controls = new JPanel(new GridLayout(0, 1));
		final JPanel tmp1 = new JPanel(new GridLayout(1, 2));
		tmp1.add(captureDelays);
		tmp1.add(addDelayButton);
		controls.add(tmp1);

		final JPanel tmp2 = new JPanel(new GridLayout(1, 2));
		tmp2.add(editButton);
		tmp2.add(deleteButton);
		controls.add(tmp2);

		// Disable focus traversal to capture all key events on the button itself.
		recordButton.setFocusTraversalKeysEnabled(false);
		controls.add(recordButton);

		add(controls, BorderLayout.SOUTH);
    }

    /**
     * Attaches all necessary event listeners to the UI components.
     */
    private void attachListeners() {
        editButton.addActionListener(e -> edit());
        deleteButton.addActionListener(e -> delete());
        addDelayButton.addActionListener(e -> addDelay());
        recordButton.addActionListener(e -> startStopRecording());

        macroSelectionBox.addActionListener(e -> selectMacro());
        macroList.setCellRenderer(new MacroStepCellRenderer());

        // Add a listener for double-clicking list items to edit them.
        macroList.addMouseListener(new MouseAdapter() {
            @Override
            public void mouseClicked(MouseEvent e) {
                if (e.getClickCount() > 1) {
                    edit();
                }
            }
        });
        
        // Update button states based on list selection.
        macroList.getSelectionModel().addListSelectionListener(e -> {
            if (e.getValueIsAdjusting()) return;
            updateButtonStates();
        });
        
        // Listener to indicate that the macro name has been changed but not saved.
        nameText.getDocument().addDocumentListener(new DocumentListener() {
			@Override public void changedUpdate(DocumentEvent e) { updateNameColor(); }
			@Override public void insertUpdate(DocumentEvent e) { updateNameColor(); }
			@Override public void removeUpdate(DocumentEvent e) { updateNameColor(); }
			private void updateNameColor() {
				// Change text color to red to signify unsaved changes.
				nameText.setForeground(loadingData ? Color.black : Color.red);
			}
		});

        // Save the macro name when the user presses Enter.
        nameText.addActionListener(e -> {
			nameText.setForeground(Color.black); // Revert color to black.
			saveMacro();
			macroSelectionBox.repaint(); // Repaint to show the new name in the combo box.
		});

        // The master KeyListener for capturing macro events.
        KeyListener keyListener = new KeyListener() {
			@Override
			public void keyPressed(KeyEvent event) {
				if (!captureMode) return;
				// If recording delays, add a delay step before the key down event.
				if (lastCapture != 0 && captureDelays.isSelected()) {
					listModel.addElement("d." + (event.getWhen() - lastCapture));
				}
				lastCapture = event.getWhen();
				listModel.addElement("kd." + JavaToLinuxKeymapping.keyEventToCCode(event));
			}

			@Override
			public void keyReleased(KeyEvent event) {
				if (!captureMode) return;
				// If recording delays, add a delay step before the key up event.
				if (lastCapture != 0 && captureDelays.isSelected()) {
					listModel.addElement("d." + (event.getWhen() - lastCapture));
				}
				lastCapture = event.getWhen();
				listModel.addElement("ku." + JavaToLinuxKeymapping.keyEventToCCode(event));
			}
			@Override public void keyTyped(KeyEvent e) { /* Not used */ }
        };

        // Add the listener to all components to ensure it captures events globally within the panel.
		final JComponent[] componentsToListen = { this, macroSelectionBox, macroList, nameText, addDelayButton, captureDelays, recordButton, editButton, deleteButton };
		for (final JComponent c : componentsToListen) {
			c.addKeyListener(keyListener);
		}
    }

    /**
     * Updates the enabled state of the 'Edit' and 'Delete' buttons
     * based on the current selection in the macro step list.
     */
    private void updateButtonStates() {
        boolean canModify = canModifyMacro();
        int[] selectedIndices = macroList.getSelectedIndices();
        boolean selectionExists = selectedIndices.length > 0;

        deleteButton.setEnabled(selectionExists && canModify);
        
        boolean singleSelection = selectedIndices.length == 1;
        // The 'edit' button only works for delay steps.
        boolean isDelay = singleSelection && listModel.getElementAt(selectedIndices[0]).startsWith("d.");
        editButton.setEnabled(singleSelection && isDelay && canModify);
    }
    
    /**
     * A helper to enable or disable a set of components all at once.
     * @param enabled The desired enabled state.
     */
    private void setComponentStates(boolean enabled) {
        final JComponent[] components = { macroSelectionBox, macroList, nameText, addDelayButton, captureDelays, editButton, deleteButton };
        for (JComponent c : components) {
            c.setEnabled(enabled);
        }
        recordButton.setEnabled(canModifyMacro());
    }

	/**
	 * Checks if the currently selected macro can be modified.
	 * Default macros (first N macros) are read-only.
	 * @return true if the macro is user-definable and can be modified, false otherwise.
	 */
	private boolean canModifyMacro() {
		return macroSelectionBox.getSelectedIndex() >= Configs.DEFAULT_MACROS_COUNT;
	}

	/**
	 * Toggles the macro recording mode on and off.
	 */
	public void startStopRecording() {
		captureMode = !captureMode;
        // Disable most components during recording.
        setComponentStates(!captureMode && canModifyMacro());
		
		if (captureMode) {
			recordButton.setText("Stop Recording");
			listModel.removeAllElements(); // Clear the previous sequence.
			lastCapture = 0;
            nameText.setEnabled(false); // Prevent name changes during recording.
		} else {
			recordButton.setText("Clear & Record");
            nameText.setEnabled(canModifyMacro());
			saveMacro();
		}
	}

	/**
	 * Deletes the selected steps from the macro sequence list.
	 */
	public void delete() {
        if (!canModifyMacro()) return;
        int[] indices = macroList.getSelectedIndices();
        if (indices == null || indices.length == 0) return;

        // Iterate backwards when removing items to avoid index shifting issues.
        for (int i = indices.length - 1; i >= 0; i--) {
            listModel.removeElementAt(indices[i]);
        }
        saveMacro();
    }
	
	/**
	 * Edits the value of a selected delay step in the macro sequence.
	 */
	public void edit() {
		if (!canModifyMacro()) return;
		int selectedIndex = macroList.getSelectedIndex();
		if (selectedIndex == -1) return;

		final String str = listModel.getElementAt(selectedIndex);
		if (!str.startsWith("d.")) return; // Only 'delay' steps are editable.

		try {
            int currentDelay = Integer.parseInt(str.substring(2));
            String newDelayStr = JOptionPane.showInputDialog(this, "Enter the delay in milliseconds", currentDelay);
            if (newDelayStr == null) return; // User cancelled.

            int newDelay = Integer.parseInt(newDelayStr);
            listModel.set(selectedIndex, "d." + newDelay);
            saveMacro();
        } catch (NumberFormatException e) {
            JOptionPane.showMessageDialog(this, "Invalid delay value: " + e.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
        }
	}
	
	/**
	 * Adds a delay step to the macro sequence.
	 */
	public void addDelay() {
        if (!canModifyMacro()) return;
        int pos = macroList.getSelectedIndex();
        if (pos == -1) {
            pos = listModel.getSize(); // Add to the end if nothing is selected.
        } else {
            pos++; // Insert after the selected item.
        }

        try {
            String newDelayStr = JOptionPane.showInputDialog(this, "Enter the delay in milliseconds", 100);
            if (newDelayStr == null) return; // User cancelled.
            
            int newDelay = Integer.parseInt(newDelayStr);
            listModel.insertElementAt("d." + newDelay, pos);
            saveMacro();
        } catch (NumberFormatException e) {
            JOptionPane.showMessageDialog(this, "Invalid delay value: " + e.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
        }
	}
	
	/**
	 * Populates the macro selection combo box.
	 * @param macros An array of Properties, each representing a macro.
	 */
	public void setMacros(final Properties[] macros) {
        loadingData = true;
		macroSelectionBox.removeAllItems();
		for (final Properties properties : macros) {
			macroSelectionBox.addItem(properties);
		}
        if (macroSelectionBox.getItemCount() > 0) {
		    macroSelectionBox.setSelectedIndex(0);
        }
        loadingData = false;
        selectMacro(); // Load the first macro by default.
	}
	
	/**
	 * Loads the data of the currently selected macro from the combo box into the UI controls.
	 */
	private void selectMacro() {
		if (loadingData || macroSelectionBox.getSelectedItem() == null) return;
        loadingData = true;

        // Enable/disable components based on whether the macro is editable.
        boolean canModify = canModifyMacro();
        captureDelays.setEnabled(canModify);
        addDelayButton.setEnabled(canModify);
        nameText.setEditable(canModify);
        recordButton.setEnabled(canModify);
        updateButtonStates();

		listModel.clear();
		
		final Properties macro = (Properties)macroSelectionBox.getSelectedItem();
		nameText.setText(macro.getProperty("name", ""));
		
		final String sequence = macro.getProperty("sequence", "");
		if (!sequence.isEmpty()) {
            // Split the sequence string into individual steps and add them to the list.
			for (String token : sequence.split(",")) {
                listModel.addElement(token);
            }
        }
		
		loadingData = false;
        nameText.setForeground(Color.black); // Reset name color to black.
	}
	
	/**
	 * Saves the current state of the macro (name and sequence) to its properties file.
	 */
	private void saveMacro() {
        if (loadingData || !canModifyMacro()) return;
		final int id = macroSelectionBox.getSelectedIndex();
		if (id == -1) return;

		final Properties macro = (Properties)macroSelectionBox.getSelectedItem();
		macro.setProperty("name", nameText.getText());
		
        // Reconstruct the sequence string from the list model.
		final StringBuilder buf = new StringBuilder();
		for (int i = 0; i < listModel.getSize(); i++) {
			if (i > 0) {
				buf.append(",");
			}
			buf.append(listModel.getElementAt(i));
		}
		macro.setProperty("sequence", buf.toString());
		
		// Persist the changes.
		try {
			Configs.saveMacro(id, macro);
		} catch (Exception e) {
			e.printStackTrace();
			JOptionPane.showMessageDialog(this, "Could not save macro: " + e.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
		}
	}
    
    /**
     * A custom cell renderer for the JList that displays macro steps.
     * It shows user-friendly text and icons for key down, key up, and delay actions.
     */
    private static class MacroStepCellRenderer extends DefaultListCellRenderer {
        @Override
        public Component getListCellRendererComponent(JList<?> list, Object value, int index, boolean isSelected, boolean cellHasFocus) {
            super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);

            if (value instanceof String val) {
                String[] parts = val.split("\\.");
                if (parts.length == 2) {
                    String type = parts[0];
                    try {
                        int code = Integer.parseInt(parts[1]);
                        switch (type) {
                            case "d": // Delay
                                setText(String.format("%.3f seconds", code / 1000.0));
                                setIcon(DELAY_ICON);
                                break;
                            case "kd": // Key Down
                                setText(JavaToLinuxKeymapping.cKeyCodeToString(code));
                                setIcon(DOWN_ICON);
                                break;
                            case "ku": // Key Up
                                setText(JavaToLinuxKeymapping.cKeyCodeToString(code));
                                setIcon(UP_ICON);
                                break;
                            default: // Unknown type
                                setIcon(null);
                        }
                    } catch (NumberFormatException e) {
                        setText(val); // Fallback to raw text if parsing fails.
                        setIcon(null);
                    }
                } else {
                    setText(val);
                    setIcon(null);
                }
            }
            return this;
        }
    }
}