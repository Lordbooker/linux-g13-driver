#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>

#include <unistd.h>

namespace {

// Name fuer die Einzelinstanz-Pruefung (pro Benutzer).
QString instanceName()
{
    return QString("g13-gui-%1").arg(getuid());
}

// Laeuft bereits eine Instanz? Dann dieser die Nachricht schicken.
bool notifyRunningInstance(const QByteArray &message)
{
    QLocalSocket socket;
    socket.connectToServer(instanceName());
    if (!socket.waitForConnected(300))
        return false;
    socket.write(message);
    socket.flush();
    socket.waitForBytesWritten(300);
    return true;
}

// Wie lange im Tray-Modus auf einen System-Tray gewartet wird. Beim Anmelden
// startet der Autostart oft, bevor die Taskleiste bereit ist.
constexpr int TrayWaitSeconds = 30;

}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("G13 Config Tool");
    app.setApplicationVersion(G13_VERSION);
    app.setOrganizationName("linux-g13-project");
    app.setDesktopFileName("g13-gui");          // Zuordnung zu g13-gui.desktop (Icon unter Wayland)
    app.setWindowIcon(MainWindow::appIcon());

    // Fenster schliessen beendet die App nicht, solange das Tray-Symbol aktiv ist
    // (siehe MainWindow::closeEvent).
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription("Konfigurationstool fuer den Logitech-G13-Treiber");
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption trayOption({"t", "tray"},
        "Nur das Tray-Symbol starten, ohne Fenster (fuer den Autostart).");
    parser.addOption(trayOption);
    parser.process(app);
    const bool trayOnly = parser.isSet(trayOption);

    // Einzelinstanz: Laeuft g13-gui schon (z. B. im Tray), dort das Fenster
    // oeffnen bzw. beim Autostart nichts tun.
    if (notifyRunningInstance(trayOnly ? "tray" : "show"))
        return 0;

    QLocalServer::removeServer(instanceName());   // Reste nach einem Absturz
    QLocalServer server;
    server.listen(instanceName());

    MainWindow w;

    QObject::connect(&server, &QLocalServer::newConnection, &w, [&server, &w] {
        while (QLocalSocket *client = server.nextPendingConnection()) {
            QObject::connect(client, &QLocalSocket::readyRead, &w, [client, &w] {
                if (client->readAll().contains("show"))
                    w.showWindow();
                client->disconnectFromServer();
            });
            QObject::connect(client, &QLocalSocket::disconnected, client, &QObject::deleteLater);
        }
    });

    if (!trayOnly) {
        w.showWindow();
    } else if (!w.hasTray()) {
        // Auf die Taskleiste warten; ohne Tray (z. B. GNOME ohne Erweiterung)
        // still beenden, statt bei jeder Anmeldung ein Fenster zu oeffnen.
        auto *timer = new QTimer(&app);
        QObject::connect(timer, &QTimer::timeout, &app, [timer, &w, attempts = 0]() mutable {
            if (w.setupTrayIcon()) {
                timer->stop();
                return;
            }
            if (++attempts >= TrayWaitSeconds) {
                timer->stop();
                qInfo("g13-gui: Kein System-Tray verfuegbar, Tray-Modus wird beendet.");
                QCoreApplication::exit(0);
            }
        });
        timer->start(1000);
    }

    return app.exec();
}
