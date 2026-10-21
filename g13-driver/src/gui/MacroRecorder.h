#pragma once
#include <QWidget>
#include <QElapsedTimer>
#include <QList>

#include "MacroSequence.h"

class KeyCaptureEdit;
class QLabel;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QSpinBox;

// Macro editor: record key sequences, edit them as a list of events and save
// them in the driver format (kd./ku./d.) to ~/.config/g13/macro-<id>.properties.
class MacroRecorder : public QWidget {
    Q_OBJECT
public:
    explicit MacroRecorder(QWidget *parent = nullptr);
    ~MacroRecorder() override;

public slots:
    void setMacroId(int id);

protected:
    bool eventFilter(QObject *watched, QEvent *event) override;

private slots:
    void onLoadClicked();
    void onSaveClicked();
    void onRecordToggled();
    void onMoveUp();
    void onMoveDown();
    void onRemove();
    void onClear();
    void onInsertKeyDown();
    void onInsertKeyUp();
    void onInsertDelay();
    void onApplyDelayToSelection();
    void onApplyDelayToAll();
    void onSelectionChanged();

private:
    using Event = MacroSequence::Event;

    // Recording
    void startRecording();
    void stopRecording();
    void recordKey(int keyCode, bool pressed);

    // List
    QList<Event> events() const;
    void setEvents(const QList<Event> &list);
    void insertEvent(const Event &ev);
    void appendEvent(const Event &ev);
    static void applyToItem(QListWidgetItem *item, const Event &ev);
    static Event eventFromItem(const QListWidgetItem *item);

    // File
    void loadFromFile();
    bool saveToFile();
    QString macroFilePath() const;
    bool confirmDiscardChanges();

    void setDirty(bool dirty);
    void updateButtons();

    int  m_macroId = 0;
    bool m_dirty = false;

    bool          m_recording = false;
    QElapsedTimer m_sinceLastEvent;
    QList<int>    m_pressedKeys;     // keys pressed during the recording

    QSpinBox    *spnId;
    QPushButton *btnLoad;
    QLineEdit   *editName;

    QPushButton *btnRecord;
    QLabel      *lblRecordHint;
    QListWidget *listEvents;
    QPushButton *btnUp;
    QPushButton *btnDown;
    QPushButton *btnRemove;
    QPushButton *btnClear;

    KeyCaptureEdit *editKey;
    QPushButton *btnInsertDown;
    QPushButton *btnInsertUp;
    QSpinBox    *spnDelay;
    QPushButton *btnInsertDelay;
    QPushButton *btnDelaySelection;
    QPushButton *btnDelayAll;

    QPushButton *btnSave;
    QLabel      *lblStatus;
};
