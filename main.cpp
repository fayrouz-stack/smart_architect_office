#include <QApplication>
#include "mainwindow.h"
#include <QTranslator>
#include <QLocale>
#include <QDebug>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    QTranslator translator;
    QString locale = QLocale::system().name();
    qDebug() << "System Locale:" << locale;
    translator.load(":/translations/app_" + locale + ".qm");
    app.installTranslator(&translator);

    MainWindow mainWindow;
    mainWindow.show();
    return app.exec();
}

