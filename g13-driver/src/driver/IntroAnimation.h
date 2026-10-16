#ifndef INTRO_ANIMATION_H
#define INTRO_ANIMATION_H

#include <cstdint>
#include <vector>

/**
 * @brief Startanimation fuer das G13-LCD (160x43, 1 Bit).
 *
 * Eine Flammenwand steigt von unten auf, verbrennt das Firmware-Logo, bedeckt
 * kurz das ganze Display und zieht sich wieder zurueck. Dabei wird das Endbild
 * (Tux und "G13") freigegeben. Die Flammen entstehen aus einem Hitzefeld nach
 * dem Prinzip des klassischen Doom-Feuers und werden mit einem 4x4-Bayer-Raster
 * auf 1 Bit gebracht.
 *
 * Der Zufall ist fest geseedet (mulberry32), die Animation ist daher bei jedem
 * Lauf gleich und Bild fuer Bild identisch mit der Vorschau "G13 Flammen-Intro".
 *
 * Die Klasse rechnet nur; das Senden an das Geraet uebernimmt G13.
 */
class IntroAnimation {
public:
    static constexpr int Width  = 160;
    static constexpr int Height = 43;
    static constexpr int Fps    = 20;

    IntroAnimation();

    /// Anzahl der Animationsbilder (ohne das abschliessende Endbild).
    static int frameCount();

    /// Naechstes Bild berechnen.
    void step();
    bool finished() const { return frame_ >= frameCount(); }
    int  frame() const { return frame_; }

    /// Aktuelles Bild: out[y * Width + x] = 1 bedeutet Pixel an (dunkel).
    void compose(uint8_t *out) const;

    /// Pixel der festen Bilder (1 = an).
    static bool firmwarePixel(int x, int y);
    static bool finalPixel(int x, int y);

private:
    static constexpr int FieldHeight = Height + 4;   // unsichtbare Zeilen unter dem Display
    static constexpr int MaxHeat     = 48;

    double random();
    double wallLevel(double t) const;

    uint32_t rngState_;
    std::vector<int>     heat_;     // Width * FieldHeight
    std::vector<uint8_t> burned_;   // Width * Height: hier ist das Firmware-Logo verbrannt
    std::vector<double>  top_;      // Oberkante des Wandkoerpers je Spalte
    std::vector<double>  cool_;     // Abkuehlung je Spalte (ergibt die Flammenzungen)
    bool swapped_ = false;          // Endbild statt Firmware-Logo als Grund
    int  frame_   = 0;
};

#endif // INTRO_ANIMATION_H
