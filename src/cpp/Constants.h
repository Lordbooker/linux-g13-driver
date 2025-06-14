#ifndef __CONSTANTS_H__
#define __CONSTANTS_H__

#pragma once // Modern, simpler header guard

#include <cstdint> // For fixed-width integer types

// USB Device Constants
constexpr uint16_t G13_VENDOR_ID = 0x046d;
constexpr uint16_t G13_PRODUCT_ID = 0xc21c;

// USB Interface and Endpoint Constants
constexpr int G13_INTERFACE = 0;
constexpr int G13_KEY_ENDPOINT = 1;
constexpr int G13_LCD_ENDPOINT = 2;

// USB Transfer Constants
constexpr int G13_KEY_READ_TIMEOUT_MS = 100; // Reduced timeout for better responsiveness
constexpr int G13_REPORT_SIZE = 8;
constexpr int G13_LCD_BUFFER_SIZE = 0x3c0;

// G13 Device Constants
constexpr size_t G13_NUM_KEYS = 40;

// Stick mode definition using a scoped enum for type safety
enum class stick_mode_t { 
    STICK_KEYS, 
    STICK_ABSOLUTE 
    /* STICK_RELATIVE, */ 
};

// Stick key mapping using a scoped enum
enum class stick_key_t { 
    STICK_LEFT, 
    STICK_UP, 
    STICK_DOWN, 
    STICK_RIGHT 
};

// G13 Key mapping using a scoped enum for type safety and to avoid name clashes.
// The underlying type is specified as uint8_t to match usage.
enum class G13_KEYS : uint8_t {
    /* byte 3 */
    G1 = 0, G2, G3, G4, G5, G6, G7, G8,

    /* byte 4 */
    G9, G10, G11, G12, G13, G14, G15, G16,

    /* byte 5 */
    G17, G18, G19, G20, G21, G22, UNDEF1, LIGHT_STATE,

    /* byte 6 */
    BD, L1, L2, L3, L4, M1, M2, M3,

    /* byte 7 */
    MR, LEFT, DOWN, TOP, UNDEF3, LIGHT, LIGHT2, MISC_TOGGLE
};

#endif // __CONSTANTS_H__