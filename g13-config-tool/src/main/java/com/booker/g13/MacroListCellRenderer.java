package com.booker.g13;

import java.awt.Component;
import java.util.Properties;

import javax.swing.DefaultListCellRenderer;
import javax.swing.JList;

/**
 * A custom ListCellRenderer responsible for displaying macro information
 * within a JComboBox or JList.
 * It formats the display to show the macro's ID and name.
 */
public class MacroListCellRenderer extends DefaultListCellRenderer {
	private static final long serialVersionUID = 1L;

	/**
	 * Configures the component to display the macro's information.
	 * This method is called by Swing to render each item in the list.
	 */
	@Override
	public Component getListCellRendererComponent(JList<?> list, Object value, int index, boolean isSelected, boolean cellHasFocus) {
		// Get the default component styling (e.g., for selection background).
		super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
		
		// Check if the value is a Properties object, as expected for a macro.
		if (value instanceof Properties props) {
			final String name = props.getProperty("name", "Unnamed Macro");
			final String id = props.getProperty("id", "?");
			// Set the display text to "[ID] Macro Name".
			setText("[" + id + "] " + name);
		}
		
		return this;
	}
}