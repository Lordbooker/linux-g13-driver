#ifndef __G13_ACTION_H__
#define __G13_ACTION_H__

#pragma once

class G13Action {
private:
    bool _is_pressed = false;

protected:
    virtual void key_down() {}
    virtual void key_up() {}

public:
    G13Action() = default;
    virtual ~G13Action() = default;

    G13Action(const G13Action&) = delete;
    G13Action& operator=(const G13Action&) = delete;

    virtual bool set(bool state);
    bool isPressed() const;
};

#endif // __G13_ACTION_H__