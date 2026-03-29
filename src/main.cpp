#include <QApplication>
#include "App.h"

int main(int argc, char* argv[])
{
    QApplication qtApp(argc, argv);
    qtApp.setApplicationName("ChronicleForge");
    qtApp.setApplicationVersion(CF_APP_VERSION);
    qtApp.setOrganizationName("ChronicleForge");

    // App owns all DI wiring — main stays minimal
    CF::App app;

    if (!app.initialize()) {
        return EXIT_FAILURE;
    }

    app.showMainWindow();
    return qtApp.exec();
}