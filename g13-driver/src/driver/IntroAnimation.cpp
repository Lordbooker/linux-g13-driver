#include "IntroAnimation.h"
#include "IntroImages.h"

#include <algorithm>
#include <cmath>

static_assert(IntroAnimation::Width == INTRO_WIDTH && IntroAnimation::Height == INTRO_HEIGHT,
              "Bitmaps passen nicht zur Displaygroesse");

namespace {

// Zeitplan in Sekunden (identisch mit der Vorschau)
constexpr double THold    = 1.6;   // Firmware-Logo stehen lassen
constexpr double TRise    = 2.4;   // Flammenwand steigt
constexpr double TCover   = 0.6;   // Display ganz bedeckt, Logo wird getauscht
constexpr double TRecede  = 2.6;   // Wand zieht sich zurueck
constexpr double TSettle  = 1.6;   // Glut verlischt
constexpr double TRiseEnd   = THold + TRise;
constexpr double TCoverEnd  = TRiseEnd + TCover;
constexpr double TRecedeEnd = TCoverEnd + TRecede;
constexpr double TTotal     = TRecedeEnd + TSettle;

constexpr uint32_t Seed = 1313;

// 4x4-Bayer-Matrix fuer das Raster
constexpr double bayer(int x, int y) {
    constexpr int m[16] = {0, 8, 2, 10, 12, 4, 14, 6, 3, 11, 1, 9, 15, 7, 13, 5};
    return (m[(y & 3) * 4 + (x & 3)] + 0.5) / 16.0;
}

double ease(double t) {
    return t < 0.5 ? 2 * t * t : 1 - std::pow(-2 * t + 2, 2) / 2;
}

bool bitmapPixel(const uint8_t (&bitmap)[INTRO_HEIGHT][INTRO_ROW_BYTES], int x, int y) {
    return (bitmap[y][x >> 3] >> (7 - (x & 7))) & 1;
}

}

IntroAnimation::IntroAnimation()
    : rngState_(Seed),
      heat_(Width * FieldHeight, 0),
      burned_(Width * Height, 0),
      top_(Width, 0.0),
      cool_(Width, 0.0)
{
}

int IntroAnimation::frameCount() {
    return static_cast<int>(std::lround(TTotal * Fps));
}

bool IntroAnimation::firmwarePixel(int x, int y) { return bitmapPixel(intro_firmware_logo, x, y); }
bool IntroAnimation::finalPixel(int x, int y)    { return bitmapPixel(intro_final_logo, x, y); }

// mulberry32: liefert dieselbe Folge wie die Vorschau (JavaScript)
double IntroAnimation::random() {
    rngState_ += 0x6D2B79F5u;
    uint32_t t = (rngState_ ^ (rngState_ >> 15)) * (1u | rngState_);
    t = (t + ((t ^ (t >> 7)) * (61u | t))) ^ t;
    return static_cast<double>(t ^ (t >> 14)) / 4294967296.0;
}

// Hoehe des heissen Wandkoerpers in Zeilen von unten; <= 0: keine Wand.
double IntroAnimation::wallLevel(double t) const {
    const double full = FieldHeight + 6;
    if (t < THold)      return -1;
    if (t < TRiseEnd)   return ease((t - THold) / TRise) * full;
    if (t < TCoverEnd)  return full;
    if (t < TRecedeEnd) return (1 - ease((t - TCoverEnd) / TRecede)) * (full + 3) - 3;
    return -1;
}

void IntroAnimation::step() {
    const double t     = static_cast<double>(frame_) / Fps;
    const double level = wallLevel(t);
    const double ph    = frame_ * 0.3;

    // Wandkoerper: unterhalb der (leicht wogenden) Oberkante maximal heiss
    for (int x = 0; x < Width; ++x) {
        if (level > 0) {
            const double wave = std::sin(x * 0.09 + ph) * 1.6 + std::sin(x * 0.23 - ph * 1.4) * 1.0;
            top_[x] = FieldHeight - level + wave;
        } else {
            top_[x] = FieldHeight + 99;
        }
        for (int y = std::max(0, static_cast<int>(std::ceil(top_[x]))); y < FieldHeight; ++y)
            heat_[y * Width + x] = MaxHeat - static_cast<int>(random() * 6);
        if (top_[x] >= FieldHeight) {
            // Die unterste Zeile wird nie von unten nachgespeist: ohne Wand abkuehlen lassen
            int &bottom = heat_[(FieldHeight - 1) * Width + x];
            bottom = bottom > 6 ? bottom - 6 : 0;
        }
    }

    // Abkuehlung je Spalte: wandernde Muster ergeben hohe und niedrige Zungen
    for (int x = 0; x < Width; ++x) {
        const double n = 0.5 + 0.5 * (std::sin(x * 0.42 + ph * 1.9) * 0.55
                                    + std::sin(x * 0.97 - ph * 2.7) * 0.3
                                    + std::sin(x * 0.17 + ph * 0.8) * 0.15);
        cool_[x] = 2 + n * n * 11;
    }

    // Hitze nach oben ausbreiten (mit seitlichem Versatz und zufaelligem Abkuehlen).
    // Drei Teilschritte pro Bild, damit die Zungen schneller wachsen als die Wand steigt.
    for (int sub = 0; sub < 3; ++sub) {
        for (int y = 1; y < FieldHeight; ++y) {
            for (int x = 0; x < Width; ++x) {
                const int p = heat_[y * Width + x];
                const int r = static_cast<int>(random() * 3);
                const int dx = std::clamp(x - r + 1, 0, Width - 1);
                if (y - 1 >= top_[dx])
                    continue;                                   // Ziel liegt im Wandkoerper
                int &dst = heat_[(y - 1) * Width + dx];
                if (p == 0) {
                    dst = 0;
                    continue;
                }
                const int c = static_cast<int>(random() * cool_[dx]);
                dst = p > c ? p - c : 0;
            }
        }
    }

    // Das Firmware-Logo verbrennt, wo die Flammen heiss genug sind
    for (int i = 0; i < Width * Height; ++i) {
        if (heat_[i] > MaxHeat * 0.35)
            burned_[i] = 1;
    }
    if (!swapped_ && t >= TRiseEnd + TCover * 0.5)
        swapped_ = true;

    ++frame_;
}

// Helle Flammen auf dunklem Grund (Darstellung wie die Firmware)
void IntroAnimation::compose(uint8_t *out) const {
    for (int y = 0; y < Height; ++y) {
        for (int x = 0; x < Width; ++x) {
            const int i = y * Width + x;
            const double h = static_cast<double>(heat_[i]) / MaxHeat;
            if (h > 0.1) {
                const double density = std::min(0.97, std::max(0.0, (h - 0.2) / 0.62));
                out[i] = density > bayer(x, y) ? 0 : 1;
            } else if (swapped_) {
                out[i] = finalPixel(x, y) ? 1 : 0;
            } else {
                out[i] = (burned_[i] || firmwarePixel(x, y)) ? 1 : 0;
            }
        }
    }
}
