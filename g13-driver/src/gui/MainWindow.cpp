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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>
#include <QStandardPaths>
#include <QTextStream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    setupToolbar();
    setupOptionsMenu();
    setupTrayIcon();

    // Click on a key in the visualizer -> switch to the binding editor
    connect(ui->visualizer, &G13Visualizer::keySelected, this, [this](const QString &keyName) {
        ui->tabWidget->setCurrentIndex(0);
        ui->editor->setKey(keyName);
    });

    // Pass profile changes on to all tabs
    connect(this, &MainWindow::profileChanged, ui->visualizer,    &G13Visualizer::setProfile);
    connect(this, &MainWindow::profileChanged, ui->editor,        &BindingEditor::setProfile);
    connect(this, &MainWindow::profileChanged, ui->colorSelector, &ColorSelector::setProfile);

    // Menu actions
    connect(ui->actionSave_Profile, &QAction::triggered, this, &MainWindow::onSaveProfile);
    connect(ui->actionAbout,        &QAction::triggered, this, &MainWindow::onAbout);
    // "Quit" exits the config tool including the tray icon (the driver keeps running).
    connect(ui->actionQuit,         &QAction::triggered, qApp, &QApplication::quit);

    statusBar()->showMessage(tr("G13 Config Tool ready."), 3000);
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
    QToolBar *tb = addToolBar(tr("Main toolbar"));
    tb->setMovable(false);
    tb->addWidget(new QLabel("  " + tr("Profile:") + " ", this));

    m_profileCombo = new QComboBox(this);
    // Profiles are selected on the device with L1..L4 (keys below the display).
    for (int i = 1; i <= 4; ++i)
        m_profileCombo->addItem(tr("Profile %1  (L%1)").arg(i));
    tb->addWidget(m_profileCombo);
    tb->addSeparator();
    tb->addAction(ui->actionSave_Profile);

    connect(m_profileCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::onProfileComboChanged);
}

// ---------------------------------------------------------------------------
// Options (global settings in ~/.config/g13/settings.properties)
// ---------------------------------------------------------------------------
namespace {

QString settingsFilePath()
{
    return QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
           + "/g13/settings.properties";
}

// Reads a value; if the key is missing, defaultValue applies.
QString readSetting(const QString &key, const QString &defaultValue)
{
    QFile f(settingsFilePath());
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return defaultValue;
    QString value = defaultValue;
    QTextStream in(&f);
    while (!in.atEnd()) {
        const QString line = in.readLine().trimmed();
        if (line.isEmpty() || line.startsWith('#'))
            continue;
        const int eq = line.indexOf('=');
        if (eq > 0 && line.left(eq).trimmed() == key)
            value = line.mid(eq + 1).trimmed();
    }
    return value;
}

// Sets a value and leaves all other lines (including comments) unchanged.
bool writeSetting(const QString &key, const QString &value)
{
    QStringList lines;
    QFile in(settingsFilePath());
    if (in.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QTextStream ts(&in);
        while (!ts.atEnd())
            lines << ts.readLine();
        in.close();
    } else {
        lines << "# G13 settings - edited by g13-gui";
    }
    bool replaced = false;
    for (QString &line : lines) {
        const QString t = line.trimmed();
        const int eq = t.indexOf('=');
        if (!t.startsWith('#') && eq > 0 && t.left(eq).trimmed() == key) {
            line = key + "=" + value;
            replaced = true;
        }
    }
    if (!replaced)
        lines << key + "=" + value;

    QDir().mkpath(QFileInfo(settingsFilePath()).absolutePath());
    QSaveFile out(settingsFilePath());
    if (!out.open(QIODevice::WriteOnly | QIODevice::Text))
        return false;
    QTextStream ts(&out);
    for (const QString &line : lines)
        ts << line << "\n";
    ts.flush();
    return out.commit();
}

}

void MainWindow::setupOptionsMenu()
{
    auto *menu = new QMenu(tr("Options"), this);
    menuBar()->insertMenu(ui->menuHelp->menuAction(), menu);

    m_actIntro = menu->addAction(tr("Start animation on the display"));
    m_actIntro->setCheckable(true);
    m_actIntro->setChecked(readSetting("intro", "1") != "0");
    m_actIntro->setToolTip(tr("Plays the flame animation when the G13 is connected or the driver starts."));
    menu->setToolTipsVisible(true);

    connect(m_actIntro, &QAction::toggled, this, [this](bool on) {
        if (writeSetting("intro", on ? "1" : "0")) {
            statusBar()->showMessage(on
                ? tr("Start animation on. It plays the next time the G13 is connected or the driver restarts.")
                : tr("Start animation off. The logo is shown directly from now on."), 6000);
        } else {
            statusBar()->showMessage(tr("Error: could not write the settings file."), 6000);
            const QSignalBlocker block(m_actIntro);
            m_actIntro->setChecked(!on);
        }
    });
}

// ---------------------------------------------------------------------------
// App icon and window
// ---------------------------------------------------------------------------
QIcon MainWindow::appIcon()
{
    QIcon fallback;
    for (int size : {22, 32, 48, 64, 128, 256})
        fallback.addFile(QString(":/icons/g13-gui-%1.png").arg(size), QSize(size, size));
    // Prefer the installed theme icon (hicolor/g13-gui), else the embedded one.
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
    m_tray->setToolTip(tr("G13 Config Tool"));

    auto *menu = new QMenu(this);
    auto *actOpen = menu->addAction(tr("Open configuration"));
    menu->addSeparator();
    m_actStatus = menu->addAction(tr("Driver: checking …"));
    m_actStatus->setEnabled(false);
    m_actStartStop = menu->addAction(tr("Stop driver"));
    m_actRestart   = menu->addAction(tr("Restart driver"));
    menu->addSeparator();
    auto *actQuit = menu->addAction(tr("Quit tray icon"));

    connect(actOpen, &QAction::triggered, this, &MainWindow::showWindow);
    connect(m_actStartStop, &QAction::triggered, this, [this] {
        if (m_driverState == DriverState::Running)
            runSystemctl("stop", tr("Driver stopped."));
        else
            runSystemctl("start", tr("Driver started."));
    });
    connect(m_actRestart, &QAction::triggered, this, [this] {
        runSystemctl("restart", tr("Driver restarted."));
    });
    connect(actQuit, &QAction::triggered, qApp, &QApplication::quit);
    connect(menu, &QMenu::aboutToShow, this, &MainWindow::refreshDriverStatus);

    m_tray->setContextMenu(menu);
    connect(m_tray, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
    m_tray->show();

    // Update the status regularly (tooltip and menu).
    m_statusTimer = new QTimer(this);
    connect(m_statusTimer, &QTimer::timeout, this, &MainWindow::refreshDriverStatus);
    m_statusTimer->start(10000);
    refreshDriverStatus();
    return true;
}

// Queries the state of the systemd user service "g13" (asynchronously).
void MainWindow::refreshDriverStatus()
{
    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc](int exitCode, QProcess::ExitStatus status) {
        // "systemctl is-active" returns 0 exactly when the service is running.
        applyDriverStatus(status == QProcess::NormalExit && exitCode == 0
                              ? DriverState::Running : DriverState::Stopped);
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            applyDriverStatus(DriverState::Unknown);   // no systemctl available
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
    case DriverState::Running: text = tr("Driver: running");  break;
    case DriverState::Stopped: text = tr("Driver: stopped");  break;
    case DriverState::Unknown: text = tr("Driver: unknown");  break;
    }
    m_actStatus->setText(text);
    m_actStartStop->setText(state == DriverState::Running ? tr("Stop driver") : tr("Start driver"));
    m_actStartStop->setEnabled(state != DriverState::Unknown);
    m_actRestart->setEnabled(state != DriverState::Unknown);
    m_tray->setToolTip(tr("G13 Config Tool") + " – " + text);
}

// Starts/stops the driver via the systemd user service and reports the result.
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
                m_tray->showMessage("G13", err.isEmpty() ? tr("Action failed.")
                                                         : tr("Action failed: %1").arg(err),
                                    QSystemTrayIcon::Warning, 5000);
            }
        }
        proc->deleteLater();
        refreshDriverStatus();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            if (m_tray)
                m_tray->showMessage("G13", tr("Could not run systemctl."),
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
    statusBar()->showMessage(tr("Profile %1 (L%1) active.").arg(index + 1), 2000);
}

void MainWindow::onSaveProfile()
{
    statusBar()->showMessage(
        tr("Profile is saved – the driver picks up changes automatically."), 4000);
}

void MainWindow::onAbout()
{
    QMessageBox::about(this, tr("About G13 Config Tool"),
        "<h2>Linux G13 Driver</h2>"
        "<p>" + tr("Version %1").arg(QStringLiteral(G13_VERSION)) + "</p>"
        "<p>" + tr("License: GNU GPL v2. Based on linux-g13-driver by Jim Gupta.") + "</p>"
        "<p>" + tr("Changes are saved directly to %1.").arg("<code>~/.config/g13/</code>") + "<br>"
        + tr("The driver reloads changed files automatically.") + "</p>"
    );
}

void MainWindow::onTrayActivated(QSystemTrayIcon::ActivationReason reason)
{
    // Left click: show/hide the window
    if (reason == QSystemTrayIcon::Trigger) {
        if (isVisible() && !isMinimized())
            hide();
        else
            showWindow();
    }
}

// ---------------------------------------------------------------------------
// Close -> tray (do not quit)
// ---------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tray && m_tray->isVisible()) {
        hide();
        if (!m_trayHintShown) {
            m_tray->showMessage(
                tr("G13 Config Tool"),
                tr("Still running in the background. Click the tray icon to open it."),
                QSystemTrayIcon::Information, 3000);
            m_trayHintShown = true;
        }
        event->ignore();
    } else {
        // Without a tray icon the app would otherwise keep running invisibly.
        event->accept();
        qApp->quit();
    }
}
