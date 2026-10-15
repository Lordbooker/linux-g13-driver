#include "MainWindow.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QLibraryInfo>
#include <QLocale>
#include <QLocalServer>
#include <QLocalSocket>
#include <QTimer>
#include <QTranslator>

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

// Laedt die Uebersetzungen passend zur Systemsprache (QLocale::system(), also
// LANGUAGE/LC_ALL/LC_MESSAGES/LANG). Die eigenen Uebersetzungen sind im Programm
// eingebettet (:/i18n/g13-gui_<sprache>.qm); fehlt eine Sprache, bleibt es bei
// den englischen Texten aus dem Quelltext. Qts eigene Uebersetzungen (z. B.
// Standard-Schaltflaechen wie "Cancel") werden geladen, sofern installiert.
void installTranslations(QApplication &app)
{
    const QLocale locale;

    auto *qtTranslator = new QTranslator(&app);
    if (qtTranslator->load(locale, "qtbase", "_",
                            QLibraryInfo::path(QLibraryInfo::TranslationsPath)))
        app.installTranslator(qtTranslator);

    auto *appTranslator = new QTranslator(&app);
    if (appTranslator->load(locale, "g13-gui", "_", ":/i18n"))
        app.installTranslator(appTranslator);
}

}

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("G13 Config Tool");
    app.setApplicationVersion(G13_VERSION);
    app.setOrganizationName("linux-g13-project");
    app.setDesktopFileName("g13-gui");          // Zuordnung zu g13-gui.desktop (Icon unter Wayland)
    app.setWindowIcon(MainWindow::appIcon());
    installTranslations(app);

    // Fenster schliessen beendet die App nicht, solange das Tray-Symbol aktiv ist
    // (siehe MainWindow::closeEvent).
    app.setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(
        QCoreApplication::translate("main", "Configuration tool for the Logitech G13 driver"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption trayOption({"t", "tray"},
        QCoreApplication::translate("main", "Start only the tray icon, without a window (for autostart)."));
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
                qInfo("g13-gui: No system tray available, exiting tray mode.");
                QCoreApplication::exit(0);
            }
        });
        timer->start(1000);
    }

    return app.exec();
}
