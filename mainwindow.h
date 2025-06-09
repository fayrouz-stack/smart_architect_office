#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QAbstractButton>
#include <QSqlDatabase>
#include <QStatusBar>       // Add this
#include <QFormLayout>      // Add this
#include <QSqlQuery>
#include <QtCharts/QChartView>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QSettings>
#include <QPushButton>


QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

struct Material {
    QString id;
    QString name;
    QString category;
    QString supplier;
    double unitCost;
    QString unitType;
    int stockQuantity;
    QString imagePath;

    bool saveToDatabase() const;
    bool deleteFromDatabase() const;
    static QVector<Material> loadAllFromDatabase();
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_navMaterialsBtn_clicked();
    void on_navClientsBtn_clicked();
    void on_navProjectsBtn_clicked();
    void on_navInvoicesBtn_clicked();
    void on_navTasksBtn_clicked();
    void on_addBtn_clicked();
    void on_modifyBtn_clicked();
    void on_deleteBtn_clicked();
    void on_exportBtn_clicked();
    void on_searchBtn_clicked();
    void on_importImageBtn_clicked();
    void on_orderBtn_clicked();
    void toggleTheme();

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;
    QVector<Material> materials;
    QSettings* settings;
    bool darkTheme;

    void setupNavigation();
    void setupMaterialsTable();
    void setupStatsPanel();
    void loadDataFromDatabase();
    void addSampleDataToDatabase();
    void updateCharts();
    void ensureScrollable();

    QChartView* createCostChart();
    QChartView* createStockChart();
    void handleNavButtonClick(QAbstractButton *clickedButton);
    void applyTheme(bool dark);
    void setupThemeToggle();
    void updateThemeToggleIcon(QPushButton* btn);  // Add this line

};

#endif // MAINWINDOW_H
