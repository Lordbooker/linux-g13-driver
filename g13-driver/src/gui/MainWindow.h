#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QCloseEvent>

class QComboBox;
namespace Ui { class MainWindow; }

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

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
    void setupTrayIcon();
    void setupToolbar();

    Ui::MainWindow  *ui;
    QSystemTrayIcon *m_tray;
    QComboBox       *m_profileCombo;
};

#endif // MAINWINDOW_H
