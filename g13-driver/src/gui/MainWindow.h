#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <cstdint>
#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QCloseEvent>

class QAction;
class QComboBox;
class QTimer;
namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override;
    Q_DISABLE_COPY_MOVE(MainWindow)

    // App icon (embedded PNGs in several sizes, theme icon preferred).
    static QIcon appIcon();

    bool hasTray() const { return m_tray != nullptr; }

public slots:
    // Creates the tray icon as soon as a system tray is available.
    // Returns true if the tray icon exists (now).
    bool setupTrayIcon();

    // Show the window, restore it and bring it to the front.
    void showWindow();

signals:
    void profileChanged(int profile);

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    void onProfileComboChanged(int index);
    void onSaveProfile();
    void onAbout();
    void onTrayActivated(QSystemTrayIcon::ActivationReason reason);

private:
    enum class DriverState : std::uint8_t { Unknown, Running, Stopped };

    void setupToolbar();
    void setupOptionsMenu();
    void refreshDriverStatus();
    void applyDriverStatus(DriverState state);
    void runSystemctl(const QString &verb, const QString &successMessage);

    Ui::MainWindow  *ui;
    QSystemTrayIcon *m_tray = nullptr;
    QComboBox       *m_profileCombo = nullptr;

    // Tray menu: driver control via the systemd user service "g13"
    QAction *m_actStatus = nullptr;
    QAction *m_actStartStop = nullptr;
    QAction *m_actRestart = nullptr;
    QTimer  *m_statusTimer = nullptr;
    DriverState m_driverState = DriverState::Unknown;
    bool m_trayHintShown = false;

    // Options: start animation on the display (settings.properties, key "intro")
    QAction *m_actIntro = nullptr;
};

#endif // MAINWINDOW_H
