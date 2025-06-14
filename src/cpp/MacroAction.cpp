#include <iostream>
#include <string_view>
#include <cstring>
#include <charconv>
#include <chrono>

#include "MacroAction.h"
#include "Output.h"
#include <linux/uinput.h>

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

std::unique_ptr<MacroAction::Event> MacroAction::tokenToEvent(std::string_view token) {
    if (token.rfind("kd.", 0) == 0) {
        // Token starts with "kd."
        int parsed_code = 0;
        std::string_view number_part = token.substr(3); // Part after "kd."
        auto [ptr, ec] = std::from_chars(number_part.data(), number_part.data() + number_part.size(), parsed_code);
        if (ec == std::errc()) { // Check if parsing was successful
            return std::make_unique<KeyDownEvent>(parsed_code);
        }
        std::cerr << "MacroAction::tokenToEvent() failed to parse keycode from token: " << token << "\n";
        return nullptr; // Return nullptr if parsing failed
    }
    if (token.rfind("ku.", 0) == 0) {
        // Token starts with "ku."
        int parsed_code = 0;
        std::string_view number_part = token.substr(3); // Part after "ku."
        auto [ptr, ec] = std::from_chars(number_part.data(), number_part.data() + number_part.size(), parsed_code);
        if (ec == std::errc()) { // Check if parsing was successful
            return std::make_unique<KeyUpEvent>(parsed_code);
        }
        std::cerr << "MacroAction::tokenToEvent() failed to parse keycode from token: " << token << "\n";
        return nullptr; // Return nullptr if parsing failed
    }
    if (token.rfind("d.", 0) == 0) {
        // Token starts with "d."
        int parsed_delay = 0;
        std::string_view number_part = token.substr(2); // Part after "d."
        auto [ptr, ec] = std::from_chars(number_part.data(), number_part.data() + number_part.size(), parsed_delay);
        if (ec == std::errc()) { // Check if parsing was successful
            return std::make_unique<DelayEvent>(parsed_delay);
        }
        std::cerr << "MacroAction::tokenToEvent() failed to parse delay from token: " << token << "\n";
        return nullptr; // Return nullptr if parsing failed
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
    
    auto token = sequence_sv.substr(start);
    if (!token.empty()) {
        if (auto event = tokenToEvent(token)) {
            _events.push_back(std::move(event));
        }
    }
}

MacroAction::~MacroAction() {
    std::thread temp_thread_to_join;
    {
        // Lock the mutex to safely check and manipulate _macro_thread
        std::lock_guard<std::mutex> lock(_thread_mutex);
        if (_macro_thread.joinable()) {
            _stop_requested.store(true, std::memory_order_relaxed); // Signal the thread to stop
            // Move the thread handle to a temporary.
            // this->_macro_thread will become non-joinable.
            temp_thread_to_join = std::move(_macro_thread);
        }
    } // Mutex is released here

    if (temp_thread_to_join.joinable()) {
        temp_thread_to_join.join(); // Join the thread outside the lock
    }
}

void MacroAction::key_down() {
    std::lock_guard<std::mutex> lock(_thread_mutex);
    if (_macro_thread.joinable()) {
        return;
    }

    if (_events.empty()) {
        return;
    }

    _stop_requested.store(false, std::memory_order_relaxed);
    _macro_thread = std::thread(&MacroAction::execute_macro_loop, this);
}

void MacroAction::key_up() {
    _stop_requested.store(true, std::memory_order_relaxed);
}

void MacroAction::execute_macro_loop() {
    bool repeat_macro = (_repeats_on_press == 1);
    bool stopped_early = false;

    do {
        for (const auto& event : _events) {
            if (_stop_requested.load(std::memory_order_relaxed)) {
                stopped_early = true;
                break; // Break from inner for-loop
            }
            if (event) {
                event->execute();
            }
        }
        if (stopped_early) {
            break; // Break from outer do-while loop
        }
    } while (repeat_macro && !_stop_requested.load(std::memory_order_relaxed)); // Check _stop_requested again for repeat case

    // Cleanup: Detach the thread object if this thread is the one it represents.
    std::lock_guard<std::mutex> lock(_thread_mutex);
    if (_macro_thread.joinable() && _macro_thread.get_id() == std::this_thread::get_id()) {
        _macro_thread.detach();
    }
}

void MacroAction::setRepeats(int repeats) {
    _repeats_on_press = (repeats == 1)? 1 : 0;
}