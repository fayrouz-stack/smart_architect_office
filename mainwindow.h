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
#include <QComboBox>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QStandardPaths>
#include <QSystemTrayIcon>
#include <QCalendarWidget>
#include <QSerialPort>
#include <QSerialPortInfo>

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
    void on_addBtn_clicked();
    void on_modifyBtn_clicked();
    void on_deleteBtn_clicked();
    void on_exportBtn_clicked();
    void on_searchBtn_clicked();
    void on_sortBtn_clicked();
    void on_notificationBtn_clicked();

    void handleNavButtonClick(QAbstractButton* clickedButton);
    void on_navTasksBtn_clicked();
    void on_navCalendarBtn_clicked();
    void on_navStatsBtn_clicked();

    void on_showStatusStats_clicked();
    void on_showDurationStats_clicked();

    void on_taskTable_cellActivated(int row, int column);

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    QSqlDatabase db;
    QSystemTrayIcon *trayIcon;
    QCalendarWidget *calendarWidget;
    QSerialPort *arduino;
    QString arduinoPortName;
    bool arduinoIsAvailable;

    bool initializeDatabase();
    void loadTasksFromDatabase();
    void saveTaskToDatabase(const QStringList &taskData);
    void updateTaskInDatabase(const QStringList &taskData, int row);
    void deleteTaskFromDatabase(const QString &taskId);

    void setupTaskTable();
    void setupCalendar();
    void setupSystemTray();
    void setupArduino();
    void checkDeadlineNotifications();

    QChart* createStatusPieChart();
    QChart* createDurationBarChart();

    void updateCharts();
    void sortTasks(int column, Qt::SortOrder order);
    bool validateTaskData(const QList<QLineEdit*>& fields, const QList<QComboBox*>& combos);
    bool validateRowSelection(bool requireSelection = true);
    void showCalendar();
    void readSerialData();
    void sendToArduino(const QString &message);
};

#endif // MAINWINDOW_H
