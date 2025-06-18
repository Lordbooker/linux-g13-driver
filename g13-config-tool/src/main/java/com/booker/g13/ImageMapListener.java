package com.booker.g13;

/**
 * An interface for listeners that want to receive events from the ImageMap component.
 * This allows other parts of the application to react when the user interacts with the keypad image.
 */
public interface ImageMapListener {

	/**
	 * Called when a key on the ImageMap is clicked (selected).
	 * @param key The Key object that was selected, or null if the click was outside any key.
	 */
	void selected(final Key key);
	
	/**
	 * Called when the mouse cursor moves over a key on the ImageMap.
	 * @param key The Key object the mouse is currently over, or null if it's not over any key.
	 */
	void mouseover(final Key key);
}