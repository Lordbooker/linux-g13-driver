package com.booker.g13;

import java.awt.Color;
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

public class ImageMap extends JLabel {

	private static final long serialVersionUID = 1L;

	public static final ImageIcon G13_KEYPAD = ImageIconHelper.loadEmbeddedImage("/com/booker/g13/images/g13.gif");
	
	private final List<ImageMapListener> listeners = new ArrayList<>();

	private final Color outlineColor = Color.red.darker();
	private final Color selectedColor = new Color(0, 255, 0, 128);
	private final Color mouseoverColor = new Color(255, 0, 0, 128);
    
    // Verwendung eines Sets für die M-Tasten zur besseren Lesbarkeit
    private static final Set<Integer> BINDING_SWITCH_KEYS = Set.of(25, 26, 27, 28);

	private Key selected = null;
	private Key mouseover = null;
	
	public ImageMap() {
		super(G13_KEYPAD);
		
        // KORREKTUR: Zurück zur ursprünglichen, expliziten Listener-Struktur, um 1:1-Verhalten sicherzustellen.
		addMouseMotionListener(new MouseMotionListener() {
			@Override
			public void mouseDragged(MouseEvent e) { }

			@Override
			public void mouseMoved(MouseEvent e) {
				Key key = Key.getKeyAt(e.getPoint().x, e.getPoint().y);
				if (key == null && mouseover == null) {
					return;
				}
				
				if ((key != null && mouseover == null) || (key == null && mouseover != null)) {
					mouseover = key;
					repaint();
					fireMouseover();
					return;
				}
				
                // Diese Bedingung ist wichtig, wenn man von einer Taste direkt auf die nächste wechselt.
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

			@Override public void mousePressed(MouseEvent e) { }
			@Override public void mouseReleased(MouseEvent e) { }
			@Override public void mouseEntered(MouseEvent e) { }
			@Override public void mouseExited(MouseEvent e) { }
		});
	}
	
	public void addListener(final ImageMapListener listener) {
		synchronized (listeners) {
			listeners.add(listener);
		}
	}
	
	public void removeListener(final ImageMapListener listener) {
		synchronized (listeners) {
			listeners.remove(listener);
		}
	}
	
	protected void fireSelected() {
		synchronized (listeners) {
			for (final ImageMapListener listener: listeners) {
				listener.selected(selected);
			}
		}
	}
	
	protected void fireMouseover() {
		synchronized (listeners) {
			for (final ImageMapListener listener: listeners) {
				listener.mouseover(mouseover);
			}
		}
	}
	
	// KORREKTUR: Die korrekte Methode zum Überschreiben ist paintComponent.
    // Das Überschreiben von paint() wird vermieden, um Rendering-Probleme zu verhindern.
	@Override
	protected void paintComponent(Graphics g) {
		super.paintComponent(g);
		
        // Es ist gute Praxis, eine Kopie des Graphics-Objekts zu erstellen.
		final Graphics2D g2d = (Graphics2D) g.create();
        try {
            paintSelected(g2d);
            paintMouseover(g2d);
            paintKeyOutlines(g2d);
        } finally {
            g2d.dispose(); // Wichtig: Immer das erstellte Graphics-Objekt freigeben.
        }
	}

	void paintSelected(final Graphics2D g) {
		if (selected == null) return;
		
		g.setColor(selectedColor);
		g.fill(selected.getShape());
		g.setColor(selectedColor.darker());
		g.draw(selected.getShape());
	}
	
	void paintMouseover(final Graphics2D g) {
		if (mouseover == null) return;
		
		g.setColor(mouseoverColor);
		g.fill(mouseover.getShape());
		g.setColor(mouseoverColor.darker());
		g.draw(mouseover.getShape());
		
		// Tooltip-Zeichnung bleibt erhalten
		drawTooltip(g, mouseover);
	}

    private void drawTooltip(Graphics2D g, Key key) {
        String[][] lines;
        // Verwendung des Sets für bessere Lesbarkeit
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
		
		final int x0 = 110;
		final int x1 = 245;
		int y = 550;
		for (final String [] line: lines) {
			g.setColor(mouseoverColor.darker());
			g.drawString(line[0], x0, y);
			g.setColor(mouseoverColor.brighter());
			g.drawString(line[1], x1, y);
			y+= 2*getFont().getSize();
		}
    }

	private void paintKeyOutlines(Graphics2D g) {
		g.setColor(outlineColor);
		for (final Key key: Key.getAllMasks()) {
			g.draw(key.getShape());
		}
	}
}