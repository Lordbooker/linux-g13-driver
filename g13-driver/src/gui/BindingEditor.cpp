#include "BindingEditor.h"
#include "KeyNames.h"
#include "G13Keys.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QGroupBox>
#include <QFile>
#include <QTextStream>
#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>
#include <QSet>
#include <QMap>

// ---------------------------------------------------------------------------
// Datei-Hilfsfunktionen
// ---------------------------------------------------------------------------
static QMap<QString, QString> readProps(const QString &path)
{
    QMap<QString, QString> map;
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return map;
    QTextStream in(&f);
    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        int eq = line.indexOf('=');
        if (eq < 0) continue;
        map[line.left(eq).trimmed()] = line.mid(eq + 1).trimmed();
    }
    return map;
}

static bool writeProps(const QString &path, const QMap<QString, QString> &map)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream out(&f);
    out << "# G13 Bindings – bearbeitet von g13-gui\n";
    for (auto it = map.constBegin(); it != map.constEnd(); ++it)
        out << it.key() << "=" << it.value() << "\n";
    return true;
}

// ---------------------------------------------------------------------------
// BindingEditor
// ---------------------------------------------------------------------------
BindingEditor::BindingEditor(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);

    // Kopfzeile
    auto *infoBox    = new QGroupBox("Aktuelle Auswahl", this);
    auto *infoLayout = new QFormLayout(infoBox);
    lblProfile = new QLabel("M1", this);
    lblKey     = new QLabel("–", this);
    QFont f = lblKey->font();
    f.setBold(true);
    f.setPointSize(13);
    lblKey->setFont(f);
    infoLayout->addRow("Profil:", lblProfile);
    infoLayout->addRow("Taste:",  lblKey);
    root->addWidget(infoBox);

    // Typ-Auswahl
    auto *typeBox    = new QGroupBox("Aktionstyp", this);
    auto *typeLayout = new QVBoxLayout(typeBox);
    cmbType = new QComboBox(this);
    cmbType->addItem("Tastencode (Pass-Through)");
    cmbType->addItem("Makro");
    cmbType->addItem("Keine Aktion");
    typeLayout->addWidget(cmbType);
    root->addWidget(typeBox);

    // Stack: 3 Seiten
    stack = new QStackedWidget(this);

    // Seite 0 – Tastencode
    auto *pageKey       = new QWidget();
    auto *pageKeyLayout = new QFormLayout(pageKey);
    cmbKeyName = new QComboBox(pageKey);
    for (const auto &[name, code] : KeyNames::all())
        cmbKeyName->addItem(QString("%1  (%2)").arg(name).arg(code), code);
    pageKeyLayout->addRow("Taste:", cmbKeyName);
    stack->addWidget(pageKey);

    // Seite 1 – Makro
    auto *pageMacro       = new QWidget();
    auto *pageMacroLayout = new QFormLayout(pageMacro);
    spnMacroId = new QSpinBox(pageMacro);
    spnMacroId->setRange(0, 199);
    spnRepeats = new QSpinBox(pageMacro);
    spnRepeats->setRange(1, 100);
    spnRepeats->setValue(1);
    pageMacroLayout->addRow("Makro-ID:",       spnMacroId);
    pageMacroLayout->addRow("Wiederholungen:", spnRepeats);
    stack->addWidget(pageMacro);

    // Seite 2 – Keine Aktion (Platzhalter)
    stack->addWidget(new QWidget());

    root->addWidget(stack);

    // Speichern
    btnSave = new QPushButton("Speichern", this);
    btnSave->setEnabled(false);
    root->addWidget(btnSave);

    lblStatus = new QLabel("", this);
    lblStatus->setWordWrap(true);
    root->addWidget(lblStatus);
    root->addStretch();

    connect(cmbType,  &QComboBox::currentIndexChanged, this, &BindingEditor::onTypeChanged);
    connect(btnSave,  &QPushButton::clicked,            this, &BindingEditor::onSaveClicked);
}

// ---------------------------------------------------------------------------
void BindingEditor::setProfile(int profile)
{
    m_profile = profile;
    const QString names[] = {"M1","M2","M3","M4"};
    lblProfile->setText(names[profile]);
    if (!m_currentKey.isEmpty())
        loadFromFile();
}

void BindingEditor::setKey(const QString &keyName)
{
    m_currentKey = keyName;
    lblKey->setText(G13Keys::displayName(keyName));

    if (!G13Keys::isConfigurable(keyName)) {
        cmbType->setEnabled(false);
        stack->setEnabled(false);
        btnSave->setEnabled(false);
        lblStatus->setText("Diese Taste ist nicht frei belegbar (Profil-/LCD-Taste).");
        return;
    }

    cmbType->setEnabled(true);
    stack->setEnabled(true);
    btnSave->setEnabled(true);
    lblStatus->clear();
    loadFromFile();
}

void BindingEditor::onTypeChanged(int index)
{
    stack->setCurrentIndex(index);
}

void BindingEditor::onSaveClicked()
{
    saveToFile();
}

// ---------------------------------------------------------------------------
QString BindingEditor::bindingFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QString("/g13/bindings-%1.properties").arg(m_profile);
}

void BindingEditor::loadFromFile()
{
    const QString fileKey = G13Keys::fileKey(m_currentKey);
    const auto    props   = readProps(bindingFilePath());

    if (!props.contains(fileKey)) {
        cmbType->setCurrentIndex(2);
        return;
    }

    const QStringList parts = props[fileKey].split(',');
    if (parts.isEmpty()) return;
    const QString type = parts[0].trimmed();

    if (type == "p" && parts.size() >= 2) {
        QString codeStr = parts[1].trimmed();
        if (codeStr.startsWith("k.")) {
            int code = codeStr.mid(2).toInt();
            for (int i = 0; i < cmbKeyName->count(); ++i) {
                if (cmbKeyName->itemData(i).toInt() == code) {
                    cmbKeyName->setCurrentIndex(i);
                    break;
                }
            }
        }
        cmbType->setCurrentIndex(0);
    } else if (type == "m" && parts.size() >= 3) {
        spnMacroId->setValue(parts[1].trimmed().toInt());
        spnRepeats->setValue(parts[2].trimmed().toInt());
        cmbType->setCurrentIndex(1);
    } else {
        cmbType->setCurrentIndex(2);
    }
}

void BindingEditor::saveToFile()
{
    if (m_currentKey.isEmpty() || !G13Keys::isConfigurable(m_currentKey))
        return;

    const QString fileKey = G13Keys::fileKey(m_currentKey);
    auto props = readProps(bindingFilePath());

    switch (cmbType->currentIndex()) {
    case 0:
        props[fileKey] = QString("p,k.%1").arg(cmbKeyName->currentData().toInt());
        break;
    case 1:
        props[fileKey] = QString("m,%1,%2").arg(spnMacroId->value()).arg(spnRepeats->value());
        break;
    case 2:
        props.remove(fileKey);
        break;
    }

    if (writeProps(bindingFilePath(), props))
        lblStatus->setText(QString("Gespeichert: %1 → %2")
            .arg(m_currentKey, props.value(fileKey, "–")));
    else
        lblStatus->setText("Fehler: Datei konnte nicht geschrieben werden.");
}
