package com.booker.g13;

import java.awt.Component;
import java.awt.Image;
import java.awt.MediaTracker;
import java.awt.Toolkit;
import java.io.IOException;
import java.net.URL;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.swing.ImageIcon;

public final class ImageIconHelper {
	private static final Logger log = Logger.getLogger(ImageIconHelper.class.getName());
	private static final Component component = new Component() {};
	private static final Map<String, ImageIcon> loadedImages = new ConcurrentHashMap<>();

	private ImageIconHelper() {
		// Utility class, not meant to be instantiated
	}

	public static ImageIcon resize(ImageIcon src, int destWidth, int destHeight) {
		if (src == null || destWidth <= 0 || destHeight <= 0) {
			return new ImageIcon();
		}
		return new ImageIcon(src.getImage().getScaledInstance(destWidth, destHeight, Image.SCALE_SMOOTH));
	}

	public static ImageIcon loadEmbeddedImage(String name) {
		if (name == null || name.trim().isEmpty()) {
			log.warning("Image name is null or empty.");
			return null;
		}

		return loadedImages.computeIfAbsent(name, key -> {
			log.fine("Loading embedded image: " + key);
			try {
				URL url = ImageIconHelper.class.getResource(key);
				if (url == null) {
					log.warning("Image \"" + key + "\" not found in classpath.");
					return null;
				}
				Image image = Toolkit.getDefaultToolkit().createImage(url);
				MediaTracker tracker = new MediaTracker(component);
				tracker.addImage(image, 0);
				tracker.waitForID(0);
				if (tracker.isErrorID(0)) {
					log.warning("Error loading image: " + key);
					return null;
				}
				return new ImageIcon(image);
			} catch (InterruptedException e) {
				Thread.currentThread().interrupt();
				log.log(Level.WARNING, "Image loading interrupted for: " + key, e);
				return null;
			} catch (Exception e) {
				log.log(Level.WARNING, "Failed to load image: " + key, e);
				return null;
			}
		});
	}

	public static ImageIcon loadEmbeddedImage(String name, int maxWidth, int maxHeight) {
		if ((name == null) || (maxWidth <= 0) || (maxHeight <= 0)) {
			return new ImageIcon();
		}

		final ImageIcon bigIcon = loadEmbeddedImage(name);
		if (bigIcon == null) {
			return new ImageIcon();
		}

		return resize(bigIcon, maxWidth, maxHeight);
	}
}