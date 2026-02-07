#ifndef __G13_ACTION_H__
#define __G13_ACTION_H__

/**
 * @class G13Action
 * @brief Abstract base class for all actions that can be assigned to a G13 key.
 *
 * This class provides a common interface for different types of actions,
 * such as passing through a key press, executing a macro, etc. It manages
 * the pressed/released state of the key.
 */
class G13Action {
private:
    /** Stores the current state of the key (0 for released, 1 for pressed). */
	int pressed;

protected:
    /**
     * @brief Virtual method executed when the key is pressed.
     * Subclasses must implement the specific logic for this event.
     */
	virtual void key_down();

    /**
     * @brief Virtual method executed when the key is released.
     * Subclasses must implement the specific logic for this event.
     */
	virtual void key_up();

public:
    /** @brief Default constructor. */
	G13Action();

    /** @brief Virtual destructor to ensure proper cleanup of derived classes. */
	virtual ~G13Action();

    /**
     * @brief Updates the key's state and triggers key_down/key_up if it changed.
     * @param state The new state (non-zero for pressed, 0 for released).
     * @return 1 if the state changed, 0 otherwise.
     */
	virtual int set(int state);

    /**
     * @brief Checks if the key is currently considered pressed.
     * @return The current pressed state (1 or 0).
     */
	int isPressed() const;
};

#endif