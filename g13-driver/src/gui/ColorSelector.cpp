#include "ColorSelector.h"
#include "PropertiesFile.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>

// ---------------------------------------------------------------------------
ColorSelector::ColorSelector(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);

    colorPreview = new QLabel(this);
    colorPreview->setFixedHeight(70);
    colorPreview->setAlignment(Qt::AlignCenter);
    colorPreview->setStyleSheet(
        "background-color:rgb(0,0,0);"
        "border:2px solid #555;"
        "border-radius:6px;"
        "color:white;");
    colorPreview->setText("R:0  G:0  B:0");
    root->addWidget(colorPreview);

    auto *sliderBox    = new QGroupBox(tr("RGB color"), this);
    auto *sliderLayout = new QFormLayout(sliderBox);
    slRed   = createSlider();
    slGreen = createSlider();
    slBlue  = createSlider();
    sliderLayout->addRow(tr("Red:"),   slRed);
    sliderLayout->addRow(tr("Green:"), slGreen);
    sliderLayout->addRow(tr("Blue:"),  slBlue);
    root->addWidget(sliderBox);

    btnApply = new QPushButton(tr("Apply color"), this);
    root->addWidget(btnApply);

    lblStatus = new QLabel("", this);
    lblStatus->setWordWrap(true);
    root->addWidget(lblStatus);
    root->addStretch();

    connect(slRed,    &QSlider::valueChanged, this, &ColorSelector::updatePreview);
    connect(slGreen,  &QSlider::valueChanged, this, &ColorSelector::updatePreview);
    connect(slBlue,   &QSlider::valueChanged, this, &ColorSelector::updatePreview);
    connect(btnApply, &QPushButton::clicked,  this, &ColorSelector::onApplyClicked);
}

QSlider *ColorSelector::createSlider()
{
    auto *s = new QSlider(Qt::Horizontal, this);
    s->setRange(0, 255);
    return s;
}

void ColorSelector::setProfile(int profile)
{
    m_profile = profile;
    lblStatus->clear();

    const auto props = PropertiesFile::read(PropertiesFile::bindingsPath(m_profile));
    if (props.contains("color")) {
        const QStringList parts = props["color"].split(',');
        if (parts.size() == 3) {
            slRed->blockSignals(true);
            slGreen->blockSignals(true);
            slBlue->blockSignals(true);
            slRed->setValue(parts[0].trimmed().toInt());
            slGreen->setValue(parts[1].trimmed().toInt());
            slBlue->setValue(parts[2].trimmed().toInt());
            slRed->blockSignals(false);
            slGreen->blockSignals(false);
            slBlue->blockSignals(false);
            updatePreview();
        }
    }
}

void ColorSelector::updatePreview()
{
    const int r = slRed->value();
    const int g = slGreen->value();
    const int b = slBlue->value();
    const int brightness = ((r * 299) + (g * 587) + (b * 114)) / 1000;
    const QString tc = brightness > 128 ? "black" : "white";
    colorPreview->setStyleSheet(
        QString("background-color:rgb(%1,%2,%3);"
                "border:2px solid #888;"
                "border-radius:6px;"
                "color:%4;").arg(r).arg(g).arg(b).arg(tc));
    colorPreview->setText(QString("R:%1  G:%2  B:%3").arg(r).arg(g).arg(b));
}

void ColorSelector::onApplyClicked()
{
    sendColorToDriver(slRed->value(), slGreen->value(), slBlue->value());
}

void ColorSelector::sendColorToDriver(int r, int g, int b)
{
    const QString color = QString("%1,%2,%3").arg(r).arg(g).arg(b);
    if (PropertiesFile::update(PropertiesFile::bindingsPath(m_profile), {{"color", color}},
                               "# G13 bindings - edited by g13-gui"))
        lblStatus->setText(
            tr("Color saved: R%1 G%2 B%3 – the driver reloads it automatically.")
            .arg(r).arg(g).arg(b));
    else
        lblStatus->setText(tr("Error: could not write the file."));
}
