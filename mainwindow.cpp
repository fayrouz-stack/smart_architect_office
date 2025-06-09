#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QInputDialog>
#include <QFileDialog>
#include <QPrinter>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QLabel>
#include <QButtonGroup>
#include <QComboBox>
#include <QRegularExpression>
#include <QRegularExpressionValidator>
#include <QSqlError>
#include <QSqlRecord>
#include <QStandardPaths>
#include <QDir>
#include <QDebug>
#include <QStatusBar>
#include <QDate>
#include <QDesktopServices>
#include <QUrl>
#include <QRandomGenerator>
#include <QPainter>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QChartView>
#include <QtCharts/QBarCategoryAxis>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QJsonDocument>
#include <QJsonObject>
#include <QScopedPointer>



MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent),
    ui(new Ui::MainWindow),
    networkManager(new QNetworkAccessManager(this))

{
    ui->setupUi(this);
    setWindowTitle("Project Management System");
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
        "#addBtn {"
        "   background: #28a745;"
        "}"
        "#modifyBtn {"
        "   background: #17a2b8;"
        "}"
        "#deleteBtn {"
        "   background: #dc3545;"
        "}"
        "#exportBtn {"
        "   background: #6f42c1;"
        "}"
        "#sortBtn {"
        "   background: #fd7e14;"
        "}"
        "#searchBtn {"
        "   background: #20c997;"
        "}"
        "#notifyClientBtn {"
        "   background: #d63384;"
        "}"
        "#showMapBtn {"
        "   background: #1abc9c;"
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

    ui->navProjectsBtn->setProperty("active", true);
    for(auto btn : navGroup->buttons()) {
        btn->setStyleSheet(navButtonStyle);
    }

    connect(navGroup, QOverload<QAbstractButton*>::of(&QButtonGroup::buttonClicked),
            this, &MainWindow::handleNavButtonClick);

    setupProjectTable();
    loadProjectsFromDatabase();
}

bool MainWindow::initializeDatabase()
{
    // Chemin vers la base de données
    QString dbPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dbPath);
    dbPath += "/projects.db";

    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName(dbPath);

    if (!db.open()) {
        QMessageBox::critical(this, "Database Error",
                              "Cannot open database: " + db.lastError().text());
        return false;
    }

    // Vérifier si la table projects existe, sinon la créer
    QSqlQuery query(db);
    if (!query.exec("CREATE TABLE IF NOT EXISTS projects ("
                    "id TEXT PRIMARY KEY, "
                    "name TEXT NOT NULL, "
                    "client_email TEXT NOT NULL, "
                    "start_date TEXT NOT NULL, "
                    "end_date TEXT NOT NULL, "
                    "status TEXT NOT NULL, "
                    "budget REAL NOT NULL, "
                    "client_name TEXT NOT NULL, "
                    "location TEXT NOT NULL)")) {
        QMessageBox::critical(this, "Database Error",
                              "Cannot create table: " + query.lastError().text());
        return false;
    }

    checkDatabaseVersion();
    return true;
}


void MainWindow::setupProjectTable()
{
    ui->projectTable->setColumnCount(9);
    QStringList headers = {"ID", "Nom", "client_email", "Date début", "Date fin", "Statut", "Budget", "Client", "Location"};
    ui->projectTable->setHorizontalHeaderLabels(headers);
    ui->projectTable->horizontalHeader()->setStretchLastSection(true);
    ui->projectTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->projectTable->setSelectionMode(QAbstractItemView::SingleSelection);
}

void MainWindow::loadProjectsFromDatabase()
{
    if (!db.isOpen()) return;

    ui->projectTable->setRowCount(0);

    QSqlQuery query("SELECT id, name, client_email, start_date, end_date, status, budget, client_name, location FROM projects", db);

    while (query.next()) {
        int row = ui->projectTable->rowCount();
        ui->projectTable->insertRow(row);

        for (int col = 0; col < 9; ++col) {
            QTableWidgetItem *item = new QTableWidgetItem(query.value(col).toString());
            ui->projectTable->setItem(row, col, item);
        }
    }

    updateCharts();
}

bool MainWindow::saveProjectToDatabase(const QStringList &projectData)
{
    if (!db.isOpen() || projectData.size() < 9) return false;

    QSqlQuery query(db);
    query.prepare("INSERT INTO projects (id, name, client_email, start_date, end_date, status, budget, client_name, location) "
                  "VALUES (:id, :name, :client_email, :start_date, :end_date, :status, :budget, :client_name, :location)");

    query.bindValue(":id", projectData[0]);
    query.bindValue(":name", projectData[1]);
    query.bindValue(":client_email", projectData[2]);
    query.bindValue(":start_date", projectData[3]);
    query.bindValue(":end_date", projectData[4]);
    query.bindValue(":status", projectData[5]);
    query.bindValue(":budget", projectData[6].toDouble());
    query.bindValue(":client_name", projectData[7]);
    query.bindValue(":location", projectData[8]);

    if (!query.exec()) {
        QMessageBox::critical(this, "Database Error",
                              "Failed to save project: " + query.lastError().text());
        return false;
    }

    loadProjectsFromDatabase();
    return true;
}

bool MainWindow::updateProjectInDatabase(const QStringList &projectData, int row)
{
    if (!db.isOpen() || projectData.size() < 9 || row < 0) return false;

    QString projectId = ui->projectTable->item(row, 0)->text();

    QSqlQuery query(db);
    query.prepare("UPDATE projects SET "
                  "name = :name, "
                  "client_email = :client_email, "
                  "start_date = :start_date, "
                  "end_date = :end_date, "
                  "status = :status, "
                  "budget = :budget, "
                  "client_name = :client_name, "
                  "location = :location "
                  "WHERE id = :id");

    query.bindValue(":id", projectId);
    query.bindValue(":name", projectData[1]);
    query.bindValue(":client_email", projectData[2]);
    query.bindValue(":start_date", projectData[3]);
    query.bindValue(":end_date", projectData[4]);
    query.bindValue(":status", projectData[5]);
    query.bindValue(":budget", projectData[6].toDouble());
    query.bindValue(":client_name", projectData[7]);
    query.bindValue(":location", projectData[8]);

    if (!query.exec()) {
        QMessageBox::critical(this, "Database Error",
                              "Failed to update project: " + query.lastError().text());
        return false;
    }

    loadProjectsFromDatabase();
    return true;
}

void MainWindow::updateCharts()
{
    showProjectStatusStats();
    showBudgetStats();
}


void MainWindow::checkDatabaseVersion()
{
    QSqlQuery query(db);
    if (!query.exec("PRAGMA user_version")) {
        qWarning() << "Failed to check database version:" << query.lastError();
        return;
    }

    if (query.next()) {
        int version = query.value(0).toInt();
        if (version < 1) {
            // Mettre à jour la base de données si nécessaire
            if (!query.exec("PRAGMA user_version = 1")) {
                qWarning() << "Failed to update database version:" << query.lastError();
            }
        }
    }
}

void MainWindow::deleteProjectFromDatabase(const QString &projectId)
{
    if (!db.isOpen()) return;

    QSqlQuery query(db);
    query.prepare("DELETE FROM projects WHERE id = :id");
    query.bindValue(":id", projectId);

    if (!query.exec()) {
        QMessageBox::critical(this, "Database Error",
                              QString("Failed to delete project: %1").arg(query.lastError().text()));
    }
}

void MainWindow::on_addBtn_clicked()
{
    // Créer une boîte de dialogue pour saisir les informations du projet
    QDialog dialog(this);
    QFormLayout form(&dialog);

    // Champs de saisie
    QList<QLineEdit*> fields;
    QLineEdit *idField = new QLineEdit(&dialog);
    QLineEdit *nameField = new QLineEdit(&dialog);
    QLineEdit *clientEmailField = new QLineEdit(&dialog);
    QLineEdit *startDateField = new QLineEdit(&dialog);
    QLineEdit *endDateField = new QLineEdit(&dialog);
    QLineEdit *statusField = new QLineEdit(&dialog);
    QLineEdit *budgetField = new QLineEdit(&dialog);
    QLineEdit *clientNameField = new QLineEdit(&dialog);
    QLineEdit *locationField = new QLineEdit(&dialog);

    // Ajouter les champs au formulaire
    form.addRow("ID:", idField);
    form.addRow("Nom:", nameField);
    form.addRow("Email client:", clientEmailField);
    form.addRow("Date début (YYYY-MM-DD):", startDateField);
    form.addRow("Date fin (YYYY-MM-DD):", endDateField);
    form.addRow("Statut:", statusField);
    form.addRow("Budget:", budgetField);
    form.addRow("Nom client:", clientNameField);
    form.addRow("Localisation:", locationField);

    fields << idField << nameField << clientEmailField << startDateField
           << endDateField << statusField << budgetField << clientNameField << locationField;

    // Boutons OK/Annuler
    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    QObject::connect(&buttonBox, &QDialogButtonBox::accepted, [&]() {
        if (validateProjectData(fields)) {
            QStringList projectData;
            for (const auto& field : fields) {
                projectData << field->text();
            }
            if (saveProjectToDatabase(projectData)) {
                dialog.accept();
            }
        }
    });
    QObject::connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        loadProjectsFromDatabase();
    }
}



QString MainWindow::cleanAmountInput(const QString &input) {
    QString cleaned = input;
    QRegularExpression re("[^0-9\\.]");
    cleaned.remove(re);
    return cleaned;
}

void MainWindow::on_modifyBtn_clicked()
{
    int row = ui->projectTable->currentRow();
    QDialog dialog(this);
    QFormLayout form(&dialog);
    dialog.setWindowTitle("Edit Project");

    QStringList labels = {"Project ID:", "Name:", "client_email:", "Start Date:",
                          "End Date:", "Status:", "Budget:", "Client ID:", "Location:"};
    QList<QLineEdit*> fields;

    for (int i = 0; i < labels.size(); ++i) {
        QLineEdit *lineEdit = new QLineEdit(ui->projectTable->item(row, i)->text(), &dialog);
        form.addRow(labels[i], lineEdit);
        fields << lineEdit;
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                               Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted && validateProjectData(fields)) {
        QStringList projectData;
        for (int i = 0; i < fields.size(); ++i) {
            QString value = fields[i]->text().trimmed();
            if (i == 6) value = "$" + cleanAmountInput(value);
            projectData << value;
            ui->projectTable->item(row, i)->setText(value);
        }

        updateProjectInDatabase(projectData, row);
    }
}

void MainWindow::on_deleteBtn_clicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("Confirm Delete");
    msgBox.setText("Are you sure you want to delete this project?");
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);

    if (msgBox.exec() == QMessageBox::Yes) {
        QString projectId = ui->projectTable->item(ui->projectTable->currentRow(), 0)->text();
        deleteProjectFromDatabase(projectId);
        ui->projectTable->removeRow(ui->projectTable->currentRow());
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
    QString html = "<h1>Project Report</h1><table border='1'><tr>";

    // Header
    for (int col = 0; col < ui->projectTable->columnCount(); ++col) {
        html += "<th>" + ui->projectTable->horizontalHeaderItem(col)->text() + "</th>";
    }
    html += "</tr>";

    // Data
    for (int row = 0; row < ui->projectTable->rowCount(); ++row) {
        html += "<tr>";
        for (int col = 0; col < ui->projectTable->columnCount(); ++col) {
            html += "<td>" + ui->projectTable->item(row, col)->text() + "</td>";
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

    for (int row = 0; row < ui->projectTable->rowCount(); ++row) {
        bool matchFound = false;
        // Search in ID, Name and Client (columns 0,1,7)
        for (int col : {0, 1, 7}) {
            if (ui->projectTable->item(row, col)->text().toLower().contains(searchText)) {
                matchFound = true;
                break;
            }
        }
        ui->projectTable->setRowHidden(row, !matchFound);
    }
}



void MainWindow::on_notifyClientBtn_clicked()
{// Vérification de sélection avec message en français
    int currentRow = ui->projectTable->currentRow();
    if (currentRow < 0) {
        QMessageBox::warning(this, "Sélection requise",
                             "Veuillez sélectionner un projet dans le tableau.");
        return;
    }

    // Récupération des données selon l'ordre exact de votre schéma
    QTableWidgetItem* idItem = ui->projectTable->item(currentRow, 0);
    QTableWidgetItem* emailItem = ui->projectTable->item(currentRow, 2);
    QTableWidgetItem* nameItem = ui->projectTable->item(currentRow, 7);
    QTableWidgetItem* projectNameItem = ui->projectTable->item(currentRow, 1);
    QTableWidgetItem* statusItem = ui->projectTable->item(currentRow, 5);

    // Validation robuste des données
    if (!emailItem || emailItem->text().isEmpty() || !emailItem->text().contains('@')) {
        QMessageBox::critical(this, "Email invalide",
                              "L'email du client est manquant ou invalide.");
        return;
    }

    if (!idItem || idItem->text().isEmpty()) {
        QMessageBox::critical(this, "ID manquant",
                              "Le projet sélectionné n'a pas d'identifiant valide.");
        return;
    }

    // Construction des paramètres avec valeurs par défaut si null
    QString projectId = idItem->text();
    QString clientEmail = emailItem->text();
    QString clientName = nameItem ? nameItem->text() : "Client";
    QString projectName = projectNameItem ? projectNameItem->text() : "Projet sans nom";
    QString status = statusItem ? statusItem->text() : "Statut inconnu";

    sendNotificationEmail(clientEmail, clientName, projectName, status, projectId);

}

void MainWindow::on_showMapBtn_clicked()
{
    int row = ui->projectTable->currentRow();
    QString location = ui->projectTable->item(row, 8)->text();

    if (location.isEmpty()) {
        QMessageBox::warning(this, "Erreur", "Aucune localisation spécifiée pour ce projet.");
        return;
    }

    QString encodedLocation = QUrl::toPercentEncoding(location);
    QString mapUrl = QString("https://www.google.com/maps/place/%1").arg(encodedLocation);

    QDesktopServices::openUrl(QUrl(mapUrl));
}
QChart* MainWindow::createProjectStatusChart() {
    if (!db.isOpen()) return nullptr;

    // Récupérer les données de statut des projets depuis la base de données
    QSqlQuery query(db);
    query.prepare("SELECT status, COUNT(*) FROM projects GROUP BY status");

    if (!query.exec()) {
        qDebug() << "Error fetching project status data:" << query.lastError().text();
        return nullptr;
    }

    // Créer un diagramme circulaire
    QPieSeries *series = new QPieSeries();

    while (query.next()) {
        QString status = query.value(0).toString();
        int count = query.value(1).toInt();
        series->append(status + " (" + QString::number(count) + ")", count);
    }

    // Configuration du graphique
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Project Status Distribution");
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->legend()->setAlignment(Qt::AlignRight);

    // Style des tranches
    for (QPieSlice *slice : series->slices()) {
        slice->setLabelVisible();
        slice->setLabelColor(Qt::white);
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setBorderColor(Qt::white);
    }

    return chart;
}

QChart* MainWindow::createBudgetChart() {
    if (!db.isOpen()) return nullptr;

    // Récupérer les données de budget par client
    QSqlQuery query(db);
    query.prepare("SELECT client_id, SUM(budget) FROM projects GROUP BY client_id");

    if (!query.exec()) {
        qDebug() << "Error fetching budget data:" << query.lastError().text();
        return nullptr;
    }

    // Créer un diagramme à barres
    QBarSeries *series = new QBarSeries();
    QStringList categories;

    while (query.next()) {
        QBarSet *set = new QBarSet(query.value(0).toString());
        *set << query.value(1).toDouble();
        series->append(set);
        categories << "Client " + query.value(0).toString();
    }

    // Configuration du graphique
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Budget by Client");
    chart->setAnimationOptions(QChart::SeriesAnimations);

    // Axes
    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->append(categories);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelFormat("$%d");
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    return chart;
}

void MainWindow::showProjectStatusStats() {
    QChart *chart = createProjectStatusChart();
    if (!chart) return;

    // Créer une vue pour le graphique
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(400, 300);

    // Afficher dans une boîte de dialogue
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Project Status Statistics");
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->addWidget(chartView);
    dialog->resize(500, 400);
    dialog->exec();
}

void MainWindow::showBudgetStats() {
    QChart *chart = createBudgetChart();
    if (!chart) return;

    // Créer une vue pour le graphique
    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(600, 400);

    // Afficher dans une boîte de dialogue
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Budget Statistics");
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    layout->addWidget(chartView);
    dialog->resize(700, 500);
    dialog->exec();
}
void MainWindow::on_showProjectStats_clicked() {
    showProjectStatusStats();
}

void MainWindow::on_showBudgetStats_clicked() {
    showBudgetStats();
}


void MainWindow::on_sortBtn_clicked()
{


    QMenu sortMenu;
    QAction *sortByIdAsc = sortMenu.addAction("By ID (Ascending)");
    QAction *sortByIdDesc = sortMenu.addAction("By ID (Descending)");
    QAction *sortByEndDateAsc = sortMenu.addAction("By End Date (Soonest)");
    QAction *sortByEndDateDesc = sortMenu.addAction("By End Date (Farthest)");
    QAction *sortByBudgetAsc = sortMenu.addAction("By Budget (Lowest)");
    QAction *sortByBudgetDesc = sortMenu.addAction("By Budget (Highest)");

    connect(sortByIdAsc, &QAction::triggered, [this]() { sortProjects(0, Qt::AscendingOrder); });
    connect(sortByIdDesc, &QAction::triggered, [this]() { sortProjects(0, Qt::DescendingOrder); });
    connect(sortByEndDateAsc, &QAction::triggered, [this]() { sortProjects(4, Qt::AscendingOrder); });
    connect(sortByEndDateDesc, &QAction::triggered, [this]() { sortProjects(4, Qt::DescendingOrder); });
    connect(sortByBudgetAsc, &QAction::triggered, [this]() { sortProjects(6, Qt::AscendingOrder); });
    connect(sortByBudgetDesc, &QAction::triggered, [this]() { sortProjects(6, Qt::DescendingOrder); });

    sortMenu.exec(ui->sortBtn->mapToGlobal(QPoint(0, ui->sortBtn->height())));
}
void MainWindow::sortProjects(int column, Qt::SortOrder order)
{
    ui->projectTable->sortItems(column, order);
}



void MainWindow::sendNotificationEmail(const QString &clientEmail,
                                       const QString &clientName,
                                       const QString &projectName,
                                       const QString &projectStatus,
                                       const QString &projectId)
{
    if (!networkManager) {
        qCritical("NetworkManager non initialisé!");
        QMessageBox::critical(this, "Erreur système",
                              "Le service de notification n'est pas disponible.");
        return;
    }

    // Configuration de la requête HTTPS
    QNetworkRequest request(QUrl("https://api.emailjs.com/api/v1.0/email/send"));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    request.setRawHeader("Accept", "application/json");

    // Construction du payload JSON conforme à votre DB
    QJsonObject emailParams {
                            {"to_email", clientEmail},
                            {"client_name", clientName},
                            {"project_name", projectName},
                            {"project_status", projectStatus},
                            {"project_id", projectId},

                            };

    QJsonObject payload {
        {"service_id", "service_errhzkl"},
        {"template_id", "template_lbmy5mh"},
        {"user_id", "rIWgGoIZXZOXhd7oF"},
        {"template_params", emailParams}
    };

    // Envoi asynchrone avec gestion d'erreur détaillée
    QNetworkReply* reply = networkManager->post(request, QJsonDocument(payload).toJson());

    connect(reply, &QNetworkReply::finished, [this, reply, clientEmail]() {
        reply->deleteLater(); // Nettoyage automatique

        if (reply->error() != QNetworkReply::NoError) {
            qWarning("Erreur réseau: %s", qPrintable(reply->errorString()));
            QMessageBox::warning(this, "Échec d'envoi",
                                 QString("Impossible d'envoyer à %1\nErreur: %2")
                                     .arg(clientEmail)
                                     .arg(reply->errorString()));
            return;
        }

        // Vérification de la réponse JSON
        QJsonDocument response = QJsonDocument::fromJson(reply->readAll());
        if (response["status"].toString() != "success") {
            QMessageBox::warning(this, "Erreur API",
                                 "L'API a renvoyé une erreur: " + response["message"].toString());
            return;
        }

        // Succès - journalisation et feedback
        qInfo("Notification envoyée à %s pour le projet %s",
              qPrintable(clientEmail),
              qPrintable(response["project_id"].toString()));

        QMessageBox::information(this, "Succès",
                                 "Le client a été notifié avec succès.");
    });
}

bool MainWindow::validateProjectData(const QList<QLineEdit*>& fields)
{
    if (fields.size() < 9) return false;

    // Vérifier que tous les champs sont remplis
    for (const auto& field : fields) {
        if (field->text().isEmpty()) {
            QMessageBox::warning(this, "Validation Error", "Tous les champs doivent être remplis.");
            return false;
        }
    }

    // Vérifier le format des dates
    QDate startDate = QDate::fromString(fields[3]->text(), "yyyy-MM-dd");
    QDate endDate = QDate::fromString(fields[4]->text(), "yyyy-MM-dd");

    if (!startDate.isValid() || !endDate.isValid()) {
        QMessageBox::warning(this, "Validation Error", "Format de date invalide. Utilisez YYYY-MM-DD.");
        return false;
    }

    if (startDate > endDate) {
        QMessageBox::warning(this, "Validation Error", "La date de fin doit être après la date de début.");
        return false;
    }

    // Vérifier que le budget est un nombre valide
    bool ok;
    fields[6]->text().toDouble(&ok);
    if (!ok) {
        QMessageBox::warning(this, "Validation Error", "Le budget doit être un nombre valide.");
        return false;
    }

    return true;
}
void MainWindow::on_navClientsBtn_clicked()
{
    // Implementation of navClientsBtn_clicked
}

void MainWindow::on_navProjectsBtn_clicked()
{
    // Implementation of navProjectsBtn_clicked
}

void MainWindow::on_navInvoicesBtn_clicked()
{
    // Implementation of navInvoicesBtn_clicked
}

void MainWindow::on_navEmployeesBtn_clicked()
{
    // Implementation of navEmployeesBtn_clicked
}

void MainWindow::on_navMaterialsBtn_clicked()
{
    // Implementation of navMaterialsBtn_clicked
}

void MainWindow::on_navTasksBtn_clicked()
{
    // Implementation of navTasksBtn_clicked
}

void MainWindow::handleNavButtonClick(QAbstractButton *button)
{
    Q_UNUSED(button);
    // Implementation of handleNavButtonClick
}

MainWindow::~MainWindow()
{
    if (db.isOpen()) {
        db.close();
    }
    delete ui;
}
