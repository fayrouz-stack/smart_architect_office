#include "invoice.h"
#include "databaseconnection.h"

bool Invoice::saveToDatabase() const
{
    QSqlQuery query;
    query.prepare("INSERT INTO invoices VALUES (:id, :number, :date, :amount, :status)");
    query.bindValue(":id", id);
    query.bindValue(":number", number);
    query.bindValue(":date", date);
    query.bindValue(":amount", amount);
    query.bindValue(":status", status);

    if(!query.exec()) {
        qCritical() << "Erreur d'insertion :" << query.lastError().text();
        return false;
    }
    return true;
}

bool Invoice::deleteFromDatabase() const
{
    QSqlQuery query;
    query.prepare("DELETE FROM invoices WHERE id = :id");
    query.bindValue(":id", id);
    return DatabaseConnection::instance()->executeQuery(query);
}

QVector<Invoice> Invoice::loadAllFromDatabase()
{
    QVector<Invoice> invoices;
    QSqlQuery query("SELECT * FROM invoices");

    while (query.next()) {
        Invoice inv;
        inv.id = query.value("id").toString();
        inv.number = query.value("number").toString();
        inv.date = query.value("date").toString();
        inv.amount = query.value("amount").toDouble();
        inv.status = query.value("status").toString();
        invoices.append(inv);
    }
    return invoices;
}
