#pragma once
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>

class BindingEditor : public QWidget {
    Q_OBJECT
public:
    explicit BindingEditor(QWidget *parent = nullptr);

public slots:
    void setKey(const QString &keyName);

private:
    QLabel *lblCurrentKey;
    QLineEdit *editBinding;
    QCheckBox *chkRepeat;
};