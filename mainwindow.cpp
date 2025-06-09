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
#include <QScrollArea>
#include <QTimer>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>





ChatDialog::ChatDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle("HR Assistant Chat");
    resize(500, 600);
    setStyleSheet("background: #f5f7fa;");

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(12, 12, 12, 12);
    layout->setSpacing(8);

    // Message List
    messageList = new QListWidget(this);
    messageList->setStyleSheet(
        "QListWidget {"
        "   background: white;"
        "   border: 1px solid #e1e5eb;"
        "   border-radius: 8px;"
        "   padding: 5px;"
        "}"
        "QListWidget::item {"
        "   border: none;"
        "   padding: 0;"
        "   margin: 4px 0;"
        "   background: transparent;"
        "}"
        );
    messageList->setSpacing(4);

    // Input Area
    QHBoxLayout *inputLayout = new QHBoxLayout();
    inputLine = new QLineEdit(this);
    inputLine->setPlaceholderText("Type your message here...");
    inputLine->setStyleSheet(
        "QLineEdit {"
        "   border: 1px solid #d1d5db;"
        "   border-radius: 18px;"
        "   padding: 8px 16px;"
        "   font-size: 14px;"
        "   background: white;"
        "}"
        );

    sendButton = new QPushButton("Send", this);
    sendButton->setStyleSheet(
        "QPushButton {"
        "   background: #3b82f6;"
        "   color: white;"
        "   border: none;"
        "   border-radius: 18px;"
        "   padding: 8px 20px;"
        "   font-weight: 500;"
        "}"
        );

    inputLayout->addWidget(inputLine);
    inputLayout->addWidget(sendButton);

    layout->addWidget(messageList);
    layout->addLayout(inputLayout);

    connect(sendButton, &QPushButton::clicked, this, &ChatDialog::onSendClicked);
    connect(inputLine, &QLineEdit::returnPressed, this, &ChatDialog::onSendClicked);
}

void ChatDialog::addMessage(const QString &sender, const QString &message, bool isUser)
{
    // Create the list item
    QListWidgetItem *item = new QListWidgetItem();
    item->setSizeHint(QSize(messageList->width() - 20, QFontMetrics(font()).height() * (message.count('\n') + 3) + 20));
    messageList->addItem(item);

    // Create the bubble widget
    QWidget *bubble = new QWidget();
    QVBoxLayout *bubbleLayout = new QVBoxLayout(bubble);
    bubbleLayout->setContentsMargins(12, 8, 12, 8);
    bubbleLayout->setSpacing(4);

    // Sender label
    QLabel *senderLabel = new QLabel(sender + ":", bubble);
    senderLabel->setStyleSheet(QString(
        "font-weight: bold;"
        "color: %1;"
    ).arg(isUser ? "white" : "black"));

    // Message label
    QLabel *messageLabel = new QLabel(message, bubble);
    messageLabel->setStyleSheet(QString(
        "color: %1;"
    ).arg(isUser ? "white" : "black"));
    messageLabel->setWordWrap(true);
    messageLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

    // Add to layout
    bubbleLayout->addWidget(senderLabel);
    bubbleLayout->addWidget(messageLabel);

    // Style the bubble
    bubble->setStyleSheet(QString(
        "background: %1;"
        "border-radius: 12px;"
        "margin-%2: 40px;"
        "margin-%3: 0px;"
    ).arg(
        isUser ? "#3b82f6" : "#e5e7eb",
        isUser ? "left" : "right",
        isUser ? "right" : "left"
    ));

    // Set the widget
    messageList->setItemWidget(item, bubble);
    messageList->scrollToBottom();
}

void ChatDialog::onSendClicked()
{
    QString message = inputLine->text().trimmed();
    if (!message.isEmpty()) {
        emit sendMessage(message);  // This will now work
        addMessage("You", message, true);
        inputLine->clear();
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    networkManager(new QNetworkAccessManager(this)),
    chatDialog(new ChatDialog(this))
{
    ui->setupUi(this);
    connect(chatDialog, &ChatDialog::sendMessage, this, &MainWindow::on_chatbotBtn_clicked);
    setWindowTitle("Employee Management System");
    resize(1400, 900);
    connect(ui->showSalaryStats, &QPushButton::clicked,
            this, &MainWindow::on_showHiringTrends_clicked);

    if (!initializeDatabase()) {
        QMessageBox::critical(this, "Database Error", "Failed to initialize database");
        return;
    }

    // Style setup
    this->setStyleSheet(
        // Base window styling
        "QMainWindow {"
        "   background: #f8f9fa;"
        "   font-family: 'Segoe UI', Arial, sans-serif;"
        "}"

        // Base button styling (applies to all buttons)
        "QPushButton {"
        "   color: white;"
        "   padding: 8px 12px;"
        "   border-radius: 4px;"
        "   border: none;"
        "   min-width: 80px;"
        "   font-weight: 500;"
        "   transition: background 0.2s ease;"
        "}"

        // Button hover state
        "QPushButton:hover {"
        "   opacity: 0.9;"
        "}"

        // Button pressed state
        "QPushButton:pressed {"
        "   padding: 9px 12px 7px 12px;"  // Subtle push effect
        "}"

        // Active navigation button
        "QPushButton[active=true] {"
        "   background: #007bff;"
        "   border-left: 4px solid #ffc107;"
        "}"

        // CRUD Buttons - Specific colors
        "#addBtn {"
        "   background: #28a745;"  // Green
        "}"
        "#addBtn:hover {"
        "   background: #218838;"
        "}"

        "#modifyBtn {"
        "   background: #17a2b8;"  // Teal
        "}"
        "#modifyBtn:hover {"
        "   background: #138496;"
        "}"

        "#deleteBtn {"
        "   background: #dc3545;"  // Red
        "}"
        "#deleteBtn:hover {"
        "   background: #c82333;"
        "}"

        "#exportBtn {"
        "   background: #6f42c1;"  // Purple
        "}"
        "#exportBtn:hover {"
        "   background: #5a32a3;"
        "}"

        "#sortBtn {"
        "   background: #fd7e14;"  // Orange
        "}"
        "#sortBtn:hover {"
        "   background: #e36209;"
        "}"

        "#searchBtn {"
        "   background: #20c997;"  // Green-blue
        "}"
        "#searchBtn:hover {"
        "   background: #17a589;"
        "}"

        "#predictBtn {"
        "   background: #6610f2;"  // Dark purple
        "   font-weight: bold;"
        "}"

        "#chatbotBtn {"
        "   background: #d63384;"  // Pink
        "   font-weight: bold;"
        "}"

        // Table styling
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

        // Input fields
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

        // Dialog buttons
        "QDialogButtonBox QPushButton {"
        "   min-width: 80px;"
        "   padding: 8px;"
        "   border-radius: 4px;"
        "   color: white;"
        "}"
        "QDialogButtonBox QPushButton[text=\"OK\"],"
        "QDialogButtonBox QPushButton[text=\"Save\"],"
        "QDialogButtonBox QPushButton[text=\"Add\"],"
        "QDialogButtonBox QPushButton[text=\"Yes\"] {"
        "   background: #28a745;"  // Green
        "}"
        "QDialogButtonBox QPushButton[text=\"Cancel\"],"
        "QDialogButtonBox QPushButton[text=\"Close\"],"
        "QDialogButtonBox QPushButton[text=\"No\"] {"
        "   background: #dc3545;"  // Red
        "}"

        // Message box styling - UPDATED VERSION
        "QMessageBox {"
        "   background: #f8f9fa;"
        "   font-size: 14px;"
        "}"
        "QMessageBox QLabel {"
        "   color: #212529;"
        "   qproperty-alignment: AlignCenter;"
        "}"
        "QMessageBox QPushButton {"
        "   min-width: 80px !important;"
        "   min-height: 30px !important;"
        "   padding: 8px 16px !important;"
        "   margin: 4px !important;"
        "   border-radius: 4px !important;"
        "   color: white !important;"
        "   font-weight: 500 !important;"
        "   background-color: #6c757d !important;"  // Default gray
        "}"
        "QMessageBox QPushButton[text=\"OK\"],"
        "QMessageBox QPushButton[text=\"Yes\"] {"
        "   background-color: #28a745 !important;"  // Green
        "}"
        "QMessageBox QPushButton[text=\"Cancel\"],"
        "QMessageBox QPushButton[text=\"No\"] {"
        "   background-color: #dc3545 !important;"  // Red
        "}"
        "QMessageBox QPushButton:hover {"
        "   opacity: 0.9 !important;"
        "}"
        );


    // Special button styles
    ui->predictBtn->setStyleSheet("background: #28a745; font-weight: bold;");
    ui->chatbotBtn->setStyleSheet("background: #17a2b8; font-weight: bold;");
    ui->showEmployeeStats->setStyleSheet("background: #6f42c1; color: white;");
    ui->showSalaryStats->setText("Hiring Trends");
    ui->showSalaryStats->setStyleSheet("background: #20c997; color: white;");
    ui->sortBtn->setStyleSheet("background: #2980b9; color: white; padding: 8px; border-radius: 4px;");
    // Navigation buttons setup
    QButtonGroup *navGroup = new QButtonGroup(this);
    navGroup->addButton(ui->navEmployeesBtn);
    navGroup->addButton(ui->navClientsBtn);
    navGroup->addButton(ui->navProjectsBtn);
    navGroup->addButton(ui->navInvoicesBtn);
    navGroup->addButton(ui->navMaterialsBtn);
    navGroup->addButton(ui->navTasksBtn);

    QString navButtonStyle =
        "QPushButton { text-align: left; padding: 12px; color: white; border: none; background: #343a40; font-size: 12px; }"
        "QPushButton:hover { background: #23272b; }"
        "QPushButton[active=true] { background: #2980b9; border-left: 4px solid #ffc107; }";

    ui->navEmployeesBtn->setProperty("active", true);
    for(auto btn : navGroup->buttons()) {
        btn->setStyleSheet(navButtonStyle);
    }

    connect(navGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::handleNavButtonClick);

    setupEmployeeTable();
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

void MainWindow::on_navEmployeesBtn_clicked() { handleNavButtonClick(ui->navEmployeesBtn); }
void MainWindow::on_navClientsBtn_clicked() { handleNavButtonClick(ui->navClientsBtn); }
void MainWindow::on_navProjectsBtn_clicked() { handleNavButtonClick(ui->navProjectsBtn); }
void MainWindow::on_navInvoicesBtn_clicked() { handleNavButtonClick(ui->navInvoicesBtn); }
void MainWindow::on_navMaterialsBtn_clicked() { handleNavButtonClick(ui->navMaterialsBtn); }
void MainWindow::on_navTasksBtn_clicked() { handleNavButtonClick(ui->navTasksBtn); }

void MainWindow::setupEmployeeTable()
{
    ui->employeeTable->setColumnCount(8);
    QStringList headers = {"ID", "First Name", "Last Name", "Position", "Department", "Email", "Salary", "Hire Date"};
    ui->employeeTable->setHorizontalHeaderLabels(headers);
    ui->employeeTable->horizontalHeader()->setStretchLastSection(true);
    ui->employeeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->employeeTable->setSelectionMode(QAbstractItemView::SingleSelection);

    // Style for table (keep your existing styling)
    ui->employeeTable->setStyleSheet(
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

    // Load data from database instead of static array
    loadEmployeesFromDatabase();
}

QChart* MainWindow::createPieChart()
{
    // 1. Count employees and calculate percentages
    QMap<QString, int> deptCounts;
    int totalEmployees = 0;

    for (int row = 0; row < ui->employeeTable->rowCount(); ++row) {
        QString department = ui->employeeTable->item(row, 4)->text(); // Department column
        if (!department.isEmpty()) {
            deptCounts[department]++;
            totalEmployees++;
        }
    }

    // 2. Create pie series with detailed labels
    QPieSeries *series = new QPieSeries();
    for (auto it = deptCounts.begin(); it != deptCounts.end(); ++it) {
        double percentage = (it.value() * 100.0) / totalEmployees;
        QPieSlice *slice = series->append(
            QString("%1\n%2/%3 (%4%)")
                .arg(it.key())               // Department name
                .arg(it.value())             // Employee count
                .arg(totalEmployees)         // Total employees
                .arg(QString::number(percentage, 'f', 1)), // Percentage
            it.value()
            );

        // 3. Visual enhancements (corrected method names)
        slice->setLabelVisible();
        slice->setLabelArmLengthFactor(0.3); // Corrected method - value between 0-1
        slice->setLabelPosition(QPieSlice::LabelOutside);
    }

    // 4. Configure chart
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("Department Distribution\nTotal Employees: %1").arg(totalEmployees));
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setMarkerShape(QLegend::MarkerShapeRectangle);

    // 5. Additional styling
    chart->setTitleFont(QFont("Arial", 12, QFont::Bold));
    chart->legend()->setFont(QFont("Arial", 9));

    return chart;
}


void MainWindow::on_showEmployeeStats_clicked()
{
    QChart *chart = createPieChart();

    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Department Statistics");
    dialog->resize(800, 600);  // Increased size for better visibility

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Create export button
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

    // Connect export functionality
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
void MainWindow::on_showHiringTrends_clicked()
{
    // 1. Collect and validate data
    QMap<int, int> hiresByYear;
    int totalHires = 0;
    QDate earliestDate = QDate::currentDate();

    for (int row = 0; row < ui->employeeTable->rowCount(); ++row) {
        QTableWidgetItem* item = ui->employeeTable->item(row, 7); // Hire Date column
        if (item && !item->text().isEmpty()) {
            QDate hireDate = QDate::fromString(item->text(), "yyyy-MM-dd");
            if (hireDate.isValid()) {
                int year = hireDate.year();
                hiresByYear[year]++;
                totalHires++;
                if (hireDate < earliestDate) earliestDate = hireDate;
            }
        }
    }

    if (hiresByYear.isEmpty()) {
        QMessageBox::warning(this, "No Data", "No valid hire dates found");
        return;
    }

    // 2. Prepare chart data
    QBarSeries *series = new QBarSeries();
    QBarSet *barSet = new QBarSet("Hires");

    QList<int> years = hiresByYear.keys();
    std::sort(years.begin(), years.end());

    QStringList categories;
    int maxHires = 0;
    int currentYear = QDate::currentDate().year();
    double avgHires = (double)totalHires / years.count();

    for (int year : years) {
        int count = hiresByYear[year];
        *barSet << count;
        categories << QString::number(year);
        if (count > maxHires) maxHires = count;
    }

    // 3. Configure chart
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle(QString("Hiring Trends (%1-%2)\nTotal Hires: %3 | Avg/Year: %4")
                        .arg(years.first())
                        .arg(years.last())
                        .arg(totalHires)
                        .arg(QString::number(avgHires, 'f', 1)));

    // 4. Add average line
    QLineSeries *avgSeries = new QLineSeries();
    avgSeries->setName("Yearly Average");
    avgSeries->append(years.first(), avgHires);
    avgSeries->append(years.last(), avgHires);
    chart->addSeries(avgSeries);

    // 5. Customize appearance
    QPen avgPen(Qt::red);
    avgPen.setWidth(2);
    avgSeries->setPen(avgPen);

    series->append(barSet);
    barSet->setColor(QColor(32, 159, 223));

    // 6. Configure axes
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);
    avgSeries->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setRange(0, maxHires + 2);
    axisY->setTitleText("Number of Hires");
    axisY->setLabelFormat("%d");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);
    avgSeries->attachAxis(axisY);

    // 7. Create dialog with export and close buttons
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Hiring Trends");
    dialog->resize(900, 600);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    // Create buttons
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

    // Connect buttons
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

    // Layout
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
    dialog.setWindowTitle("Add New Employee");

    QStringList labels = {"ID:", "First Name:", "Last Name:", "Position:",
                          "Department:", "Email:", "Salary:", "Hire Date (YYYY-MM-DD):"};
    QList<QLineEdit*> fields;

    foreach (const QString &label, labels) {
        QLineEdit *lineEdit = new QLineEdit(&dialog);
        form.addRow(label, lineEdit);
        fields << lineEdit;

        // Set input masks/constraints
        if (label.contains("ID")) {
            lineEdit->setMaxLength(10);
        }
        else if (label.contains("Salary")) {
            lineEdit->setPlaceholderText("$50,000");
        }
        else if (label.contains("Date")) {
            lineEdit->setPlaceholderText("2023-01-15");
        }
        else if (label.contains("Email")) {
            lineEdit->setPlaceholderText("user@company.com");
        }
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted && validateEmployeeData(fields)) {
        QStringList employeeData;
        for (int i = 0; i < fields.size(); ++i) {
            QString value = fields[i]->text().trimmed();
            if (i == 6) value = "$" + cleanSalaryInput(value); // Format salary
            employeeData << value;
        }

        // Save to database first
        saveEmployeeToDatabase(employeeData);

        // Then update the table
        int row = ui->employeeTable->rowCount();
        ui->employeeTable->insertRow(row);
        for (int i = 0; i < employeeData.size(); ++i) {
            ui->employeeTable->setItem(row, i, new QTableWidgetItem(employeeData[i]));
        }

        updateCharts();
    }
}


void MainWindow::on_modifyBtn_clicked() {
    if (!validateRowSelection()) return;

    int row = ui->employeeTable->currentRow();
    QDialog dialog(this);
    QFormLayout form(&dialog);
    dialog.setWindowTitle("Edit Employee");

    QStringList labels = {"ID:", "First Name:", "Last Name:", "Position:",
                          "Department:", "Email:", "Salary:", "Hire Date:"};
    QList<QLineEdit*> fields;

    for (int i = 0; i < labels.size(); ++i) {
        QLineEdit *lineEdit = new QLineEdit(ui->employeeTable->item(row, i)->text(), &dialog);
        form.addRow(labels[i], lineEdit);
        fields << lineEdit;
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Remove validation check - just update fields directly
        for (int i = 0; i < fields.size(); ++i) {
            QString value = fields[i]->text().trimmed();
            if (i == 6) value = "$" + cleanSalaryInput(value); // Keep salary formatting
            ui->employeeTable->item(row, i)->setText(value);
        }

        // Still update database
        updateEmployeeInDatabase(row);
        updateCharts();
    }
}

void MainWindow::on_deleteBtn_clicked() {
    if (!validateRowSelection()) return;

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirm Delete",
                                  "Are you sure you want to delete this employee?",
                                  QMessageBox::Yes | QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        QString id = ui->employeeTable->item(ui->employeeTable->currentRow(), 0)->text();

        // Delete from database first
        deleteEmployeeFromDatabase(id);

        // Then remove from table
        ui->employeeTable->removeRow(ui->employeeTable->currentRow());
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
    QString html = "<h1>Employee Report</h1><table border='1'><tr>";

    // Header
    for (int col = 0; col < ui->employeeTable->columnCount(); ++col) {
        html += "<th>" + ui->employeeTable->horizontalHeaderItem(col)->text() + "</th>";
    }
    html += "</tr>";

    // Data
    for (int row = 0; row < ui->employeeTable->rowCount(); ++row) {
        html += "<tr>";
        for (int col = 0; col < ui->employeeTable->columnCount(); ++col) {
            html += "<td>" + ui->employeeTable->item(row, col)->text() + "</td>";
        }
        html += "</tr>";
    }

    html += "</table>";
    doc.setHtml(html);
    doc.print(&printer);

    QMessageBox::information(this, "Success", "PDF exported successfully");
}

void MainWindow::on_searchBtn_clicked()
{
    QString searchText = ui->searchInput->text().trimmed().toLower();

    for (int row = 0; row < ui->employeeTable->rowCount(); ++row) {
        bool matchFound = false;
        for (int col = 0; col < ui->employeeTable->columnCount(); ++col) {
            if (ui->employeeTable->item(row, col)->text().toLower().contains(searchText)) {
                matchFound = true;
                break;
            }
        }
        ui->employeeTable->setRowHidden(row, !matchFound);
    }
}

void MainWindow::on_predictBtn_clicked()
{
    if (ui->employeeTable->rowCount() == 0) {
        QMessageBox::information(this, "Info", "No employees to analyze");
        return;
    }

    QString bestEmployee = predictBestEmployee();
    QMessageBox::information(this, "Best Employee",
                             "The predicted best employee is:\n" + bestEmployee);
}

void MainWindow::on_chatbotBtn_clicked()
{
    if (!chatDialog->isVisible()) {
        chatDialog->show();
        chatDialog->addMessage("HR Assistant",
                               "Hello! I'm your HR Assistant. How can I help you today?",
                               false);
        return;
    }

    QString question;
    if (sender() == chatDialog) {
        question = chatDialog->inputLine->text();
    } else {
        question = QInputDialog::getText(this, "HR Assistant", "Ask your question:");
        if (question.isEmpty()) return;
        chatDialog->addMessage("You", question, true);
    }

    // Show typing indicator
    chatDialog->addMessage("HR Assistant", "<i>Typing...</i>", false);

    // Configure API request
    QString apiKey = "AIzaSyAPs0_5kZk-R0fItYZoiZ6hRNDdtXEqPQ8";
    QUrl apiUrl("https://generativelanguage.googleapis.com/v1beta/models/gemini-2.0-flash:generateContent");

    QUrlQuery query;
    query.addQueryItem("key", apiKey);
    apiUrl.setQuery(query);

    QNetworkRequest request(apiUrl);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // Updated payload structure matching API spec
    QJsonObject payload;
    payload["contents"] = QJsonArray{
        QJsonObject{
            {"parts", QJsonArray{
                          QJsonObject{
                              {"text", QString("Act as an HR assistant. Keep responses professional and concise:\n%1").arg(question)}
                          }
                      }}
        }
    };

    // Optional: Add safety settings
    QJsonArray safetySettings;
    safetySettings.append(QJsonObject{
        {"category", "HARM_CATEGORY_HARASSMENT"},
        {"threshold", "BLOCK_MEDIUM_AND_ABOVE"}
    });
    payload["safetySettings"] = safetySettings;

    // Optional: Add generation config
    QJsonObject generationConfig;
    generationConfig["temperature"] = 0.7;
    generationConfig["topP"] = 0.9;
    generationConfig["maxOutputTokens"] = 1024;
    payload["generationConfig"] = generationConfig;

    // Send request
    QNetworkReply* reply = networkManager->post(request, QJsonDocument(payload).toJson());

    // Improved response handling
    connect(reply, &QNetworkReply::finished, [this, reply]() {
        // Remove the "Typing..." message
        if (chatDialog->messageList->count() > 0) {
            QListWidgetItem* lastItem = chatDialog->messageList->item(chatDialog->messageList->count() - 1);
            QWidget* widget = chatDialog->messageList->itemWidget(lastItem);
            if (widget) {
                QLabel* label = widget->findChild<QLabel*>();
                if (label && label->text().contains("Typing...")) {
                    delete chatDialog->messageList->takeItem(chatDialog->messageList->count() - 1);
                }
            }
        }

        if (reply->error() == QNetworkReply::NoError) {
            QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
            QJsonObject responseObj = response.object();

            QString answer;
            if (responseObj.contains("candidates")) {
                QJsonArray candidates = responseObj["candidates"].toArray();
                if (!candidates.isEmpty()) {
                    QJsonObject content = candidates[0].toObject()["content"].toObject();
                    QJsonArray parts = content["parts"].toArray();
                    if (!parts.isEmpty()) {
                        answer = parts[0].toObject()["text"].toString();
                    }
                }
            } else if (responseObj.contains("error")) {
                answer = "Error: " + responseObj["error"].toObject()["message"].toString();
            } else {
                answer = "Sorry, I didn't understand that response.";
            }

            // Display the actual response
            chatDialog->addMessage("HR Assistant", answer, false);
        } else {
            QString errorMsg = QString("Network Error (%1): %2")
            .arg(reply->error())
                .arg(reply->errorString());
            chatDialog->addMessage("HR Assistant", errorMsg, false);
        }

        reply->deleteLater();
    });
}

QString MainWindow::predictBestEmployee()
{
    int bestRow = 0;
    double maxScore = 0;
    QDate today = QDate::currentDate();

    for (int row = 0; row < ui->employeeTable->rowCount(); ++row) {
        // Calculate score based on seniority and salary
        QDate hireDate = QDate::fromString(ui->employeeTable->item(row, 7)->text(), "yyyy-MM-dd");
        int daysEmployed = hireDate.daysTo(today);

        QString salaryStr = ui->employeeTable->item(row, 6)->text();
        salaryStr.remove('$').remove(',');
        double salary = salaryStr.toDouble();

        double score = (daysEmployed * 0.4) + (salary * 0.00002);

        if (score > maxScore) {
            maxScore = score;
            bestRow = row;
        }
    }

    return ui->employeeTable->item(bestRow, 1)->text() + " " +
           ui->employeeTable->item(bestRow, 2)->text();
}

void MainWindow::updateCharts()
{
    // Close any open stats dialogs
    QList<QDialog*> dialogs = findChildren<QDialog*>();
    foreach (QDialog* dialog, dialogs) {
        if (dialog->windowTitle() == "Employee Statistics" ||
            dialog->windowTitle() == "Salary Statistics") {
            dialog->close();
            dialog->deleteLater();
        }
    }

    // Note: Charts will be recreated when buttons are clicked again
}
void MainWindow::on_sortBtn_clicked()
{
    QMenu sortMenu;

    // Create sort actions
    QAction *sortByIdAsc = sortMenu.addAction("By ID (Ascending)");
    QAction *sortByIdDesc = sortMenu.addAction("By ID (Descending)");
    QAction *sortByNameAsc = sortMenu.addAction("By Name (A-Z)");
    QAction *sortByNameDesc = sortMenu.addAction("By Name (Z-A)");
    QAction *sortBySalaryAsc = sortMenu.addAction("By Salary (Low-High)");
    QAction *sortBySalaryDesc = sortMenu.addAction("By Salary (High-Low)");
    QAction *sortBySeniorityAsc = sortMenu.addAction("By Seniority (Newest)");
    QAction *sortBySeniorityDesc = sortMenu.addAction("By Seniority (Oldest)");

    // Connect actions to sorting
    connect(sortByIdAsc, &QAction::triggered, [this]() { sortEmployees(0, Qt::AscendingOrder); });
    connect(sortByIdDesc, &QAction::triggered, [this]() { sortEmployees(0, Qt::DescendingOrder); });
    connect(sortByNameAsc, &QAction::triggered, [this]() { sortEmployees(1, Qt::AscendingOrder); }); // First Name
    connect(sortByNameDesc, &QAction::triggered, [this]() { sortEmployees(1, Qt::DescendingOrder); });
    connect(sortBySalaryAsc, &QAction::triggered, [this]() {
        // Special handling for salary (remove $ and commas)
        ui->employeeTable->sortByColumn(6, Qt::AscendingOrder);
    });
    connect(sortBySalaryDesc, &QAction::triggered, [this]() {
        ui->employeeTable->sortByColumn(6, Qt::DescendingOrder);
    });
    connect(sortBySeniorityAsc, &QAction::triggered, [this]() { sortEmployees(7, Qt::AscendingOrder); }); // Hire Date
    connect(sortBySeniorityDesc, &QAction::triggered, [this]() { sortEmployees(7, Qt::DescendingOrder); });

    // Show the menu at the button position
    sortMenu.exec(ui->sortBtn->mapToGlobal(QPoint(0, ui->sortBtn->height())));
}

void MainWindow::sortEmployees(int column, Qt::SortOrder order)
{
    // For name sorting, we need to consider both first and last names
    if (column == 1) {
        // Custom sorting for names (combine first and last name)
        ui->employeeTable->sortItems(1, order); // Sort by first name first
        ui->employeeTable->sortItems(2, order); // Then by last name
        return;
    }

    // Standard sorting for other columns
    ui->employeeTable->sortItems(column, order);

    // Special case for salary - we need to clean the data first
    if (column == 6) {
        for (int row = 0; row < ui->employeeTable->rowCount(); ++row) {
            QTableWidgetItem *item = ui->employeeTable->item(row, 6);
            QString cleanSalary = item->text().remove('$').remove(',');
            item->setData(Qt::UserRole, cleanSalary.toInt());
        }
        ui->employeeTable->sortItems(6, order);
    }
}
bool MainWindow::validateRowSelection(bool requireSelection) {
    if (requireSelection && ui->employeeTable->currentRow() < 0) {
        QMessageBox::warning(this, "Selection Required", "Please select an employee first");
        return false;
    }
    return true;
}

QString MainWindow::cleanSalaryInput(const QString& salary) {
    return salary.trimmed().remove('$').remove(',');
}

bool MainWindow::validateEmployeeData(const QList<QLineEdit*>& fields) {
    // Field names for error messages
    const QStringList fieldNames = {
        "ID", "First Name", "Last Name", "Position",
        "Department", "Email", "Salary", "Hire Date"
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

    // 2. Validate ID (alphanumeric, 3-10 chars)
    QRegularExpression idRegex("^[a-zA-Z0-9]{3,10}$");
    if (!idRegex.match(fields[0]->text()).hasMatch()) {
        QMessageBox::warning(this, "Invalid ID",
                             "ID must be 3-10 alphanumeric characters");
        fields[0]->setFocus();
        return false;
    }

    // 3. Validate Names (letters and basic punctuation)
    QRegularExpression nameRegex("^[a-zA-Z'-]+( [a-zA-Z'-]+)*$");
    if (!nameRegex.match(fields[1]->text()).hasMatch()) { // First Name
        QMessageBox::warning(this, "Invalid Name",
                             "First name can only contain letters and hyphens");
        fields[1]->setFocus();
        return false;
    }
    if (!nameRegex.match(fields[2]->text()).hasMatch()) { // Last Name
        QMessageBox::warning(this, "Invalid Name",
                             "Last name can only contain letters and hyphens");
        fields[2]->setFocus();
        return false;
    }

    // 4. Validate Email
    QString newEmail = fields[5]->text().trimmed().toLower();
    QSqlQuery emailQuery;
    emailQuery.prepare("SELECT id, first_name, last_name FROM employees WHERE email = :email");
    emailQuery.bindValue(":email", newEmail);

    if (emailQuery.exec() && emailQuery.next()) {
        QMessageBox::warning(this, "Duplicate Email",
                             QString("This email address is already registered to:\n%1 %2 (ID: %3)")
                                 .arg(emailQuery.value(1).toString())
                                 .arg(emailQuery.value(2).toString())
                                 .arg(emailQuery.value(0).toString()));
        fields[5]->setFocus();
        return false;
    }

    // 5. Validate Salary (formats: $50,000 or 50000 or 50000.00)
    QRegularExpression salaryRegex(R"(^\$?\d{1,3}(,\d{3})*(\.\d{2})?$)");
    QString salary = fields[6]->text().trimmed();
    if (!salaryRegex.match(salary).hasMatch()) {
        QMessageBox::warning(this, "Invalid Salary",
                             "Salary must be in format like:\n$50,000 or 50000 or 50000.00");
        fields[6]->setFocus();
        return false;
    }

    // 6. Validate Hire Date (YYYY-MM-DD)
    QDate hireDate = QDate::fromString(fields[7]->text(), "yyyy-MM-dd");
    if (!hireDate.isValid() || hireDate > QDate::currentDate()) {
        QMessageBox::warning(this, "Invalid Date",
                             "Hire date must be in YYYY-MM-DD format\nand cannot be in the future");
        fields[7]->setFocus();
        return false;
    }

    // 7. Check for duplicate ID
    QString newId = fields[0]->text().trimmed();
    QSqlQuery idQuery;
    idQuery.prepare("SELECT first_name, last_name FROM employees WHERE id = :id");
    idQuery.bindValue(":id", newId);

    if (idQuery.exec() && idQuery.next()) {
        QMessageBox::warning(this, "Duplicate ID",
                             QString("An employee with this ID already exists:\n%1 %2")
                                 .arg(idQuery.value(0).toString())
                                 .arg(idQuery.value(1).toString()));
        fields[0]->setFocus();
        return false;
    }

    return true; // All validations passed
}

//data base code

bool MainWindow::initializeDatabase()
{
    database = QSqlDatabase::addDatabase("QSQLITE");
    database.setDatabaseName("employees.db");

    if (!database.open()) {
        qDebug() << "Error: Failed to connect to database:" << database.lastError();
        return false;
    }

    return createEmployeeTable();
}

bool MainWindow::createEmployeeTable()
{
    QSqlQuery query;
    QString createTableQuery =
        "CREATE TABLE IF NOT EXISTS employees ("
        "id TEXT PRIMARY KEY, "
        "first_name TEXT NOT NULL, "
        "last_name TEXT NOT NULL, "
        "position TEXT NOT NULL, "
        "department TEXT NOT NULL, "
        "email TEXT UNIQUE NOT NULL, "
        "salary TEXT NOT NULL, "
        "hire_date TEXT NOT NULL"
        ")";

    if (!query.exec(createTableQuery)) {
        qDebug() << "Error creating table:" << query.lastError();
        return false;
    }

    return true;
}

void MainWindow::loadEmployeesFromDatabase()
{
    ui->employeeTable->setRowCount(0); // Clear existing data

    QSqlQuery query("SELECT * FROM employees ORDER BY last_name, first_name");

    while (query.next()) {
        int row = ui->employeeTable->rowCount();
        ui->employeeTable->insertRow(row);

        for (int col = 0; col < 8; col++) {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());
            ui->employeeTable->setItem(row, col, item);
        }
    }
}

void MainWindow::saveEmployeeToDatabase(const QStringList &employeeData)
{
    QSqlQuery query;
    query.prepare("INSERT INTO employees (id, first_name, last_name, position, department, email, salary, hire_date) "
                  "VALUES (:id, :first_name, :last_name, :position, :department, :email, :salary, :hire_date)");

    query.bindValue(":id", employeeData[0]);
    query.bindValue(":first_name", employeeData[1]);
    query.bindValue(":last_name", employeeData[2]);
    query.bindValue(":position", employeeData[3]);
    query.bindValue(":department", employeeData[4]);
    query.bindValue(":email", employeeData[5]);
    query.bindValue(":salary", employeeData[6]);
    query.bindValue(":hire_date", employeeData[7]);

    if (!query.exec()) {
        qDebug() << "Error inserting employee:" << query.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to save employee to database");
    }
}

void MainWindow::updateEmployeeInDatabase(int row)
{
    QSqlQuery query;
    query.prepare("UPDATE employees SET "
                  "first_name = :first_name, "
                  "last_name = :last_name, "
                  "position = :position, "
                  "department = :department, "
                  "email = :email, "
                  "salary = :salary, "
                  "hire_date = :hire_date "
                  "WHERE id = :id");

    query.bindValue(":id", ui->employeeTable->item(row, 0)->text());
    query.bindValue(":first_name", ui->employeeTable->item(row, 1)->text());
    query.bindValue(":last_name", ui->employeeTable->item(row, 2)->text());
    query.bindValue(":position", ui->employeeTable->item(row, 3)->text());
    query.bindValue(":department", ui->employeeTable->item(row, 4)->text());
    query.bindValue(":email", ui->employeeTable->item(row, 5)->text());
    query.bindValue(":salary", ui->employeeTable->item(row, 6)->text());
    query.bindValue(":hire_date", ui->employeeTable->item(row, 7)->text());

    if (!query.exec()) {
        qDebug() << "Error updating employee:" << query.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to update employee in database");
    }
}

void MainWindow::deleteEmployeeFromDatabase(const QString &id)
{
    QSqlQuery query;
    query.prepare("DELETE FROM employees WHERE id = :id");
    query.bindValue(":id", id);

    if (!query.exec()) {
        qDebug() << "Error deleting employee:" << query.lastError();
        QMessageBox::critical(this, "Database Error", "Failed to delete employee from database");
    }
}

MainWindow::~MainWindow()
{
    database.close();
    delete ui;
    delete networkManager;
}
