#include "MacroRecorder.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

MacroRecorder::MacroRecorder(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);

    // Makro-Auswahl
    auto *idBox    = new QGroupBox("Makro auswählen", this);
    auto *idLayout = new QFormLayout(idBox);
    spnId = new QSpinBox(this);
    spnId->setRange(0, 199);
    btnLoad = new QPushButton("Laden", this);
    auto *idRow = new QHBoxLayout();
    idRow->addWidget(spnId, 1);
    idRow->addWidget(btnLoad);
    idLayout->addRow("Makro-ID:", idRow);
    root->addWidget(idBox);

    // Makro-Details
    auto *detailBox    = new QGroupBox("Makro-Details", this);
    auto *detailLayout = new QFormLayout(detailBox);
    editName = new QLineEdit(this);
    editName->setPlaceholderText("z.B. Snipe, Combo …");
    detailLayout->addRow("Name:", editName);

    editSequence = new QPlainTextEdit(this);
    editSequence->setPlaceholderText(
        "Format: k.<code>:<delay_ms>,k.<code>:<delay_ms>, …\n\n"
        "Beispiel – W drücken (17) dann loslassen:\n"
        "  k.17:0,k.17:100\n\n"
        "code  = Linux-Input-Keycode\n"
        "delay = Wartezeit in ms nach dem Event"
    );
    editSequence->setMinimumHeight(130);
    detailLayout->addRow("Sequenz:", editSequence);
    root->addWidget(detailBox);

    // Buttons
    auto *btnRow = new QHBoxLayout();
    btnSave = new QPushButton("Speichern", this);
    btnRow->addStretch();
    btnRow->addWidget(btnSave);
    root->addLayout(btnRow);

    lblStatus = new QLabel("", this);
    lblStatus->setWordWrap(true);
    root->addWidget(lblStatus);
    root->addStretch();

    connect(btnLoad, &QPushButton::clicked, this, &MacroRecorder::onLoadClicked);
    connect(btnSave, &QPushButton::clicked, this, &MacroRecorder::onSaveClicked);
}

void MacroRecorder::setMacroId(int id)
{
    spnId->setValue(id);
    m_macroId = id;
    loadFromFile();
}

void MacroRecorder::onLoadClicked()
{
    m_macroId = spnId->value();
    loadFromFile();
}

void MacroRecorder::onSaveClicked()
{
    m_macroId = spnId->value();
    saveToFile();
}

QString MacroRecorder::macroFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QString("/g13/macro-%1.properties").arg(m_macroId);
}

void MacroRecorder::loadFromFile()
{
    lblStatus->clear();
    QFile f(macroFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        editName->clear();
        editSequence->clear();
        lblStatus->setText(QString("Makro %1 existiert noch nicht.").arg(m_macroId));
        return;
    }
    QTextStream in(&f);
    QString name, seq;
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.startsWith('#') || line.isEmpty()) continue;
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        QString key = line.left(eq).trimmed();
        QString val = line.mid(eq + 1).trimmed();
        if (key == "name")     name = val;
        if (key == "sequence") seq  = val;
    }
    editName->setText(name);
    editSequence->setPlainText(seq);
    lblStatus->setText(QString("Makro %1 geladen.").arg(m_macroId));
}

void MacroRecorder::saveToFile()
{
    QDir().mkpath(QFileInfo(macroFilePath()).absolutePath());
    QFile f(macroFilePath());
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        lblStatus->setText("Fehler: Datei konnte nicht geschrieben werden.");
        return;
    }
    QTextStream out(&f);
    out << "# G13 Makro – bearbeitet von g13-gui\n";
    out << "name="     << editName->text().trimmed()            << "\n";
    out << "sequence=" << editSequence->toPlainText().trimmed() << "\n";
    lblStatus->setText(QString("Makro %1 gespeichert.").arg(m_macroId));
}
