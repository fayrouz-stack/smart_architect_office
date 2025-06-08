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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    setWindowTitle("Client Management System");
    resize(1400, 900);

    // Initialize database
    if (!initializeDatabase()) {
        QMessageBox::critical(this, "Database Error", "Failed to initialize database. Some features may not work.");
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
        "#recommendBtn {"
        "   background: #6610f2;"
        "   font-weight: bold;"
        "}"
        "#accessBtn {"
        "   background: #d63384;"
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

    // Navigation buttons setup
    QButtonGroup *navGroup = new QButtonGroup(this);
    navGroup->addButton(ui->navClientsBtn);
    navGroup->addButton(ui->navProjectsBtn);
    navGroup->addButton(ui->navInvoicesBtn);
    navGroup->addButton(ui->navEmployeesBtn);
    navGroup->addButton(ui->navMaterialsBtn);
    navGroup->addButton(ui->navTasksBtn);

    QString navButtonStyle =
        "QPushButton { text-align: left; padding: 12px; color: white; border: none; background: #343a40; font-size: 12px; }"
        "QPushButton:hover { background: #23272b; }"
        "QPushButton[active=true] { background: #2980b9; border-left: 4px solid #ffc107; }";

    ui->navClientsBtn->setProperty("active", true);
    for(auto btn : navGroup->buttons()) {
        btn->setStyleSheet(navButtonStyle);
    }

    connect(navGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::handleNavButtonClick);

    setupClientTable();
    loadClientsFromDatabase();
    setupAccessLevels();
    currentAccessLevel = "Admin"; // Default to Admin for testing
    updateUIForAccessLevel();
}

bool MainWindow::initializeDatabase()
{
    try {
        // Set up database in Documents folder
        QString dbPath = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + "/SmartOffice";

        // Create directory if it doesn't exist
        QDir dir;
        if (!dir.mkpath(dbPath)) {
            throw std::runtime_error("Could not create database directory");
        }

        dbPath += "/client.db";
        qDebug() << "Database path:" << dbPath;

        // Remove any existing connection
        QString connectionName = "ClientConnection";
        if (QSqlDatabase::contains(connectionName)) {
            QSqlDatabase::removeDatabase(connectionName);
        }

        // Create and configure database connection
        db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
        db.setDatabaseName(dbPath);

        // Try to open the database
        if (!db.open()) {
            throw std::runtime_error(QString("Failed to open database: %1").arg(db.lastError().text()).toStdString());
        }

        // Create Clients table if it doesn't exist
        QSqlQuery query(db);
        QString createTableSQL = R"(
            CREATE TABLE IF NOT EXISTS clients (
                id TEXT PRIMARY KEY,
                last_name TEXT NOT NULL,
                first_name TEXT NOT NULL,
                phone TEXT NOT NULL,
                email TEXT NOT NULL,
                project TEXT NOT NULL,
                invoice_amount TEXT NOT NULL,
                deadline TEXT NOT NULL,
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

void MainWindow::loadClientsFromDatabase()
{
    if (!db.isOpen()) return;

    QSqlQuery query("SELECT id, last_name, first_name, phone, email, project, invoice_amount, deadline FROM clients ORDER BY last_name", db);

    ui->clientTable->setRowCount(0); // Clear existing data

    while (query.next()) {
        int row = ui->clientTable->rowCount();
        ui->clientTable->insertRow(row);

        for (int col = 0; col < 8; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());
            ui->clientTable->setItem(row, col, item);
        }
    }
}

void MainWindow::saveClientToDatabase(const QStringList &clientData)
{
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("INSERT INTO clients (id, last_name, first_name, phone, email, project, invoice_amount, deadline) "
                  "VALUES (:id, :last_name, :first_name, :phone, :email, :project, :invoice_amount, :deadline)");

    query.bindValue(":id", clientData[0]);
    query.bindValue(":last_name", clientData[1]);
    query.bindValue(":first_name", clientData[2]);
    query.bindValue(":phone", clientData[3]);
    query.bindValue(":email", clientData[4]);
    query.bindValue(":project", clientData[5]);
    query.bindValue(":invoice_amount", clientData[6]);
    query.bindValue(":deadline", clientData[7]);

    if (!query.exec()) {
        QMessageBox::critical(this, "Database Error",
                              QString("Failed to save client: %1").arg(query.lastError().text()));
    }
}

void MainWindow::updateClientInDatabase(const QStringList &clientData, int row)
{
    if (!db.isOpen()) return;

    QString clientId = ui->clientTable->item(row, 0)->text();

    QSqlQuery query(db);
    query.prepare("UPDATE clients SET "
                  "id = :id, "
                  "last_name = :last_name, "
                  "first_name = :first_name, "
                  "phone = :phone, "
                  "email = :email, "
                  "project = :project, "
                  "invoice_amount = :invoice_amount, "
                  "deadline = :deadline, "
                  "updated_at = CURRENT_TIMESTAMP "
                  "WHERE id = :old_id");

    query.bindValue(":id", clientData[0]);
    query.bindValue(":last_name", clientData[1]);
    query.bindValue(":first_name", clientData[2]);
    query.bindValue(":phone", clientData[3]);
    query.bindValue(":email", clientData[4]);
    query.bindValue(":project", clientData[5]);
    query.bindValue(":invoice_amount", clientData[6]);
    query.bindValue(":deadline", clientData[7]);
    query.bindValue(":old_id", clientId);

    if (!query.exec()) {
        QMessageBox::critical(this, "Database Error",
                              QString("Failed to update client: %1").arg(query.lastError().text()));
    }
}

void MainWindow::deleteClientFromDatabase(const QString &clientId)
{
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("DELETE FROM clients WHERE id = :id");
    query.bindValue(":id", clientId);

    if (!query.exec()) {
        QMessageBox::critical(this, "Database Error",
                              QString("Failed to delete client: %1").arg(query.lastError().text()));
    }
}

void MainWindow::setupClientTable()
{
    ui->clientTable->setColumnCount(8);
    QStringList headers = {"ID", "Nom", "Prénom", "Phone", "Email", "Project", "Invoice ($)", "Deadline"};
    ui->clientTable->setHorizontalHeaderLabels(headers);
    ui->clientTable->horizontalHeader()->setStretchLastSection(true);
    ui->clientTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->clientTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // Style for table
    ui->clientTable->setStyleSheet(
        "QTableView {"
        "   background-color: white;"
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
        "}");
}

void MainWindow::setupAccessLevels()
{
    QMap<QString, QStringList> accessLevels;
    accessLevels["Admin"] = QStringList() << "All";
    accessLevels["Manager"] = QStringList() << "View" << "Add" << "Modify" << "Export";
    accessLevels["Sales"] = QStringList() << "View" << "Add" << "Export";
    accessLevels["Support"] = QStringList() << "View";

    ui->accessCombo->addItems(accessLevels.keys());
    connect(ui->accessCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), [this]() {
        currentAccessLevel = ui->accessCombo->currentText();
        updateUIForAccessLevel();
    });
}

void MainWindow::updateUIForAccessLevel()
{
    bool canAdd = hasAccess("Add");
    bool canModify = hasAccess("Modify");
    bool canDelete = hasAccess("Delete");
    bool canExport = hasAccess("Export");

    ui->addBtn->setEnabled(canAdd);
    ui->modifyBtn->setEnabled(canModify);
    ui->deleteBtn->setEnabled(canDelete);
    ui->exportBtn->setEnabled(canExport);
    ui->sortBtn->setEnabled(hasAccess("View"));
    ui->searchBtn->setEnabled(hasAccess("View"));
    ui->recommendBtn->setEnabled(hasAccess("View"));
    ui->showClientStats->setEnabled(hasAccess("View"));
    ui->showProjectStats->setEnabled(hasAccess("View"));
}

bool MainWindow::hasAccess(const QString& requiredPermission)
{
    if (currentAccessLevel == "Admin") return true;
    if (requiredPermission == "View") return true;

    if (currentAccessLevel == "Manager") {
        return (requiredPermission == "Add" || requiredPermission == "Modify" || requiredPermission == "Export");
    }
    else if (currentAccessLevel == "Sales") {
        return (requiredPermission == "Add" || requiredPermission == "Export");
    }

    return false;
}

void MainWindow::handleNavButtonClick(QAbstractButton* clickedButton)
{
    foreach(QAbstractButton* btn, clickedButton->group()->buttons()) {
        btn->setProperty("active", false);
        btn->style()->polish(btn);
    }
    clickedButton->setProperty("active", true);
    clickedButton->style()->polish(clickedButton);
}

void MainWindow::on_navClientsBtn_clicked() { handleNavButtonClick(ui->navClientsBtn); }
void MainWindow::on_navProjectsBtn_clicked() { handleNavButtonClick(ui->navProjectsBtn); }
void MainWindow::on_navInvoicesBtn_clicked() { handleNavButtonClick(ui->navInvoicesBtn); }
void MainWindow::on_navEmployeesBtn_clicked() { handleNavButtonClick(ui->navEmployeesBtn); }
void MainWindow::on_navMaterialsBtn_clicked() { handleNavButtonClick(ui->navMaterialsBtn); }
void MainWindow::on_navTasksBtn_clicked() { handleNavButtonClick(ui->navTasksBtn); }

QChart* MainWindow::createClientPieChart()
{
    // Count clients by project value
    QMap<QString, int> valueCategories;
    int totalClients = 0;

    for (int row = 0; row < ui->clientTable->rowCount(); ++row) {
        QString invoiceStr = ui->clientTable->item(row, 6)->text(); // Invoice column
        invoiceStr.remove('$').remove(',');
        int invoiceValue = invoiceStr.toInt();

        QString category;
        if (invoiceValue < 20000) category = "Small (<$20k)";
        else if (invoiceValue < 40000) category = "Medium ($20k-$40k)";
        else category = "Large (>$40k)";

        valueCategories[category]++;
        totalClients++;
    }

    // Create pie series
    QPieSeries *series = new QPieSeries();
    for (auto it = valueCategories.begin(); it != valueCategories.end(); ++it) {
        double percentage = (it.value() * 100.0) / totalClients;
        QPieSlice *slice = series->append(
            QString("%1\n%2/%3 (%4%)")
                .arg(it.key())
                .arg(it.value())
                .arg(totalClients)
                .arg(QString::number(percentage, 'f', 1)),
            it.value()
            );

        slice->setLabelVisible();
        slice->setLabelArmLengthFactor(0.3);
        slice->setLabelPosition(QPieSlice::LabelOutside);
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("Client Distribution by Project Value\nTotal Clients: %1").arg(totalClients));
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);

    chart->setTitleFont(QFont("Arial", 12, QFont::Bold));
    chart->legend()->setFont(QFont("Arial", 9));

    return chart;
}

QChart* MainWindow::createProjectBarChart()
{
    // Collect data by project status (based on deadline)
    QMap<QString, int> projectsByStatus;
    QDate today = QDate::currentDate();
    int totalProjects = 0;

    for (int row = 0; row < ui->clientTable->rowCount(); ++row) {
        QTableWidgetItem* item = ui->clientTable->item(row, 7); // Deadline column
        if (item && !item->text().isEmpty()) {
            QDate deadline = QDate::fromString(item->text(), "yyyy-MM-dd");
            if (deadline.isValid()) {
                QString status;
                if (deadline < today) status = "Completed";
                else if (deadline <= today.addDays(30)) status = "Urgent (<30d)";
                else if (deadline <= today.addDays(90)) status = "Active (30-90d)";
                else status = "Future (>90d)";

                projectsByStatus[status]++;
                totalProjects++;
            }
        }
    }

    if (projectsByStatus.isEmpty()) {
        return nullptr;
    }

    // Prepare chart data
    QBarSeries *series = new QBarSeries();
    QBarSet *barSet = new QBarSet("Projects");

    QStringList categories = {"Completed", "Urgent (<30d)", "Active (30-90d)", "Future (>90d)"};
    int maxCount = 0;

    for (const QString& category : categories) {
        int count = projectsByStatus.value(category, 0);
        *barSet << count;
        if (count > maxCount) maxCount = count;
    }

    // Configure chart
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("Project Status Distribution\nTotal Projects: %1").arg(totalProjects));

    series->append(barSet);
    barSet->setColor(QColor(32, 159, 223));

    // Configure axes
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, maxCount + 2);
    axisY->setTitleText("Number of Projects");
    axisY->setLabelFormat("%d");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    return chart;
}

void MainWindow::on_showClientStats_clicked()
{
    if (!hasAccess("View")) return;

    QChart *chart = createClientPieChart();

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Client Statistics");
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

void MainWindow::on_showProjectStats_clicked()
{
    if (!hasAccess("View")) return;

    QChart *chart = createProjectBarChart();
    if (!chart) {
        QMessageBox::warning(this, "No Data", "No valid project deadlines found");
        return;
    }

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Project Status");
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
    if (!hasAccess("Add")) return;

    QDialog dialog(this);
    QFormLayout form(&dialog);
    dialog.setWindowTitle("Add New Client");

    QStringList labels = {"Client ID:", "Nom:", "Prénom:", "Phone:",
                          "Email:", "Project:", "Invoice Amount:", "Deadline (YYYY-MM-DD):"};
    QList<QLineEdit*> fields;

    foreach (const QString &label, labels) {
        QLineEdit *lineEdit = new QLineEdit(&dialog);
        form.addRow(label, lineEdit);
        fields << lineEdit;

        // Set input constraints
        if (label.contains("ID")) {
            lineEdit->setMaxLength(10);
            lineEdit->setPlaceholderText("C001");
        }
        else if (label.contains("Phone")) {
            lineEdit->setInputMask("99999999");
            lineEdit->setPlaceholderText("12345678");
        }
        else if (label.contains("Amount")) {
            lineEdit->setPlaceholderText("25,000");
        }
        else if (label.contains("Date")) {
            lineEdit->setPlaceholderText("2023-12-31");
        }
        else if (label.contains("Email")) {
            lineEdit->setPlaceholderText("contact@company.com");
        }
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted && validateClientData(fields)) {
        QStringList clientData;
        for (int i = 0; i < fields.size(); ++i) {
            QString value = fields[i]->text().trimmed();
            if (i == 6) value = "$" + cleanAmountInput(value);
            clientData << value;
        }

        int row = ui->clientTable->rowCount();
        ui->clientTable->insertRow(row);
        for (int i = 0; i < clientData.size(); ++i) {
            ui->clientTable->setItem(row, i, new QTableWidgetItem(clientData[i]));
        }

        saveClientToDatabase(clientData);
        updateCharts();
    }
}

void MainWindow::on_modifyBtn_clicked() {
    if (!hasAccess("Modify")) return;
    if (!validateRowSelection()) return;

    int row = ui->clientTable->currentRow();
    QDialog dialog(this);
    QFormLayout form(&dialog);
    dialog.setWindowTitle("Edit Client");

    QStringList labels = {"Client ID:", "Company:", "Contact:", "Phone:",
                          "Email:", "Project:", "Invoice Amount:", "Deadline:"};
    QList<QLineEdit*> fields;

    for (int i = 0; i < labels.size(); ++i) {
        QLineEdit *lineEdit = new QLineEdit(ui->clientTable->item(row, i)->text(), &dialog);
        form.addRow(labels[i], lineEdit);
        fields << lineEdit;
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted && validateClientData(fields)) {
        QStringList clientData;
        for (int i = 0; i < fields.size(); ++i) {
            QString value = fields[i]->text().trimmed();
            if (i == 6) value = "$" + cleanAmountInput(value);
            clientData << value;
            ui->clientTable->item(row, i)->setText(value);
        }

        updateClientInDatabase(clientData, row);
        updateCharts();
    }
}

void MainWindow::on_deleteBtn_clicked() {
    if (!hasAccess("Delete")) return;
    if (!validateRowSelection()) return;

    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Confirm Delete");
    msgBox.setText("Are you sure you want to delete this client?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        QString clientId = ui->clientTable->item(ui->clientTable->currentRow(), 0)->text();
        deleteClientFromDatabase(clientId);
        ui->clientTable->removeRow(ui->clientTable->currentRow());
        updateCharts();
    }
}

void MainWindow::on_exportBtn_clicked()
{
    if (!hasAccess("Export")) return;

    QString fileName = QFileDialog::getSaveFileName(this, "Export PDF", "", "PDF Files (*.pdf)");
    if (fileName.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QTextDocument doc;
    QString html = "<h1>Client Report</h1><table border='1'><tr>";

    // Header
    for (int col = 0; col < ui->clientTable->columnCount(); ++col) {
        html += "<th>" + ui->clientTable->horizontalHeaderItem(col)->text() + "</th>";
    }
    html += "</tr>";

    // Data
    for (int row = 0; row < ui->clientTable->rowCount(); ++row) {
        html += "<tr>";
        for (int col = 0; col < ui->clientTable->columnCount(); ++col) {
            html += "<td>" + ui->clientTable->item(row, col)->text() + "</td>";
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

    for (int row = 0; row < ui->clientTable->rowCount(); ++row) {
        bool matchFound = false;
        // Recherche dans ID, Nom, Prénom et Email (colonnes 0,1,2,4)
        for (int col : {0, 1, 2, 4}) {
            if (ui->clientTable->item(row, col)->text().toLower().contains(searchText)) {
                matchFound = true;
                break;
            }
        }
        ui->clientTable->setRowHidden(row, !matchFound);
    }
}

QList<QPair<int, double>> MainWindow::calculateClientScores()
{
    QList<QPair<int, double>> scores;
    QDate today = QDate::currentDate();

    for (int row = 0; row < ui->clientTable->rowCount(); ++row) {
        // Get invoice amount
        QString amountStr = ui->clientTable->item(row, 6)->text();
        amountStr.remove('$').remove(',');
        double amount = amountStr.toDouble();

        // Get deadline date
        QDate deadline = QDate::fromString(ui->clientTable->item(row, 7)->text(), "yyyy-MM-dd");
        int daysRemaining = today.daysTo(deadline);

        // Calculate score components
        double amountScore = amount * 0.00005; // Normalize amount
        double urgencyScore = 1000.0 / (daysRemaining + 1); // Higher for more urgent
        double projectSizeScore = (amount > 30000) ? 50 : (amount > 15000 ? 30 : 10);

        // Combine scores with weights
        double totalScore = (amountScore * 0.4) + (urgencyScore * 0.5) + (projectSizeScore * 0.1);

        scores.append(qMakePair(row, totalScore));
    }

    // Sort by score (highest first)
    std::sort(scores.begin(), scores.end(), [](const QPair<int, double>& a, const QPair<int, double>& b) {
        return a.second > b.second;
    });

    return scores;
}

QString MainWindow::generateRecommendationDetails(int row, double score)
{
    QString details;
    QTableWidgetItem *item;

    // Client info
    details += QString("<h3>%1 %2</h3>")
                   .arg(ui->clientTable->item(row, 1)->text())  // Nom
                   .arg(ui->clientTable->item(row, 2)->text()); // Prénom

    // Project info
    details += QString("<p><b>Project:</b> %1</p>")
                   .arg(ui->clientTable->item(row, 5)->text());

    // Financial info
    details += QString("<p><b>Invoice Amount:</b> %1</p>")
                   .arg(ui->clientTable->item(row, 6)->text());

    // Deadline info with color coding
    QDate deadline = QDate::fromString(ui->clientTable->item(row, 7)->text(), "yyyy-MM-dd");
    QDate today = QDate::currentDate();
    int daysRemaining = today.daysTo(deadline);

    QString deadlineColor = (daysRemaining < 15) ? "red" : (daysRemaining < 30 ? "orange" : "green");
    details += QString("<p><b>Deadline:</b> <span style='color:%1;'>%2 (%3 days remaining)</span></p>")
                   .arg(deadlineColor)
                   .arg(deadline.toString("MMMM d, yyyy"))
                   .arg(daysRemaining);

    // Contact info
    details += QString("<p><b>Contact:</b> %1<br>")
                   .arg(ui->clientTable->item(row, 4)->text());
    details += QString("<b>Phone:</b> %1</p>")
                   .arg(ui->clientTable->item(row, 3)->text());

    // Score info
    details += QString("<p><b>Priority Score:</b> %1/100</p>")
                   .arg(QString::number(score, 'f', 1));

    // Recommendation reason
    QString reason;
    if (score > 80) reason = "High priority - Large project with urgent deadline";
    else if (score > 60) reason = "Medium priority - Important project nearing deadline";
    else reason = "Standard priority - Monitor progress";

    details += QString("<p><b>Recommendation:</b> %1</p>").arg(reason);

    return details;
}

void MainWindow::on_recommendBtn_clicked()
{
    if (!hasAccess("View")) return;

    if (ui->clientTable->rowCount() == 0) {
        QMessageBox::information(this, "Info", "No clients to analyze");
        return;
    }

    // Calculate scores for all clients
    QList<QPair<int, double>> clientScores = calculateClientScores();

    // Create recommendation dialog
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Client Recommendations");
    dialog->resize(600, 500);

    QTabWidget *tabWidget = new QTabWidget(dialog);

    // Create tabs for top 3 recommendations
    for (int i = 0; i < qMin(3, clientScores.size()); ++i) {
        int row = clientScores[i].first;
        double score = clientScores[i].second;

        QWidget *tab = new QWidget();
        QVBoxLayout *tabLayout = new QVBoxLayout(tab);

        QLabel *content = new QLabel(generateRecommendationDetails(row, score));
        content->setWordWrap(true);
        content->setTextFormat(Qt::RichText);

        tabLayout->addWidget(content);
        tab->setLayout(tabLayout);

        tabWidget->addTab(tab, QString("Recommendation #%1").arg(i+1));
    }

    // Add full list as another tab
    QWidget *fullListTab = new QWidget();
    QVBoxLayout *fullListLayout = new QVBoxLayout(fullListTab);

    QTableWidget *scoreTable = new QTableWidget();
    scoreTable->setColumnCount(3);
    scoreTable->setHorizontalHeaderLabels({"Client", "Project", "Score"});
    scoreTable->setRowCount(clientScores.size());

    for (int i = 0; i < clientScores.size(); ++i) {
        int row = clientScores[i].first;

        QTableWidgetItem *clientItem = new QTableWidgetItem(
            ui->clientTable->item(row, 1)->text() + " " + ui->clientTable->item(row, 2)->text());
        QTableWidgetItem *projectItem = new QTableWidgetItem(ui->clientTable->item(row, 5)->text());
        QTableWidgetItem *scoreItem = new QTableWidgetItem(QString::number(clientScores[i].second, 'f', 1));

        scoreTable->setItem(i, 0, clientItem);
        scoreTable->setItem(i, 1, projectItem);
        scoreTable->setItem(i, 2, scoreItem);
    }

    scoreTable->resizeColumnsToContents();
    fullListLayout->addWidget(scoreTable);
    fullListTab->setLayout(fullListLayout);

    tabWidget->addTab(fullListTab, "All Clients");

    QPushButton *closeBtn = new QPushButton("Close", dialog);
    connect(closeBtn, &QPushButton::clicked, dialog, &QDialog::accept);

    QVBoxLayout *mainLayout = new QVBoxLayout(dialog);
    mainLayout->addWidget(tabWidget);
    mainLayout->addWidget(closeBtn, 0, Qt::AlignRight);

    dialog->exec();
    delete dialog;
}


void MainWindow::on_accessBtn_clicked()
{
    QString message;

    if (currentAccessLevel == "Admin") {
        message = "You have full access to all client management functions.";
    }
    else if (currentAccessLevel == "Manager") {
        message = "You can view, add, modify, and export client data.";
    }
    else if (currentAccessLevel == "Sales") {
        message = "You can view, add, and export client data.";
    }
    else {
        message = "You have read-only access to client data.";
    }

    QMessageBox::information(this, "Access Level", message);
}

void MainWindow::updateCharts()
{
    // Close any open stats dialogs
    QList<QDialog*> dialogs = findChildren<QDialog*>();
    foreach (QDialog* dialog, dialogs) {
        if (dialog->windowTitle() == "Client Statistics" ||
            dialog->windowTitle() == "Project Status") {
            dialog->close();
            dialog->deleteLater();
        }
    }
}

void MainWindow::on_sortBtn_clicked()
{
    if (!hasAccess("View")) return;

    QMenu sortMenu;

    // Create sort actions
    QAction *sortByIdAsc = sortMenu.addAction("By ID (Ascending)");
    QAction *sortByIdDesc = sortMenu.addAction("By ID (Descending)");
    QAction *sortByDeadlineAsc = sortMenu.addAction("By Deadline (Soonest)");
    QAction *sortByDeadlineDesc = sortMenu.addAction("By Deadline (Farthest)");

    // Connect actions to sorting
    connect(sortByIdAsc, &QAction::triggered, [this]() { sortClients(0, Qt::AscendingOrder); });
    connect(sortByIdDesc, &QAction::triggered, [this]() { sortClients(0, Qt::DescendingOrder); });
    connect(sortByDeadlineAsc, &QAction::triggered, [this]() { sortClients(7, Qt::AscendingOrder); });
    connect(sortByDeadlineDesc, &QAction::triggered, [this]() { sortClients(7, Qt::DescendingOrder); });

    // Show the menu at the button position
    sortMenu.exec(ui->sortBtn->mapToGlobal(QPoint(0, ui->sortBtn->height())));
}

void MainWindow::sortClients(int column, Qt::SortOrder order)
{
    // Special handling for deadline dates
    if (column == 7) {
        for (int row = 0; row < ui->clientTable->rowCount(); ++row) {
            QTableWidgetItem *item = ui->clientTable->item(row, 7);
            QDate date = QDate::fromString(item->text(), "yyyy-MM-dd");
            if (date.isValid()) {
                item->setData(Qt::UserRole, date);
            }
        }
    }

    ui->clientTable->sortItems(column, order);
}

bool MainWindow::validateRowSelection(bool requireSelection) {
    if (requireSelection && ui->clientTable->currentRow() < 0) {
        QMessageBox::warning(this, "Selection Required", "Please select a client first");
        return false;
    }
    return true;
}

QString MainWindow::cleanAmountInput(const QString& amount) {
    return amount.trimmed().remove('$').remove(',');
}

bool MainWindow::validateClientData(const QList<QLineEdit*>& fields) {
    // Field names for error messages
    const QStringList fieldNames = {
        "Client ID", "Nom", "Prénom", "Phone",
        "Email", "Project", "Invoice Amount", "Deadline"
    };

    // 1. Check for empty fields
    for (int i = 0; i < fields.size(); ++i) {
        if (fields[i]->text().trimmed().isEmpty()) {
            QMessageBox::warning(this, "Missing Data",
                                 QString("%1 cannot be empty").arg(fieldNames[i]));
            fields[i]->setFocus();
            return false;
        }
    }

    // 2. Validate ID (format CXXX)
    QRegularExpression idRegex("^C\\d{3}$");
    if (!idRegex.match(fields[0]->text()).hasMatch()) {
        QMessageBox::warning(this, "Invalid ID",
                             "Client ID must be in format C followed by 3 digits (e.g., C001)");
        fields[0]->setFocus();
        return false;
    }

    // 3. Validate Contact Name (letters only)
    QRegularExpression nameRegex("^[a-zA-Z\\s]+$");
    if (!nameRegex.match(fields[2]->text()).hasMatch()) {
        QMessageBox::warning(this, "Invalid Contact Name",
                             "Contact name must contain only letters");
        fields[2]->setFocus();
        return false;
    }

    // 4. Validate Phone
    QRegularExpression phoneRegex("^\\d{8}$"); // Exactement 8 chiffres
    if (!phoneRegex.match(fields[3]->text()).hasMatch()) {
        QMessageBox::warning(this, "Invalid Phone",
                             "Le téléphone doit contenir exactement 8 chiffres");
        fields[3]->setFocus();
        return false;
    }
    // 5. Validate Email (basic check)
    QRegularExpression emailRegex(R"(^[a-zA-Z0-9._%+-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$)");
    if (!emailRegex.match(fields[4]->text()).hasMatch()) {
        QMessageBox::warning(this, "Invalid Email",
                             "Please enter a valid email address (user@domain.com)");
        fields[4]->setFocus();
        return false;
    }

    // 6. Validate Invoice Amount (formats: $25,000 or 25000 or 25000.00)
    QRegularExpression amountRegex(R"(^\$?\d{1,3}(,\d{3})*(\.\d{2})?$)");
    QString amount = fields[6]->text().trimmed();
    if (!amountRegex.match(amount).hasMatch()) {
        QMessageBox::warning(this, "Invalid Amount",
                             "Amount must be in format like:\n$25,000 or 25000 or 25000.00");
        fields[6]->setFocus();
        return false;
    }

    // 7. Validate Deadline (YYYY-MM-DD and not in past)
    QDate deadline = QDate::fromString(fields[7]->text(), "yyyy-MM-dd");
    if (!deadline.isValid()) {
        QMessageBox::warning(this, "Invalid Date",
                             "Deadline must be in YYYY-MM-DD format");
        fields[7]->setFocus();
        return false;
    }
    if (deadline < QDate::currentDate()) {
        QMessageBox::warning(this, "Invalid Date",
                             "Deadline cannot be in the past");
        fields[7]->setFocus();
        return false;
    }

    // 8. Check for duplicate ID (only for new entries)
    if (ui->clientTable->currentRow() < 0) { // Only check for new entries
        QString newId = fields[0]->text().trimmed();
        for (int row = 0; row < ui->clientTable->rowCount(); ++row) {
            if (ui->clientTable->item(row, 0)->text() == newId) {
                QMessageBox::warning(this, "Duplicate ID",
                                     "A client with this ID already exists");
                fields[0]->setFocus();
                return false;
            }
        }
    }

    return true; // All validations passed
}

MainWindow::~MainWindow()
{
    if (db.isOpen()) {
        db.close();
    }
    delete ui;
    delete networkManager;
}
