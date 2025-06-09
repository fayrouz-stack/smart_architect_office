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
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QUrlQuery>
#include <QListWidget>
#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QListWidgetItem>
#include <QSqlDatabase>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class ChatDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ChatDialog(QWidget *parent = nullptr);
    void addMessage(const QString &sender, const QString &message, bool isUser);

    // Keep these public for now
    QListWidget *messageList;
    QLineEdit *inputLine;
    QPushButton *sendButton;

signals:  // Add this signals section
    void sendMessage(const QString &message);

private slots:
    void onSendClicked();
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Button click handlers
    void on_addBtn_clicked();
    void on_modifyBtn_clicked();
    void on_deleteBtn_clicked();
    void on_exportBtn_clicked();
    void on_searchBtn_clicked();
    void on_predictBtn_clicked();
    void on_chatbotBtn_clicked();
    void on_showEmployeeStats_clicked();
    void on_showHiringTrends_clicked();
    void on_sortBtn_clicked();

    // Navigation handlers
    void on_navEmployeesBtn_clicked();
    void on_navClientsBtn_clicked();
    void on_navProjectsBtn_clicked();
    void on_navInvoicesBtn_clicked();
    void on_navMaterialsBtn_clicked();
    void on_navTasksBtn_clicked();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    ChatDialog *chatDialog;

    //data base

    bool initializeDatabase();
    bool createEmployeeTable();
    void loadEmployeesFromDatabase();
    void saveEmployeeToDatabase(const QStringList &employeeData);
    void updateEmployeeInDatabase(int row);
    void deleteEmployeeFromDatabase(const QString &id);

    QSqlDatabase database;

    // Helper methods
    void setupEmployeeTable();
    void handleNavButtonClick(QAbstractButton* clickedButton);
    void sortEmployees(int column, Qt::SortOrder order);
    bool validateRowSelection(bool requireSelection = true);
    QString cleanSalaryInput(const QString& salary);
    bool validateEmployeeData(const QList<QLineEdit*>& fields);
    QString predictBestEmployee();
    void updateCharts();
    QChart* createPieChart();
};

#endif // MAINWINDOW_H
