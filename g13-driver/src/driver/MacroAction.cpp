#include "MacroAction.h"
#include "Output.h"

#include <linux/input.h>
#include <syslog.h>

#include <algorithm>
#include <charconv>
#include <chrono>
#include <sstream>

MacroAction::MacroAction(const std::string &sequence, int repeats)
    : m_repeats(std::max(repeats, RunOnce))
{
    std::stringstream stream(sequence);
    std::string token;
    while (std::getline(stream, token, ',')) {
        if (token.empty())
            continue;
        Step step{};
        if (parseStep(token, step))
            m_steps.push_back(step);
        else
            syslog(LOG_WARNING, "Invalid macro step ignored: %s", token.c_str());
    }
}

MacroAction::~MacroAction()
{
    stopAndJoin();
}

bool MacroAction::parseStep(const std::string &token, Step &step)
{
    struct Prefix {
        const char *text;
        Step::Type type;
    };
    static constexpr Prefix prefixes[] = {
        {"kd.", Step::Type::KeyDown},
        {"ku.", Step::Type::KeyUp},
        {"d.", Step::Type::Delay},
    };
    for (const auto &prefix : prefixes) {
        const std::string_view text(prefix.text);
        if (!token.starts_with(text))
            continue;
        const char *first = token.data() + text.size();
        const char *last = token.data() + token.size();
        int value = 0;
        const auto [end, error] = std::from_chars(first, last, value);
        if (error != std::errc() || end != last || value < 0)
            return false;
        if (prefix.type != Step::Type::Delay && (value == 0 || value > KEY_MAX))
            return false;
        step = Step{prefix.type, value};
        return true;
    }
    return false;
}

void MacroAction::key_down()
{
    if (m_running) {
        // Pressing the key again stops a running macro.
        stopAndJoin();
        return;
    }
    if (m_steps.empty())
        return;

    if (m_thread.joinable())
        m_thread.join();   // previous run has finished, only clean up
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = false;   // reset before the thread starts, so a quick key_up is not lost
    }
    m_running = true;
    try {
        m_thread = std::thread(&MacroAction::run, this);
    } catch (const std::system_error &e) {
        m_running = false;
        syslog(LOG_ERR, "Could not start macro thread: %s", e.what());
    }
}

void MacroAction::key_up()
{
    if (m_repeats == RepeatWhileHeld)
        requestStop();
}

void MacroAction::requestStop()
{
    {
        const std::lock_guard<std::mutex> lock(m_mutex);
        m_stop = true;
    }
    m_wakeup.notify_all();
}

void MacroAction::stopAndJoin()
{
    requestStop();
    if (m_thread.joinable())
        m_thread.join();
}

void MacroAction::run()
{
    std::vector<int> pressed;   // keys pressed by this macro and not released yet
    int runs = 0;
    while (runSteps(pressed)) {
        ++runs;
        if (m_repeats == RunOnce || (m_repeats > RepeatWhileHeld && runs >= m_repeats))
            break;
    }

    // Release what is still held (macro stopped in the middle, or a sequence
    // without its ku. steps), otherwise the keys stay pressed in the system.
    for (const int keycode : pressed) {
        UInput::send_event(EV_KEY, keycode, 0);
        UInput::send_event(EV_SYN, SYN_REPORT, 0);
    }
    m_running = false;
}

bool MacroAction::runSteps(std::vector<int> &pressed)
{
    for (const Step &step : m_steps) {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_stop)
            return false;
        switch (step.type) {
        case Step::Type::Delay:
            if (m_wakeup.wait_for(lock, std::chrono::milliseconds(step.value),
                                  [this] { return m_stop; }))
                return false;
            break;
        case Step::Type::KeyDown:
            lock.unlock();
            UInput::send_event(EV_KEY, step.value, 1);
            UInput::send_event(EV_SYN, SYN_REPORT, 0);
            if (std::ranges::find(pressed, step.value) == pressed.end())
                pressed.push_back(step.value);
            break;
        case Step::Type::KeyUp:
            lock.unlock();
            UInput::send_event(EV_KEY, step.value, 0);
            UInput::send_event(EV_SYN, SYN_REPORT, 0);
            std::erase(pressed, step.value);
            break;
        }
    }
    const std::lock_guard<std::mutex> lock(m_mutex);
    return !m_stop;
}
