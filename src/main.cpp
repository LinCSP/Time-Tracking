#include "ui/mainwindow.h"
#include <QApplication>
#include <QFile>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPalette>
#include <QStyleFactory>

static const QString kSingleInstanceKey = "TimeTracker_SingleInstance";

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);
    app.setApplicationName("TimeTracker");
    app.setOrganizationName("alex");
    app.setApplicationVersion("0.1.0");

    // Single-instance guard: try to connect to an already-running instance
    {
        QLocalSocket probe;
        probe.connectToServer(kSingleInstanceKey);
        if (probe.waitForConnected(300)) {
            probe.disconnectFromServer();
            return 0;
        }
    }
    QLocalServer singleInstanceServer;
    QLocalServer::removeServer(kSingleInstanceKey);
    singleInstanceServer.listen(kSingleInstanceKey);

    app.setStyle(QStyleFactory::create("Fusion"));

    // Dark palette — forces all widgets to use dark background by default
    QPalette p;
    p.setColor(QPalette::Window,          QColor(0x1E, 0x1E, 0x2E));
    p.setColor(QPalette::WindowText,      QColor(0xC1, 0xC2, 0xC5));
    p.setColor(QPalette::Base,            QColor(0x1E, 0x1E, 0x2E));
    p.setColor(QPalette::AlternateBase,   QColor(0x25, 0x25, 0x38));
    p.setColor(QPalette::Text,            QColor(0xC1, 0xC2, 0xC5));
    p.setColor(QPalette::Button,          QColor(0x25, 0x25, 0x38));
    p.setColor(QPalette::ButtonText,      QColor(0xC1, 0xC2, 0xC5));
    p.setColor(QPalette::BrightText,      QColor(0xFF, 0xFF, 0xFF));
    p.setColor(QPalette::Link,            QColor(0x4D, 0xAB, 0xF7));
    p.setColor(QPalette::Highlight,       QColor(0x4D, 0xAB, 0xF7));
    p.setColor(QPalette::HighlightedText, QColor(0x1E, 0x1E, 0x2E));
    p.setColor(QPalette::ToolTipBase,     QColor(0x25, 0x25, 0x38));
    p.setColor(QPalette::ToolTipText,     QColor(0xC1, 0xC2, 0xC5));
    p.setColor(QPalette::PlaceholderText, QColor(0x55, 0x55, 0x70));
    app.setPalette(p);

    QFile styleFile(":/style.qss");
    if (styleFile.open(QFile::ReadOnly))
        app.setStyleSheet(QString::fromUtf8(styleFile.readAll()));

    MainWindow window;
    window.resize(1000, 700);
    window.setMinimumSize(700, 500);
    window.show();

    return app.exec();
}
