#include "ColorSelector.h"
#include <QVBoxLayout>
#include <QHBoxLayout>

ColorSelector::ColorSelector(QWidget *parent) : QWidget(parent) {
    auto layout = new QVBoxLayout(this);

    // Preview Box
    colorPreview = new QLabel(this);
    colorPreview->setFixedHeight(60);
    colorPreview->setStyleSheet("background-color: black; border: 2px solid grey; border-radius: 5px;");
    layout->addWidget(colorPreview);

    // Sliders
    slRed = createSlider("red");
    slGreen = createSlider("green");
    slBlue = createSlider("blue");

    layout->addWidget(new QLabel("Red"));
    layout->addWidget(slRed);
    layout->addWidget(new QLabel("Green"));
    layout->addWidget(slGreen);
    layout->addWidget(new QLabel("Blue"));
    layout->addWidget(slBlue);
    
    layout->addStretch();
}

QSlider* ColorSelector::createSlider(const char* colorName) {
    QSlider* slider = new QSlider(Qt::Horizontal);
    slider->setRange(0, 255);
    // Use Qt Style sheets to colorize the slider handle/groove
    slider->setStyleSheet(QString("QSlider::handle:horizontal { background-color: %1; border: 1px solid grey; width: 15px; margin: -5px 0; border-radius: 5px; }").arg(colorName));
    connect(slider, &QSlider::valueChanged, this, &ColorSelector::updateColor);
    return slider;
}

void ColorSelector::updateColor() {
    int r = slRed->value();
    int g = slGreen->value();
    int b = slBlue->value();
    
    colorPreview->setStyleSheet(QString("background-color: rgb(%1, %2, %3); border: 2px solid white;").arg(r).arg(g).arg(b));
    sendColorToDriver(r, g, b);
}

void ColorSelector::sendColorToDriver(int r, int g, int b) {
    // TODO: Write "rgb r g b" to the driver pipe
}