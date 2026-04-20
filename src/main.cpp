#include "ui/mainwindow.h"
#include <QApplication>
#include <QFile>

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("TimeTracker");
    app.setOrganizationName("alex");
    app.setApplicationVersion("0.1.0");

    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));

    MainWindow window;
    window.resize(1000, 700);
    window.setMinimumSize(700, 500);
    window.show();

    return app.exec();
}
