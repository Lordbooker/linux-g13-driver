#pragma once
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QStackedWidget>
#include <QPushButton>

class KeyCaptureEdit;

class BindingEditor : public QWidget {
    Q_OBJECT
public:
    explicit BindingEditor(QWidget *parent = nullptr);

public slots:
    void setKey(const QString &keyName);
    void setProfile(int profile);

private slots:
    void onTypeChanged(int index);
    void onSaveClicked();

private:
    void loadFromFile();
    void saveToFile();


    int     m_profile    = 0;
    QString m_currentKey;

    QLabel         *lblKey;
    QLabel         *lblProfile;
    QComboBox      *cmbType;    // 0=key code  1=macro  2=no action
    QStackedWidget *stack;
    // Page 0: key code (input field: press a key)
    KeyCaptureEdit *editKey;
    // Page 1: macro
    QSpinBox       *spnMacroId;
    QComboBox      *cmbRepeatMode;   // 0=once  1=while held  2=count
    QSpinBox       *spnRepeats;      // count for mode 2 (>= 2)
    // (page 2 is an empty QWidget for "no action")
    QPushButton    *btnSave;
    QLabel         *lblStatus;
};
