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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_tray(nullptr)
    , m_profileCombo(nullptr)
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
    m_profileCombo->addItem("M1  (Profil 0)");
    m_profileCombo->addItem("M2  (Profil 1)");
    m_profileCombo->addItem("M3  (Profil 2)");
    m_profileCombo->addItem("M4  (Profil 3)");
    tb->addWidget(m_profileCombo);
    tb->addSeparator();
    tb->addAction(ui->actionSave_Profile);

    connect(m_profileCombo, &QComboBox::currentIndexChanged,
            this, &MainWindow::onProfileComboChanged);
}

// ---------------------------------------------------------------------------
// System-Tray
// ---------------------------------------------------------------------------
void MainWindow::setupTrayIcon()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(QIcon::fromTheme("input-gaming", QIcon(":/g13_bg")));
    m_tray->setToolTip("G13 Config Tool");

    auto *menu    = new QMenu(this);
    auto *actShow = menu->addAction("Fenster anzeigen");
    menu->addSeparator();
    auto *actQuit = menu->addAction("Beenden");

    connect(actShow, &QAction::triggered, this,  &QWidget::showNormal);
    connect(actQuit, &QAction::triggered, qApp,  &QApplication::quit);

    m_tray->setContextMenu(menu);
    m_tray->show();

    connect(m_tray, &QSystemTrayIcon::activated, this, &MainWindow::onTrayActivated);
}

// ---------------------------------------------------------------------------
// Slots
// ---------------------------------------------------------------------------
void MainWindow::onProfileComboChanged(int index)
{
    emit profileChanged(index);
    statusBar()->showMessage(QString("Profil M%1 aktiv.").arg(index + 1), 2000);
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
    if (reason == QSystemTrayIcon::Trigger) {
        setVisible(!isVisible());
        if (isVisible())
            raise();
    }
}

// ---------------------------------------------------------------------------
// Schließen → Tray (nicht beenden)
// ---------------------------------------------------------------------------
void MainWindow::closeEvent(QCloseEvent *event)
{
    if (m_tray && m_tray->isVisible()) {
        hide();
        m_tray->showMessage(
            "G13 Config Tool",
            "Läuft im Hintergrund. Tray-Icon anklicken zum Öffnen.",
            QSystemTrayIcon::Information, 2000);
        event->ignore();
    } else {
        event->accept();
    }
}
