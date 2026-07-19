#ifndef __COMBO_PASS_THROUGH_ACTION_H__
#define __COMBO_PASS_THROUGH_ACTION_H__

#include <vector>
#include "G13Action.h"

/**
 * @class ComboPassThroughAction
 * @brief Passthrough for a key combo (e.g. Ctrl+C). While the G-key is held,
 * every code in the list is held down (pressed in listed order, released in
 * reverse), so key repeat behaves like holding the real chord.
 */
class ComboPassThroughAction : public G13Action {
private:
	std::vector<int> keycodes;

protected:
	void key_down() override;
	void key_up() override;

public:
	ComboPassThroughAction(const std::vector<int>& codes);
	virtual ~ComboPassThroughAction();
};

#endif
