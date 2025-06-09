#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QPrinter>
#include <QTextDocument>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QVBoxLayout>
#include <QLabel>
#include <QButtonGroup>
#include <QtCharts/QChartView>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QLineSeries>
#include <QComboBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QDate>
#include <QCalendarWidget>
#include <QSystemTrayIcon>
#include <QAction>
#include <QMenu>

// Identifiants USB pour Arduino
const quint16 arduino_uno_vendor_id = 9025;
const quint16 arduino_uno_product_id = 67;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    networkManager(new QNetworkAccessManager(this)),
    trayIcon(nullptr),
    calendarWidget(nullptr),
    arduino(nullptr),
    arduinoIsAvailable(false)
{
    ui->setupUi(this);
    setWindowTitle("Task Management System");
    resize(1400, 900);

    if (!initializeDatabase()) {
        QMessageBox::critical(this, "Database Error", "Failed to initialize database.");
    }

    // Style setup
    this->setStyleSheet(
        "QMainWindow {"
        "   background: #f8f9fa;"
        "   font-family: 'Segoe UI', Arial, sans-serif;"
        "}"
        "QPushButton {"
        "   color: white;"
        "   padding: 8px 12px;"
        "   border-radius: 4px;"
        "   border: none;"
        "   min-width: 80px;"
        "   font-weight: 500;"
        "   transition: background 0.2s ease;"
        "}"
        "QPushButton:hover {"
        "   opacity: 0.9;"
        "}"
        "QPushButton:pressed {"
        "   padding: 9px 12px 7px 12px;"
        "}"
        "QPushButton[active=true] {"
        "   background: #007bff;"
        "   border-left: 4px solid #ffc107;"
        "}"
        "#addBtn {"
        "   background: #28a745;"
        "}"
        "#addBtn:hover {"
        "   background: #218838;"
        "}"
        "#modifyBtn {"
        "   background: #17a2b8;"
        "}"
        "#modifyBtn:hover {"
        "   background: #138496;"
        "}"
        "#deleteBtn {"
        "   background: #dc3545;"
        "}"
        "#deleteBtn:hover {"
        "   background: #c82333;"
        "}"
        "#exportBtn {"
        "   background: #6f42c1;"
        "}"
        "#exportBtn:hover {"
        "   background: #5a32a3;"
        "}"
        "#sortBtn {"
        "   background: #fd7e14;"
        "}"
        "#sortBtn:hover {"
        "   background: #e36209;"
        "}"
        "#searchBtn {"
        "   background: #20c997;"
        "}"
        "#searchBtn:hover {"
        "   background: #17a589;"
        "}"
        "#notificationBtn {"
        "   background: #17a2b8;"
        "   font-weight: bold;"
        "}"
        "QTableView {"
        "   background: white;"
        "   alternate-background-color: #f8f9fa;"
        "   gridline-color: #dee2e6;"
        "   border: 1px solid #dee2e6;"
        "   border-radius: 5px;"
        "   selection-background-color: #007bff;"
        "   selection-color: white;"
        "}"
        "QHeaderView::section {"
        "   background-color: #343a40;"
        "   color: white;"
        "   padding: 8px;"
        "   border: none;"
        "   font-weight: bold;"
        "}"
        "QLineEdit {"
        "   padding: 8px;"
        "   border: 1px solid #ced4da;"
        "   border-radius: 4px;"
        "   background: white;"
        "}"
        "QLineEdit:focus {"
        "   border: 1px solid #80bdff;"
        "   outline: none;"
        "}"
        "QComboBox {"
        "   padding: 8px;"
        "   border: 1px solid #ced4da;"
        "   border-radius: 4px;"
        "   background: white;"
        "}"
        "QDialogButtonBox QPushButton {"
        "   background-color: black;"
        "   color: white;"
        "   padding: 5px 10px;"
        "   border: 1px solid #555;"
        "   border-radius: 3px;"
        "}"
        "QDialogButtonBox QPushButton:hover {"
        "   background-color: #333;"
        "}");

    // Configuration des boutons de navigation
    QButtonGroup *navGroup = new QButtonGroup(this);
    navGroup->addButton(ui->navTasksBtn);
    navGroup->addButton(ui->navCalendarBtn);
    navGroup->addButton(ui->navStatsBtn);

    QString navButtonStyle =
        "QPushButton { text-align: left; padding: 12px; color: white; border: none; background: #343a40; font-size: 12px; }"
        "QPushButton:hover { background: #23272b; }"
        "QPushButton[active=true] { background: #2980b9; border-left: 4px solid #ffc107; }";

    ui->navTasksBtn->setProperty("active", true);
    for(auto btn : navGroup->buttons()) {
        btn->setStyleSheet(navButtonStyle);
    }

    connect(navGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::handleNavButtonClick);

    connect(ui->notificationBtn, &QPushButton::clicked,
            this, &MainWindow::on_notificationBtn_clicked);

    setupTaskTable();
    loadTasksFromDatabase();
    setupCalendar();
    setupSystemTray();
    setupArduino();
}

void MainWindow::setupArduino()
{
    arduinoIsAvailable = false;
    arduinoPortName = "";
    arduino = new QSerialPort(this);

    // Recherche du port Arduino
    foreach(const QSerialPortInfo &serialPortInfo, QSerialPortInfo::availablePorts()) {
        if(serialPortInfo.hasVendorIdentifier() && serialPortInfo.hasProductIdentifier()) {
            if(serialPortInfo.vendorIdentifier() == arduino_uno_vendor_id &&
                serialPortInfo.productIdentifier() == arduino_uno_product_id) {
                arduinoPortName = serialPortInfo.portName();
                arduinoIsAvailable = true;
            }
        }
    }

    if(arduinoIsAvailable) {
        // Configuration du port série
        arduino->setPortName(arduinoPortName);
        arduino->open(QSerialPort::ReadWrite);
        arduino->setBaudRate(QSerialPort::Baud9600);
        arduino->setDataBits(QSerialPort::Data8);
        arduino->setParity(QSerialPort::NoParity);
        arduino->setStopBits(QSerialPort::OneStop);
        arduino->setFlowControl(QSerialPort::NoFlowControl);

        connect(arduino, &QSerialPort::readyRead, this, &MainWindow::readSerialData);

        QMessageBox::information(this, "Arduino Connected",
                                 QString("Connected to Arduino on %1").arg(arduinoPortName));
    } else {
        QMessageBox::warning(this, "Arduino Error",
                             "Could not find Arduino. Check connection and try again.");
    }
}

void MainWindow::readSerialData()
{
    while (arduino->canReadLine()) {
        QString data = arduino->readLine().trimmed();

        if (data == "TASK_COMPLETED") {
            int row = ui->taskTable->currentRow();
            if (row >= 0) {
                ui->taskTable->item(row, 3)->setText("Completed");
                updateTaskInDatabase({
                                         ui->taskTable->item(row, 0)->text(),
                                         ui->taskTable->item(row, 1)->text(),
                                         ui->taskTable->item(row, 2)->text(),
                                         "Completed",
                                         ui->taskTable->item(row, 4)->text(),
                                         ui->taskTable->item(row, 5)->text(),
                                         ui->taskTable->item(row, 6)->text(),
                                         ui->taskTable->item(row, 7)->text()
                                     }, row);

                QMessageBox::information(this, "Task Completed",
                                         "Current task marked as completed via Arduino");
            }
        }
    }
}

void MainWindow::sendToArduino(const QString &message)
{
    if (arduinoIsAvailable && arduino->isWritable()) {
        arduino->write(message.toUtf8());
        arduino->write("\n");
    }
}

bool MainWindow::initializeDatabase()
{
    try {
        QString dbPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/TaskManager";
        QDir dir;
        if (!dir.mkpath(dbPath)) {
            throw std::runtime_error("Could not create database directory");
        }

        dbPath += "/tasks.db";
        qDebug() << "Database path:" << dbPath;

        QString connectionName = "TaskConnection";
        if (QSqlDatabase::contains(connectionName)) {
            QSqlDatabase::removeDatabase(connectionName);
        }

        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(dbPath);

        if (!db.open()) {
            throw std::runtime_error(QString("Failed to open database: %1").arg(db.lastError().text()).toStdString());
        }

        QSqlQuery query(db);
        QString createTableSQL = R"(
            CREATE TABLE IF NOT EXISTS tasks (
                id TEXT PRIMARY KEY,
                name TEXT NOT NULL,
                description TEXT,
                status TEXT NOT NULL,
                priority TEXT NOT NULL,
                start_date TEXT NOT NULL,
                end_date TEXT NOT NULL,
                assigned_to TEXT,
                created_at DATETIME DEFAULT CURRENT_TIMESTAMP,
                updated_at DATETIME DEFAULT CURRENT_TIMESTAMP
            )
        )";

        if (!query.exec(createTableSQL)) {
            throw std::runtime_error(QString("Failed to create table: %1").arg(query.lastError().text()).toStdString());
        }

        qDebug() << "Database initialized successfully";
        return true;

    } catch (const std::exception& e) {
        qCritical() << "Database initialization error:" << e.what();
        QMessageBox::critical(nullptr, "Database Error",
                              QString("Failed to initialize database: %1").arg(e.what()));
        return false;
    } catch (...) {
        qCritical() << "Unknown error during database initialization";
        QMessageBox::critical(nullptr, "Database Error",
                              "An unknown error occurred while initializing database");
        return false;
    }
}

void MainWindow::loadTasksFromDatabase()
{
    if (!db.isOpen()) return;

    QSqlQuery query("SELECT id, name, description, status, priority, start_date, end_date, assigned_to FROM tasks ORDER BY end_date", db);

    ui->taskTable->setRowCount(0); // Clear existing data

    while (query.next()) {
        int row = ui->taskTable->rowCount();
        ui->taskTable->insertRow(row);

        for (int col = 0; col < 8; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());
            ui->taskTable->setItem(row, col, item);
        }
    }
}

void MainWindow::saveTaskToDatabase(const QStringList &taskData)
{
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("INSERT INTO tasks (id, name, description, status, priority, start_date, end_date, assigned_to) "
                  "VALUES (:id, :name, :description, :status, :priority, :start_date, :end_date, :assigned_to)");

    query.bindValue(":id", taskData[0]);
    query.bindValue(":name", taskData[1]);
    query.bindValue(":description", taskData[2]);
    query.bindValue(":status", taskData[3]);
    query.bindValue(":priority", taskData[4]);
    query.bindValue(":start_date", taskData[5]);
    query.bindValue(":end_date", taskData[6]);
    query.bindValue(":assigned_to", taskData[7]);

    if (!query.exec()) {
        QMessageBox::critical(this, "Database Error",
                              QString("Failed to save task: %1").arg(query.lastError().text()));
    }
}

void MainWindow::updateTaskInDatabase(const QStringList &taskData, int row)
{
    if (!db.isOpen()) return;

    QString taskId = ui->taskTable->item(row, 0)->text();

    QSqlQuery query(db);
    query.prepare("UPDATE tasks SET "
                  "id = :id, "
                  "name = :name, "
                  "description = :description, "
                  "status = :status, "
                  "priority = :priority, "
                  "start_date = :start_date, "
                  "end_date = :end_date, "
                  "assigned_to = :assigned_to, "
                  "updated_at = CURRENT_TIMESTAMP "
                  "WHERE id = :old_id");

    query.bindValue(":id", taskData[0]);
    query.bindValue(":name", taskData[1]);
    query.bindValue(":description", taskData[2]);
    query.bindValue(":status", taskData[3]);
    query.bindValue(":priority", taskData[4]);
    query.bindValue(":start_date", taskData[5]);
    query.bindValue(":end_date", taskData[6]);
    query.bindValue(":assigned_to", taskData[7]);
    query.bindValue(":old_id", taskId);

    if (!query.exec()) {
        QMessageBox::critical(this, "Database Error",
                              QString("Failed to update task: %1").arg(query.lastError().text()));
    }
}

void MainWindow::deleteTaskFromDatabase(const QString &taskId)
{
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("DELETE FROM tasks WHERE id = :id");
    query.bindValue(":id", taskId);

    if (!query.exec()) {
        QMessageBox::critical(this, "Database Error",
                              QString("Failed to delete task: %1").arg(query.lastError().text()));
    }
}

void MainWindow::setupTaskTable()
{
    ui->taskTable->setColumnCount(8);
    QStringList headers = {"ID", "Name", "Description", "Status", "Priority", "Start Date", "End Date", "Assigned To"};
    ui->taskTable->setHorizontalHeaderLabels(headers);
    ui->taskTable->horizontalHeader()->setStretchLastSection(true);
    ui->taskTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->taskTable->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MainWindow::setupCalendar()
{
    calendarWidget = new QCalendarWidget();
    calendarWidget->setWindowFlags(Qt::Window);
    calendarWidget->setWindowTitle("Task Calendar");
    calendarWidget->resize(600, 400);
}

void MainWindow::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        qWarning() << "System tray not available on this system";
        return;
    }

    if (trayIcon) {
        delete trayIcon;
    }

    trayIcon = new QSystemTrayIcon(this);

    if (QIcon::hasThemeIcon("task")) {
        trayIcon->setIcon(QIcon::fromTheme("task"));
    } else {
        QPixmap pixmap(32, 32);
        pixmap.fill(Qt::transparent);
        QPainter painter(&pixmap);
        painter.setPen(Qt::black);
        painter.setBrush(Qt::blue);
        painter.drawEllipse(2, 2, 28, 28);
        painter.setPen(Qt::white);
        painter.drawText(pixmap.rect(), Qt::AlignCenter, "T");
        trayIcon->setIcon(QIcon(pixmap));
    }

    trayIcon->setToolTip("Task Manager");

    QMenu *trayMenu = new QMenu(this);
    trayMenu->addAction("Show", this, &QWidget::showNormal);
    trayMenu->addAction("Quit", qApp, &QCoreApplication::quit);
    trayIcon->setContextMenu(trayMenu);

    trayIcon->show();
}

void MainWindow::on_notificationBtn_clicked()
{
    checkDeadlineNotifications();
}

void MainWindow::checkDeadlineNotifications()
{
    if (!trayIcon || !trayIcon->isVisible()) {
        QMessageBox::warning(this, "Notifications", "System tray not available. Notifications will not be shown.");
        return;
    }

    QDate today = QDate::currentDate();
    QDate tomorrow = today.addDays(1);
    int nearDeadlineCount = 0;
    QStringList nearDeadlineTasks;

    for (int row = 0; row < ui->taskTable->rowCount(); ++row) {
        QTableWidgetItem* endDateItem = ui->taskTable->item(row, 6);
        if (endDateItem && !endDateItem->text().isEmpty()) {
            QDate endDate = QDate::fromString(endDateItem->text(), "yyyy-MM-dd");
            if (endDate.isValid()) {
                QString taskName = ui->taskTable->item(row, 1)->text();
                QString status = ui->taskTable->item(row, 3)->text();

                if (endDate == today) {
                    nearDeadlineTasks << QString("• Today: %1 (Status: %2)").arg(taskName, status);
                    nearDeadlineCount++;
                } else if (endDate == tomorrow) {
                    nearDeadlineTasks << QString("• Tomorrow: %1 (Status: %2)").arg(taskName, status);
                    nearDeadlineCount++;
                } else if (endDate < today && status != "Completed") {
                    nearDeadlineTasks << QString("• Overdue: %1 (Status: %2)").arg(taskName, status);
                    nearDeadlineCount++;
                }
            }
        }
    }

    if (nearDeadlineCount > 0) {
        QString message = QString("You have %1 task(s) with deadlines:\n%2")
        .arg(nearDeadlineCount)
            .arg(nearDeadlineTasks.join("\n"));

        trayIcon->showMessage("Task Deadlines",
                              message,
                              QSystemTrayIcon::Information,
                              10000);

        // Envoyer une notification à l'Arduino
        sendToArduino("NOTIFICATION");
    } else {
        trayIcon->showMessage("Task Deadlines",
                              "No upcoming deadlines found",
                              QSystemTrayIcon::Information,
                              3000);
    }
}

void MainWindow::handleNavButtonClick(QAbstractButton* clickedButton)
{
    foreach(QAbstractButton* btn, clickedButton->group()->buttons()) {
        btn->setProperty("active", false);
        btn->style()->polish(btn);
    }
    clickedButton->setProperty("active", true);
    clickedButton->style()->polish(clickedButton);

    if (clickedButton == ui->navCalendarBtn) {
        showCalendar();
    }
}

void MainWindow::on_navTasksBtn_clicked()
{
    handleNavButtonClick(ui->navTasksBtn);
}

void MainWindow::on_navCalendarBtn_clicked()
{
    handleNavButtonClick(ui->navCalendarBtn);
}

void MainWindow::on_navStatsBtn_clicked()
{
    handleNavButtonClick(ui->navStatsBtn);
}

QChart* MainWindow::createStatusPieChart()
{
    QMap<QString, int> statusCounts;
    int totalTasks = 0;

    for (int row = 0; row < ui->taskTable->rowCount(); ++row) {
        QString status = ui->taskTable->item(row, 3)->text();
        statusCounts[status]++;
        totalTasks++;
    }

    if (totalTasks == 0) {
        return nullptr;
    }

    QPieSeries *series = new QPieSeries();
    for (auto it = statusCounts.begin(); it != statusCounts.end(); ++it) {
        double percentage = (it.value() * 100.0) / totalTasks;
        QPieSlice *slice = series->append(
            QString("%1\n%2/%3 (%4%)")
                .arg(it.key())
                .arg(it.value())
                .arg(totalTasks)
                .arg(QString::number(percentage, 'f', 1)),
            it.value()
            );

        slice->setLabelVisible();
        slice->setLabelArmLengthFactor(0.3);
        slice->setLabelPosition(QPieSlice::LabelOutside);
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("Task Distribution by Status\nTotal Tasks: %1").arg(totalTasks));
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);

    chart->setTitleFont(QFont("Arial", 12, QFont::Bold));
    chart->legend()->setFont(QFont("Arial", 9));

    return chart;
}

QChart* MainWindow::createDurationBarChart()
{
    QMap<QString, int> durationCategories;
    int totalTasks = 0;

    for (int row = 0; row < ui->taskTable->rowCount(); ++row) {
        QTableWidgetItem* startItem = ui->taskTable->item(row, 5);
        QTableWidgetItem* endItem = ui->taskTable->item(row, 6);

        if (startItem && endItem && !startItem->text().isEmpty() && !endItem->text().isEmpty()) {
            QDate startDate = QDate::fromString(startItem->text(), "yyyy-MM-dd");
            QDate endDate = QDate::fromString(endItem->text(), "yyyy-MM-dd");

            if (startDate.isValid() && endDate.isValid()) {
                int days = startDate.daysTo(endDate) + 1;

                QString category;
                if (days <= 1) category = "1 day";
                else if (days <= 7) category = "2-7 days";
                else if (days <= 30) category = "1-4 weeks";
                else category = "1+ months";

                durationCategories[category]++;
                totalTasks++;
            }
        }
    }

    if (durationCategories.isEmpty()) {
        return nullptr;
    }

    QBarSeries *series = new QBarSeries();
    QBarSet *barSet = new QBarSet("Tasks");

    QStringList categories = {"1 day", "2-7 days", "1-4 weeks", "1+ months"};
    int maxCount = 0;

    for (const QString& category : categories) {
        int count = durationCategories.value(category, 0);
        *barSet << count;
        if (count > maxCount) maxCount = count;
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("Task Duration Distribution\nTotal Tasks: %1").arg(totalTasks));

    series->append(barSet);
    barSet->setColor(QColor(32, 159, 223));

    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, maxCount + 2);
    axisY->setTitleText("Number of Tasks");
    axisY->setLabelFormat("%d");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    return chart;
}

void MainWindow::on_showStatusStats_clicked()
{
    QChart *chart = createStatusPieChart();
    if (!chart) {
        QMessageBox::warning(this, "No Data", "No tasks available for statistics");
        return;
    }

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Task Status Statistics");
    dialog->resize(800, 600);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QPushButton *exportBtn = new QPushButton("Export as Image", dialog);
    exportBtn->setStyleSheet(
        "QPushButton {"
        "   background: #3498db;"
        "   color: white;"
        "   padding: 8px;"
        "   border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "   background: #2980b9;"
        "}"
        );

    connect(exportBtn, &QPushButton::clicked, [chartView]() {
        QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                        "Save Chart as Image", "", "PNG Images (*.png);;JPEG Images (*.jpg *.jpeg)");

        if (!fileName.isEmpty()) {
            QPixmap p = chartView->grab();
            p.save(fileName);
            QMessageBox::information(nullptr, "Success", "Chart exported successfully!");
        }
    });

    QPushButton *closeBtn = new QPushButton("Close", dialog);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "   background: #e74c3c;"
        "   color: white;"
        "   padding: 8px;"
        "   border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "   background: #c0392b;"
        "}"
        );

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(exportBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->addWidget(chartView);
    mainLayout->addLayout(buttonLayout);

    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    dialog->exec();
    delete dialog;
}

void MainWindow::on_showDurationStats_clicked()
{
    QChart *chart = createDurationBarChart();
    if (!chart) {
        QMessageBox::warning(this, "No Data", "No tasks with valid dates available");
        return;
    }

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Task Duration Statistics");
    dialog->resize(900, 600);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QPushButton *exportBtn = new QPushButton("Export as Image", dialog);
    exportBtn->setStyleSheet(
        "QPushButton {"
        "   background: #3498db;"
        "   color: white;"
        "   padding: 8px;"
        "   border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "   background: #2980b9;"
        "}"
        );

    QPushButton *closeBtn = new QPushButton("Close", dialog);
    closeBtn->setStyleSheet(
        "QPushButton {"
        "   background: #e74c3c;"
        "   color: white;"
        "   padding: 8px;"
        "   border-radius: 4px;"
        "}"
        "QPushButton:hover {"
        "   background: #c0392b;"
        "}"
        );

    connect(exportBtn, &QPushButton::clicked, [chartView]() {
        QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                        "Save Chart", "", "PNG Images (*.png);;JPEG Images (*.jpg *.jpeg)");
        if (!fileName.isEmpty()) {
            QPixmap p = chartView->grab();
            p.save(fileName);
            QMessageBox::information(nullptr, "Success", "Chart exported successfully!");
        }
    });

    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    QHBoxLayout *buttonLayout = new QHBoxLayout();
    buttonLayout->addWidget(exportBtn);
    buttonLayout->addStretch();
    buttonLayout->addWidget(closeBtn);

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->addWidget(chartView);
    mainLayout->addLayout(buttonLayout);

    dialog->exec();
    delete dialog;
}

void MainWindow::on_addBtn_clicked() {
    QDialog dialog(this);
    QFormLayout form(&dialog);
    dialog.setWindowTitle("Add New Task");

    QStringList labels = {"Task ID:", "Name:", "Description:", "Status:", "Priority:",
                          "Start Date (YYYY-MM-DD):", "End Date (YYYY-MM-DD):", "Assigned To:"};
    QList<QLineEdit*> fields;
    QList<QComboBox*> combos;

    for (int i = 0; i < labels.size(); ++i) {
        if (i == 3 || i == 4) {
            QComboBox *combo = new QComboBox(&dialog);
            if (i == 3) {
                combo->addItems({"Not Started", "In Progress", "Completed", "On Hold"});
            } else {
                combo->addItems({"Low", "Medium", "High", "Critical"});
            }
            form.addRow(labels[i], combo);
            combos << combo;
        } else {
            QLineEdit *lineEdit = new QLineEdit(&dialog);
            form.addRow(labels[i], lineEdit);
            fields << lineEdit;

            if (i == 0) {
                lineEdit->setMaxLength(10);
                lineEdit->setPlaceholderText("T001");
            }
            else if (i == 5 || i == 6) {
                lineEdit->setPlaceholderText("2023-12-31");
            }
        }
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted && validateTaskData(fields, combos)) {
        QStringList taskData;
        for (int i = 0; i < fields.size(); ++i) {
            taskData << fields[i]->text().trimmed();
        }
        taskData.insert(3, combos[0]->currentText());
        taskData.insert(4, combos[1]->currentText());

        int row = ui->taskTable->rowCount();
        ui->taskTable->insertRow(row);
        for (int i = 0; i < taskData.size(); ++i) {
            ui->taskTable->setItem(row, i, new QTableWidgetItem(taskData[i]));
        }

        saveTaskToDatabase(taskData);
        updateCharts();
    }
}

void MainWindow::on_modifyBtn_clicked() {
    if (!validateRowSelection()) return;

    int row = ui->taskTable->currentRow();
    QDialog dialog(this);
    QFormLayout form(&dialog);
    dialog.setWindowTitle("Edit Task");

    QStringList labels = {"Task ID:", "Name:", "Description:", "Status:", "Priority:",
                          "Start Date:", "End Date:", "Assigned To:"};
    QList<QLineEdit*> fields;
    QList<QComboBox*> combos;

    for (int i = 0; i < labels.size(); ++i) {
        if (i == 3 || i == 4) {
            QComboBox *combo = new QComboBox(&dialog);
            if (i == 3) {
                combo->addItems({"Not Started", "In Progress", "Completed", "On Hold"});
                combo->setCurrentText(ui->taskTable->item(row, i)->text());
            } else {
                combo->addItems({"Low", "Medium", "High", "Critical"});
                combo->setCurrentText(ui->taskTable->item(row, i)->text());
            }
            form.addRow(labels[i], combo);
            combos << combo;
        } else {
            QLineEdit *lineEdit = new QLineEdit(ui->taskTable->item(row, i)->text(), &dialog);
            form.addRow(labels[i], lineEdit);
            fields << lineEdit;
        }
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted && validateTaskData(fields, combos)) {
        QStringList taskData;
        for (int i = 0; i < fields.size(); ++i) {
            taskData << fields[i]->text().trimmed();
        }
        taskData.insert(3, combos[0]->currentText());
        taskData.insert(4, combos[1]->currentText());

        for (int i = 0; i < taskData.size(); ++i) {
            ui->taskTable->item(row, i)->setText(taskData[i]);
        }

        updateTaskInDatabase(taskData, row);
        updateCharts();
    }
}

void MainWindow::on_deleteBtn_clicked() {
    if (!validateRowSelection()) return;

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Confirm Delete");
    msgBox.setText("Are you sure you want to delete this task?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        QString taskId = ui->taskTable->item(ui->taskTable->currentRow(), 0)->text();
        deleteTaskFromDatabase(taskId);
        ui->taskTable->removeRow(ui->taskTable->currentRow());
        updateCharts();
    }
}

void MainWindow::on_exportBtn_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Export PDF", "", "PDF Files (*.pdf)");
    if (fileName.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QTextDocument doc;
    QString html = "<h1>Task Report</h1><table border='1'><tr>";

    for (int col = 0; col < ui->taskTable->columnCount(); ++col) {
        html += "<th>" + ui->taskTable->horizontalHeaderItem(col)->text() + "</th>";
    }
    html += "</tr>";

    for (int row = 0; row < ui->taskTable->rowCount(); ++row) {
        html += "<tr>";
        for (int col = 0; col < ui->taskTable->columnCount(); ++col) {
            html += "<td>" + ui->taskTable->item(row, col)->text() + "</td>";
        }
        html += "</tr>";
    }

    html += "</table>";
    doc.setHtml(html);
    doc.print(&printer);

    QMessageBox::information(this, "Success", "PDF exported successfully");
}

void MainWindow::on_searchBtn_clicked() {
    QString searchText = ui->searchInput->text().trimmed().toLower();

    for (int row = 0; row < ui->taskTable->rowCount(); ++row) {
        bool matchFound = false;
        for (int col : {0, 1}) {
            if (ui->taskTable->item(row, col)->text().toLower().contains(searchText)) {
                matchFound = true;
                break;
            }
        }
        ui->taskTable->setRowHidden(row, !matchFound);
    }
}

void MainWindow::on_sortBtn_clicked()
{
    QMenu sortMenu;

    QAction *sortByIdAsc = sortMenu.addAction("By ID (Ascending)");
    QAction *sortByIdDesc = sortMenu.addAction("By ID (Descending)");
    QAction *sortByStartDateAsc = sortMenu.addAction("By Start Date (Oldest)");
    QAction *sortByStartDateDesc = sortMenu.addAction("By Start Date (Newest)");
    QAction *sortByEndDateAsc = sortMenu.addAction("By End Date (Soonest)");
    QAction *sortByEndDateDesc = sortMenu.addAction("By End Date (Farthest)");

    connect(sortByIdAsc, &QAction::triggered, [this]() { sortTasks(0, Qt::AscendingOrder); });
    connect(sortByIdDesc, &QAction::triggered, [this]() { sortTasks(0, Qt::DescendingOrder); });
    connect(sortByStartDateAsc, &QAction::triggered, [this]() { sortTasks(5, Qt::AscendingOrder); });
    connect(sortByStartDateDesc, &QAction::triggered, [this]() { sortTasks(5, Qt::DescendingOrder); });
    connect(sortByEndDateAsc, &QAction::triggered, [this]() { sortTasks(6, Qt::AscendingOrder); });
    connect(sortByEndDateDesc, &QAction::triggered, [this]() { sortTasks(6, Qt::DescendingOrder); });

    sortMenu.exec(ui->sortBtn->mapToGlobal(QPoint(0, ui->sortBtn->height())));
}

void MainWindow::sortTasks(int column, Qt::SortOrder order)
{
    if (column == 5 || column == 6) {
        for (int row = 0; row < ui->taskTable->rowCount(); ++row) {
            QTableWidgetItem *item = ui->taskTable->item(row, column);
            QDate date = QDate::fromString(item->text(), "yyyy-MM-dd");
            if (date.isValid()) {
                item->setData(Qt::UserRole, date);
            }
        }
    }

    ui->taskTable->sortItems(column, order);
}

void MainWindow::showCalendar()
{
    QTextCharFormat defaultFormat;
    calendarWidget->setDateTextFormat(QDate(), defaultFormat);

    for (int row = 0; row < ui->taskTable->rowCount(); ++row) {
        QTableWidgetItem* startItem = ui->taskTable->item(row, 5);
        QTableWidgetItem* endItem = ui->taskTable->item(row, 6);

        if (startItem && endItem && !startItem->text().isEmpty() && !endItem->text().isEmpty()) {
            QDate startDate = QDate::fromString(startItem->text(), "yyyy-MM-dd");
            QDate endDate = QDate::fromString(endItem->text(), "yyyy-MM-dd");

            if (startDate.isValid() && endDate.isValid()) {
                QString status = ui->taskTable->item(row, 3)->text();
                QColor color;

                if (status == "Completed") color = QColor(40, 167, 69);
                else if (status == "In Progress") color = QColor(23, 162, 184);
                else if (status == "On Hold") color = QColor(108, 117, 125);
                else color = QColor(220, 53, 69);

                QTextCharFormat format;
                format.setBackground(color);
                format.setForeground(Qt::white);

                for (QDate date = startDate; date <= endDate; date = date.addDays(1)) {
                    calendarWidget->setDateTextFormat(date, format);
                }
            }
        }
    }

    calendarWidget->show();
}

bool MainWindow::validateRowSelection(bool requireSelection) {
    if (requireSelection && ui->taskTable->currentRow() < 0) {
        QMessageBox::warning(this, "Selection Required", "Please select a task first");
        return false;
    }
    return true;
}

bool MainWindow::validateTaskData(const QList<QLineEdit*>& fields, const QList<QComboBox*>& combos) {
    const QStringList fieldNames = {
        "Task ID", "Name", "Description", "Status", "Priority",
        "Start Date", "End Date", "Assigned To"
    };

    for (int i = 0; i < fields.size(); ++i) {
        if (fields[i]->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Missing Data",
                                 QString("%1 cannot be empty").arg(fieldNames[i]));
            fields[i]->setFocus();
            return false;
        }
    }

    QRegularExpression idRegex("^T\\d{3}$");
    if (!idRegex.match(fields[0]->text()).hasMatch()) {
        QMessageBox::warning(this, "Invalid ID",
                             "Task ID must be in format T followed by 3 digits (e.g., T001)");
        fields[0]->setFocus();
        return false;
    }

    QDate startDate = QDate::fromString(fields[4]->text(), "yyyy-MM-dd");
    if (!startDate.isValid()) {
        QMessageBox::warning(this, "Invalid Start Date",
                             "Start date must be in YYYY-MM-DD format");
        fields[4]->setFocus();
        return false;
    }

    QDate endDate = QDate::fromString(fields[5]->text(), "yyyy-MM-dd");
    if (!endDate.isValid()) {
        QMessageBox::warning(this, "Invalid End Date",
                             "End date must be in YYYY-MM-DD format");
        fields[5]->setFocus();
        return false;
    }

    if (endDate < startDate) {
        QMessageBox::warning(this, "Invalid Date Range",
                             "End date cannot be before start date");
        fields[5]->setFocus();
        return false;
    }

    if (ui->taskTable->currentRow() < 0) {
        QString newId = fields[0]->text().trimmed();
        for (int row = 0; row < ui->taskTable->rowCount(); ++row) {
            if (ui->taskTable->item(row, 0)->text() == newId) {
                QMessageBox::warning(this, "Duplicate ID",
                                     "A task with this ID already exists");
                fields[0]->setFocus();
                return false;
            }
        }
    }

    return true;
}

void MainWindow::updateCharts()
{
    QList<QDialog*> dialogs = findChildren<QDialog*>();
    foreach (QDialog* dialog, dialogs) {
        if (dialog->windowTitle() == "Task Status Statistics" ||
            dialog->windowTitle() == "Task Duration Statistics") {
            dialog->close();
            dialog->deleteLater();
        }
    }
}

void MainWindow::on_taskTable_cellActivated(int row, int column)
{
    // Implement if needed
}

MainWindow::~MainWindow()
{
    if (db.isOpen()) {
        db.close();
    }
    delete ui;
    delete networkManager;
    delete calendarWidget;
    if (arduino && arduino->isOpen()) {
        arduino->close();
        delete arduino;
    }
    if (trayIcon) {
        trayIcon->hide();
        delete trayIcon;
    }
}
