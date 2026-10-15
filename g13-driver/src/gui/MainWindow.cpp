#include "MainWindow.h"
#include "ui_MainWindow.h"

#include "G13Visualizer.h"
#include "BindingEditor.h"
#include "ColorSelector.h"
#include "MacroRecorder.h"

#include <QComboBox>
#include <QToolBar>
#include <QLabel>
#include <QMenu>
#include <QAction>
#include <QMessageBox>
#include <QApplication>
#include <QProcess>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupToolbar();
    setupTrayIcon();

    // Klick auf Taste im Visualizer → Binding-Editor wechseln
    connect(ui->visualizer, &G13Visualizer::keySelected, this, [this](const QString &keyName) {
        ui->tabWidget->setCurrentIndex(0);
        ui->editor->setKey(keyName);
    });

    // Profil-Wechsel an alle Tabs weitergeben
    connect(this, &MainWindow::profileChanged, ui->visualizer,    &G13Visualizer::setProfile);
    connect(this, &MainWindow::profileChanged, ui->editor,        &BindingEditor::setProfile);
    connect(this, &MainWindow::profileChanged, ui->colorSelector, &ColorSelector::setProfile);

    // Menü-Aktionen
    connect(ui->actionSave_Profile, &QAction::triggered, this, &MainWindow::onSaveProfile);
    connect(ui->actionAbout,        &QAction::triggered, this, &MainWindow::onAbout);
    // "Beenden" beendet das Config-Tool inkl. Tray-Symbol (der Treiber laeuft weiter).
    connect(ui->actionQuit,         &QAction::triggered, qApp, &QApplication::quit);

    statusBar()->showMessage("G13 Config Tool bereit.", 3000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// ---------------------------------------------------------------------------
// Toolbar
// ---------------------------------------------------------------------------
void MainWindow::setupToolbar()
{
    QToolBar *tb = addToolBar("Hauptleiste");
    tb->setMovable(false);
    tb->addWidget(new QLabel("  Profil: ", this));

    m_profileCombo = new QComboBox(this);
    // Die Profile werden am Geraet mit L1..L4 (Tasten unter dem Display) gewaehlt.
    for (int i = 1; i <= 4; ++i)
        m_profileCombo->addItem(QString("Profil %1  (L%1)").arg(i));
    tb->addWidget(m_profileCombo);
    tb->addSeparator();
    tb->addAction(ui->actionSave_Profile);

    connect(m_profileCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::onProfileComboChanged);
}

// ---------------------------------------------------------------------------
// App-Icon und Fenster
// ---------------------------------------------------------------------------
QIcon MainWindow::appIcon()
{
    QIcon fallback;
    for (int size : {22, 32, 48, 64, 128, 256})
        fallback.addFile(QString(":/icons/g13-gui-%1.png").arg(size), QSize(size, size));
    // Installiertes Theme-Icon bevorzugen (hicolor/g13-gui), sonst eingebettet.
    return QIcon::fromTheme("g13-gui", fallback);
}

void MainWindow::showWindow()
{
    if (isMinimized())
        showNormal();
    show();
    raise();
    activateWindow();
}

// ---------------------------------------------------------------------------
// System-Tray
// ---------------------------------------------------------------------------
bool MainWindow::setupTrayIcon()
{
    if (m_tray)
        return true;
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return false;

    m_tray = new QSystemTrayIcon(appIcon(), this);
    m_tray->setToolTip("G13 Config Tool");

    auto *menu = new QMenu(this);
    auto *actOpen = menu->addAction("Konfiguration öffnen");
    menu->addSeparator();
    m_actStatus = menu->addAction("Treiber: wird geprüft …");
    m_actStatus->setEnabled(false);
    m_actStartStop = menu->addAction("Treiber stoppen");
    m_actRestart   = menu->addAction("Treiber neu starten");
    menu->addSeparator();
    auto *actQuit = menu->addAction("Tray-Symbol beenden");

    connect(actOpen, &QAction::triggered, this, &MainWindow::showWindow);
    connect(m_actStartStop, &QAction::triggered, this, [this] {
        if (m_driverState == DriverState::Running)
            runSystemctl("stop", "Treiber gestoppt.");
        else
            runSystemctl("start", "Treiber gestartet.");
    });
    connect(m_actRestart, &QAction::triggered, this, [this] {
        runSystemctl("restart", "Treiber neu gestartet.");
    });
    connect(actQuit, &QAction::triggered, qApp, &QApplication::quit);
    connect(menu, &QMenu::aboutToShow, this, &MainWindow::refreshDriverStatus);

    m_tray->setContextMenu(menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
    m_tray->show();

    // Status regelmaessig aktualisieren (Tooltip und Menue).
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::refreshDriverStatus);
    m_statusTimer->start(10000);
    refreshDriverStatus();
    return true;
}

// Fragt den Zustand des systemd-User-Dienstes "g13" ab (asynchron).
void MainWindow::refreshDriverStatus()
{
    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc](int exitCode, QProcess::ExitStatus status) {
        // "systemctl is-active" liefert 0 genau dann, wenn der Dienst laeuft.
        applyDriverStatus(status == QProcess::NormalExit && exitCode == 0
                              ? DriverState::Running : DriverState::Stopped);
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            applyDriverStatus(DriverState::Unknown);   // kein systemctl verfuegbar
            proc->deleteLater();
        }
    });
    proc->start("systemctl", {"--user", "is-active", "--quiet", "g13"});
}

void MainWindow::applyDriverStatus(DriverState state)
{
    m_driverState = state;
    if (!m_tray)
        return;

    QString text;
    switch (state) {
    case DriverState::Running: text = "Treiber: läuft";        break;
    case DriverState::Stopped: text = "Treiber: gestoppt";     break;
    case DriverState::Unknown: text = "Treiber: unbekannt";    break;
    }
    m_actStatus->setText(text);
    m_actStartStop->setText(state == DriverState::Running ? "Treiber stoppen" : "Treiber starten");
    m_actStartStop->setEnabled(state != DriverState::Unknown);
    m_actRestart->setEnabled(state != DriverState::Unknown);
    m_tray->setToolTip("G13 Config Tool – " + text);
}

// Startet/stoppt den Treiber ueber den systemd-User-Dienst und meldet das Ergebnis.
void MainWindow::runSystemctl(const QString &verb, const QString &successMessage)
{
    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, this,
            [this, proc, successMessage](int exitCode, QProcess::ExitStatus status) {
        if (m_tray) {
            if (status == QProcess::NormalExit && exitCode == 0) {
                m_tray->showMessage("G13", successMessage, QSystemTrayIcon::Information, 3000);
            } else {
                const QString err = QString::fromLocal8Bit(proc->readAllStandardError()).trimmed();
                m_tray->showMessage("G13", "Aktion fehlgeschlagen" + (err.isEmpty() ? QString() : ": " + err),
                                    QSystemTrayIcon::Warning, 5000);
            }
        }
        proc->deleteLater();
        refreshDriverStatus();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            if (m_tray)
                m_tray->showMessage("G13", "systemctl konnte nicht gestartet werden.",
                                    QSystemTrayIcon::Warning, 5000);
            proc->deleteLater();
        }
    });
    proc->start("systemctl", {"--user", verb, "g13"});
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
void MainWindow::onProfileComboChanged(int index)
{
    emit profileChanged(index);
    statusBar()->showMessage(QString("Profil %1 (L%1) aktiv.").arg(index + 1), 2000);
}

void MainWindow::onSaveProfile()
{
    statusBar()->showMessage(
        "Profil ist gespeichert – der Daemon erkennt Änderungen automatisch.", 4000);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, "Über G13 Config Tool",
        "<h2>Linux G13 Driver</h2>"
        "<p>Version 2.0 – Phase 3: Qt6 Config Tool</p>"
        "<p>Änderungen werden direkt in <code>~/.config/g13/</code> gespeichert.<br>"
        "Der Daemon erkennt Dateiänderungen per Live-Reload automatisch.</p>"
    );
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    // Linksklick: Fenster ein-/ausblenden
    if (reason == QSystemTrayIcon::Trigger) {
        if (isVisible() && !isMinimized())
            hide();
        else
            showWindow();
    }
}

// ---------------------------------------------------------------------------
// Schließen → Tray (nicht beenden)
// ---------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tray && m_tray->isVisible()) {
        hide();
        if (!m_trayHintShown) {
            m_tray->showMessage(
                "G13 Config Tool",
                "Läuft im Hintergrund weiter. Tray-Symbol anklicken zum Öffnen.",
                QSystemTrayIcon::Information, 3000);
            m_trayHintShown = true;
        }
        event->ignore();
    } else {
        // Ohne Tray-Symbol wuerde die App sonst unsichtbar weiterlaufen.
        event->accept();
        qApp->quit();
    }
}
