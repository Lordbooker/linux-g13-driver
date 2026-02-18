#pragma once
#include <QWidget>
#include <QLabel>
#include <QComboBox>
#include <QSpinBox>
#include <QStackedWidget>
#include <QPushButton>

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

    QString bindingFilePath() const;
    static QString toFileKey(const QString &guiKey);
    static bool isConfigurable(const QString &guiKey);

    int     m_profile    = 0;
    QString m_currentKey;

    QLabel         *lblKey;
    QLabel         *lblProfile;
    QComboBox      *cmbType;    // 0=Tastencode  1=Makro  2=Keine Aktion
    QStackedWidget *stack;
    // Seite 0: Tastencode-Auswahl
    QComboBox      *cmbKeyName;
    // Seite 1: Makro
    QSpinBox       *spnMacroId;
    QSpinBox       *spnRepeats;
    // (Seite 2 ist eine leere QWidget fuer "Keine Aktion")
    QPushButton    *btnSave;
    QLabel         *lblStatus;
};
