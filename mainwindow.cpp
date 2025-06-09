#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "invoice.h"
#include "databaseconnection.h"
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
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QComboBox>
#include <QDateEdit>

void MainWindow::refreshTable()
{
    ui->employeeTable->setRowCount(0);
    invoices = Invoice::loadAllFromDatabase();
    try {
        ui->employeeTable->setRowCount(0);

        invoices = Invoice::loadAllFromDatabase();

        if (invoices.isEmpty()) {
            qDebug() << "Aucune facture trouvée dans la base de données";
            return;
        }

        ui->employeeTable->setRowCount(invoices.size());
        // Basculer le texte du bouton
        switchLanguageButton->setText(isFrench ? "Switch to English" : "Passer au français");
        // Configurer les en-têtes de colonnes selon la langue
        ui->employeeTable->setColumnCount(5);
        ui->employeeTable->setHorizontalHeaderLabels(
                isFrench
                    ? QStringList{"ID", "Numéro", "Date", "Montant", "Statut"}
                    : QStringList{"ID", "Number", "Date", "Amount", "Status"}
                );
        for (int row = 0; row < invoices.size(); ++row) {
            const Invoice& inv = invoices[row];
            if (inv.id.isEmpty() || inv.number.isEmpty()) {
                qWarning() << "Facture invalide à la ligne" << row;
                continue;
            }

            QTableWidgetItem* idItem = new QTableWidgetItem(inv.id);
            QTableWidgetItem* numberItem = new QTableWidgetItem(inv.number);
            QTableWidgetItem* dateItem = new QTableWidgetItem(inv.date);
            QTableWidgetItem* amountItem = new QTableWidgetItem(QString::number(inv.amount, 'f', 2));
            QTableWidgetItem* statusItem = new QTableWidgetItem(translateStatus(inv.status, isFrench));

            idItem->setFlags(idItem->flags() ^ Qt::ItemIsEditable);
            numberItem->setFlags(numberItem->flags() ^ Qt::ItemIsEditable);
            amountItem->setFlags(amountItem->flags() | Qt::ItemIsEditable);
            dateItem->setFlags(dateItem->flags() | Qt::ItemIsEditable);
            statusItem->setFlags(statusItem->flags() | Qt::ItemIsEditable);

            ui->employeeTable->setItem(row, 0, idItem);
            ui->employeeTable->setItem(row, 1, numberItem);
            ui->employeeTable->setItem(row, 2, dateItem);
            ui->employeeTable->setItem(row, 3, amountItem);
            ui->employeeTable->setItem(row, 4, statusItem);
        }

        // 5. Ajuster les colonnes
        ui->employeeTable->resizeColumnsToContents();
    } catch (const std::exception& e) {
        qCritical() << "Erreur lors du rafraîchissement du tableau :" << e.what();
        QMessageBox::critical(this, "Erreur", "Impossible de charger les données des factures");
    }
}

QString MainWindow::translateStatus(const QString &status, bool isFrench)
{
    if (isFrench) {
        if (status == "Paid") return "Payé";
        if (status == "Pending") return "En attente";
        if (status == "Overdue") return "En retard";
    }
    return status;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), isDarkMode(false), isFrench(false),
    networkManager(new QNetworkAccessManager(this))
{
    ui->setupUi(this);
    setWindowTitle("Invoice Management System");
    resize(1400, 900);
    // Configuration du style
    this->setStyleSheet(
        "QMainWindow { background: #f8f9fa; }"
        "QPushButton { background: #6c757d; color: white; padding: 8px; border-radius: 4px; }"
        "QPushButton:hover { background: #5a6268; }"
        "QLineEdit { padding: 8px; border: 1px solid #ced4da; border-radius: 4px; }"
        "QTableView { background: white; alternate-background-color: #f8f9fa; }"
        "QHeaderView::section { background: #343a40; color: white; padding: 8px; }"
        "QPushButton[active=true] { background: #007bff; border-left: 4px solid #ffc107; }"
        );

    themeToggleButton = new QPushButton(tr("Mode Sombre/Clair"), this);
    themeToggleButton->setStyleSheet("background: #007bff; color: white; padding: 8px; border-radius: 4px;");
    connect(themeToggleButton, &QPushButton::clicked, this, &MainWindow::toggleTheme);
    ui->navLayout->addWidget(themeToggleButton);

    // Création dynamique du bouton de langue
    switchLanguageButton = new QPushButton(tr("Switch Language"), this);
    switchLanguageButton->setStyleSheet("background: #007bff; color: white; padding: 8px; border-"
                                        "radius: 4px;");
    connect(switchLanguageButton, &QPushButton::clicked, this, &MainWindow::switchLanguage);
    ui->navLayout->addWidget(switchLanguageButton);
    switchLanguageButton->setText("Passer au français");
    // Style des boutons spéciaux
    ui->predictBtn->setStyleSheet("background: #28a745; font-weight: bold;");
    ui->showInvoiceStats->setStyleSheet("background: #6f42c1; color: white;");
    ui->showPaymentTrends->setStyleSheet("background: #20c997; color: white;");

    // Configuration de la navigation
    QButtonGroup *navGroup = new QButtonGroup(this);
    navGroup->addButton(ui->navInvoicesBtn);
    navGroup->addButton(ui->navClientsBtn);
    navGroup->addButton(ui->navProjectsBtn);
    navGroup->addButton(ui->navEmployeesBtn);
    navGroup->addButton(ui->navMaterialsBtn);
    navGroup->addButton(ui->navTasksBtn);

    QString navButtonStyle =
        "QPushButton { text-align: left; padding: 12px; color: white; border: none; background: #343a40; font-size: 12px; }"
        "QPushButton:hover { background: #23272b; }"
        "QPushButton[active=true] { background: #007bff; border-left: 4px solid #ffc107; }";

    ui->navInvoicesBtn->setProperty("active", true);
    for(auto btn : navGroup->buttons()) {
        btn->setStyleSheet(navButtonStyle);
    }

    connect(navGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::handleNavButtonClick);

    setupInvoiceTable();
    DatabaseConnection::instance()->initializeDatabase();
    DatabaseConnection::instance()->addSampleData();
    refreshTable();
}

void MainWindow::toggleTheme()
{
    static bool isDarkMode = false;
    isDarkMode = !isDarkMode;

    if (isDarkMode) {
        qApp->setStyleSheet(
            "QMainWindow, QWidget { background-color: #2e2e2e; color: #ffffff; }"
            "QPushButton { background: #6c757d; color: white; padding: 8px; border-radius: 4px; }"
            "QPushButton:hover { background: #5a6268; }"
            "QLineEdit, QComboBox, QTextEdit, QPlainTextEdit { background: #343a40; color: white; border: 1px solid #ced4da; border-radius: 4px; }"
            "QTableView { background-color: #2e2e2e; color: white; alternate-background-color: #3e3e3e; }"
            "QHeaderView::section { background: #444; color: white; }"
            "QTableView QHeaderView { background-color: #2e2e2e; }"
            "QTableView QTableCornerButton::section { background-color: #2e2e2e; }"
            "QTableView::item { background: #3e3e3e; color: #ffffff; }"
            "QScrollArea, QFrame, QGroupBox, QToolBox, QStackedWidget, QTabWidget::pane { background-color: #2e2e2e; color: white; }"
            "QScrollBar:vertical { background: #2e2e2e; }"
            "QScrollBar:horizontal { background: #2e2e2e; }"
            "QToolBar { background: #2e2e2e; border: none; }"
            "QStatusBar { background: #2e2e2e; color: white; }"
            );
        themeToggleButton->setText("Light Mode");
    } else {
        qApp->setStyleSheet(
            "QMainWindow, QWidget { background-color: #f8f9fa; color: #000000; }"
            "QPushButton { background: #6c757d; color: white; padding: 8px; border-radius: 4px; }"
            "QPushButton:hover { background: #5a6268; }"
            "QLineEdit, QComboBox, QTextEdit, QPlainTextEdit { background: white; color: black; border: 1px solid #ced4da; border-radius: 4px; }"
            "QTableView { background: white; color: black; alternate-background-color: #f8f9fa; }"
            "QHeaderView::section { background: #343a40; color: white; }"
            "QTableView QHeaderView { background-color: #f8f9fa; }"
            "QTableView QTableCornerButton::section { background-color: #f8f9fa; }"
            "QTableView::item { background: white; color: black; }"
            "QScrollArea, QFrame, QGroupBox, QToolBox, QStackedWidget, QTabWidget::pane { background-color: #f8f9fa; color: black; }"
            "QScrollBar:vertical { background: #f8f9fa; }"
            "QScrollBar:horizontal { background: #f8f9fa; }"
            "QToolBar { background: #f8f9fa; border: none; }"
            "QStatusBar { background: #f8f9fa; color: black; }"
            );
        themeToggleButton->setText("Dark Mode");
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
}

// Implémentations des slots de navigation
void MainWindow::on_navInvoicesBtn_clicked() { /* Logique spécifique */ }
void MainWindow::on_navClientsBtn_clicked() { /* Logique clients */ }
void MainWindow::on_navProjectsBtn_clicked() { /* Logique projets */ }
void MainWindow::on_navEmployeesBtn_clicked() { /* Logique employés */ }
void MainWindow::on_navMaterialsBtn_clicked() { /* Logique matériels */ }
void MainWindow::on_navTasksBtn_clicked() { /* Logique tâches */ }

void MainWindow::setupInvoiceTable()
{
    ui->employeeTable->setColumnCount(5);
    QStringList headers = {"ID", "Number", "Date", "Amount", "Payment Status"};
    ui->employeeTable->setHorizontalHeaderLabels(headers);
    ui->employeeTable->horizontalHeader()->setStretchLastSection(true);
    ui->employeeTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->employeeTable->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->employeeTable->setRowCount(10);
}

void MainWindow::on_addBtn_clicked()
{
    QDialog dialog(this);
    QFormLayout form(&dialog);

    // Champs avec validateurs
    QLineEdit *idField = new QLineEdit(&dialog);
    QRegularExpression idRegex("[A-Za-z0-9-]+");
    idField->setValidator(new QRegularExpressionValidator(idRegex, this));
    QLineEdit *numberField = new QLineEdit(&dialog);
    QRegularExpression invoiceRegex("INV-\\d{4}-\\d{3}");
    numberField->setValidator(new QRegularExpressionValidator(invoiceRegex, this));

    QDateEdit *dateField = new QDateEdit(QDate::currentDate(), &dialog);
    dateField->setDisplayFormat("yyyy-MM-dd");

    QLineEdit *amountField = new QLineEdit(&dialog);
    amountField->setValidator(new QDoubleValidator(0, 1000000, 2, this));

    QComboBox *statusCombo = new QComboBox(&dialog);
    statusCombo->addItems({"Paid", "Pending", "Overdue"});

    form.addRow("ID*:", idField);
    form.addRow("Number* (format INV-YYYY-NNN):", numberField);
    form.addRow("Date*:", dateField);
    form.addRow("Amount*:", amountField);
    form.addRow("Status*:", statusCombo);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal,
                               &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if(dialog.exec() == QDialog::Accepted) {
        // Validation finale
        if(idField->text().isEmpty() || numberField->text().isEmpty()) {
            QMessageBox::warning(this, "Erreur", "Les champs marqués d'un * sont obligatoires");
            return;
        }

        // Vérifier l'unicité de l'ID
        QSqlQuery checkQuery;
        checkQuery.prepare("SELECT id FROM invoices WHERE id = ?");
        checkQuery.addBindValue(idField->text());
        if(checkQuery.exec() && checkQuery.next()) {
            QMessageBox::warning(this, "Erreur", "Cet ID existe déjà");
            return;
        }

        Invoice newInv;
        newInv.id = idField->text();
        newInv.number = numberField->text();
        newInv.date = dateField->date().toString("yyyy-MM-dd");
        newInv.amount = amountField->text().toDouble();
        newInv.status = statusCombo->currentText();

        if(newInv.saveToDatabase()) {
            refreshTable();
            updateCharts();
        }
        else {
            QMessageBox::warning(this, "Erreur", "Échec de l'ajout");
        }
    }
}

void MainWindow::on_modifyBtn_clicked()
{
    int row = ui->employeeTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Error", "Please select an invoice to modify");
        return;
    }

    QDialog dialog(this);
    QFormLayout form(&dialog);

    QString originalId = ui->employeeTable->item(row, 0)->text();

    // ID (non modifiable)
    QLabel *idLabel = new QLabel(originalId, &dialog);
    form.addRow("ID:", idLabel);

    // Number
    QLineEdit *numberField = new QLineEdit(ui->employeeTable->item(row, 1)->text(), &dialog);
    QRegularExpression invoiceRegex("INV-\\d{4}-\\d{3}");
    numberField->setValidator(new QRegularExpressionValidator(invoiceRegex, this));

    // Date
    QDateEdit *dateField = new QDateEdit(QDate::fromString(ui->employeeTable->item(row, 2)->text(), "yyyy-MM-dd"), &dialog);
    dateField->setDisplayFormat("yyyy-MM-dd");

    // Amount
    QLineEdit *amountField = new QLineEdit(ui->employeeTable->item(row, 3)->text(), &dialog);
    amountField->setValidator(new QDoubleValidator(0, 1000000, 2, this));

    // Status
    QComboBox *statusCombo = new QComboBox(&dialog);
    statusCombo->addItems({"Paid", "Pending", "Overdue"});
    statusCombo->setCurrentText(ui->employeeTable->item(row, 4)->text());

    form.addRow("Number*:", numberField);
    form.addRow("Date*:", dateField);
    form.addRow("Amount*:", amountField);
    form.addRow("Status*:", statusCombo);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        // Validation
        if(numberField->text().isEmpty()) {
            QMessageBox::warning(this, "Erreur", "Le numéro est obligatoire");
            return;
        }

        // Mise à jour de la base
        QSqlQuery query;
        query.prepare("UPDATE invoices SET "
                      "number = ?, date = ?, amount = ?, status = ? "
                      "WHERE id = ?");
        query.addBindValue(numberField->text());
        query.addBindValue(dateField->date().toString("yyyy-MM-dd"));
        query.addBindValue(amountField->text().toDouble());
        query.addBindValue(statusCombo->currentText());
        query.addBindValue(originalId);

        if(DatabaseConnection::instance()->executeQuery(query)) {
            refreshTable();
            updateCharts();
        }
    }
}

void MainWindow::on_deleteBtn_clicked() {
    int row = ui->employeeTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Error", "Please select an invoice to delete");
        return;
    }

    QString id = ui->employeeTable->item(row, 0)->text(); // Récupérer l'ID

    // Supprimer de la base de données
    QSqlQuery query;
    query.prepare("DELETE FROM invoices WHERE id = ?");
    query.addBindValue(id);

    if (query.exec()) {
        ui->employeeTable->removeRow(row); // Supprimer de l'UI
        QSqlDatabase::database().commit(); // Valider la transaction
        refreshTable(); // Rafraîchir le tableau
    } else {
        QMessageBox::critical(this, "Error", "Failed to delete from database: " + query.lastError().text());
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
    QString html = "<h1>Invoice Report</h1><table border='1'><tr>";

    // En-têtes
    for (int col = 0; col < ui->employeeTable->columnCount(); ++col) {
        html += "<th>" + ui->employeeTable->horizontalHeaderItem(col)->text() + "</th>";
    }
    html += "</tr>";

    // Données
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

// Statistiques
void MainWindow::on_showInvoiceStats_clicked()
{
    QVector<Invoice> invoices = Invoice::loadAllFromDatabase();
    QPieSeries *series = new QPieSeries();
    QMap<QString, int> statusCounts;

    for (int row = 0; row < ui->employeeTable->rowCount(); ++row) {
        QString status = ui->employeeTable->item(row, 4)->text();
        statusCounts[status]++;
    }

    for (auto it = statusCounts.begin(); it != statusCounts.end(); ++it) {
        QPieSlice *slice = series->append(
            QString("%1 (%2)").arg(it.key()).arg(it.value()),
            it.value()
            );
        slice->setLabelVisible();
    }

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Payment Status Distribution");
    chart->legend()->setAlignment(Qt::AlignRight);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QDialog *dialog = new QDialog(this);
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->addWidget(chartView);
    dialog->resize(600, 400);
    dialog->exec();
}

void MainWindow::on_showPaymentTrends_clicked()
{
    QBarSeries *series = new QBarSeries();
    QBarSet *barSet = new QBarSet("Monthly Payments");

    // Exemple de données
    *barSet << 1500 << 2450 << 980 << 3200 << 1650 << 4300 << 275 << 8900 << 150 << 675;
    series->append(barSet);

    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Payment Trends");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    QStringList categories = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct"};
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat("$%d");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);

    QDialog dialog(this);
    dialog.setWindowTitle("Payment Trends");
    dialog.resize(800, 600);
    QVBoxLayout layout(&dialog);
    layout.addWidget(chartView);
    dialog.exec();
}

QString MainWindow::predictUnpaidInvoice()
{
    double maxAmount = 0;
    int maxRow = -1;

    for (int row = 0; row < ui->employeeTable->rowCount(); ++row) {
        if (ui->employeeTable->item(row, 4)->text() == "Pending") {
            QString amountStr = ui->employeeTable->item(row, 3)->text();
            double amount = amountStr.toDouble();
            if (amount > maxAmount) {
                maxAmount = amount;
                maxRow = row;
            }
        }
    }

    if (maxRow == -1) return "No pending invoices";

    return QString("Highest pending invoice: %1\nAmount: %2")
        .arg(ui->employeeTable->item(maxRow, 1)->text())
        .arg(ui->employeeTable->item(maxRow, 3)->text());
}

void MainWindow::on_predictBtn_clicked()
{
    QString prediction = predictUnpaidInvoice();
    QMessageBox::information(this, "Prediction", prediction);
}

void MainWindow::updateCharts()
{
    QList<QDialog*> dialogs = findChildren<QDialog*>();
    foreach (QDialog* dialog, dialogs) {
        if (dialog->windowTitle().contains("Statistics")) {
            dialog->close();
            dialog->deleteLater();
        }
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    delete networkManager;
}

void MainWindow::switchLanguage()
{
    isFrench = !isFrench;
    switchLanguageButton->setText(isFrench ? "Switch to English" : "Passer au français");
    if (isFrench) {
        ui->navInvoicesBtn->setText("Factures");
        ui->navClientsBtn->setText("Clients");
        ui->navProjectsBtn->setText("Projets");
        ui->navEmployeesBtn->setText("Employés");
        ui->navMaterialsBtn->setText("Matériaux");
        ui->navTasksBtn->setText("Tâches");
        switchLanguageButton->setText(isFrench ? "Switch to English" : "Passer au français");
        setWindowTitle("Système de gestion des factures");
        ui->addBtn->setText("Ajouter");
        ui->modifyBtn->setText("Modifier");
        ui->deleteBtn->setText("Supprimer");
        ui->exportBtn->setText("Exporter");
        ui->searchBtn->setText("Rechercher");
        ui->showInvoiceStats->setText("Statistiques des factures");
        ui->showPaymentTrends->setText("Tendances des paiements");
        ui->predictBtn->setText("Prédire les impayés");
        themeToggleButton->setText(isDarkMode ? "Mode Clair" : "Mode Sombre");
    } else {
        ui->navInvoicesBtn->setText("Invoices");
        ui->navClientsBtn->setText("Clients");
        ui->navProjectsBtn->setText("Projects");
        ui->navEmployeesBtn->setText("Employees");
        ui->navMaterialsBtn->setText("Materials");
        ui->navTasksBtn->setText("Tasks");
        switchLanguageButton->setText(isFrench ? "Switch to English" : "Passer au français");
        setWindowTitle("Invoice Management System");
        ui->addBtn->setText("Add");
        ui->modifyBtn->setText("Edit");
        ui->deleteBtn->setText("Delete");
        ui->exportBtn->setText("Export");
        ui->searchBtn->setText("Search");
        ui->showInvoiceStats->setText("Invoice Stats");
        ui->showPaymentTrends->setText("Payment Trends");
        ui->predictBtn->setText("Predict Unpaid");
        themeToggleButton->setText(isDarkMode ? "Light Mode" : "Dark Mode");
    }
    themeToggleButton->setText(isDarkMode ?
                                   (isFrench ? "Mode Clair" : "Light Mode") :
                                   (isFrench ? "Mode Sombre" : "Dark Mode"));
    refreshTable(); // Rafraîchir le tableau après le changement de langue
}

