#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtNetwork/QNetworkAccessManager>
#include <QButtonGroup>
#include <QPainter>
#include <QFormLayout>
#include <QMenu>
#include <QLineEdit>
#include <QtSql/QSqlDatabase>
#include <QtSql/QSqlQuery>
#include <QStandardPaths>
#include <QtCharts/QChart>

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
    // Boutons principaux
    void on_addBtn_clicked();
    void on_modifyBtn_clicked();
    void on_deleteBtn_clicked();
    void on_exportBtn_clicked();
    void on_searchBtn_clicked();
    void on_sortBtn_clicked();
    void on_notifyClientBtn_clicked();
    void on_showMapBtn_clicked();

    // Navigation
    void on_navClientsBtn_clicked();
    void on_navProjectsBtn_clicked();
    void on_navInvoicesBtn_clicked();
    void on_navEmployeesBtn_clicked();
    void on_navMaterialsBtn_clicked();
    void on_navTasksBtn_clicked();
    void handleNavButtonClick(QAbstractButton* clickedButton);

    // Statistiques
    void on_showProjectStats_clicked();
    void on_showBudgetStats_clicked();
    void updateCharts();

    // Tri
    void sortProjects(int column, Qt::SortOrder order);

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    QSqlDatabase db;
    // Chart creation methods
    QChart* createProjectStatusChart();
    QChart* createBudgetChart();

    // Chart display methods
    void showProjectStatusStats();
    void showBudgetStats();
    // Initialisation
    void setupUI();
    void setupDatabase();
    void setupProjectTable();
    void updateUIForAccessLevel();

    // Gestion base de données
    bool initializeDatabase();
    void loadProjectsFromDatabase();
    bool saveProjectToDatabase(const QStringList &projectData);  // Changé de void à bool
    bool updateProjectInDatabase(const QStringList &projectData, int row);  // Changé de void à bool
    void deleteProjectFromDatabase(const QString &projectId);
    void checkDatabaseVersion();



    // Utilitaires
    bool validateProjectData(const QList<QLineEdit*>& fields);
    bool validateRowSelection(bool requireSelection = true);
    QString cleanAmountInput(const QString& amount);
    void sendNotificationEmail(const QString &clientEmail,
                               const QString &clientName,
                               const QString &projectName,
                               const QString &projectStatus,
                               const QString &projectId                                                                                                                        );
};

#endif // MAINWINDOW_H
