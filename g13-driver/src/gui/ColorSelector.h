#pragma once
#include <QWidget>
#include <QSlider>
#include <QLabel>

class ColorSelector : public QWidget {
    Q_OBJECT
public:
    explicit ColorSelector(QWidget *parent = nullptr);

private slots:
    void updateColor();

private:
    QSlider *createSlider(const char* color);
    QSlider *slRed, *slGreen, *slBlue;
    QLabel *colorPreview;
    void sendColorToDriver(int r, int g, int b);
};