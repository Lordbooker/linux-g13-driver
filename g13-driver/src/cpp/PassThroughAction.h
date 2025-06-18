#ifndef __PASS_THROUGH_ACTION_H__
#define __PASS_THROUGH_ACTION_H__

#include "G13Action.h"

/**
 * @class PassThroughAction
 * @brief A concrete G13Action that simulates a single standard keyboard key press.
 *
 * When triggered, this action sends a key-down event to the kernel, and when
 * released, it sends a key-up event, effectively "passing through" the press.
 */
class PassThroughAction : public G13Action {
private:
    /** The Linux keycode that this action will send. */
	int keycode;

protected:
    /** @brief Overridden to send the key press event. */
	void key_down() override;
    /** @brief Overridden to send the key release event. */
	void key_up() override;

public:
    /**
     * @brief Constructor.
     * @param code The Linux keycode for this action.
     */
	PassThroughAction(int code);

    /** @brief Virtual destructor. */
	virtual ~PassThroughAction();

    /** @brief Gets the current keycode. */
	int getKeyCode() const;

    /** @brief Sets a new keycode. */
	void setKeyCode(int code);
};

#endif