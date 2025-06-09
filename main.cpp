#include "mainwindow.h"
#include "databaseconnection.h"
#include <QApplication>
#include <QMessageBox>
#include <QDebug>
#include <QPluginLoader>  // Add this include for QPluginLoader
#include <QSqlDatabase>   // Add this include for QSqlDatabase

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Set organization info for QSettings
    QCoreApplication::setOrganizationName("YourCompany");
    QCoreApplication::setApplicationName("MaterialManagementSystem");

    a.setStyle("Fusion");

    // Debug plugin paths
    qDebug() << "Library paths:" << QCoreApplication::libraryPaths();

    // Force load SQLite driver
    QPluginLoader loader("C:/Qt/6.9.0/mingw_64/plugins/sqldrivers/qsqlite.dll");
    if (!loader.load()) {
        QMessageBox::critical(nullptr, "Error",
                              "Failed to load SQLite driver:\n" + loader.errorString());
        return -1;
    }

    qDebug() << "Available drivers:" << QSqlDatabase::drivers();

    // Initialize database
    if (!DatabaseConnection::instance()->initializeDatabase()) {
        QMessageBox::critical(nullptr, "Error", "Failed to initialize database!");
        return -1;
    }

    MainWindow w;
    w.show();
    return a.exec();
}
