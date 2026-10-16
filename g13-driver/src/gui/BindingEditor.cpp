#include "BindingEditor.h"
#include "KeyNames.h"
#include "G13Keys.h"
#include "KeyCaptureEdit.h"

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
    out << "# G13 bindings - edited by g13-gui\n";
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
    auto *infoBox    = new QGroupBox(tr("Current selection"), this);
    auto *infoLayout = new QFormLayout(infoBox);
    lblProfile = new QLabel("1 (L1)", this);
    lblKey     = new QLabel("–", this);
    QFont f = lblKey->font();
    f.setBold(true);
    f.setPointSize(13);
    lblKey->setFont(f);
    infoLayout->addRow(tr("Profile:"), lblProfile);
    infoLayout->addRow(tr("Key:"),     lblKey);
    root->addWidget(infoBox);

    // Typ-Auswahl
    auto *typeBox    = new QGroupBox(tr("Action type"), this);
    auto *typeLayout = new QVBoxLayout(typeBox);
    cmbType = new QComboBox(this);
    cmbType->addItem(tr("Key code (pass-through)"));
    cmbType->addItem(tr("Macro"));
    cmbType->addItem(tr("No action"));
    typeLayout->addWidget(cmbType);
    root->addWidget(typeBox);

    // Stack: 3 Seiten
    stack = new QStackedWidget(this);

    // Seite 0 – Tastencode
    auto *pageKey       = new QWidget();
    auto *pageKeyLayout = new QFormLayout(pageKey);
    editKey = new KeyCaptureEdit(pageKey);
    pageKeyLayout->addRow(tr("Key:"), editKey);
    auto *lblKeyHint = new QLabel(
        tr("Click into the field and press a key on your keyboard, then save. "
           "One key per G13 key – use a macro for key combinations such as Ctrl+C."),
        pageKey);
    lblKeyHint->setWordWrap(true);
    pageKeyLayout->addRow(lblKeyHint);
    connect(editKey, &KeyCaptureEdit::keyCaptured, this, [this](int code, bool withModifiers) {
        lblStatus->setText(withModifiers
            ? tr("Key detected: %1. Key combinations are not possible here, only this key "
                 "is assigned – use a macro for combinations. Click “Save” to assign it.")
                  .arg(KeyNames::nameFor(code))
            : tr("Key detected: %1 – click “Save” to assign it.").arg(KeyNames::nameFor(code)));
    });
    stack->addWidget(pageKey);

    // Seite 1 – Makro
    auto *pageMacro       = new QWidget();
    auto *pageMacroLayout = new QFormLayout(pageMacro);
    spnMacroId = new QSpinBox(pageMacro);
    spnMacroId->setRange(0, 199);
    // Wiederholungsmodus wie im Treiber (MacroAction):
    //   0 = einmal ausfuehren, 1 = wiederholen solange gedrueckt, n>1 = n-mal
    cmbRepeatMode = new QComboBox(pageMacro);
    cmbRepeatMode->addItem(tr("Run once"));
    cmbRepeatMode->addItem(tr("Repeat while held"));
    cmbRepeatMode->addItem(tr("Run several times"));
    spnRepeats = new QSpinBox(pageMacro);
    spnRepeats->setRange(2, 100);
    spnRepeats->setValue(2);
    spnRepeats->setSuffix(" ×");
    spnRepeats->setEnabled(false);
    connect(cmbRepeatMode, &QComboBox::currentIndexChanged, spnRepeats,
            [this](int mode) { spnRepeats->setEnabled(mode == 2); });
    pageMacroLayout->addRow(tr("Macro ID:"), spnMacroId);
    pageMacroLayout->addRow(tr("Run mode:"), cmbRepeatMode);
    pageMacroLayout->addRow(tr("Count:"),    spnRepeats);
    auto *lblRepeatHint = new QLabel(
        tr("Pressing the key again while the macro is running stops it."), pageMacro);
    lblRepeatHint->setWordWrap(true);
    pageMacroLayout->addRow(lblRepeatHint);
    stack->addWidget(pageMacro);

    // Seite 2 – Keine Aktion (Platzhalter)
    stack->addWidget(new QWidget());

    root->addWidget(stack);

    // Speichern
    btnSave = new QPushButton(tr("Save"), this);
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
    lblProfile->setText(QString("%1 (L%1)").arg(profile + 1));
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
        lblStatus->setText(tr("This key switches the profile and cannot be assigned."));
        return;
    }

    cmbType->setEnabled(true);
    stack->setEnabled(true);
    btnSave->setEnabled(true);
    lblStatus->clear();
    loadFromFile();
    if (cmbType->currentIndex() == 0)
        editKey->setFocus();        // direkt Taste druecken koennen
}

void BindingEditor::onTypeChanged(int index)
{
    stack->setCurrentIndex(index);
    if (index == 0 && isVisible())
        editKey->setFocus();        // direkt Taste druecken koennen
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

    editKey->clearKey();
    if (!props.contains(fileKey)) {
        cmbType->setCurrentIndex(2);
        return;
    }

    const QStringList parts = props[fileKey].split(',');
    if (parts.isEmpty()) return;
    const QString type = parts[0].trimmed();

    if (type == "p" && parts.size() >= 2) {
        const QString codeStr = parts[1].trimmed();
        if (codeStr.startsWith("k."))
            editKey->setKeyCode(codeStr.mid(2).toInt());
        cmbType->setCurrentIndex(0);
    } else if (type == "m" && parts.size() >= 3) {
        spnMacroId->setValue(parts[1].trimmed().toInt());
        const int repeats = parts[2].trimmed().toInt();
        if (repeats <= 0) {
            cmbRepeatMode->setCurrentIndex(0);
        } else if (repeats == 1) {
            cmbRepeatMode->setCurrentIndex(1);
        } else {
            cmbRepeatMode->setCurrentIndex(2);
            spnRepeats->setValue(repeats);
        }
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
        if (editKey->keyCode() <= 0) {
            lblStatus->setText(tr("Press a key in the input field first."));
            editKey->setFocus();
            return;
        }
        props[fileKey] = QString("p,k.%1").arg(editKey->keyCode());
        break;
    case 1: {
        const int mode = cmbRepeatMode->currentIndex();
        const int repeats = mode == 2 ? spnRepeats->value() : mode;   // 0 bzw. 1
        props[fileKey] = QString("m,%1,%2").arg(spnMacroId->value()).arg(repeats);
        break;
    }
    case 2:
        props.remove(fileKey);
        break;
    }

    if (writeProps(bindingFilePath(), props))
        lblStatus->setText(tr("Saved: %1 → %2")
            .arg(m_currentKey, props.value(fileKey, "–")));
    else
        lblStatus->setText(tr("Error: could not write the file."));
}
