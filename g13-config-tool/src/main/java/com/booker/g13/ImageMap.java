package com.booker.g13;

import java.awt.Color;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.RenderingHints;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.List;
import java.util.Objects;
import javax.swing.ImageIcon;
import javax.swing.JLabel;

public class ImageMap extends JLabel {

	private static final long serialVersionUID = 1L;
	public static final ImageIcon G13_KEYPAD = ImageIconHelper.loadEmbeddedImage("/com/booker/g13/images/g13.gif");

	private final List<ImageMapListener> listeners = new ArrayList<>();
	private final Color outlineColor = Color.RED.darker();
	private final Color selectedColor = new Color(0, 255, 0, 128);
	private final Color mouseoverColor = new Color(255, 0, 0, 128);

	private Key selected = null;
	private Key mouseover = null;

	public ImageMap() {
		super(G13_KEYPAD);

		MouseAdapter mouseAdapter = new MouseAdapter() {
			@Override
			public void mouseMoved(MouseEvent e) {
				Key key = Key.getKeyAt(e.getX(), e.getY());
				if (!Objects.equals(mouseover, key)) {
					mouseover = key;
					fireMouseover();
					repaint();
				}
			}

			@Override
			public void mouseClicked(MouseEvent e) {
				Key key = Key.getKeyAt(e.getX(), e.getY());
				if (!Objects.equals(selected, key)) {
					selected = key;
					// For M1-M4 keys, selection highlight is not needed as they trigger an action
                    if (key != null && key.getG13KeyCode() >= 25 && key.getG13KeyCode() <= 28) {
                        selected = null;
                    }
					fireSelected();
					repaint();
				}
			}

            @Override
            public void mouseExited(MouseEvent e) {
                if (mouseover != null) {
                    mouseover = null;
                    fireMouseover();
                    repaint();
                }
            }
		};

		addMouseMotionListener(mouseAdapter);
		addMouseListener(mouseAdapter);
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
			for (final ImageMapListener listener : listeners) {
				listener.selected(selected);
			}
		}
	}

	protected void fireMouseover() {
		synchronized (listeners) {
			for (final ImageMapListener listener : listeners) {
				listener.mouseover(mouseover);
			}
		}
	}

	@Override
	protected void paintComponent(Graphics g) {
		super.paintComponent(g);
		Graphics2D g2d = (Graphics2D) g.create();
		g2d.setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);

		paintKeyOutlines(g2d);
		paintKeyHighlight(g2d, selected, selectedColor);
		paintKeyHighlight(g2d, mouseover, mouseoverColor);
		paintMouseoverInfo(g2d);

		g2d.dispose();
	}

	private void paintKeyHighlight(Graphics2D g, Key key, Color color) {
		if (key == null) {
			return;
		}
		g.setColor(color);
		g.fill(key.getShape());
		g.setColor(color.darker());
		g.draw(key.getShape());
	}

	private void paintMouseoverInfo(Graphics2D g) {
		if (mouseover == null) {
			return;
		}

		String[][] lines;
		int gCode = mouseover.getG13KeyCode();

		if (gCode >= 25 && gCode <= 28) { // M1-M4 keys
			lines = new String[][]{
					{"Profile Key", "M" + (gCode - 24)},
					{"Action", "Load Profile " + (gCode - 24)},
					{"Configuration", "bindings-" + (gCode - 25) + ".properties"},
			};
		} else {
			lines = new String[][]{
					{"Key", "G" + gCode},
					{"Mapped To", mouseover.getMappedValue()},
					{"Repeats", mouseover.getRepeats()},
			};
		}

		drawInfoBox(g, lines);
	}
    
    private void drawInfoBox(Graphics2D g, String[][] lines) {
        final int x0 = 110;
		final int x1 = 245;
		int y = 550;
        Font originalFont = g.getFont();
        g.setFont(originalFont.deriveFont(Font.BOLD));

		for (final String[] line : lines) {
			g.setColor(mouseoverColor.darker());
			g.drawString(line[0] + ":", x0, y);
			g.setColor(Color.BLACK);
			g.drawString(line[1], x1, y);
			y += originalFont.getSize() * 1.5;
		}
        g.setFont(originalFont);
    }

	private void paintKeyOutlines(Graphics2D g) {
		g.setColor(outlineColor);
		for (final Key key : Key.getAllKeys()) {
			g.draw(key.getShape());
		}
	}
}