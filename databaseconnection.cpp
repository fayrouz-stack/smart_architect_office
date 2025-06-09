#include "databaseconnection.h"

DatabaseConnection* DatabaseConnection::m_instance = nullptr;
bool DatabaseConnection::executeQuery(const QString &query)
{
    QSqlQuery q;
    return executeQuery(query, q);
}

bool DatabaseConnection::executeQuery(const QString &query, QSqlQuery &result)
{
    if (!result.exec(query)) {
        qCritical() << "Échec de la requête:" << query;
        qCritical() << "Erreur SQL:" << result.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseConnection::executeQuery(QSqlQuery &query)
{
    if (!query.exec()) {
        qCritical() << "Requête préparée échouée:" << query.lastQuery();
        qCritical() << "Erreur:" << query.lastError().text();
        return false;
    }
    return true;
}

DatabaseConnection::DatabaseConnection(QObject *parent) : QObject(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
}

DatabaseConnection::~DatabaseConnection()
{
    if (m_db.isOpen()) m_db.close();
    m_instance = nullptr;
}

DatabaseConnection* DatabaseConnection::instance()
{
    if (!m_instance) m_instance = new DatabaseConnection();
    return m_instance;
}

QString DatabaseConnection::getDatabasePath() const
{
    QString path = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(path);
    if (!dir.exists()) dir.mkpath(path);
    return path + "/invoices.db";
}

bool DatabaseConnection::initializeDatabase()
{
    m_db.setDatabaseName(getDatabasePath());

    if (!m_db.open()) {
        qCritical() << "Erreur d'ouverture de la base :" << m_db.lastError().text();
        return false;
    }

    QSqlQuery query;
    return query.exec(
        "CREATE TABLE IF NOT EXISTS invoices ("
        "id TEXT PRIMARY KEY,"
        "number TEXT NOT NULL,"
        "date TEXT NOT NULL,"
        "amount REAL NOT NULL,"
        "status TEXT NOT NULL)"
        );
}

bool DatabaseConnection::addSampleData()
{
    QStringList sampleInvoices = {
        "INSERT INTO invoices VALUES ('001', 'INV-2023-001', '2023-01-15', 1500.00, 'Paid')",
        "INSERT INTO invoices VALUES ('002', 'INV-2023-002', '2023-02-20', 2450.50, 'Pending')",
    };

    foreach (const QString &sql, sampleInvoices) {
        if (!executeQuery(sql)) return false;
    }
    return true;
}
