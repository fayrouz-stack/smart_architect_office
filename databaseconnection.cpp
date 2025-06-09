#include "databaseconnection.h"
#include <QMessageBox>

DatabaseConnection* DatabaseConnection::m_instance = nullptr;

DatabaseConnection::DatabaseConnection(QObject *parent) : QObject(parent)
{
    m_db = QSqlDatabase::addDatabase("QSQLITE");
}

DatabaseConnection::~DatabaseConnection()
{
    if (m_db.isOpen()) {
        m_db.close();
    }
    m_instance = nullptr;
}

DatabaseConnection* DatabaseConnection::instance()
{
    if (!m_instance) {
        m_instance = new DatabaseConnection();
    }
    return m_instance;
}

QString DatabaseConnection::getDatabasePath() const
{
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir dir(appDataPath);
    if (!dir.exists()) {
        dir.mkpath(appDataPath);
    }
    return appDataPath + "/architecture_management.db";
}

bool DatabaseConnection::initializeDatabase()
{
    if (!QSqlDatabase::drivers().contains("QSQLITE")) {
        qCritical() << "SQLite driver not available! Available drivers:" << QSqlDatabase::drivers();
        QMessageBox::critical(nullptr, "Error", "SQLite driver not available!");
        return false;
    }

    QString dbPath = getDatabasePath();
    qDebug() << "Database path:" << QDir::toNativeSeparators(dbPath);

    m_db.setDatabaseName(dbPath);

    if (!m_db.open()) {
        QString error = QString("Cannot open database:\n%1\nPath: %2")
        .arg(m_db.lastError().text())
            .arg(QDir::toNativeSeparators(dbPath));
        qCritical() << error;
        QMessageBox::critical(nullptr, "Database Error", error);
        return false;
    }

    qDebug() << "Database connected successfully";
    return createTables();
}
bool DatabaseConnection::createTables()
{
    QSqlQuery query;
    QStringList tables;

    if (query.exec("SELECT name FROM sqlite_master WHERE type='table'")) {
        while (query.next()) {
            tables << query.value(0).toString();
        }
    }

    if (!tables.contains("materials")) {
        QString createTable =
            "CREATE TABLE materials ("
            "id TEXT PRIMARY KEY,"
            "name TEXT NOT NULL,"
            "category TEXT NOT NULL,"
            "supplier TEXT NOT NULL,"
            "unit_cost REAL NOT NULL,"
            "unit_type TEXT NOT NULL,"
            "stock_quantity INTEGER NOT NULL,"
            "image_path TEXT)";

        if (!executeQuery(createTable)) {
            return false;
        }
        qDebug() << "Created materials table";
    }

    return true;
}

bool DatabaseConnection::addSampleData()
{
    QSqlQuery checkQuery("SELECT COUNT(*) FROM materials");
    if (checkQuery.next() && checkQuery.value(0).toInt() > 0) {
        qDebug() << "Database already contains data - skipping sample data";
        return true;
    }

    QStringList sampleMaterials = {
        "INSERT INTO materials VALUES ('MAT-001', 'Plywood Sheet', 'Wood', 'Timber Inc', 45.99, 'Sheet', 25, '')",
        "INSERT INTO materials VALUES ('MAT-002', 'Steel Beam', 'Metal', 'MetalWorks', 89.50, 'Piece', 12, '')",
        "INSERT INTO materials VALUES ('MAT-003', 'Concrete Block', 'Construction', 'BuildRite', 3.20, 'Unit', 150, '')",
        "INSERT INTO materials VALUES ('MAT-004', 'Glass Panel', 'Glass', 'ClearView', 120.00, 'Panel', 8, '')",
        "INSERT INTO materials VALUES ('MAT-005', 'Paint Bucket', 'Finishing', 'ColorCoat', 32.75, 'Bucket', 30, '')"
    };

    foreach (const QString &sql, sampleMaterials) {
        if (!executeQuery(sql)) {
            qWarning() << "Failed to insert sample data:" << sql;
            return false;
        }
    }

    qDebug() << "Successfully added sample data";
    return true;
}

bool DatabaseConnection::executeQuery(const QString &query, QSqlQuery &result)
{
    if (!result.exec(query)) {
        qCritical() << "Query failed:" << query;
        qCritical() << "Error:" << result.lastError().text();
        return false;
    }
    return true;
}

bool DatabaseConnection::executeQuery(const QString &query)
{
    QSqlQuery q;
    return executeQuery(query, q);
}
