#ifndef G13_DRIVER_MACROACTION_H
#define G13_DRIVER_MACROACTION_H

#include "G13Action.h"

#include <atomic>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

/**
 * @brief Plays a macro (key presses, releases and delays) in its own thread.
 *
 * Sequence format (as written by g13-gui): "kd.<code>,ku.<code>,d.<ms>,..."
 *
 * Repeat mode:
 *  - 0: run once
 *  - 1: repeat while the key is held
 *  - n > 1: run n times
 *
 * Pressing the key again while the macro runs stops it. A stopped macro
 * releases all keys it pressed, and delays end immediately on a stop.
 */
class MacroAction : public G13Action {
public:
    MacroAction(const std::string &sequence, int repeats);
    ~MacroAction() override;

    MacroAction(const MacroAction &) = delete;
    MacroAction &operator=(const MacroAction &) = delete;
    MacroAction(MacroAction &&) = delete;
    MacroAction &operator=(MacroAction &&) = delete;

    static constexpr int RunOnce = 0;
    static constexpr int RepeatWhileHeld = 1;

protected:
    void key_down() override;
    void key_up() override;

private:
    struct Step {
        enum class Type : std::uint8_t { KeyDown, KeyUp, Delay } type;
        int value;   // key code or milliseconds
    };

    static bool parseStep(const std::string &token, Step &step);

    void run();
    bool runSteps(std::vector<int> &pressed);   // false when stopped
    void requestStop();
    void stopAndJoin();

    std::vector<Step> m_steps;
    int m_repeats;

    std::thread m_thread;
    std::atomic<bool> m_running{false};
    std::mutex m_mutex;                 // protects m_stop for the condition variable
    std::condition_variable m_wakeup;
    bool m_stop = false;
};

#endif // G13_DRIVER_MACROACTION_H
