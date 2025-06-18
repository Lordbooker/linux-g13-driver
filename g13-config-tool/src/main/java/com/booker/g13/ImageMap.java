package com.booker.g13;

import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.event.MouseEvent;
import java.awt.event.MouseListener;
import java.awt.event.MouseMotionListener;
import java.util.ArrayList;
import java.util.List;
import java.util.Set;

import javax.swing.ImageIcon;
import javax.swing.JLabel;

/**
 * A custom JLabel that displays an image of the G13 keypad and acts as an interactive map.
 * It detects mouse movements and clicks over specific key areas (defined by polygons)
 * and notifies listeners about these events.
 */
public class ImageMap extends JLabel {

	private static final long serialVersionUID = 1L;

	/** The background image of the G13 keypad. */
	public static final ImageIcon G13_KEYPAD = ImageIconHelper.loadEmbeddedImage("/com/booker/g13/images/g13.gif");
	
	/** A list of listeners to be notified of mouse events on keys. */
	private final List<ImageMapListener> listeners = new ArrayList<>();

	// --- Colors used for highlighting keys ---
	private final Color outlineColor = Color.red.darker(); // Color for the key outlines.
	private final Color selectedColor = new Color(0, 255, 0, 128); // Semi-transparent green for selected key.
	private final Color mouseoverColor = new Color(255, 0, 0, 128); // Semi-transparent red for hovered key.
    
    /** A set of keycodes for the M1-M3 and MR keys, which are used to switch binding profiles. */
    private static final Set<Integer> BINDING_SWITCH_KEYS = Set.of(25, 26, 27, 28);

	private Key selected = null; // The currently clicked/selected key.
	private Key mouseover = null; // The key currently under the mouse cursor.
	
	/**
	 * Constructs the ImageMap component and initializes its mouse listeners.
	 */
	public ImageMap() {
		super(G13_KEYPAD);
		
		addMouseMotionListener(new MouseMotionListener() {
			@Override
			public void mouseDragged(MouseEvent e) { /* Not used */ }

			@Override
			public void mouseMoved(MouseEvent e) {
				Key key = Key.getKeyAt(e.getPoint().x, e.getPoint().y);
				// Repaint only if the mouseover state changes to avoid unnecessary redraws.
				if (key == null && mouseover == null) {
					return;
				}
				
				if ((key != null && mouseover == null) || (key == null && mouseover != null)) {
					mouseover = key;
					repaint();
					fireMouseover();
					return;
				}
				
				if (key != null && mouseover != null && key.getG13KeyCode() != mouseover.getG13KeyCode()) {
					mouseover = key;
					repaint();
					fireMouseover();
				}
			}
		});
		
		addMouseListener(new MouseListener() {
			@Override
			public void mouseClicked(MouseEvent e) {
				Key key = Key.getKeyAt(e.getPoint().x, e.getPoint().y);
				
				// Repaint only if the selection state changes.
				if (key == null && selected == null) {
					return;
				}
				
				if ((key != null && selected == null) || (key == null && selected != null)) {
					selected = key;
					repaint();
					fireSelected();
					return;
				}
				
				if (key != null && selected != null && key.getG13KeyCode() != selected.getG13KeyCode()) {
					selected = key;
					repaint();
					fireSelected();
				}
			}

			// Unused mouse listener methods.
			@Override public void mousePressed(MouseEvent e) { }
			@Override public void mouseReleased(MouseEvent e) { }
			@Override public void mouseEntered(MouseEvent e) { }
			@Override public void mouseExited(MouseEvent e) { }
		});
	}
	
	/**
	 * Adds a listener to be notified of events.
	 * @param listener The listener to add.
	 */
	public void addListener(final ImageMapListener listener) {
		synchronized (listeners) {
			listeners.add(listener);
		}
	}
	
	/**
	 * Removes a listener.
	 * @param listener The listener to remove.
	 */
	public void removeListener(final ImageMapListener listener) {
		synchronized (listeners) {
			listeners.remove(listener);
		}
	}
	
	/**
	 * Notifies all registered listeners that a key has been selected.
	 */
	protected void fireSelected() {
		synchronized (listeners) {
			for (final ImageMapListener listener: listeners) {
				listener.selected(selected);
			}
		}
	}
	
	/**
	 * Notifies all registered listeners that the mouse is over a key.
	 */
	protected void fireMouseover() {
		synchronized (listeners) {
			for (final ImageMapListener listener: listeners) {
				listener.mouseover(mouseover);
			}
		}
	}
	
	@Override
	protected void paintComponent(Graphics g) {
		super.paintComponent(g);
		
		// Create a copy of the Graphics object to avoid side effects.
		final Graphics2D g2d = (Graphics2D) g.create();
        try {
            // Paint highlights and outlines in order.
            paintSelected(g2d);
            paintMouseover(g2d);
            paintKeyOutlines(g2d);
        } finally {
            g2d.dispose(); // Always dispose of the created graphics context.
        }
	}

	/**
	 * Paints the highlight for the currently selected key.
	 * @param g The graphics context to draw on.
	 */
	void paintSelected(final Graphics2D g) {
		if (selected == null) return;
		
		g.setColor(selectedColor);
		g.fill(selected.getShape());
		g.setColor(selectedColor.darker());
		g.draw(selected.getShape());
	}
	
	/**
	 * Paints the highlight and tooltip for the key currently under the mouse.
	 * @param g The graphics context to draw on.
	 */
	void paintMouseover(final Graphics2D g) {
		if (mouseover == null) return;
		
		g.setColor(mouseoverColor);
        g.fill(mouseover.getShape());
		g.setColor(mouseoverColor.darker());
		g.draw(mouseover.getShape());
		
		drawTooltip(g, mouseover);
	}

    /**
     * Draws a tooltip-like box with details about the hovered key.
     * @param g The graphics context to draw on.
     * @param key The key to display information for.
     */
    private void drawTooltip(Graphics2D g, Key key) {
        String[][] lines;
        // Display different information for binding-switch keys vs. regular keys.
        if (BINDING_SWITCH_KEYS.contains(key.getG13KeyCode())) {
            lines = new String[][]{
                {"G13 Key", "M" + (key.getG13KeyCode() - 24)},
                {"Configuration", "bindings-" + (key.getG13KeyCode() - 25) + ".properties"},
                {"This button is reserved to load bindings", ""},
            };
        } else {
            lines = new String[][]{
                {"G13 Key", "G" + key.getG13KeyCode()},
                {"Mapped Value",   key.getMappedValue()},
                {"Repeats",        key.getRepeats()},
            };
        }
		
		final int x0 = 110; // X-coordinate for the labels.
		final int x1 = 245; // X-coordinate for the values.
		int y = 550; // Starting Y-coordinate.
        g.setFont(getFont().deriveFont(Font.BOLD));

		for (final String [] line: lines) {
			g.setColor(mouseoverColor.darker());
			g.drawString(line[0], x0, y);
			g.setColor(mouseoverColor.brighter());
			g.drawString(line[1], x1, y);
			y += 2 * getFont().getSize(); // Move to the next line.
		}
    }

	/**
	 * Draws a subtle outline around all defined keys.
	 * @param g The graphics context to draw on.
	 */
	private void paintKeyOutlines(Graphics2D g) {
		g.setColor(outlineColor);
		for (final Key key: Key.getAllMasks()) {
			g.draw(key.getShape());
		}
	}
}