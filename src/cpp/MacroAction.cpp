#include <unistd.h>
#include <linux/uinput.h>
#include <vector>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <iostream>
#include <vector> // For std::vector<char>

#include "MacroAction.h"

// Static function to be called by pthread_create
void* MacroAction::run_macro_thread(void *context) {
    MacroAction* self = static_cast<MacroAction*>(context);
    self->execute_macro_loop();
    return nullptr;
}

void MacroAction::execute_macro_loop() {
    bool repeat_this_invocation = (_repeats_on_press == 1);
    _thread_keep_repeating_flag = repeat_this_invocation; // Initialize for this run

    do {
        for (const auto& event_ptr : _events) {
            if (!event_ptr) continue; // Should not happen with unique_ptr if properly managed
            
            // Check flag before each event if repeating
            if (repeat_this_invocation && !_thread_keep_repeating_flag) {
                 _is_macro_running = false;
                return; // Exit loop if signaled to stop
            }
            event_ptr->execute();
            usleep(100); // Small delay between events in sequence
        }
    } while (repeat_this_invocation && _thread_keep_repeating_flag);

    _is_macro_running = false; // Mark as not running when loop finishes
}


std::unique_ptr<MacroAction::Event> MacroAction::tokenToEvent(const char *token) {
	if (token == nullptr) {
		return nullptr;
	}

	if (strncmp(token, "kd.", 3) == 0) {
		int code = atoi(&(token[3]));
		return std::make_unique<KeyDownEvent>(code);
	}
	else if (strncmp(token, "ku.", 3) == 0) {
		int code = atoi(&(token[3]));
		return std::make_unique<KeyUpEvent>(code);
	}
	else if (strncmp(token, "d.", 2) == 0) {
		int delay = atoi(&(token[2]));
		return std::make_unique<DelayEvent>(delay);
	}
	else {
		std::cerr << "MacroAction::tokenToEvent() unknown token: " << token << "\n";
	}
	return nullptr;
}

MacroAction::MacroAction(const std::string& tokens_str)
    : _repeats_on_press(0), _is_macro_running(false), _thread_keep_repeating_flag(false), _macro_thread_id(0) {
	if (tokens_str.empty()) {
		return;
	}

    // strtok modifies the string, so we need a mutable copy.
    std::vector<char> tokens_copy(tokens_str.begin(), tokens_str.end());
    tokens_copy.push_back('\0'); // Null-terminate for strtok

	// kd.keycode,ku.keycode,d.time
	char *token = strtok(tokens_copy.data(), ",");
	while (token != nullptr) {
		auto event = tokenToEvent(token);
		if (event) {
			_events.push_back(std::move(event));
		}
		token = strtok(nullptr, ",");
	}
}

MacroAction::~MacroAction() {
    if (_is_macro_running) {
        _thread_keep_repeating_flag = false; // Signal thread to stop
        pthread_join(_macro_thread_id, nullptr); // Wait for thread to finish
    }
    // _events will be cleared automatically by unique_ptr destructors
}

void MacroAction::key_down() {
	if (_is_macro_running) {
		std::cerr << "MacroAction::key_down(): macro already running.\n";
		return;
	}

    if (_events.empty()) {
        return; // No events to execute
    }

    _is_macro_running = true;
    if (pthread_create(&_macro_thread_id, nullptr, &MacroAction::run_macro_thread, this) != 0) {
        _is_macro_running = false;
        std::cerr << "Error creating macro thread\n";
    }
}

void MacroAction::key_up() {
    // If the macro is set to repeat, signal it to stop.
    // The thread itself will set _is_macro_running to false when it exits.
    _thread_keep_repeating_flag = false;
}

int MacroAction::getRepeats() const {
	return _repeats_on_press;
}

void MacroAction::setRepeats(int repeats) {
	this->_repeats_on_press = (repeats == 1) ? 1 : 0; // Ensure it's 0 or 1
}
