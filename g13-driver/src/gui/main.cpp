#include "MainWindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    
    // Metadaten für die App (wichtig für Settings-Speicherung später)
    app.setApplicationName("Linux G13 GUI");
    app.setOrganizationName("Booker");

    MainWindow w;
    w.show();

    return app.exec();
}