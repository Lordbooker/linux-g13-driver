#pragma once
#include <QWidget>
#include <QList>
#include <QPainterPath>
#include <QPixmap>
#include <QSet>
#include <QTransform>

class QFileSystemWatcher;

// Clickable image of the G13.
// - The image is drawn centered without distortion (fixed aspect ratio).
// - The click areas are defined in coordinates of the original image
//   (491 x 710) and scale with it.
// - The selected key, the hovered key and keys bound in the active profile are highlighted.
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
        QPainterPath path;   // in image coordinates
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
    QSet<QString> m_bound;      // key names with a binding in the active profile
    int          m_profile = 0;
    QFileSystemWatcher *m_watcher = nullptr;
};
