package com.booker.g13;

import java.awt.Image;
import java.io.IOException;
import java.net.URL;
import java.util.Map;
import java.util.concurrent.ConcurrentHashMap;
import java.util.logging.Level;
import java.util.logging.Logger;

import javax.imageio.ImageIO;
import javax.swing.ImageIcon;

/**
 * A utility class for loading and resizing ImageIcons.
 * It provides a thread-safe caching mechanism to avoid reloading images from resources.
 */
public class ImageIconHelper {
	private static final Logger log = Logger.getLogger(ImageIconHelper.class.getName());

	/**
	 * A thread-safe cache to store already loaded images, mapping resource names to ImageIcon objects.
	 */
	private static final Map<String, ImageIcon> loadedImages = new ConcurrentHashMap<>();

	/**
	 * Private constructor to prevent instantiation of this utility class.
	 */
	private ImageIconHelper() {
    }

	/**
	 * Resizes a given ImageIcon to the specified dimensions.
	 * @param src The source ImageIcon.
	 * @param destWidth The target width.
	 * @param destHeight The target height.
	 * @return A new, resized ImageIcon.
	 */
	public static ImageIcon resize(ImageIcon src, int destWidth, int destHeight) {
		// Use SCALE_SMOOTH for a higher quality resized image.
		return new ImageIcon(src.getImage().getScaledInstance(destWidth,
				destHeight, Image.SCALE_SMOOTH));
	}

	/**
	 * Loads an image from the embedded application resources.
	 * It uses a cache to return an existing instance if the image has been loaded before.
	 * This method blocks until the image is fully loaded.
	 *
	 * @param name The fully qualified path to the image resource (e.g., "/com/booker/g13/images/icon.png").
	 * @return The loaded ImageIcon, or null if loading fails.
	 */
	public static ImageIcon loadEmbeddedImage(String name) {
		// computeIfAbsent ensures the loading logic is executed only once per key in a thread-safe way.
		return loadedImages.computeIfAbsent(name, key -> {
            log.fine("ImageIconHelper::loadEmbeddedImage(" + key + ")");
            try {
                URL url = ImageIconHelper.class.getResource(key);
                if (url == null) {
                    log.warning("Image \"" + key + "\" not found");
                    return null;
                }
                // ImageIO.read blocks until the entire image is loaded, which is more reliable than MediaTracker.
                Image image = ImageIO.read(url);
                if (image == null) {
                    log.warning("File " + key + " could not be loaded as an image.");
                    return null;
                }
                return new ImageIcon(image);
            } catch (IOException ioe) {
                log.log(Level.WARNING, "Failed to Load Image " + key, ioe);
                return null;
            }
        });
	}
	
	/**
	 * Loads an embedded image and scales it to fit within a bounding box, maintaining aspect ratio.
	 * @param name The path to the image resource.
	 * @param maxWidth The maximum desired width.
	 * @param maxHeight The maximum desired height.
	 * @return A new, scaled ImageIcon, or an empty ImageIcon on failure.
	 */
	public static ImageIcon loadEmbeddedImage(String name, int maxWidth, int maxHeight) {
		if ((name == null) || (maxWidth < 0) || (maxHeight < 0)) {
			// Return an empty icon to prevent NullPointerExceptions.
			return new ImageIcon();
		}

		final ImageIcon bigIcon = loadEmbeddedImage(name);
		if (bigIcon == null) {
			return new ImageIcon(); // Return empty icon if loading failed.
		}
		
		// This scaling logic preserves the image's aspect ratio.
		final double largeSide = Math.max(bigIcon.getIconHeight(), bigIcon.getIconWidth());
		if (largeSide == 0) return bigIcon; // Avoid division by zero for an empty icon.

		final int sWidth = (int) Math.round((double) maxWidth * bigIcon.getIconWidth() / largeSide);
		final int sHeight = (int) Math.round((double) maxHeight * bigIcon.getIconHeight() / largeSide);

		return new ImageIcon(bigIcon.getImage().getScaledInstance(sWidth, sHeight, Image.SCALE_SMOOTH));
	}
}