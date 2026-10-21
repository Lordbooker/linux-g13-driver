#ifndef INTRO_ANIMATION_H
#define INTRO_ANIMATION_H

#include <cstdint>
#include <vector>

/**
 * @brief Start animation for the G13 LCD (160x43, 1 bit).
 *
 * A flame wall rises from the bottom, burns the firmware logo, briefly covers
 * the whole display and recedes again, revealing the final image (Tux and
 * "G13"). The flames come from a heat field following the principle of the
 * classic Doom fire and are reduced to 1 bit with a 4x4 Bayer dither.
 *
 * The random generator has a fixed seed (mulberry32), so the animation is the
 * same on every run and identical frame by frame to the preview "G13 Flammen-Intro".
 *
 * The class only computes frames; sending them to the device is done by G13.
 */
class IntroAnimation {
public:
    static constexpr int Width  = 160;
    static constexpr int Height = 43;
    static constexpr int Fps    = 20;

    IntroAnimation();

    /// Number of animation frames (without the final image).
    static int frameCount();

    /// Compute the next frame.
    void step();
    bool finished() const { return frame_ >= frameCount(); }
    int  frame() const { return frame_; }

    /// Current frame: out[y * Width + x] = 1 means pixel on (dark).
    void compose(uint8_t *out) const;

    /// Pixels of the fixed images (1 = on).
    static bool firmwarePixel(int x, int y);
    static bool finalPixel(int x, int y);

private:
    static constexpr int FieldHeight = Height + 4;   // invisible rows below the display
    static constexpr int MaxHeat     = 48;

    double random();
    double wallLevel(double t) const;

    uint32_t rngState_;
    std::vector<int>     heat_;     // Width * FieldHeight
    std::vector<uint8_t> burned_;   // Width * Height: the firmware logo has burned here
    std::vector<double>  top_;      // top edge of the wall body per column
    std::vector<double>  cool_;     // cooling per column (creates the flame tongues)
    bool swapped_ = false;          // final image instead of firmware logo as background
    int  frame_   = 0;
};

#endif // INTRO_ANIMATION_H
