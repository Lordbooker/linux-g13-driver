#include "MacroRecorder.h"
#include "KeyNames.h"

#include <QApplication>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFormLayout>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSaveFile>
#include <QSpinBox>
#include <QStandardPaths>
#include <QTextStream>
#include <QVBoxLayout>

namespace {
constexpr int RoleType  = Qt::UserRole;
constexpr int RoleValue = Qt::UserRole + 1;
constexpr int MaxMacroId = 199;      // Treiber: G13_MAX_MACROS = 200
constexpr int MaxDelayMs = 60000;
}

// ---------------------------------------------------------------------------
// Aufbau
// ---------------------------------------------------------------------------
MacroRecorder::MacroRecorder(QWidget *parent) : QWidget(parent)
{
    auto *root = new QVBoxLayout(this);

    // --- Makro-Auswahl ------------------------------------------------------
    auto *idBox    = new QGroupBox("Makro", this);
    auto *idLayout = new QFormLayout(idBox);
    spnId = new QSpinBox(this);
    spnId->setRange(0, MaxMacroId);
    btnLoad = new QPushButton("Laden", this);
    auto *idRow = new QHBoxLayout();
    idRow->addWidget(spnId, 1);
    idRow->addWidget(btnLoad);
    idLayout->addRow("Makro-ID:", idRow);
    editName = new QLineEdit(this);
    editName->setPlaceholderText("z. B. Snipe, Combo …");
    idLayout->addRow("Name:", editName);
    root->addWidget(idBox);

    // --- Ablauf (Aufnahme + Liste) -----------------------------------------
    auto *seqBox    = new QGroupBox("Ablauf", this);
    auto *seqLayout = new QGridLayout(seqBox);

    btnRecord = new QPushButton("●  Aufnahme starten", this);
    btnRecord->setCheckable(true);
    lblRecordHint = new QLabel(
        "Tastendrücke werden mit ihren Zeitabständen ans Ende der Liste angehängt. "
        "Das Fenster muss dabei im Vordergrund sein.", this);
    lblRecordHint->setWordWrap(true);
    seqLayout->addWidget(btnRecord, 0, 0);
    seqLayout->addWidget(lblRecordHint, 0, 1);

    listEvents = new QListWidget(this);
    listEvents->setSelectionMode(QAbstractItemView::ExtendedSelection);
    listEvents->setMinimumHeight(180);
    seqLayout->addWidget(listEvents, 1, 0, 1, 2);

    auto *listButtons = new QVBoxLayout();
    btnUp     = new QPushButton("Nach oben", this);
    btnDown   = new QPushButton("Nach unten", this);
    btnRemove = new QPushButton("Entfernen", this);
    btnClear  = new QPushButton("Alle löschen", this);
    listButtons->addWidget(btnUp);
    listButtons->addWidget(btnDown);
    listButtons->addWidget(btnRemove);
    listButtons->addStretch();
    listButtons->addWidget(btnClear);
    seqLayout->addLayout(listButtons, 1, 2);
    seqLayout->setColumnStretch(1, 1);
    root->addWidget(seqBox, 1);

    // --- Manuell bearbeiten -------------------------------------------------
    auto *editBox    = new QGroupBox("Manuell einfügen und anpassen", this);
    auto *editLayout = new QGridLayout(editBox);

    cmbKey = new QComboBox(this);
    for (const auto &[name, code] : KeyNames::all())
        cmbKey->addItem(QString("%1  (%2)").arg(name).arg(code), code);
    btnInsertDown = new QPushButton("Drücken einfügen", this);
    btnInsertUp   = new QPushButton("Loslassen einfügen", this);
    editLayout->addWidget(new QLabel("Taste:", this), 0, 0);
    editLayout->addWidget(cmbKey,        0, 1);
    editLayout->addWidget(btnInsertDown, 0, 2);
    editLayout->addWidget(btnInsertUp,   0, 3);

    spnDelay = new QSpinBox(this);
    spnDelay->setRange(0, MaxDelayMs);
    spnDelay->setSingleStep(10);
    spnDelay->setValue(50);
    spnDelay->setSuffix(" ms");
    btnInsertDelay    = new QPushButton("Wartezeit einfügen", this);
    btnDelaySelection = new QPushButton("Auf Auswahl anwenden", this);
    btnDelayAll       = new QPushButton("Auf alle anwenden", this);
    btnDelayAll->setToolTip("Ersetzt alle Wartezeiten der Liste durch diesen Wert.");
    editLayout->addWidget(new QLabel("Wartezeit:", this), 1, 0);
    editLayout->addWidget(spnDelay,          1, 1);
    editLayout->addWidget(btnInsertDelay,    1, 2);
    editLayout->addWidget(btnDelaySelection, 1, 3);
    editLayout->addWidget(btnDelayAll,       2, 3);
    editLayout->setColumnStretch(1, 1);
    root->addWidget(editBox);

    // --- Speichern ----------------------------------------------------------
    auto *saveRow = new QHBoxLayout();
    lblStatus = new QLabel("", this);
    lblStatus->setWordWrap(true);
    btnSave = new QPushButton("Speichern", this);
    saveRow->addWidget(lblStatus, 1);
    saveRow->addWidget(btnSave);
    root->addLayout(saveRow);

    // --- Signale ------------------------------------------------------------
    connect(btnLoad,   &QPushButton::clicked, this, &MacroRecorder::onLoadClicked);
    connect(btnSave,   &QPushButton::clicked, this, &MacroRecorder::onSaveClicked);
    connect(btnRecord, &QPushButton::clicked, this, &MacroRecorder::onRecordToggled);
    connect(btnUp,     &QPushButton::clicked, this, &MacroRecorder::onMoveUp);
    connect(btnDown,   &QPushButton::clicked, this, &MacroRecorder::onMoveDown);
    connect(btnRemove, &QPushButton::clicked, this, &MacroRecorder::onRemove);
    connect(btnClear,  &QPushButton::clicked, this, &MacroRecorder::onClear);
    connect(btnInsertDown,     &QPushButton::clicked, this, &MacroRecorder::onInsertKeyDown);
    connect(btnInsertUp,       &QPushButton::clicked, this, &MacroRecorder::onInsertKeyUp);
    connect(btnInsertDelay,    &QPushButton::clicked, this, &MacroRecorder::onInsertDelay);
    connect(btnDelaySelection, &QPushButton::clicked, this, &MacroRecorder::onApplyDelayToSelection);
    connect(btnDelayAll,       &QPushButton::clicked, this, &MacroRecorder::onApplyDelayToAll);
    connect(listEvents, &QListWidget::itemSelectionChanged, this, &MacroRecorder::onSelectionChanged);
    connect(editName,   &QLineEdit::textEdited, this, [this] { setDirty(true); });

    loadFromFile();
}

MacroRecorder::~MacroRecorder()
{
    if (m_recording)
        qApp->removeEventFilter(this);
}

// ---------------------------------------------------------------------------
// Makro-Auswahl
// ---------------------------------------------------------------------------
void MacroRecorder::setMacroId(int id)
{
    if (!confirmDiscardChanges()) {
        spnId->setValue(m_macroId);
        return;
    }
    spnId->setValue(id);
    m_macroId = id;
    loadFromFile();
}

void MacroRecorder::onLoadClicked()
{
    if (!confirmDiscardChanges()) {
        spnId->setValue(m_macroId);
        return;
    }
    m_macroId = spnId->value();
    loadFromFile();
}

void MacroRecorder::onSaveClicked()
{
    // Gespeichert wird unter der aktuell eingestellten ID.
    m_macroId = spnId->value();
    saveToFile();
}

bool MacroRecorder::confirmDiscardChanges()
{
    if (!m_dirty)
        return true;
    const auto answer = QMessageBox::question(
        this, "Ungespeicherte Änderungen",
        QString("Makro %1 wurde geändert. Änderungen verwerfen?").arg(m_macroId),
        QMessageBox::Discard | QMessageBox::Cancel, QMessageBox::Cancel);
    return answer == QMessageBox::Discard;
}

// ---------------------------------------------------------------------------
// Aufnahme
// ---------------------------------------------------------------------------
void MacroRecorder::onRecordToggled()
{
    if (m_recording)
        stopRecording();
    else
        startRecording();
}

void MacroRecorder::startRecording()
{
    m_recording = true;
    m_pressedKeys.clear();
    m_sinceLastEvent.invalidate();

    // Anwendungsweiter Filter: Tasten werden abgefangen, bevor Widgets oder
    // Shortcuts (z. B. Ctrl+S) sie verarbeiten.
    qApp->installEventFilter(this);

    btnRecord->setChecked(true);
    btnRecord->setText("■  Aufnahme stoppen");
    lblStatus->setText("Aufnahme läuft … Tasten drücken, zum Beenden auf „Aufnahme stoppen“ klicken.");
    updateButtons();
}

void MacroRecorder::stopRecording()
{
    qApp->removeEventFilter(this);
    m_recording = false;

    // Noch gedrueckte Tasten loslassen, sonst haengen sie beim Abspielen.
    const QList<int> stillPressed = m_pressedKeys;
    for (int code : stillPressed)
        recordKey(code, false);
    m_pressedKeys.clear();

    btnRecord->setChecked(false);
    btnRecord->setText("●  Aufnahme starten");
    lblStatus->setText(stillPressed.isEmpty()
        ? "Aufnahme beendet."
        : QString("Aufnahme beendet. %1 noch gedrückte Taste(n) wurden automatisch losgelassen.")
              .arg(stillPressed.size()));
    updateButtons();
}

bool MacroRecorder::eventFilter(QObject *watched, QEvent *event)
{
    if (!m_recording)
        return QWidget::eventFilter(watched, event);

    switch (event->type()) {
    case QEvent::ShortcutOverride:
        // Akzeptieren unterdrueckt Shortcuts; das KeyPress folgt danach.
        event->accept();
        return true;

    case QEvent::KeyPress:
    case QEvent::KeyRelease: {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (keyEvent->isAutoRepeat())
            return true;   // Tastenwiederholung ignorieren

        const int code = linuxKeyCode(static_cast<int>(keyEvent->nativeScanCode()));
        if (code <= 0) {
            lblStatus->setText("Taste konnte nicht erkannt werden (kein Scancode vom System).");
            return true;
        }
        recordKey(code, event->type() == QEvent::KeyPress);
        return true;       // Taste nicht an die Oberflaeche weitergeben
    }

    default:
        return QWidget::eventFilter(watched, event);
    }
}

// Qt liefert unter Linux (X11/xcb und Wayland) als nativeScanCode den
// XKB-Keycode. Dieser ist der Linux-Input-Keycode + 8.
int MacroRecorder::linuxKeyCode(int nativeScanCode)
{
    constexpr int XkbOffset = 8;
    return nativeScanCode > XkbOffset ? nativeScanCode - XkbOffset : 0;
}

void MacroRecorder::recordKey(int keyCode, bool pressed)
{
    if (pressed) {
        if (m_pressedKeys.contains(keyCode))
            return;
        m_pressedKeys << keyCode;
    } else {
        // Loslassen ohne vorheriges Druecken (z. B. die Taste, mit der die
        // Aufnahme gestartet wurde) wird ignoriert.
        if (!m_pressedKeys.removeOne(keyCode))
            return;
    }

    if (m_sinceLastEvent.isValid()) {
        const qint64 elapsed = m_sinceLastEvent.restart();
        if (elapsed > 0)
            appendEvent({Event::Type::Delay, static_cast<int>(qMin<qint64>(elapsed, MaxDelayMs))});
    } else {
        m_sinceLastEvent.start();
    }

    appendEvent({pressed ? Event::Type::KeyDown : Event::Type::KeyUp, keyCode});
    lblStatus->setText(QString("Aufnahme läuft … zuletzt: %1")
                           .arg(Event{pressed ? Event::Type::KeyDown : Event::Type::KeyUp,
                                      keyCode}.displayText()));
}

// ---------------------------------------------------------------------------
// Liste bearbeiten
// ---------------------------------------------------------------------------
void MacroRecorder::applyToItem(QListWidgetItem *item, const Event &ev)
{
    item->setText(ev.displayText());
    item->setData(RoleType,  static_cast<int>(ev.type));
    item->setData(RoleValue, ev.value);
}

MacroRecorder::Event MacroRecorder::eventFromItem(const QListWidgetItem *item)
{
    Event ev;
    ev.type  = static_cast<Event::Type>(item->data(RoleType).toInt());
    ev.value = item->data(RoleValue).toInt();
    return ev;
}

QList<MacroRecorder::Event> MacroRecorder::events() const
{
    QList<Event> list;
    for (int i = 0; i < listEvents->count(); ++i)
        list << eventFromItem(listEvents->item(i));
    return list;
}

void MacroRecorder::setEvents(const QList<Event> &list)
{
    listEvents->clear();
    for (const Event &ev : list) {
        auto *item = new QListWidgetItem();
        applyToItem(item, ev);
        listEvents->addItem(item);
    }
    updateButtons();
}

void MacroRecorder::appendEvent(const Event &ev)
{
    auto *item = new QListWidgetItem();
    applyToItem(item, ev);
    listEvents->addItem(item);
    listEvents->scrollToItem(item);
    setDirty(true);
    updateButtons();
}

// Fuegt hinter der aktuellen Zeile ein (oder am Ende, wenn nichts gewaehlt ist).
void MacroRecorder::insertEvent(const Event &ev)
{
    const int current = listEvents->currentRow();
    const int row = (current >= 0 && listEvents->currentItem()->isSelected())
                        ? current + 1 : listEvents->count();
    auto *item = new QListWidgetItem();
    applyToItem(item, ev);
    listEvents->insertItem(row, item);
    listEvents->setCurrentRow(row, QItemSelectionModel::ClearAndSelect);
    setDirty(true);
    updateButtons();
}

void MacroRecorder::onMoveUp()
{
    const int row = listEvents->currentRow();
    if (row <= 0)
        return;
    QListWidgetItem *item = listEvents->takeItem(row);
    listEvents->insertItem(row - 1, item);
    listEvents->setCurrentRow(row - 1, QItemSelectionModel::ClearAndSelect);
    setDirty(true);
}

void MacroRecorder::onMoveDown()
{
    const int row = listEvents->currentRow();
    if (row < 0 || row >= listEvents->count() - 1)
        return;
    QListWidgetItem *item = listEvents->takeItem(row);
    listEvents->insertItem(row + 1, item);
    listEvents->setCurrentRow(row + 1, QItemSelectionModel::ClearAndSelect);
    setDirty(true);
}

void MacroRecorder::onRemove()
{
    const auto selected = listEvents->selectedItems();
    if (selected.isEmpty())
        return;
    for (QListWidgetItem *item : selected)
        delete item;
    setDirty(true);
    updateButtons();
}

void MacroRecorder::onClear()
{
    if (listEvents->count() == 0)
        return;
    const auto answer = QMessageBox::question(
        this, "Alle löschen", "Alle Einträge des Makros entfernen?",
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
    if (answer != QMessageBox::Yes)
        return;
    listEvents->clear();
    setDirty(true);
    updateButtons();
}

void MacroRecorder::onInsertKeyDown()
{
    insertEvent({Event::Type::KeyDown, cmbKey->currentData().toInt()});
}

void MacroRecorder::onInsertKeyUp()
{
    insertEvent({Event::Type::KeyUp, cmbKey->currentData().toInt()});
}

void MacroRecorder::onInsertDelay()
{
    insertEvent({Event::Type::Delay, spnDelay->value()});
}

void MacroRecorder::onApplyDelayToSelection()
{
    int changed = 0;
    for (QListWidgetItem *item : listEvents->selectedItems()) {
        Event ev = eventFromItem(item);
        if (ev.type != Event::Type::Delay)
            continue;
        ev.value = spnDelay->value();
        applyToItem(item, ev);
        ++changed;
    }
    if (changed > 0)
        setDirty(true);
    lblStatus->setText(QString("%1 Wartezeit(en) geändert.").arg(changed));
}

void MacroRecorder::onApplyDelayToAll()
{
    int changed = 0;
    for (int i = 0; i < listEvents->count(); ++i) {
        QListWidgetItem *item = listEvents->item(i);
        Event ev = eventFromItem(item);
        if (ev.type != Event::Type::Delay)
            continue;
        ev.value = spnDelay->value();
        applyToItem(item, ev);
        ++changed;
    }
    if (changed > 0)
        setDirty(true);
    lblStatus->setText(QString("%1 Wartezeit(en) auf %2 ms gesetzt.")
                           .arg(changed).arg(spnDelay->value()));
}

void MacroRecorder::onSelectionChanged()
{
    // Einzelne gewaehlte Wartezeit in das Eingabefeld uebernehmen.
    const auto selected = listEvents->selectedItems();
    if (selected.size() == 1) {
        const Event ev = eventFromItem(selected.first());
        if (ev.type == Event::Type::Delay)
            spnDelay->setValue(ev.value);
    }
    updateButtons();
}

void MacroRecorder::updateButtons()
{
    const bool idle      = !m_recording;
    const bool hasItems  = listEvents->count() > 0;
    const bool selection = !listEvents->selectedItems().isEmpty();
    const int  row       = listEvents->currentRow();

    spnId->setEnabled(idle);
    btnLoad->setEnabled(idle);
    editName->setEnabled(idle);
    listEvents->setEnabled(idle);
    btnUp->setEnabled(idle && selection && row > 0);
    btnDown->setEnabled(idle && selection && row >= 0 && row < listEvents->count() - 1);
    btnRemove->setEnabled(idle && selection);
    btnClear->setEnabled(idle && hasItems);
    cmbKey->setEnabled(idle);
    btnInsertDown->setEnabled(idle);
    btnInsertUp->setEnabled(idle);
    spnDelay->setEnabled(idle);
    btnInsertDelay->setEnabled(idle);
    btnDelaySelection->setEnabled(idle && selection);
    btnDelayAll->setEnabled(idle && hasItems);
    btnSave->setEnabled(idle);
}

void MacroRecorder::setDirty(bool dirty)
{
    m_dirty = dirty;
}

// ---------------------------------------------------------------------------
// Datei
// ---------------------------------------------------------------------------
QString MacroRecorder::macroFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + QString("/g13/macro-%1.properties").arg(m_macroId);
}

void MacroRecorder::loadFromFile()
{
    QFile f(macroFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        editName->clear();
        setEvents({});
        setDirty(false);
        lblStatus->setText(QString("Makro %1 ist noch leer.").arg(m_macroId));
        return;
    }

    QTextStream in(&f);
    QString name, sequence;
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        const int eq = line.indexOf('=');
        if (eq < 0)
            continue;
        const QString key = line.left(eq).trimmed();
        const QString val = line.mid(eq + 1).trimmed();
        if (key == "name")
            name = val;
        else if (key == "sequence")
            sequence = val;
    }

    const MacroSequence::ParseResult parsed = MacroSequence::parse(sequence);
    editName->setText(name);
    setEvents(parsed.events);

    if (parsed.invalidTokens.isEmpty()) {
        setDirty(false);
        lblStatus->setText(QString("Makro %1 geladen (%2 Einträge).")
                               .arg(m_macroId).arg(parsed.events.size()));
    } else {
        // Ungueltige Eintraege werden beim naechsten Speichern entfernt.
        setDirty(true);
        lblStatus->setText(QString("Makro %1 geladen. %2 ungültige(r) Eintrag/Einträge ignoriert "
                                   "(z. B. „%3“). Beim Speichern werden sie entfernt.")
                               .arg(m_macroId)
                               .arg(parsed.invalidTokens.size())
                               .arg(parsed.invalidTokens.first()));
    }
}

bool MacroRecorder::saveToFile()
{
    const QList<Event> list = events();

    const QList<int> unreleased = MacroSequence::unreleasedKeys(list);
    if (!unreleased.isEmpty()) {
        QStringList names;
        for (int code : unreleased)
            names << KeyNames::nameFor(code);
        const auto answer = QMessageBox::warning(
            this, "Taste wird nicht losgelassen",
            QString("Folgende Tasten werden gedrückt, aber nicht wieder losgelassen: %1\n\n"
                    "Beim Abspielen bleiben sie gedrückt. Trotzdem speichern?")
                .arg(names.join(", ")),
            QMessageBox::Save | QMessageBox::Cancel, QMessageBox::Cancel);
        if (answer != QMessageBox::Save)
            return false;
    }

    const QString path = macroFilePath();
    QDir().mkpath(QFileInfo(path).absolutePath());

    // QSaveFile schreibt atomar: der Treiber sieht nie eine halb geschriebene Datei.
    QSaveFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        lblStatus->setText("Fehler: Datei konnte nicht geschrieben werden.");
        return false;
    }
    QTextStream out(&f);
    out << "# G13 Makro – bearbeitet von g13-gui\n";
    out << "name="     << editName->text().trimmed() << "\n";
    out << "id="       << m_macroId << "\n";
    out << "sequence=" << MacroSequence::serialize(list) << "\n";
    out.flush();

    if (!f.commit()) {
        lblStatus->setText("Fehler: Datei konnte nicht geschrieben werden.");
        return false;
    }

    setDirty(false);
    lblStatus->setText(QString("Makro %1 gespeichert (%2 Einträge).")
                           .arg(m_macroId).arg(list.size()));
    return true;
}
