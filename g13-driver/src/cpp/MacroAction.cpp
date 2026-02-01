#include <string.h>
#include <vector>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>

#include "MacroAction.h"

/**
 * @brief The main loop for macro execution.
 */
void MacroAction::execute_macro_loop() {
    _should_stop = false;

    // Case: Run Once (_repeats == 0)
    if (_repeats == 0) {
        for (const auto& event : _events) {
            if (_should_stop) break; // Check interrupt
            event->execute();
        }
        _is_macro_running = false;
        return;
    }

    // Case: Repeating macros
    int current_repeats = 0;
    while (!_should_stop) {
        for (const auto& event : _events) {
            if (_should_stop) break;
            event->execute();
        }

        if (_should_stop) break;

        // Fixed number of repeats
        if (_repeats > 1) {
            current_repeats++;
            if (current_repeats >= _repeats) {
                break;
            }
        }
        // If _repeats == 1, loop continues until key_up sets _should_stop
    }

    _is_macro_running = false;
}

std::unique_ptr<MacroAction::Event> MacroAction::tokenToEvent(const std::string& token) {
    if (token.empty()) return nullptr;

    try {
        if (token.rfind("kd.", 0) == 0) {
            return std::make_unique<KeyDownEvent>(std::stoi(token.substr(3)));
        }
        if (token.rfind("ku.", 0) == 0) {
            return std::make_unique<KeyUpEvent>(std::stoi(token.substr(3)));
        }
        if (token.rfind("d.", 0) == 0) {
            return std::make_unique<WaitEvent>(std::stoi(token.substr(2)));
        }
    } catch (...) {
        std::cerr << "MacroAction::tokenToEvent: Error parsing token: " << token << std::endl;
    }
    return nullptr;
}

MacroAction::MacroAction(const std::string& sequence)
    : _repeats(0), _is_macro_running(false), _should_stop(false) {

    std::stringstream ss(sequence);
    std::string token;
    while (std::getline(ss, token, ',')) {
        if (!token.empty()) {
            auto event = tokenToEvent(token);
            if (event) _events.push_back(std::move(event));
        }
    }
}

MacroAction::~MacroAction() {
    // RAII: Ensure thread is stopped and joined before destruction
    _should_stop = true;
    if (_macro_thread.joinable()) {
        _macro_thread.join();
    }
}

void MacroAction::key_down() {
    if (isPressed()) {
        if (_is_macro_running) {
            // Toggle behavior: Stop if running
            _should_stop = true;
            if (_macro_thread.joinable()) {
                _macro_thread.join();
            }
            return;
        }

        if (_events.empty()) return;

        _is_macro_running = true;
        
        // Clean up previous thread if necessary (should be handled, but safe-guard)
        if (_macro_thread.joinable()) {
            _macro_thread.join();
        }

        // Start new thread
        _macro_thread = std::thread(&MacroAction::execute_macro_loop, this);
    }
}

void MacroAction::key_up() {
    if (_repeats == 1) {
       _should_stop = true;
    }
}

int MacroAction::getRepeats() const {
    return _repeats;
}

void MacroAction::setRepeats(int repeats) {
    this->_repeats = repeats;
}