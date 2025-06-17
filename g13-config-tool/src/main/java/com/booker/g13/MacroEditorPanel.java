package com.booker.g13;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.GridLayout;
import java.awt.KeyEventDispatcher;
import java.awt.KeyboardFocusManager;
import java.awt.event.KeyEvent;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.List;
import java.util.Properties;
import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;

public class MacroEditorPanel extends JPanel {

	private static final long serialVersionUID = 1L;
	private static final ImageIcon UP_ICON = ImageIconHelper.loadEmbeddedImage("/com/gupta/g13/images/up.png", 16, 16);
	private static final ImageIcon DOWN_ICON = ImageIconHelper.loadEmbeddedImage("/com/gupta/g13/images/down.png", 16, 16);
	private static final ImageIcon DELAY_ICON = ImageIconHelper.loadEmbeddedImage("/com/gupta/g13/images/pause.png", 16, 16);

	private final JComboBox<Properties> macroSelectionBox = new JComboBox<>();
	private final DefaultListModel<String> listModel = new DefaultListModel<>();
	private final JList<String> macroList = new JList<>(listModel);
	private final JTextField nameText = new JTextField();
	private final JButton addDelayButton = new JButton("Add Delay");
	private final JCheckBox captureDelays = new JCheckBox("Rec Delays", true);
	private final JButton editButton = new JButton("Edit");
	private final JButton deleteButton = new JButton("Delete");
	private final JButton recordButton = new JButton("Record Macro");

	private boolean loadingData = false;
	private boolean captureMode = false;
	private long lastCaptureTimestamp = 0;
    private final KeyEventDispatcher keyEventDispatcher;


	public MacroEditorPanel() {
		setLayout(new BorderLayout());
		setBorder(BorderFactory.createTitledBorder("Macro Editor Panel"));
		
		add(createNorthPanel(), BorderLayout.NORTH);
		add(new JScrollPane(macroList), BorderLayout.CENTER);
		add(createControlsPanel(), BorderLayout.SOUTH);

        this.keyEventDispatcher = createKeyEventDispatcher();
		setupListeners();
		updateComponentStates();
	}
    
    private KeyEventDispatcher createKeyEventDispatcher() {
        return e -> {
            if (!captureMode) return false;

            if (e.getID() == KeyEvent.KEY_PRESSED) {
                if (lastCaptureTimestamp != 0 && captureDelays.isSelected()) {
                    listModel.addElement("d." + (e.getWhen() - lastCaptureTimestamp));
                }
                lastCaptureTimestamp = e.getWhen();
                listModel.addElement("kd." + JavaToLinuxKeymapping.keyEventToCCode(e));
            } else if (e.getID() == KeyEvent.KEY_RELEASED) {
                if (lastCaptureTimestamp != 0 && captureDelays.isSelected()) {
                    listModel.addElement("d." + (e.getWhen() - lastCaptureTimestamp));
                }
                lastCaptureTimestamp = e.getWhen();
                listModel.addElement("ku." + JavaToLinuxKeymapping.keyEventToCCode(e));
            }
            // Consume the event to prevent it from being processed further
            return true;
        };
    }

	private JPanel createNorthPanel() {
		final JPanel northPanel = new JPanel(new BorderLayout(0, 5));
		macroSelectionBox.setRenderer(new MacroListCellRenderer());
		northPanel.add(macroSelectionBox, BorderLayout.NORTH);

		final JPanel namePanel = new JPanel(new BorderLayout(5, 0));
		namePanel.add(new JLabel("Name:"), BorderLayout.WEST);
		namePanel.add(nameText, BorderLayout.CENTER);
		northPanel.add(namePanel, BorderLayout.CENTER);

		return northPanel;
	}

	private JPanel createControlsPanel() {
		final JPanel controls = new JPanel(new GridLayout(0, 1, 5, 5));
		final JPanel row1 = new JPanel(new GridLayout(1, 2, 5, 0));
		row1.add(captureDelays);
		row1.add(addDelayButton);
		controls.add(row1);

		final JPanel row2 = new JPanel(new GridLayout(1, 2, 5, 0));
		row2.add(editButton);
		row2.add(deleteButton);
		controls.add(row2);

		controls.add(recordButton);
		return controls;
	}

	private void setupListeners() {
		macroSelectionBox.addActionListener(e -> selectMacro());
		recordButton.addActionListener(e -> toggleRecording());
		editButton.addActionListener(e -> editSelectedItem());
		deleteButton.addActionListener(e -> deleteSelectedItems());
		addDelayButton.addActionListener(e -> addDelay());

		nameText.getDocument().addDocumentListener(new DocumentListener() {
			public void changedUpdate(DocumentEvent e) { onNameChange(); }
			public void insertUpdate(DocumentEvent e) { onNameChange(); }
			public void removeUpdate(DocumentEvent e) { onNameChange(); }
		});
        nameText.addActionListener(e -> saveMacro());


		macroList.addListSelectionListener(e -> {
			if (!e.getValueIsAdjusting()) {
				updateComponentStates();
			}
		});
		macroList.addMouseListener(new MouseAdapter() {
			public void mouseClicked(MouseEvent e) {
				if (e.getClickCount() == 2) {
					editSelectedItem();
				}
			}
		});
		macroList.setCellRenderer(new MacroStepCellRenderer());
	}
    
    private void onNameChange() {
        if (loadingData) {
            nameText.setForeground(Color.black);
        } else {
            nameText.setForeground(Color.red);
        }
    }

	private void updateComponentStates() {
		boolean isMacroSelected = macroSelectionBox.getSelectedIndex() != -1;
		boolean isDefaultMacro = !canModifyMacro();
		boolean isListItemSelected = !macroList.isSelectionEmpty();
		
		macroSelectionBox.setEnabled(!captureMode);
		nameText.setEditable(isMacroSelected && !isDefaultMacro && !captureMode);
		macroList.setEnabled(!captureMode);
		
		captureDelays.setEnabled(!isDefaultMacro && !captureMode);
		addDelayButton.setEnabled(!isDefaultMacro && !captureMode);
		recordButton.setEnabled(!isDefaultMacro);
		
		boolean canDelete = isListItemSelected && !isDefaultMacro && !captureMode;
		deleteButton.setEnabled(canDelete);

		boolean canEdit = canDelete && macroList.getSelectedIndices().length == 1 &&
				listModel.getElementAt(macroList.getSelectedIndex()).startsWith("d.");
		editButton.setEnabled(canEdit);
	}

	private boolean canModifyMacro() {
		return macroSelectionBox.getSelectedIndex() >= Configs.defaultMacros.length;
	}

	public void toggleRecording() {
		captureMode = !captureMode;
		if (captureMode) {
			recordButton.setText("Stop Recording");
            KeyboardFocusManager.getCurrentKeyboardFocusManager().addKeyEventDispatcher(keyEventDispatcher);
			listModel.clear();
			lastCaptureTimestamp = 0;
		} else {
			recordButton.setText("Record Macro");
            KeyboardFocusManager.getCurrentKeyboardFocusManager().removeKeyEventDispatcher(keyEventDispatcher);
			saveMacro();
		}
		updateComponentStates();
	}

	private void deleteSelectedItems() {
		int[] indices = macroList.getSelectedIndices();
		for (int i = indices.length - 1; i >= 0; i--) {
			listModel.remove(indices[i]);
		}
		saveMacro();
	}

	private void editSelectedItem() {
		if (macroList.getSelectedIndices().length != 1 || !canModifyMacro()) return;

		int index = macroList.getSelectedIndex();
		String item = listModel.getElementAt(index);
		if (!item.startsWith("d.")) return;

		try {
			int currentDelay = Integer.parseInt(item.substring(2));
			String newDelayStr = JOptionPane.showInputDialog(this, "Enter the delay in milliseconds:", currentDelay);
			if (newDelayStr != null) {
				int newDelay = Integer.parseInt(newDelayStr);
				listModel.set(index, "d." + newDelay);
				saveMacro();
			}
		} catch (NumberFormatException e) {
			JOptionPane.showMessageDialog(this, "Invalid delay value entered.", "Error", JOptionPane.ERROR_MESSAGE);
		}
	}

	private void addDelay() {
		int pos = macroList.getSelectedIndex() + 1;
		if (pos == 0) { // If nothing is selected, add to the end.
			pos = listModel.getSize();
		}
		
		try {
			String delayStr = JOptionPane.showInputDialog(this, "Enter the delay in milliseconds:", 100);
			if (delayStr != null) {
				int delay = Integer.parseInt(delayStr);
				listModel.add(pos, "d." + delay);
				saveMacro();
			}
		} catch (NumberFormatException e) {
			JOptionPane.showMessageDialog(this, "Invalid delay value entered.", "Error", JOptionPane.ERROR_MESSAGE);
		}
	}
	
	public void setMacros(final Properties[] macros) {
		for (final Properties properties : macros) {
			macroSelectionBox.addItem(properties);
		}
		if (macros.length > 0) {
			macroSelectionBox.setSelectedIndex(0);
		}
	}

	private void selectMacro() {
		if (loadingData || macroSelectionBox.getSelectedItem() == null) return;
		loadingData = true;

		listModel.clear();
		Properties macro = (Properties) macroSelectionBox.getSelectedItem();
		nameText.setText(macro.getProperty("name"));
		nameText.setForeground(Color.black);

		String sequence = macro.getProperty("sequence", "");
		if (!sequence.isEmpty()) {
			for (String token : sequence.split(",")) {
				listModel.addElement(token);
			}
		}

		loadingData = false;
		updateComponentStates();
	}

	private void saveMacro() {
		if (loadingData || macroSelectionBox.getSelectedItem() == null) return;

		int id = macroSelectionBox.getSelectedIndex();
		Properties macro = (Properties) macroSelectionBox.getSelectedItem();
		macro.setProperty("name", nameText.getText());
        nameText.setForeground(Color.black);

		StringBuilder buf = new StringBuilder();
		for (int i = 0; i < listModel.getSize(); i++) {
			if (i > 0) {
				buf.append(",");
			}
			buf.append(listModel.getElementAt(i));
		}
		macro.setProperty("sequence", buf.toString());
		
		try {
			Configs.saveMacro(id, macro);
            macroSelectionBox.repaint(); // Update name in combobox
		} catch (Exception e) {
			e.printStackTrace();
			JOptionPane.showMessageDialog(this, "Failed to save macro: " + e.getMessage(), "Error", JOptionPane.ERROR_MESSAGE);
		}
	}
    
    // Custom renderer for the macro steps list
    private static class MacroStepCellRenderer extends DefaultListCellRenderer {
        @Override
        public Component getListCellRendererComponent(JList<?> list, Object value, int index, boolean isSelected, boolean cellHasFocus) {
            super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
            if (value instanceof String) {
                String val = (String) value;
                String[] parts = val.split("\\.");
                String type = parts[0];
                int code = Integer.parseInt(parts[1]);

                if ("d".equals(type)) {
                        setText(String.format("Delay: %.3f seconds", code / 1000.0));
                        setIcon(DELAY_ICON);
                } else if ("kd".equals(type)) {
                        setText("Key Down: " + JavaToLinuxKeymapping.cKeyCodeToString(code));
                        setIcon(DOWN_ICON);
                } else if ("ku".equals(type)) {
                        setText("Key Up: " + JavaToLinuxKeymapping.cKeyCodeToString(code));
                        setIcon(UP_ICON);
                } else {
 setIcon(null);
                }



            }
            return this;
        }
    }
}