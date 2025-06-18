#include <string.h>
#include <vector>
#include <iostream>
#include <memory> // For std::unique_ptr

#include "MacroAction.h"

/**
 * @brief Static thread entry function.
 * @param context A void pointer to the MacroAction instance.
 * @return Always returns nullptr.
 *
 * This function is the entry point for the new thread created to run a macro.
 * It casts the context back to a MacroAction pointer and calls the instance's
 * execution loop.
 */
void* MacroAction::run_macro_thread(void *context) {
    MacroAction* action = static_cast<MacroAction*>(context);
    action->execute_macro_loop();
    return nullptr;
}

/**
 * @brief The main loop for macro execution within its own thread.
 *
 * This loop iterates through the macro's event sequence. It respects the
 * repeat setting and can be stopped externally by setting the
 * _thread_keep_repeating_flag to false.
 */
void MacroAction::execute_macro_loop() {
    _thread_keep_repeating_flag = true;
    int current_repeats = 0;

    // Loop as long as the flag is set and repeat count is not exceeded.
    while (_thread_keep_repeating_flag && (_repeats == 0 || current_repeats < _repeats)) {
        for (const auto& event : _events) {
            event->execute();
        }
        if (_repeats > 0) {
            current_repeats++;
        }
    }
    _is_macro_running = false; // Mark macro as finished.
}

/**
 * @brief Parses a single token from a macro sequence string into an Event object.
 * @param token The string token (e.g., "+29", "-29", "W100").
 * @return A unique_ptr to the created Event, or nullptr if the token is invalid.
 */
std::unique_ptr<MacroAction::Event> MacroAction::tokenToEvent(const char *token) {
    switch (token[0]) {
        case '+': // Key Down
            return std::make_unique<KeyDownEvent>(atoi(&token[1]));
        case '-': // Key Up
            return std::make_unique<KeyUpEvent>(atoi(&token[1]));
        case 'W': // Wait/Delay
            return std::make_unique<WaitEvent>(atoi(&token[1]));
    }
    return nullptr;
}


// --- Constructor / Destructor ---

/**
 * @brief Constructs a MacroAction from a sequence string.
 * @param sequence The space-delimited string of macro events.
 *
 * This constructor parses the sequence string (e.g., "+29 +56 W20 -56 -29")
 * into a vector of Event objects.
 */
MacroAction::MacroAction(const std::string& sequence)
    : _repeats(0), _repeats_on_press(0), _is_macro_running(false), _thread_keep_repeating_flag(false), _macro_thread_id(0) {

    // Use a temporary C-style string for strtok.
    char* seq_c_str = new char[sequence.length() + 1];
    strcpy(seq_c_str, sequence.c_str());

    char* token = strtok(seq_c_str, " ");
    while (token != nullptr) {
        if (strlen(token) > 0) {
            auto event = tokenToEvent(token);
            if (event) {
                _events.push_back(std::move(event));
            }
        }
        token = strtok(nullptr, " ");
    }

    delete[] seq_c_str;
}

/**
 * @brief Destructor for MacroAction.
 *
 * If a macro thread is still running, it signals the thread to stop and
 * waits for it to terminate cleanly.
 */
MacroAction::~MacroAction() {
    if (_is_macro_running) {
        _thread_keep_repeating_flag = false; // Signal the thread to stop.
        pthread_join(_macro_thread_id, nullptr); // Wait for the thread to finish.
    }
    // _events vector is cleaned up automatically by unique_ptr.
}

// --- Key-Events ---

/**
 * @brief Handles the key-down event for this action.
 *
 * If a macro is not already running, it starts a new thread to execute it.
 * If a macro is running, pressing the key again will signal it to stop.
 */
void MacroAction::key_down() {
    if (isPressed()) {
        if (_is_macro_running) {
            // If the macro is running, pressing the key again stops it.
            _thread_keep_repeating_flag = false;
            return;
        }

        if (_events.empty()) {
            return; // Nothing to execute.
        }

        _is_macro_running = true;
        // Create a new thread to run the macro, passing 'this' as the context.
        if (pthread_create(&_macro_thread_id, nullptr, &MacroAction::run_macro_thread, this) != 0) {
            perror("pthread_create() error");
            _is_macro_running = false;
        }
    }
}

/**
 * @brief Handles the key-up event for this action.
 *
 * If the macro is set to repeat only once (run-while-pressed mode), releasing
 * the key will stop the macro execution.
 */
void MacroAction::key_up() {
    // If repeat is set to 1, this means "repeat while held".
    // Releasing the key should stop the macro.
    if (_repeats == 1) {
       _thread_keep_repeating_flag = false;
    }
}

// --- Getter / Setter ---

int MacroAction::getRepeats() const {
    return _repeats;
}

void MacroAction::setRepeats(int repeats) {
    this->_repeats = repeats;
}