#ifndef INVOICE_H
#define INVOICE_H

#include <QString>

struct Invoice {
    QString id;
    QString number;
    QString date;
    double amount;
    QString status;
    bool saveToDatabase() const;
    bool deleteFromDatabase() const;
    static QVector<Invoice> loadAllFromDatabase();

};

#endif // INVOICE_H
