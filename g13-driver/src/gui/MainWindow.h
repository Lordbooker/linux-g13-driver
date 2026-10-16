#ifndef MAINWINDOW_H
#define MAINWINDOW_H

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
    ~MainWindow();

    // App-Icon (eingebettete PNGs in mehreren Groessen, Theme-Icon bevorzugt).
    static QIcon appIcon();

    bool hasTray() const { return m_tray != nullptr; }

public slots:
    // Legt das Tray-Symbol an, sobald ein System-Tray verfuegbar ist.
    // Gibt true zurueck, wenn das Tray-Symbol (jetzt) existiert.
    bool setupTrayIcon();

    // Fenster anzeigen, wiederherstellen und in den Vordergrund holen.
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
    enum class DriverState { Unknown, Running, Stopped };

    void setupToolbar();
    void setupOptionsMenu();
    void refreshDriverStatus();
    void applyDriverStatus(DriverState state);
    void runSystemctl(const QString &verb, const QString &successMessage);

    Ui::MainWindow  *ui;
    QSystemTrayIcon *m_tray = nullptr;
    QComboBox       *m_profileCombo = nullptr;

    // Tray-Menue: Treiber-Steuerung ueber den systemd-User-Dienst "g13"
    QAction *m_actStatus = nullptr;
    QAction *m_actStartStop = nullptr;
    QAction *m_actRestart = nullptr;
    QTimer  *m_statusTimer = nullptr;
    DriverState m_driverState = DriverState::Unknown;
    bool m_trayHintShown = false;

    // Optionen: Startanimation auf dem Display (settings.properties, Schluessel "intro")
    QAction *m_actIntro = nullptr;
};

#endif // MAINWINDOW_H
