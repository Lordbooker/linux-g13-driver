#ifndef G13_DRIVER_CONSTANTS_H
#define G13_DRIVER_CONSTANTS_H

#include <cstddef>
#include <cstdint>

// USB constants of the Logitech G13.
inline constexpr std::uint16_t G13_VENDOR_ID  = 0x046d;
inline constexpr std::uint16_t G13_PRODUCT_ID = 0xc21c;
inline constexpr int G13_INTERFACE    = 0;    // interface used for keys, LCD and backlight
inline constexpr int G13_KEY_ENDPOINT = 1;    // interrupt IN: key and joystick state
inline constexpr int G13_LCD_ENDPOINT = 2;    // interrupt OUT: LCD image
inline constexpr std::size_t G13_REPORT_SIZE     = 8;      // input report size in bytes
inline constexpr std::size_t G13_LCD_BUFFER_SIZE = 0x3c0;  // 160 x 48 pixels, 1 bit each

// Logical keys. The numbers are the bit positions in bytes 3..7 of the input
// report and also the keys in bindings-*.properties ("G0" ... "G39").
inline constexpr int G13_NUM_KEYS = 40;

inline constexpr int G13_KEY_G1 = 0;            // G1..G22 = bits 0..21
inline constexpr int G13_KEY_G22 = 21;
inline constexpr int G13_KEY_BD = 24;           // round button left of L1
inline constexpr int G13_KEY_L1 = 25;           // L1..L4 = bits 25..28 (profile switch)
inline constexpr int G13_KEY_L4 = 28;
inline constexpr int G13_KEY_M1 = 29;           // M1, M2, M3, MR = bits 29..32
inline constexpr int G13_KEY_MR = 32;
inline constexpr int G13_KEY_LEFT = 33;         // thumb button left of the joystick
inline constexpr int G13_KEY_DOWN = 34;         // thumb button below the joystick
inline constexpr int G13_KEY_TOP = 35;          // joystick press

// The joystick directions have no bit of their own; the driver derives them
// from the stick position and uses the otherwise unused numbers 36..39.
inline constexpr int G13_STICK_UP = 36;
inline constexpr int G13_STICK_LEFT = 37;
inline constexpr int G13_STICK_RIGHT = 38;
inline constexpr int G13_STICK_DOWN = 39;

#endif // G13_DRIVER_CONSTANTS_H
