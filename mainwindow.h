#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QButtonGroup>
#include <QPainter>
#include <QFormLayout>
#include <QtCharts/QChart>
#include <QMenu>
#include <QLineEdit>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Client management slots
    void on_addBtn_clicked();
    void on_modifyBtn_clicked();
    void on_deleteBtn_clicked();
    void on_exportBtn_clicked();
    void on_searchBtn_clicked();
    void on_recommendBtn_clicked();
    void on_sortBtn_clicked();
    void on_accessBtn_clicked();

    // Navigation slots
    void handleNavButtonClick(QAbstractButton* clickedButton);
    void on_navClientsBtn_clicked();
    void on_navProjectsBtn_clicked();
    void on_navInvoicesBtn_clicked();
    void on_navEmployeesBtn_clicked();
    void on_navMaterialsBtn_clicked();
    void on_navTasksBtn_clicked();

    // Statistics slots
    void on_showClientStats_clicked();
    void on_showProjectStats_clicked();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    QString currentAccessLevel;
    QSqlDatabase db;

    // Database methods
    bool initializeDatabase();
    void loadClientsFromDatabase();
    void saveClientToDatabase(const QStringList &clientData);
    void updateClientInDatabase(const QStringList &clientData, int row);
    void deleteClientFromDatabase(const QString &clientId);

    // Initialization methods
    void setupClientTable();
    void updateUIForAccessLevel();

    // Chart creation methods
    QChart* createClientPieChart();
    QChart* createProjectBarChart();
    QList<QPair<int, double>> calculateClientScores();
    // Helper methods
    QString generateRecommendationDetails(int row, double score);
    void updateCharts();
    void sortClients(int column, Qt::SortOrder order);
    bool validateClientData(const QList<QLineEdit*>& fields);
    bool validateRowSelection(bool requireSelection = true);
    QString cleanAmountInput(const QString& amount);
    void setupAccessLevels();
    bool hasAccess(const QString& requiredPermission);
};

#endif // MAINWINDOW_H
