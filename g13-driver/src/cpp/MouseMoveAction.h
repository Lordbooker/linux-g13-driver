#ifndef __MOUSE_MOVE_ACTION_H__
#define __MOUSE_MOVE_ACTION_H__

#include <vector>
#include <string>
#include <thread>
#include <atomic>

#include "G13Action.h"

// Parses "42+274" into {42, 274}. Invalid segments are skipped.
std::vector<int> parse_plus_list(const std::string& value);

/**
 * @class MouseMoveAction
 * @brief While the key is held, holds a set of keys/buttons and emits
 *        relative mouse motion (dx, dy) every 10 ms. Used for camera pan.
 */
class MouseMoveAction : public G13Action {
private:
    int dx;
    int dy;
    std::vector<int> hold;
    std::thread mover;
    std::atomic<bool> stop_flag;

    void move_loop();

protected:
    void key_down() override;
    void key_up() override;

public:
    MouseMoveAction(int dx, int dy, std::vector<int> hold);
    ~MouseMoveAction() override;
};

#endif
