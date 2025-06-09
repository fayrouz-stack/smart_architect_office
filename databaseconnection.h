#ifndef DATABASECONNECTION_H
#define DATABASECONNECTION_H

#include <QObject>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QMessageBox>

class DatabaseConnection : public QObject
{
    Q_OBJECT

public:
    explicit DatabaseConnection(QObject *parent = nullptr);
    ~DatabaseConnection();

    // Database operations
    bool initializeDatabase();
    bool executeQuery(const QString &query, QSqlQuery &result);
    bool executeQuery(const QString &query);
    bool createTables();
    bool addSampleData();  // New method for adding sample records

    // Singleton pattern access
    static DatabaseConnection* instance();

    // Utility functions
    QString getDatabasePath() const;

private:
    QSqlDatabase m_db;
    static DatabaseConnection* m_instance;
};

#endif // DATABASECONNECTION_H
