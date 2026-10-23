#ifndef G13_DRIVER_G13ACTION_H
#define G13_DRIVER_G13ACTION_H

/**
 * @brief Base class for the action bound to a G13 key.
 *
 * Tracks the pressed state of the key and calls key_down()/key_up() when it
 * changes. The base class itself does nothing ("no action").
 */
class G13Action {
public:
    G13Action() = default;
    virtual ~G13Action() = default;

    G13Action(const G13Action &) = delete;
    G13Action &operator=(const G13Action &) = delete;
    G13Action(G13Action &&) = delete;
    G13Action &operator=(G13Action &&) = delete;

    /**
     * @brief Updates the key state and calls key_down()/key_up() on a change.
     * @return true if the state changed.
     */
    bool set(bool pressed);

    [[nodiscard]] bool isPressed() const { return m_pressed; }

protected:
    virtual void key_down() {}
    virtual void key_up() {}

private:
    bool m_pressed = false;
};

#endif // G13_DRIVER_G13ACTION_H
