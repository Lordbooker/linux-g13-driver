#ifndef __MACRO_ACTION_H__
#define __MACRO_ACTION_H__

#include <linux/uinput.h>
#include <vector>
#include <string>
#include <memory>       
#include <thread>
#include <atomic>
#include <chrono>

#include "G13Action.h"
#include "Output.h"

/**
 * @class MacroAction
 * @brief A G13Action that executes a sequence of key presses, releases, and delays.
 * * Modernized to use std::thread and std::atomic for thread safety.
 */
class MacroAction : public G13Action {
public:
    class Event {
    public:
        virtual ~Event() = default;
        virtual void execute() = 0;
    };

    class KeyDownEvent : public Event {
    private:
        int keycode;
    public:
        KeyDownEvent(int code) : keycode(code) {}
        void execute() override {
            UInput::send_event(EV_KEY, keycode, 1); 
            UInput::send_event(EV_SYN, SYN_REPORT, 0);
        }
    };

    class KeyUpEvent : public Event {
    private:
        int keycode;
    public:
        KeyUpEvent(int code) : keycode(code) {}
        void execute() override {
            UInput::send_event(EV_KEY, keycode, 0); 
            UInput::send_event(EV_SYN, SYN_REPORT, 0);
        }
    };

    class WaitEvent : public Event {
    private:
        int delay_ms;
    public:
        WaitEvent(int delay) : delay_ms(delay) {}
        void execute() override {
            // Modern C++ sleep
            std::this_thread::sleep_for(std::chrono::milliseconds(delay_ms));
        }
    };

    // --- Public Methods ---
    MacroAction(const std::string& sequence);
    virtual ~MacroAction();

    void setRepeats(int r);
    int getRepeats() const;

protected:
    void key_down() override;
    void key_up() override;

private:
    // --- Private Methods ---
    void execute_macro_loop();
    std::unique_ptr<MacroAction::Event> tokenToEvent(const std::string& token);

    // --- Private Member Variables ---
    std::vector<std::unique_ptr<Event>> _events;
    
    int _repeats;
    
    // Threading control
    std::atomic<bool> _is_macro_running;
    std::atomic<bool> _should_stop;
    std::thread _macro_thread;
};

#endif // __MACRO_ACTION_H__