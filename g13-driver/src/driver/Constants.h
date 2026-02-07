#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

// USB device-specific constants for the Logitech G13
#define G13_INTERFACE 0         // The interface number for the G13.
#define G13_KEY_ENDPOINT 1      // The endpoint for reading key and joystick state.
#define G13_LCD_ENDPOINT 2      // The endpoint for writing to the LCD screen.
#define G13_KEY_READ_TIMEOUT 0  // Timeout for USB interrupt reads.
#define G13_VENDOR_ID 0x046d    // Logitech's Vendor ID.
#define G13_PRODUCT_ID 0xc21c   // The Product ID for the G13.
#define G13_REPORT_SIZE 8       // Size of the input report from the G13 (in bytes).
#define G13_LCD_BUFFER_SIZE 0x3c0 // Size of the buffer for the LCD screen.
#define G13_NUM_KEYS 40         // Total number of logical keys, including stick directions.

/**
 * @enum stick_mode_t
 * @brief Defines the operating modes for the G13's joystick.
 */
enum stick_mode_t {
    STICK_KEYS = 0,   // Joystick movement emulates key presses (e.g., W, A, S, D).
    STICK_ABSOLUTE,   // Joystick provides absolute position values (like a gamepad).
    /*STICK_RELATIVE,*/ // A possible future mode for relative mouse movement.
};

/**
 * @enum stick_key_t
 * @brief Defines logical names for the four joystick directions.
 */
enum stick_key_t { STICK_LEFT, STICK_UP, STICK_DOWN, STICK_RIGHT };

/**
 * @enum G13_KEYS
 * @brief Defines the bit offset for each physical key within the G13's 8-byte input report.
 * The keys are grouped by which byte of the report they reside in (starting from byte 3).
 */
enum G13_KEYS {
    /* byte 3 of the report */
    G13_KEY_G1 = 0,
    G13_KEY_G2,
    G13_KEY_G3,
    G13_KEY_G4,

    G13_KEY_G5,
    G13_KEY_G6,
    G13_KEY_G7,
    G13_KEY_G8,

    /* byte 4 of the report */
    G13_KEY_G9,
    G13_KEY_G10,
    G13_KEY_G11,
    G13_KEY_G12,

    G13_KEY_G13,
    G13_KEY_G14,
    G13_KEY_G15,
    G13_KEY_G16,

    /* byte 5 of the report */
    G13_KEY_G17,
    G13_KEY_G18,
    G13_KEY_G19,
    G13_KEY_G20,

    G13_KEY_G21,
    G13_KEY_G22,
    G13_KEY_UNDEF1,        // An undefined/unused bit.
    G13_KEY_LIGHT_STATE,   // State of the backlight.

    /* byte 6 of the report */
    G13_KEY_BD,            // The "Backlight Dimmer" button.
    G13_KEY_L1,            // The L1 display button.
    G13_KEY_L2,            // The L2 display button.
    G13_KEY_L3,            // The L3 display button.

    G13_KEY_L4,            // The L4 display button.
    G13_KEY_M1,            // The M1 profile button.
    G13_KEY_M2,            // The M2 profile button.
    G13_KEY_M3,            // The M3 profile button.

    /* byte 7 of the report */
    G13_KEY_MR,            // The "Macro Record" (MR) profile button.
    G13_KEY_LEFT,          // Left thumb button.
    G13_KEY_DOWN,          // Down thumb button.
    G13_KEY_TOP,           // Top thumb button (joystick press).

    G13_KEY_UNDEF3,        // An undefined/unused bit.
    G13_KEY_LIGHT,         // Another backlight-related key.
    G13_KEY_LIGHT2,        // Another backlight-related key.
    G13_KEY_MISC_TOGGLE    // A miscellaneous toggle key.
};

#endif