#pragma once
#include <QWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>

class MacroRecorder : public QWidget {
    Q_OBJECT
public:
    explicit MacroRecorder(QWidget *parent = nullptr);

public slots:
    void setMacroId(int id);

private slots:
    void onLoadClicked();
    void onSaveClicked();

private:
    void loadFromFile();
    void saveToFile();
    QString macroFilePath() const;

    int            m_macroId = 0;

    QSpinBox      *spnId;
    QLineEdit     *editName;
    QPlainTextEdit *editSequence;
    QPushButton   *btnLoad;
    QPushButton   *btnSave;
    QLabel        *lblStatus;
};
