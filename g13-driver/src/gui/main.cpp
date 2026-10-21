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

// Name for the single instance check (per user).
QString instanceName()
{
    return QString("g13-gui-%1").arg(getuid());
}

// Is an instance already running? Then send it the message.
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

// How long tray mode waits for a system tray. At login the autostart often
// runs before the panel is ready.
constexpr int TrayWaitSeconds = 30;

// Loads the translations for the system language (QLocale::system(), i.e.
// LANGUAGE/LC_ALL/LC_MESSAGES/LANG). The own translations are embedded in the
// program (:/i18n/g13-gui_<language>.qm); if a language is missing, the English
// texts from the source code are used. Qt's own translations (e.g. standard
// buttons like "Cancel") are loaded if installed.
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
    app.setDesktopFileName("g13-gui");          // link to g13-gui.desktop (icon on Wayland)
    app.setWindowIcon(MainWindow::appIcon());
    installTranslations(app);

    // Closing the window does not quit the app while the tray icon is active
    // (see MainWindow::closeEvent).
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

    // Single instance: if g13-gui is already running (e.g. in the tray), open
    // the window there, or do nothing when started by autostart.
    if (notifyRunningInstance(trayOnly ? "tray" : "show"))
        return 0;

    QLocalServer::removeServer(instanceName());   // leftovers after a crash
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
        // Wait for the panel; without a tray (e.g. GNOME without the extension)
        // exit silently instead of opening a window at every login.
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
