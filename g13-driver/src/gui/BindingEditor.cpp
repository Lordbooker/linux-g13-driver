#include "BindingEditor.h"

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
// Keycode-Tabelle (Linux input event codes)
// ---------------------------------------------------------------------------
static const QList<QPair<QString, int>> KEY_MAP = {
    {"ESC",       1},  {"1",        2},  {"2",       3},  {"3",       4},
    {"4",         5},  {"5",        6},  {"6",       7},  {"7",       8},
    {"8",         9},  {"9",       10},  {"0",      11},  {"Backspace",14},
    {"Tab",      15},  {"Q",       16},  {"W",      17},  {"E",      18},
    {"R",        19},  {"T",       20},  {"Y",      21},  {"U",      22},
    {"I",        23},  {"O",       24},  {"P",      25},  {"Enter",  28},
    {"LCtrl",    29},  {"A",       30},  {"S",      31},  {"D",      32},
    {"F",        33},  {"G",       34},  {"H",      35},  {"J",      36},
    {"K",        37},  {"L",       38},  {"LShift", 42},  {"Z",      44},
    {"X",        45},  {"C",       46},  {"V",      47},  {"B",      48},
    {"N",        49},  {"M",       50},  {"RShift", 54},  {"LAlt",   56},
    {"Space",    57},  {"F1",      59},  {"F2",     60},  {"F3",     61},
    {"F4",       62},  {"F5",      63},  {"F6",     64},  {"F7",     65},
    {"F8",       66},  {"F9",      67},  {"F10",    68},  {"F11",    87},
    {"F12",      88},  {"RCtrl",   97},  {"RAlt",  100},
    {"Home",    102},  {"Up",     103},  {"PgUp",  104},  {"Left",  105},
    {"Right",   106},  {"End",    107},  {"Down",  108},  {"PgDn",  109},
    {"Insert",  110},  {"Delete", 111},
    {"Mute",    113},  {"VolDn",  114},  {"VolUp", 115},
    {"Num0",     82},  {"Num1",    79},  {"Num2",   80},  {"Num3",   81},
    {"Num4",     75},  {"Num5",    76},  {"Num6",   77},  {"Num7",   71},
    {"Num8",     72},  {"Num9",    73},  {"Num.",   83},  {"Num+",   78},
    {"Num-",     74},  {"Num*",    55},  {"Num/",   98},  {"NumEnt", 96},
};

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
    lblProfile = new QLabel("Profil: M1", this);
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
    for (const auto &[name, code] : KEY_MAP)
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
    lblKey->setText(keyName);

    if (!isConfigurable(keyName)) {
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

QString BindingEditor::toFileKey(const QString &guiKey)
{
    if (guiKey.startsWith('G')) {
        bool ok;
        int n = guiKey.mid(1).toInt(&ok);
        if (ok && n >= 1 && n <= 22)
            return QString("G%1").arg(n - 1);
    }
    static const QMap<QString, QString> special = {
        {"BD",   "G24"}, {"MR",   "G32"},
        {"LEFT", "G33"}, {"DOWN", "G34"}, {"TOP", "G35"},
    };
    return special.value(guiKey, guiKey);
}

bool BindingEditor::isConfigurable(const QString &guiKey)
{
    static const QSet<QString> locked = {"L1","L2","L3","L4","M1","M2","M3"};
    return !locked.contains(guiKey);
}

void BindingEditor::loadFromFile()
{
    const QString fileKey = toFileKey(m_currentKey);
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
    if (m_currentKey.isEmpty() || !isConfigurable(m_currentKey))
        return;

    const QString fileKey = toFileKey(m_currentKey);
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
