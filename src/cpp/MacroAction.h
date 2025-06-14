#ifndef __MACRO_ACTION_H__
#define __MACRO_ACTION_H__

#pragma once

#include <vector>
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <mutex>

#include "G13Action.h"

class MacroAction : public G13Action {
public:
    class Event {
    public:
        virtual ~Event() = default;
        virtual void execute() = 0;
    };

    class KeyDownEvent : public Event {
    public:
        explicit KeyDownEvent(int code) : _keycode(code) {}
        void execute() override;
    private:
        int _keycode;
    };

    class KeyUpEvent : public Event {
    public:
        explicit KeyUpEvent(int code) : _keycode(code) {}
        void execute() override;
    private:
        int _keycode;
    };

    class DelayEvent : public Event {
    public:
        explicit DelayEvent(int delay) : _delay_ms(delay) {}
        void execute() override;
    private:
        int _delay_ms;
    };

private:
    std::vector<std::unique_ptr<Event>> _events;
    int                                 _repeats_on_press = 0;
    
    std::thread                         _macro_thread;
    std::mutex                          _thread_mutex;
    std::atomic<bool>                   _stop_requested{false};

protected:
    void key_down() override;
    void key_up() override;

    void execute_macro_loop();
    
    static std::unique_ptr<Event> tokenToEvent(std::string_view token);

public:
    explicit MacroAction(const std::string& sequence);
    ~MacroAction() override;

    MacroAction(const MacroAction&) = delete;
    MacroAction& operator=(const MacroAction&) = delete;

    void setRepeats(int repeats);
};

#endif // __MACRO_ACTION_H__