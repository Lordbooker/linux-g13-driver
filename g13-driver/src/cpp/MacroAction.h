#ifndef __MACRO_ACTION_H__
#define __MACRO_ACTION_H__

#include <linux/uinput.h>
#include <vector>
#include <string>
#include <unistd.h>
#include <memory>       // For std::unique_ptr
#include <pthread.h>    // For pthread_t

#include "G13Action.h"
#include "Output.h"

/**
 * @class MacroAction
 * @brief A G13Action that executes a sequence of key presses, releases, and delays.
 *
 * This class manages parsing a macro string, running it in a separate thread,
 * and handling repeat settings. The execution is offloaded to a central MacroThreadPool.
 */
class MacroAction : public G13Action {
public:
    /**
     * @class Event
     * @brief Abstract base class for a single step in a macro sequence.
     */
    class Event {
    public:
        virtual ~Event() = default;
        /** @brief Executes the event (e.g., press a key, wait). */
        virtual void execute() = 0;
    };

    /** @brief An event that simulates pressing a key down. */
    class KeyDownEvent : public Event {
    private:
        int keycode;
    public:
        KeyDownEvent(int code) : keycode(code) {}
        void execute() override {
            UInput::send_event(EV_KEY, keycode, 1); // 1 for press
            UInput::send_event(EV_SYN, SYN_REPORT, 0); // Send sync event
        }
    };

    /** @brief An event that simulates releasing a key. */
    class KeyUpEvent : public Event {
    private:
        int keycode;
    public:
        KeyUpEvent(int code) : keycode(code) {}
        void execute() override {
            UInput::send_event(EV_KEY, keycode, 0); // 0 for release
            UInput::send_event(EV_SYN, SYN_REPORT, 0); // Send sync event
        }
    };

    /** @brief An event that introduces a delay in the macro sequence. */
    class WaitEvent : public Event {
    private:
        int delay_ms;
    public:
        WaitEvent(int delay) : delay_ms(delay) {}
        void execute() override {
            usleep(delay_ms * 1000); // usleep takes microseconds
        }
    };

    // --- Public Methods ---
    MacroAction(const std::string& sequence);
    virtual ~MacroAction();

    void setRepeats(int r);
    int getRepeats() const;

    /** * @brief The main execution loop for the macro.
     * This is made public to be callable by the MacroThreadPool.
     */
    void execute_macro_loop();


protected:
    /** @brief Overridden to start or stop the macro thread. */
    void key_down() override;
    /** @brief Overridden to potentially stop the macro thread on release. */
    void key_up() override;

private:
    // --- Private Methods ---

    /**
     * @brief Parses a string token (e.g., "kd.29") from the sequence into an Event object.
     * @param token The string token to parse.
     * @return A unique_ptr to the created Event, or nullptr if the token is invalid.
     */
    std::unique_ptr<MacroAction::Event> tokenToEvent(const std::string& token);

    // ENTFERNT: The static entry point is no longer needed.
    // static void* run_macro_thread(void* context);

    // --- Private Member Variables ---
    /** The sequence of events that make up the macro. */
    std::vector<std::unique_ptr<Event>> _events;
    /** Repeat behavior: 0=infinite, 1=while held, >1=fixed number of times. */
    int _repeats;
    int _repeats_on_press; // Potentially for a different repeat mode.
    /** Flag indicating if the macro thread is currently active. */
    bool _is_macro_running;
    /** Flag to signal the running thread to terminate. */
    bool _thread_keep_repeating_flag;
    
    // ENTFERNT: The thread ID is no longer managed here.
    // pthread_t _macro_thread_id;
};

#endif // __MACRO_ACTION_H__