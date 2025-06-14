#ifndef __PASS_THROUGH_ACTION_H__
#define __PASS_THROUGH_ACTION_H__

#pragma once

#include "G13Action.h"

class PassThroughAction final : public G13Action {
private:
    int _keycode;

protected:
    void key_down() override;
    void key_up() override;

public:
    explicit PassThroughAction(int code);
    ~PassThroughAction() override = default;
};

#endif // __PASS_THROUGH_ACTION_H__