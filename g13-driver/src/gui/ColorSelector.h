#pragma once
#include <QWidget>
#include <QSlider>
#include <QLabel>
#include <QPushButton>

class ColorSelector : public QWidget {
    Q_OBJECT
public:
    explicit ColorSelector(QWidget *parent = nullptr);

public slots:
    void setProfile(int profile);

private slots:
    void updatePreview();
    void onApplyClicked();

private:
    QSlider    *createSlider();
    void        sendColorToDriver(int r, int g, int b);
    QString     bindingFilePath() const;

    QSlider    *slRed, *slGreen, *slBlue;
    QLabel     *colorPreview;
    QPushButton *btnApply;
    QLabel     *lblStatus;
    int         m_profile = 0;
};
