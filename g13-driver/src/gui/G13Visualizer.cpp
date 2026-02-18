#include "G13Visualizer.h"

// NOTE: Alle Koordinaten (x, y, w, h) beziehen sich auf das 500x600-Widget.
// Falls das Hintergrundbild (g13.gif) abweichend skaliert ist, koennen die
// Werte hier angepasst werden, ohne die Logik zu aendern.

G13Visualizer::G13Visualizer(QWidget *parent) : QWidget(parent)
{
    setFixedSize(500, 600);

    setStyleSheet(
        "G13Visualizer {"
        "  border-image: url(:/g13_bg) 0 0 0 0 stretch stretch;"
        "}"
        "QPushButton {"
        "  background-color: rgba(255,255,255,0);"
        "  border: none;"
        "  color: transparent;"
        "  font-size: 9px;"
        "}"
        "QPushButton:hover {"
        "  background-color: rgba(0,255,0,50);"
        "  border: 1px solid lime;"
        "  border-radius: 4px;"
        "  color: white;"
        "}"
        "QPushButton:checked {"
        "  background-color: rgba(0,200,255,120);"
        "  border: 1px solid white;"
        "  color: white;"
        "}"
    );

    // --- Profil-Tasten (oben rechts) ---
    createButton("M1", QRect(310, 28, 40, 24));
    createButton("M2", QRect(355, 28, 40, 24));
    createButton("M3", QRect(400, 28, 40, 24));
    createButton("MR", QRect(445, 28, 40, 24));

    // --- LCD-Auswahl-Tasten (unter LCD) ---
    createButton("L1", QRect(30,  98, 38, 24));
    createButton("L2", QRect(72,  98, 38, 24));
    createButton("L3", QRect(114, 98, 38, 24));
    createButton("L4", QRect(156, 98, 38, 24));
    createButton("BD", QRect(200, 98, 38, 24));

    // --- G-Tasten Reihe 1 ---
    createButton("G1",  QRect(30,  138, 40, 30));
    createButton("G2",  QRect(76,  138, 40, 30));
    createButton("G3",  QRect(122, 138, 40, 30));
    createButton("G4",  QRect(168, 138, 40, 30));
    createButton("G5",  QRect(214, 138, 40, 30));

    // --- G-Tasten Reihe 2 ---
    createButton("G6",  QRect(30,  174, 40, 30));
    createButton("G7",  QRect(76,  174, 40, 30));
    createButton("G8",  QRect(122, 174, 40, 30));
    createButton("G9",  QRect(168, 174, 40, 30));
    createButton("G10", QRect(214, 174, 40, 30));

    // --- G-Tasten Reihe 3 ---
    createButton("G11", QRect(30,  210, 40, 30));
    createButton("G12", QRect(76,  210, 40, 30));
    createButton("G13", QRect(122, 210, 40, 30));
    createButton("G14", QRect(168, 210, 40, 30));
    createButton("G15", QRect(214, 210, 40, 30));

    // --- G-Tasten Reihe 4 ---
    createButton("G16", QRect(30,  246, 40, 30));
    createButton("G17", QRect(76,  246, 40, 30));
    createButton("G18", QRect(122, 246, 40, 30));
    createButton("G19", QRect(168, 246, 40, 30));
    createButton("G20", QRect(214, 246, 40, 30));

    // --- G-Tasten Reihe 5 ---
    createButton("G21", QRect(30,  282, 40, 30));
    createButton("G22", QRect(76,  282, 40, 30));

    // --- Daumen-Tasten (Joystick-Bereich) ---
    createButton("LEFT", QRect(295, 355, 46, 30));
    createButton("DOWN", QRect(345, 400, 46, 30));
    createButton("TOP",  QRect(345, 310, 46, 30));
}

void G13Visualizer::createButton(const QString &name, const QRect &rect)
{
    auto *btn = new QPushButton(name, this);
    btn->setObjectName(name);
    btn->setToolTip(name);
    btn->setGeometry(rect);
    btn->setCheckable(true);
    btn->setAutoExclusive(true);

    connect(btn, &QPushButton::clicked, this, [this, name]() {
        emit keySelected(name);
    });

    m_buttons[name] = btn;
}

void G13Visualizer::setProfile(int /*profile*/)
{
    for (QPushButton *btn : m_buttons)
        btn->setChecked(false);
}
