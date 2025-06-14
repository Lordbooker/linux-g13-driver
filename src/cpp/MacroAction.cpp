#include <iostream>
#include <string_view>
#include <cstring> // For strcmp
#include <charconv> // For std::from_chars

#include "MacroAction.h"
#include "Output.h"

// Event implementations
void MacroAction::KeyDownEvent::execute() {
    send_event(EV_KEY, _keycode, 1);
    send_event(EV_SYN, SYN_REPORT, 0);
}

void MacroAction::KeyUpEvent::execute() {
    send_event(EV_KEY, _keycode, 0);
    send_event(EV_SYN, SYN_REPORT, 0);
}

void MacroAction::DelayEvent::execute() {
    std::this_thread::sleep_for(std::chrono::milliseconds(_delay_ms));
}

// Helper to parse a single token into an event
std::unique_ptr<MacroAction::Event> MacroAction::tokenToEvent(std::string_view token) {
    if (token.rfind("kd.", 0) == 0) { // starts_with in C++20
        int code = 0;
        std::from_chars(token.data() + 3, token.data() + token.size(), code);
        return std::make_unique<KeyDownEvent>(code);
    }
    if (token.rfind("ku.", 0) == 0) {
        int code = 0;
        std::from_chars(token.data() + 3, token.data() + token.size(), code);
        return std::make_unique<KeyUpEvent>(code);
    }
    if (token.rfind("d.", 0) == 0) {
        int delay = 0;
        std::from_chars(token.data() + 2, token.data() + token.size(), delay);
        return std::make_unique<DelayEvent>(delay);
    }
    std::cerr << "MacroAction::tokenToEvent() unknown token: " << token << "\n";
    return nullptr;
}

MacroAction::MacroAction(const std::string& sequence) {
    if (sequence.empty()) {
        return;
    }

    std::string_view sequence_sv(sequence);
    size_t start = 0;
    size_t end = sequence_sv.find(',');

    while (end!= std::string_view::npos) {
        auto token = sequence_sv.substr(start, end - start);
        if (auto event = tokenToEvent(token)) {
            _events.push_back(std::move(event));
        }
        start = end + 1;
        end = sequence_sv.find(',', start);
    }
    
    // Handle the last token
    auto token = sequence_sv.substr(start);
    if (auto event = tokenToEvent(token)) {
        _events.push_back(std::move(event));
    }
}

MacroAction::~MacroAction() {
    // Safely stop and join the thread if it's running
    if (_macro_thread.joinable()) {
        _stop_requested.store(true, std::memory_order_relaxed);
        _macro_thread.join();
    }
}

void MacroAction::key_down() {
    std::lock_guard<std::mutex> lock(_thread_mutex);
    if (_macro_thread.joinable()) {
        // Macro is already running, decide on behavior (e.g., ignore, restart)
        // Current implementation: ignore.
        return;
    }

    if (_events.empty()) {
        return;
    }

    // Reset stop flag and start a new thread
    _stop_requested.store(false, std::memory_order_relaxed);
    _macro_thread = std::thread(&MacroAction::execute_macro_loop, this);
}

void MacroAction::key_up() {
    // Signal the thread to stop. It will finish its current loop and exit.
    _stop_requested.store(true, std::memory_order_relaxed);
}

void MacroAction::execute_macro_loop() {
    bool repeat = (_repeats_on_press == 1);

    do {
        for (const auto& event : _events) {
            // Check for stop request before each event for faster response
            if (_stop_requested.load(std::memory_order_relaxed)) {
                goto cleanup;
            }
            if (event) {
                event->execute();
            }
        }
    } while (repeat &&!_stop_requested.load(std::memory_order_relaxed));

cleanup:
    // The thread is about to exit. We can join it from another thread now.
    // The lock here is to ensure that a new thread isn't started while we are cleaning up.
    std::lock_guard<std::mutex> lock(_thread_mutex);
    if (_macro_thread.joinable() && _macro_thread.get_id() == std::this_thread::get_id()) {
        _macro_thread.detach(); // Detach from this thread object to allow destruction
    }
}

void MacroAction::setRepeats(int repeats) {
    _repeats_on_press = (repeats == 1)? 1 : 0;
}