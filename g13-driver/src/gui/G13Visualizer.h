#pragma once
#include <QWidget>
#include <QPushButton>
#include <QMap>

class G13Visualizer : public QWidget {
    Q_OBJECT
public:
    explicit G13Visualizer(QWidget *parent = nullptr);

public slots:
    void setProfile(int profile);

signals:
    void keySelected(QString keyName);

private:
    void createButton(const QString &name, const QRect &rect);
    QMap<QString, QPushButton*> m_buttons;
};