#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "invoice.h"
#include "databaseconnection.h"
#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QButtonGroup>
#include <QPainter>
#include <QFormLayout>
#include <QtCharts/QChart>
#include <QTranslator>

namespace Ui {
class MainWindow; // Déclaration anticipée correcte
}

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_addBtn_clicked();
    void on_modifyBtn_clicked();
    void on_deleteBtn_clicked();
    void on_exportBtn_clicked();
    void on_searchBtn_clicked();
    void on_predictBtn_clicked();
    void switchLanguage();

    void handleNavButtonClick(QAbstractButton* clickedButton);
    void on_navInvoicesBtn_clicked();
    void on_navClientsBtn_clicked();
    void on_navProjectsBtn_clicked();
    void on_navEmployeesBtn_clicked();
    void on_navMaterialsBtn_clicked();
    void on_navTasksBtn_clicked();

    void on_showInvoiceStats_clicked();
    void on_showPaymentTrends_clicked();
    void toggleTheme();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    QVector<Invoice> invoices;
    QTranslator translator;
    void setupInvoiceTable();
    void refreshTable();
    QChart* createPieChart();
    QString predictUnpaidInvoice();
    void updateCharts();
    QPushButton *switchLanguageButton;
    QString translateStatus(const QString &status, bool isFrench);
    QPushButton *themeToggleButton;
    bool isDarkMode = false;
    bool isFrench = false;
};

#endif // MAINWINDOW_H
