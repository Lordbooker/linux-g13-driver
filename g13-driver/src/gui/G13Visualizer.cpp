#include "G13Visualizer.h"

G13Visualizer::G13Visualizer(QWidget *parent) : QWidget(parent) {
    // Set fixed size based on your GIF dimensions
    setFixedSize(500, 600); 

    // Use Stylesheet to set background image and transparent button styles
    setStyleSheet(
        "G13Visualizer { border-image: url(:/g13_bg) 0 0 0 0 stretch stretch; }"
        "QPushButton { background-color: rgba(255, 0, 0, 0); border: none; }" 
        "QPushButton:hover { background-color: rgba(0, 255, 0, 50); border: 1px solid lime; border-radius: 4px; }"
        "QPushButton:checked { background-color: rgba(0, 255, 0, 100); border: 1px solid white; }"
    );

    // TODO: Adjust these coordinates (x, y, width, height) to match your g13.gif perfectly
    // Example for G1 - G4
    createButton("G1", QRect(45, 140, 45, 35));
    createButton("G2", QRect(95, 140, 45, 35));
    createButton("G3", QRect(145, 140, 45, 35));
    createButton("G4", QRect(195, 140, 45, 35));
    
    // Joystick / M-Keys
    createButton("M1", QRect(100, 30, 30, 20));
    // ... Add all other keys here ...
}

void G13Visualizer::createButton(const QString &name, const QRect &rect) {
    QPushButton *btn = new QPushButton(this);
    btn->setObjectName(name);
    btn->setToolTip(name);
    btn->setGeometry(rect);
    btn->setCheckable(true);
    btn->setAutoExclusive(true); // Ensure only one key is active at a time
    
    connect(btn, &QPushButton::clicked, [=]() {
        emit keySelected(name);
    });
    
    m_buttons[name] = btn;
}