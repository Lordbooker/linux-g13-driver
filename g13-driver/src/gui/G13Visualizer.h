#pragma once
#include <QWidget>
#include <QList>
#include <QPainterPath>
#include <QPixmap>
#include <QSet>
#include <QTransform>

class QFileSystemWatcher;

// Klickbares Bild des G13.
// - Das Bild wird unverzerrt (festes Seitenverhaeltnis) zentriert gezeichnet.
// - Die Klickflaechen sind in Koordinaten des Originalbildes (491 x 710)
//   definiert und skalieren mit.
// - Gewaehlte Taste, Hover und im aktiven Profil belegte Tasten werden markiert.
class G13Visualizer : public QWidget {
    Q_OBJECT
public:
    explicit G13Visualizer(QWidget *parent = nullptr);

    QSize sizeHint() const override;

public slots:
    void setProfile(int profile);

signals:
    void keySelected(QString keyName);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void leaveEvent(QEvent *event) override;
    bool event(QEvent *event) override;

private:
    struct Area {
        QString      name;
        QPainterPath path;   // in Bildkoordinaten
    };

    void addRect(const QString &name, qreal x, qreal y, qreal w, qreal h);
    void addEllipse(const QString &name, QPointF center, qreal radius);
    void addStickSector(const QString &name, qreal startAngle);

    QTransform imageToWidget() const;
    QString keyAt(const QPointF &widgetPos) const;
    QString bindingFilePath() const;
    void reloadBindings();

    QPixmap      m_image;
    QList<Area>  m_areas;
    QString      m_hover;
    QString      m_selected;
    QSet<QString> m_bound;      // Tastennamen mit Belegung im aktiven Profil
    int          m_profile = 0;
    QFileSystemWatcher *m_watcher = nullptr;
};
