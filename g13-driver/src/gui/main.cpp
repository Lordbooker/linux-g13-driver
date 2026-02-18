#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("G13 Config Tool");
    app.setApplicationVersion("2.0");
    app.setOrganizationName("linux-g13-project");

    // Fenster schließen beendet die App NICHT – Tray-Betrieb bleibt aktiv
    app.setQuitOnLastWindowClosed(false);

    MainWindow w;
    w.show();

    return app.exec();
}
