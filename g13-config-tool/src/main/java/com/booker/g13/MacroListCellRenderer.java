package lordbooker.g13;

import java.awt.Component;
import java.util.Properties;
import javax.swing.DefaultListCellRenderer;
import javax.swing.JList;

public final class MacroListCellRenderer extends DefaultListCellRenderer {
	private static final long serialVersionUID = 1L;

	@Override
	public Component getListCellRendererComponent(JList<?> list, Object value, int index, boolean isSelected, boolean cellHasFocus) {
		String displayText = "Unknown";
		
		if (value instanceof Properties) {
			String name = props.getProperty("name", "Unnamed Macro");
			String id = props.getProperty("id", "?");
            if (name.trim().isEmpty()) {
                name = "Unnamed Macro";
            }
			displayText = String.format("[%s] %s", id, name);
		}
		
		return super.getListCellRendererComponent(list, displayText, index, isSelected, cellHasFocus);
	}
}