#include "G13Visualizer.h"
#include "G13Keys.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPainter>
#include <QStandardPaths>
#include <QTextStream>
#include <QToolTip>

namespace {
// Joystick im Originalbild: Mittelpunkt, Radius des Knopfes (TOP) und des Rings.
constexpr QPointF StickCenter{440.0, 421.0};
constexpr qreal   StickInner = 11.0;
constexpr qreal   StickOuter = 33.0;
}

// ---------------------------------------------------------------------------
// Aufbau
// ---------------------------------------------------------------------------
G13Visualizer::G13Visualizer(QWidget *parent) : QWidget(parent)
{
    m_image.load(":/g13_bg");
    setMouseTracking(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    // Alle Masse in Pixeln des Originalbildes resources/g13.gif (491 x 710).

    // Tasten unter dem LCD
    addEllipse("BD", {120, 120}, 11);
    addRect("L1", 161, 115, 37, 13);
    addRect("L2", 205, 115, 37, 13);
    addRect("L3", 249, 115, 37, 13);
    addRect("L4", 293, 115, 36, 13);

    // Profil-Reihe
    addRect("M1", 101, 146, 68, 18);
    addRect("M2", 173, 146, 70, 18);
    addRect("M3", 249, 146, 71, 18);
    addRect("MR", 324, 146, 66, 18);

    // G-Tasten, Reihe 1 (7 Tasten)
    addRect("G1",  54, 187, 53, 34);
    addRect("G2", 116, 187, 44, 34);
    addRect("G3", 170, 187, 43, 34);
    addRect("G4", 224, 187, 42, 34);
    addRect("G5", 277, 187, 42, 34);
    addRect("G6", 330, 186, 43, 34);
    addRect("G7", 383, 184, 52, 34);

    // Reihe 2 (7 Tasten)
    addRect("G8",   50, 232, 56, 38);
    addRect("G9",  112, 237, 47, 35);
    addRect("G10", 167, 240, 48, 35);
    addRect("G11", 222, 241, 48, 35);
    addRect("G12", 277, 240, 47, 35);
    addRect("G13", 331, 236, 49, 35);
    addRect("G14", 386, 230, 54, 37);

    // Reihe 3 (5 Tasten)
    addRect("G15",  87, 289, 65, 36);
    addRect("G16", 165, 292, 45, 34);
    addRect("G17", 222, 293, 45, 34);
    addRect("G18", 279, 292, 46, 34);
    addRect("G19", 335, 288, 65, 36);

    // Reihe 4 (3 Tasten)
    addRect("G20", 127, 342, 80, 36);
    addRect("G21", 220, 345, 50, 34);
    addRect("G22", 280, 342, 77, 36);

    // Daumentasten und Joystick
    addRect("LEFT", 364, 392, 28, 76);
    addRect("DOWN", 398, 470, 80, 30);
    addEllipse("TOP", StickCenter, StickInner);
    addStickSector("STICK_RIGHT", -45);
    addStickSector("STICK_UP",     45);
    addStickSector("STICK_LEFT",  135);
    addStickSector("STICK_DOWN",  225);

    // Belegungen neu einlesen, wenn sich Dateien aendern (GUI oder Editor).
    const QString dir = QFileInfo(bindingFilePath()).absolutePath();
    QDir().mkpath(dir);
    m_watcher = new QFileSystemWatcher(this);
    m_watcher->addPath(dir);
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, this, &G13Visualizer::reloadBindings);
    connect(m_watcher, &QFileSystemWatcher::fileChanged,      this, &G13Visualizer::reloadBindings);

    reloadBindings();
}

void G13Visualizer::addRect(const QString &name, qreal x, qreal y, qreal w, qreal h)
{
    QPainterPath path;
    path.addRoundedRect(QRectF(x, y, w, h), 4, 4);
    m_areas.append({name, path});
}

void G13Visualizer::addEllipse(const QString &name, QPointF center, qreal radius)
{
    QPainterPath path;
    path.addEllipse(center, radius, radius);
    m_areas.append({name, path});
}

// Viertelkreis-Ring um den Joystick; startAngle in Grad, gegen den
// Uhrzeigersinn ab 3 Uhr (Qt-Konvention).
void G13Visualizer::addStickSector(const QString &name, qreal startAngle)
{
    const QRectF outer(StickCenter.x() - StickOuter, StickCenter.y() - StickOuter,
                       2 * StickOuter, 2 * StickOuter);
    const QRectF inner(StickCenter.x() - StickInner, StickCenter.y() - StickInner,
                       2 * StickInner, 2 * StickInner);
    QPainterPath path;
    path.arcMoveTo(outer, startAngle);
    path.arcTo(outer, startAngle, 90);
    path.arcTo(inner, startAngle + 90, -90);
    path.closeSubpath();
    m_areas.append({name, path});
}

QSize G13Visualizer::sizeHint() const
{
    return m_image.isNull() ? QSize(491, 710) : m_image.size();
}

// ---------------------------------------------------------------------------
// Geometrie
// ---------------------------------------------------------------------------
QTransform G13Visualizer::imageToWidget() const
{
    if (m_image.isNull())
        return {};
    const qreal scale = qMin(width()  / qreal(m_image.width()),
                             height() / qreal(m_image.height()));
    const qreal dx = (width()  - m_image.width()  * scale) / 2.0;
    const qreal dy = (height() - m_image.height() * scale) / 2.0;
    return QTransform(scale, 0, 0, scale, dx, dy);
}

QString G13Visualizer::keyAt(const QPointF &widgetPos) const
{
    bool invertible = false;
    const QPointF p = imageToWidget().inverted(&invertible).map(widgetPos);
    if (!invertible)
        return {};
    for (const Area &area : m_areas) {
        if (area.path.contains(p))
            return area.name;
    }
    return {};
}

// ---------------------------------------------------------------------------
// Zeichnen
// ---------------------------------------------------------------------------
void G13Visualizer::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    if (m_image.isNull()) {
        p.drawText(rect(), Qt::AlignCenter, "G13-Bild konnte nicht geladen werden.");
        return;
    }

    const QTransform t = imageToWidget();
    p.setTransform(t);
    p.drawPixmap(0, 0, m_image);

    const qreal px = 1.0 / t.m11();    // 1 Bildschirmpixel in Bildkoordinaten

    for (const Area &area : m_areas) {
        const bool configurable = G13Keys::isConfigurable(area.name);
        const bool selected     = area.name == m_selected;
        const bool hover        = area.name == m_hover;

        // Fuellung
        if (selected)
            p.fillPath(area.path, QColor(0, 170, 255, 110));
        else if (hover)
            p.fillPath(area.path, QColor(0, 200, 255, 60));

        // Umrandung
        QPen pen;
        pen.setWidthF((selected ? 2.0 : 1.0) * px);
        if (selected)
            pen.setColor(Qt::white);
        else if (hover)
            pen.setColor(QColor(0, 200, 255));
        else if (configurable)
            pen.setColor(QColor(255, 255, 255, 80));
        else {
            pen.setColor(QColor(255, 255, 255, 50));
            pen.setStyle(Qt::DashLine);
        }
        p.setPen(pen);
        p.setBrush(Qt::NoBrush);
        p.drawPath(area.path);

        // Belegt-Markierung: gruener Punkt oben rechts
        if (m_bound.contains(area.name)) {
            const QRectF r = area.path.boundingRect();
            const qreal radius = 2.5;
            // Joystick-Segmente: Mitte des Segments; sonst Ecke oben rechts.
            const bool round = area.name.startsWith("STICK_") || area.name == "TOP";
            const QPointF dot = round ? r.center()
                                      : QPointF(r.right() - radius - 2, r.top() + radius + 2);
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(80, 220, 120));
            p.drawEllipse(dot, radius, radius);
        }
    }
}

// ---------------------------------------------------------------------------
// Maus
// ---------------------------------------------------------------------------
void G13Visualizer::mouseMoveEvent(QMouseEvent *event)
{
    const QString key = keyAt(event->position());
    if (key != m_hover) {
        m_hover = key;
        setCursor(key.isEmpty() ? Qt::ArrowCursor : Qt::PointingHandCursor);
        update();
    }
}

void G13Visualizer::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton)
        return;
    const QString key = keyAt(event->position());
    if (key.isEmpty())
        return;
    m_selected = key;
    update();
    emit keySelected(key);
}

void G13Visualizer::leaveEvent(QEvent *)
{
    if (!m_hover.isEmpty()) {
        m_hover.clear();
        update();
    }
}

bool G13Visualizer::event(QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        auto *help = static_cast<QHelpEvent *>(event);
        const QString key = keyAt(help->pos());
        if (key.isEmpty()) {
            QToolTip::hideText();
        } else {
            QString text = G13Keys::displayName(key);
            if (!G13Keys::isConfigurable(key))
                text += " – Profil-/LCD-Taste, nicht belegbar";
            else if (m_bound.contains(key))
                text += " – belegt";
            QToolTip::showText(help->globalPos(), text, this);
        }
        return true;
    }
    return QWidget::event(event);
}

// ---------------------------------------------------------------------------
// Profil und Belegungen
// ---------------------------------------------------------------------------
void G13Visualizer::setProfile(int profile)
{
    m_profile = profile;
    reloadBindings();
}

QString G13Visualizer::bindingFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QString("/g13/bindings-%1.properties").arg(m_profile);
}

void G13Visualizer::reloadBindings()
{
    const QString path = bindingFilePath();

    // Datei (neu) beobachten: Editoren ersetzen Dateien oft statt sie zu aendern.
    if (QFile::exists(path) && !m_watcher->files().contains(path)) {
        if (!m_watcher->files().isEmpty())
            m_watcher->removePaths(m_watcher->files());
        m_watcher->addPath(path);
    }

    QSet<QString> boundFileKeys;
    QFile f(path);
    if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream in(&f);
        while (!in.atEnd()) {
            const QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith('#'))
                continue;
            const int eq = line.indexOf('=');
            if (eq < 0)
                continue;
            const QString value = line.mid(eq + 1).trimmed();
            if (value.startsWith("p,") || value.startsWith("m,"))
                boundFileKeys.insert(line.left(eq).trimmed());
        }
    }

    m_bound.clear();
    for (const QString &name : G13Keys::all()) {
        if (G13Keys::isConfigurable(name) && boundFileKeys.contains(G13Keys::fileKey(name)))
            m_bound.insert(name);
    }
    update();
}
