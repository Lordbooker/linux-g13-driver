#pragma once
#include <QWidget>
#include <QLabel>
#include <QVBoxLayout>
#include <QListWidget>
#include <QPushButton>

class MacroRecorder : public QWidget {
    Q_OBJECT
public:
    explicit MacroRecorder(QWidget *parent = nullptr);

private:
    QListWidget *stepList;
    QPushButton *btnRecord;
    QPushButton *btnDelete;
};