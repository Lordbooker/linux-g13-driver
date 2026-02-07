#pragma once
#include <QWidget>
#include <QPushButton>
#include <QMap>

class G13Visualizer : public QWidget {
    Q_OBJECT
public:
    explicit G13Visualizer(QWidget *parent = nullptr);

signals:
    // Emitted when a virtual key is clicked
    void keySelected(QString keyName);

private:
    void createButton(const QString &name, const QRect &geometry);
    QMap<QString, QPushButton*> m_buttons;
};