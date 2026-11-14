#include "global.h"
#include "mainwindow.h"

#include <QApplication>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QMessageBox>
#include <QSettings>

int main(int argc, char* argv[]) {
    QApplication application(argc, argv);

    QFile stylesheet(":/style/stylesheet.qss");
    if (stylesheet.open(QFile::ReadOnly)) {
        application.setStyleSheet(QString::fromUtf8(stylesheet.readAll()));
    }

    const auto configPath = QDir(QCoreApplication::applicationDirPath()).filePath("config.ini");
    QSettings settings(configPath, QSettings::IniFormat);
    const auto gateHost = settings.value("GateServer/host").toString().trimmed();
    const auto gatePort = settings.value("GateServer/port").toInt();
    if (gateHost.isEmpty() || gatePort < 1 || gatePort > 65535) {
        QMessageBox::critical(nullptr, QObject::tr("Configuration error"),
            QObject::tr("GateServer host or port is invalid in %1").arg(configPath));
        return EXIT_FAILURE;
    }
    gate_url_prefix = QString("http://%1:%2").arg(gateHost).arg(gatePort);
    qDebug() << "GateServer endpoint:" << gate_url_prefix;

    MainWindow window;
    window.show();
    return application.exec();
}
