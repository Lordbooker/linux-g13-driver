#include "ColorSelector.h"

#include <QVBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFile>
#include <QSaveFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QMap>

// ---------------------------------------------------------------------------
static QMap<QString, QString> readProps(const QString &path)
{
    QMap<QString, QString> map;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return map;
    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#')) continue;
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        map[line.left(eq).trimmed()] = line.mid(eq + 1).trimmed();
    }
    return map;
}

static bool writeProps(const QString &path, const QMap<QString, QString> &map)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    // QSaveFile schreibt in eine temporaere Datei und benennt sie atomar um:
    // der Treiber liest so nie eine leere oder halb geschriebene Datei.
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&f);
    out << "# G13 bindings - edited by g13-gui\n";
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        out << it.key() << "=" << it.value() << "\n";
    out.flush();
    return f.commit();
}

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

    const auto props = readProps(bindingFilePath());
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
    int r = slRed->value(), g = slGreen->value(), b = slBlue->value();
    int brightness = (r * 299 + g * 587 + b * 114) / 1000;
    QString tc = brightness > 128 ? "black" : "white";
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
    auto props = readProps(bindingFilePath());
    props["color"] = QString("%1,%2,%3").arg(r).arg(g).arg(b);
    if (writeProps(bindingFilePath(), props))
        lblStatus->setText(
            tr("Color saved: R%1 G%2 B%3 – the driver reloads it automatically.")
            .arg(r).arg(g).arg(b));
    else
        lblStatus->setText(tr("Error: could not write the file."));
}

QString ColorSelector::bindingFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QString("/g13/bindings-%1.properties").arg(m_profile);
}
