#ifndef __MACRO_ACTION_H__
#define __MACRO_ACTION_H__

#include <unistd.h>
#include <vector>
#include <string>
#include <memory> // For std::unique_ptr
#include <pthread.h>
#include <linux/uinput.h> // For EV_KEY, etc.

#include "G13Action.h"
#include "Output.h"

class MacroAction : public G13Action
{
public:
	class Event {
	public:
		Event() {};
		virtual ~Event() = default; // Important: virtual destructor
		virtual void execute() = 0; // Make pure virtual if no default behavior
	};

	class KeyDownEvent : public Event { // Inherit publicly
	private:
		int keycode;
	public:
		explicit KeyDownEvent(int code) : keycode(code) {}
		void execute() override {
			send_event(EV_KEY, keycode, 1);
			send_event(0, 0, 0);
		}
	};

	class KeyUpEvent : public Event { // Inherit publicly
	private:
		int keycode;
	public:
		explicit KeyUpEvent(int code) : keycode(code) {}
		void execute() override {
			send_event(EV_KEY, keycode, 0);
			send_event(0, 0, 0);
		}
	};

	class DelayEvent : public Event { // Inherit publicly
	private:
		int delayInMillisecs;
	public:
		explicit DelayEvent(int delay) : delayInMillisecs(delay) {}
		void execute() override {
			usleep(1000*delayInMillisecs);
		}
	};

private:
    // Structure to hold data for the macro execution thread
    struct MacroRunnerArgs {
        std::vector<Event*>* p_events; // Pointer to the events owned by MacroAction
        volatile bool* p_keepRepeating;
        int initial_repeats; // 0 for once, 1 for repeat until key_up
    };

	std::vector<std::unique_ptr<Event>>  _events;
    int                                  _repeats_on_press; // 0 = play once, 1 = repeat until key_up
    volatile bool                        _is_macro_running;
    volatile bool                        _thread_keep_repeating_flag; // Controlled by key_up
    pthread_t                            _macro_thread_id;
    // MultiEventThread             *thread; // Replaced by direct thread management

protected:
    std::unique_ptr<Event> tokenToEvent(const char *token);
	void        key_down() override;
	void        key_up() override;

    static void* run_macro_thread(void *context);
    void execute_macro_loop();

public:
    explicit MacroAction(const std::string& tokens_str);
    ~MacroAction() override;

    int  getRepeats() const;
    void setRepeats(int repeats); // This sets how it behaves on next key_down
    // const std::vector<std::unique_ptr<Event>>& getEvents() const; // If needed
};

#endif
