#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "databaseconnection.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QPrinter>
#include <QTextDocument>
#include <QVBoxLayout>
#include <QLabel>
#include <QButtonGroup>
#include <QScrollBar>
#include <QInputDialog>
#include <QStandardPaths>
#include <QHeaderView>
#include <QTableWidgetItem>
#include <QStatusBar>
#include <QFormLayout>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QDialogButtonBox>
#include <QAbstractButton>
#include <QSettings>
#include <QStyle>
#include <QIcon>
#include <QtCharts/QPieSeries>
#include <QtCharts/QBarSeries>
#include <QtCharts/QBarSet>
#include <QtCharts/QValueAxis>
#include <QtCharts/QBarCategoryAxis>
#include <QtCharts/QChartView>

bool Material::saveToDatabase() const {
    QSqlQuery query;
    query.prepare("INSERT OR REPLACE INTO materials (id, name, category, supplier, unit_cost, unit_type, stock_quantity, image_path) "
                  "VALUES (:id, :name, :category, :supplier, :unit_cost, :unit_type, :stock_quantity, :image_path)");
    query.bindValue(":id", id);
    query.bindValue(":name", name);
    query.bindValue(":category", category);
    query.bindValue(":supplier", supplier);
    query.bindValue(":unit_cost", unitCost);
    query.bindValue(":unit_type", unitType);
    query.bindValue(":stock_quantity", stockQuantity);
    query.bindValue(":image_path", imagePath);

    if (!query.exec()) {
        qCritical() << "Failed to save material:" << query.lastError().text();
        return false;
    }
    return true;
}

bool Material::deleteFromDatabase() const {
    QSqlQuery query;
    query.prepare("DELETE FROM materials WHERE id = :id");
    query.bindValue(":id", id);
    return query.exec();
}

QVector<Material> Material::loadAllFromDatabase() {
    QVector<Material> materials;
    QSqlQuery query("SELECT * FROM materials");

    while (query.next()) {
        Material m;
        m.id = query.value("id").toString();
        m.name = query.value("name").toString();
        m.category = query.value("category").toString();
        m.supplier = query.value("supplier").toString();
        m.unitCost = query.value("unit_cost").toDouble();
        m.unitType = query.value("unit_type").toString();
        m.stockQuantity = query.value("stock_quantity").toInt();
        m.imagePath = query.value("image_path").toString();
        materials.append(m);
    }
    return materials;
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow),
    networkManager(new QNetworkAccessManager(this)),
    settings(new QSettings("YourCompany", "MaterialManagementSystem", this))
{
    ui->setupUi(this);
    setWindowTitle("Gestion de Bureau d'Architecture");
    resize(1400, 900);

    // Initialize theme
    darkTheme = settings->value("darkTheme", false).toBool();

    // Initialize database and load data
    DatabaseConnection::instance()->initializeDatabase();
    loadDataFromDatabase();

    // Setup UI components
    setupNavigation();
    setupMaterialsTable();
    setupStatsPanel();
    setupThemeToggle();

    // Apply initial theme
    applyTheme(darkTheme);
    updateCharts();
    statusBar()->setStyleSheet(QString("QStatusBar { background: %1; }").arg(darkTheme ? "#212529" : "#f8f9fa"));
}

void MainWindow::loadDataFromDatabase()
{
    materials = Material::loadAllFromDatabase();
    if (materials.isEmpty()) {
        qDebug() << "Database is empty, adding sample data";
        addSampleDataToDatabase();
        materials = Material::loadAllFromDatabase();
    }
}

void MainWindow::addSampleDataToDatabase()
{
    QVector<Material> sampleMaterials = {
        {"MAT-001", "Panneau de bouleau", "Bois", "Bois & Cie", 45.99, "Panneau", 12, ""},
        {"MAT-002", "Poutre en acier", "Métal", "MétalSolutions", 8.75, "Mètre linéaire", 25, ""},
        {"MAT-003", "Plaque de verre", "Verre", "Verres Clair", 120.50, "Panneau", 8, ""},
        {"MAT-004", "Bloc de béton", "Béton", "BétonPrêt", 3.20, "Unité", 150, ""},
        {"MAT-005", "Feuille d'aluminium", "Métal", "MétalSolutions", 22.40, "Panneau", 18, ""}
    };

    foreach (const Material &m, sampleMaterials) {
        if (!m.saveToDatabase()) {
            qCritical() << "Failed to save sample material:" << m.id;
        }
    }
}

void MainWindow::setupNavigation()
{
    QButtonGroup *navGroup = new QButtonGroup(this);
    navGroup->addButton(ui->navMaterialsBtn);
    navGroup->addButton(ui->navClientsBtn);
    navGroup->addButton(ui->navProjectsBtn);
    navGroup->addButton(ui->navInvoicesBtn);
    navGroup->addButton(ui->navTasksBtn);

    QString navButtonStyle =
        "QPushButton { text-align: left; padding: 12px; color: white; border: none; background: #343a40; font-size: 12px; }"
        "QPushButton:hover { background: #23272b; }"
        "QPushButton[active=true] { background: #007bff; border-left: 4px solid #ffc107; }";

    ui->navMaterialsBtn->setProperty("active", true);
    for(auto btn : navGroup->buttons()) {
        btn->setStyleSheet(navButtonStyle);
    }

    connect(navGroup, &QButtonGroup::buttonClicked,
            this, &MainWindow::handleNavButtonClick);
}

void MainWindow::setupMaterialsTable()
{
    ui->materialsTable->setColumnCount(8);
    QStringList headers = {"ID", "Nom", "Catégorie", "Fournisseur",
                           "Prix Unitaire", "Unité", "Stock", "Image"};
    ui->materialsTable->setHorizontalHeaderLabels(headers);

    ui->materialsTable->horizontalHeader()->setStretchLastSection(true);
    ui->materialsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->materialsTable->setSelectionMode(QAbstractItemView::SingleSelection);

    ui->materialsTable->setRowCount(0);
    ui->materialsTable->setRowCount(materials.size());

    for(int row = 0; row < materials.size(); ++row) {
        const auto &m = materials[row];

        QTableWidgetItem* idItem = new QTableWidgetItem(m.id);
        idItem->setForeground(Qt::black);

        QTableWidgetItem* nameItem = new QTableWidgetItem(m.name);
        nameItem->setForeground(Qt::black);

        QTableWidgetItem* categoryItem = new QTableWidgetItem(m.category);
        categoryItem->setForeground(Qt::black);

        QTableWidgetItem* supplierItem = new QTableWidgetItem(m.supplier);
        supplierItem->setForeground(Qt::black);

        QTableWidgetItem* costItem = new QTableWidgetItem(QString::number(m.unitCost, 'f', 2) + " €");
        costItem->setForeground(Qt::black);

        QTableWidgetItem* unitItem = new QTableWidgetItem(m.unitType);
        unitItem->setForeground(Qt::black);

        QTableWidgetItem* stockItem = new QTableWidgetItem(QString::number(m.stockQuantity));
        stockItem->setForeground(Qt::black);

        ui->materialsTable->setItem(row, 0, idItem);
        ui->materialsTable->setItem(row, 1, nameItem);
        ui->materialsTable->setItem(row, 2, categoryItem);
        ui->materialsTable->setItem(row, 3, supplierItem);
        ui->materialsTable->setItem(row, 4, costItem);
        ui->materialsTable->setItem(row, 5, unitItem);
        ui->materialsTable->setItem(row, 6, stockItem);

        if(!m.imagePath.isEmpty()) {
            QLabel *imgLabel = new QLabel();
            QPixmap pixmap(m.imagePath);
            if(!pixmap.isNull()) {
                imgLabel->setPixmap(pixmap.scaled(80, 80, Qt::KeepAspectRatio));
            } else {
                imgLabel->setText("Image invalide");
                imgLabel->setAlignment(Qt::AlignCenter);
            }
            ui->materialsTable->setCellWidget(row, 7, imgLabel);
        } else {
            QLabel *emptyLabel = new QLabel("Aucune image");
            emptyLabel->setAlignment(Qt::AlignCenter);
            ui->materialsTable->setCellWidget(row, 7, emptyLabel);
        }
    }

    ui->materialsTable->resizeColumnsToContents();
    ui->materialsTable->setColumnWidth(7, 100);
}

void MainWindow::setupStatsPanel()
{
    QLayoutItem* child;
    while ((child = ui->statsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    QWidget* statsContainer = new QWidget();
    QVBoxLayout* containerLayout = new QVBoxLayout(statsContainer);
    containerLayout->setContentsMargins(10, 10, 10, 10);
    containerLayout->setSpacing(20);

    QLabel* statsTitle = new QLabel("Statistiques des Matériels");
    statsTitle->setStyleSheet("font-size: 18px; font-weight: bold; color: #2c3e50;");
    statsTitle->setAlignment(Qt::AlignCenter);
    containerLayout->addWidget(statsTitle);

    QWidget* chartsContainer = new QWidget();
    QHBoxLayout* chartsLayout = new QHBoxLayout(chartsContainer);
    chartsLayout->setContentsMargins(0, 0, 0, 0);
    chartsLayout->setSpacing(20);

    QChartView* costChart = createCostChart();
    QChartView* stockChart = createStockChart();

    costChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    stockChart->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    costChart->setMinimumSize(600, 500);
    stockChart->setMinimumSize(600, 500);

    chartsLayout->addWidget(costChart);
    chartsLayout->addWidget(stockChart);

    containerLayout->addWidget(chartsContainer);
    containerLayout->addStretch();

    ui->statsLayout->addWidget(statsContainer);
    ui->statsGroup->setMinimumHeight(650);
}

QChartView* MainWindow::createCostChart()
{
    QPieSeries *series = new QPieSeries();

    QMap<QString, double> categoryCosts;
    for (const auto &m : materials) {
        categoryCosts[m.category] += m.unitCost * m.stockQuantity;
    }
    QChart *chart = new QChart();
    chart->addSeries(series);
    chart->setTitle("Valeur Totale par Catégorie");

    // Set text colors based on theme
    QColor textColor = darkTheme ? Qt::white : Qt::black;
    chart->setTitleBrush(QBrush(textColor));
    chart->legend()->setLabelColor(textColor);

    for (auto slice : series->slices()) {
        slice->setLabelColor(textColor);
    }

    QList<QColor> colors;
    if (darkTheme) {
        colors = {
            QColor("#3498db"), QColor("#e74c3c"), QColor("#2ecc71"),
            QColor("#f39c12"), QColor("#9b59b6"), QColor("#1abc9c")
        };
    } else {
        colors = {
            QColor("#428bca"), QColor("#d9534f"), QColor("#5cb85c"),
            QColor("#f0ad4e"), QColor("#5bc0de"), QColor("#5cb85c")
        };
    }

    int colorIndex = 0;
    for (auto it = categoryCosts.begin(); it != categoryCosts.end(); ++it) {
        QPieSlice *slice = series->append(it.key() + QString(" (%1€)").arg(it.value(), 0, 'f', 2), it.value());
        slice->setLabelVisible();
        slice->setLabelPosition(QPieSlice::LabelOutside);
        slice->setLabelArmLengthFactor(0.2);
        slice->setBrush(colors[colorIndex % colors.size()]);
        colorIndex++;
    }

    chart->addSeries(series);
    chart->setTitle("Valeur Totale par Catégorie");
    chart->setTitleFont(QFont("Arial", 14, QFont::Bold));
    chart->legend()->setVisible(true);
    chart->legend()->setAlignment(Qt::AlignRight);
    chart->legend()->setFont(QFont("Arial", 10));
    chart->setBackgroundVisible(false);

    if (darkTheme) {
        chart->setTitleBrush(QBrush(Qt::white));
        chart->legend()->setLabelColor(Qt::white);
        for (auto slice : series->slices()) {
            slice->setLabelColor(Qt::white);
        }
    } else {
        chart->setTitleBrush(QBrush(Qt::black));
        chart->legend()->setLabelColor(Qt::black);
        for (auto slice : series->slices()) {
            slice->setLabelColor(Qt::black);
        }
    }

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background: transparent;");
    return chartView;
}

QChartView* MainWindow::createStockChart()
{
    QVector<Material> sorted = materials;
    std::sort(sorted.begin(), sorted.end(), [](const Material &a, const Material &b) {
        return a.stockQuantity > b.stockQuantity;
    });

    QBarSeries *series = new QBarSeries();
    QStringList categories;
    QList<QColor> colors;
    if (darkTheme) {
        colors = {QColor("#3498db"), QColor("#e74c3c"), QColor("#2ecc71"), QColor("#f39c12"), QColor("#9b59b6")};
    } else {
        colors = {QColor("#428bca"), QColor("#d9534f"), QColor("#5cb85c"), QColor("#f0ad4e"), QColor("#5bc0de")};
    }

    for (int i = 0; i < qMin(5, sorted.size()); i++) {
        QBarSet *set = new QBarSet(sorted[i].name);
        *set << sorted[i].stockQuantity;
        set->setColor(colors[i % colors.size()]);
        set->setLabelColor(darkTheme ? Qt::white : Qt::black);
        series->append(set);
        categories << sorted[i].name;
    }

    QChart *chart = new QChart();

    chart->addSeries(series);
    chart->setTitle("Top 5 Matériels par Stock");
    chart->setTitleFont(QFont("Arial", 14, QFont::Bold));
    chart->setAnimationOptions(QChart::SeriesAnimations);
    chart->setBackgroundVisible(false);

    if (darkTheme) {
        chart->setTitleBrush(QBrush(Qt::white));
    } else {
        chart->setTitleBrush(QBrush(Qt::black));
    }
    QColor textColor = darkTheme ? Qt::white : Qt::black;
    chart->setTitleBrush(QBrush(textColor));


    QBarCategoryAxis *axisX = new QBarCategoryAxis();
    axisX->setLabelsColor(textColor);
    axisX->append(categories);
    axisX->setLabelsFont(QFont("Arial", 10));
    axisX->setLabelsColor(darkTheme ? Qt::white : Qt::black);
    chart->addAxis(axisX, Qt::AlignBottom);
    series->attachAxis(axisX);

    QValueAxis *axisY = new QValueAxis();
    axisY->setLabelsColor(textColor);
    axisY->setLabelFormat("%d");
    axisY->setLabelsFont(QFont("Arial", 10));
    axisY->setLabelsColor(darkTheme ? Qt::white : Qt::black);
    chart->addAxis(axisY, Qt::AlignLeft);
    series->attachAxis(axisY);

    QChartView *chartView = new QChartView(chart);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setStyleSheet("background: transparent;");
    return chartView;
}

void MainWindow::updateCharts()
{
    // Clear existing charts
    QLayoutItem* child;
    while ((child = ui->statsLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }

    // Create new charts with current theme
    QChartView* costChart = createCostChart();
    QChartView* stockChart = createStockChart();

    // Add to layout
    QHBoxLayout* chartsLayout = new QHBoxLayout();
    chartsLayout->addWidget(costChart);
    chartsLayout->addWidget(stockChart);

    ui->statsLayout->addLayout(chartsLayout);
}

void MainWindow::handleNavButtonClick(QAbstractButton *clickedButton)
{
    foreach(QAbstractButton* btn, clickedButton->group()->buttons()) {
        btn->setProperty("active", false);
        btn->style()->polish(btn);
    }
    clickedButton->setProperty("active", true);
    clickedButton->style()->polish(clickedButton);

    statusBar()->showMessage("Affichage : " + clickedButton->text(), 2000);
}

void MainWindow::setupThemeToggle()
{
    QPushButton *themeToggleBtn = new QPushButton(this);
    themeToggleBtn->setObjectName("themeToggleBtn");
    themeToggleBtn->setFixedSize(32, 32);
    themeToggleBtn->setFlat(true);
    themeToggleBtn->setIconSize(QSize(24, 24));
    themeToggleBtn->setCursor(Qt::PointingHandCursor);

    // Set a nice hover effect
    themeToggleBtn->setStyleSheet(
        "QPushButton {"
        "border: none;"
        "background: transparent;"
        "}"
        "QPushButton:hover {"
        "background: rgba(255,255,255,0.1);"
        "border-radius: 4px;"
        "}"
        );

    updateThemeToggleIcon(themeToggleBtn);

    statusBar()->addPermanentWidget(themeToggleBtn);
    connect(themeToggleBtn, &QPushButton::clicked, this, &MainWindow::toggleTheme);
}

void MainWindow::updateThemeToggleIcon(QPushButton* btn)
{
    if (!btn) return;

    if (darkTheme) {
        // Use a sun icon for light mode
        btn->setIcon(QIcon(":/icons/light_mode.svg"));  // Make sure you have this icon in your resources
        btn->setToolTip(tr("Switch to light theme"));
    } else {
        // Use a moon icon for dark mode
        btn->setIcon(QIcon(":/icons/dark_mode.svg"));  // Make sure you have this icon in your resources
        btn->setToolTip(tr("Switch to dark theme"));
    }
}

void MainWindow::on_navMaterialsBtn_clicked() {}
void MainWindow::on_navClientsBtn_clicked() {}
void MainWindow::on_navProjectsBtn_clicked() {}
void MainWindow::on_navInvoicesBtn_clicked() {}
void MainWindow::on_navTasksBtn_clicked() {}

void MainWindow::toggleTheme()
{
    darkTheme = !darkTheme;
    settings->setValue("darkTheme", darkTheme);
    applyTheme(darkTheme);

    // Update the theme toggle button icon
    QPushButton* themeToggleBtn = findChild<QPushButton*>("themeToggleBtn");
    if (themeToggleBtn) {
        if (darkTheme) {
            themeToggleBtn->setIcon(QIcon(":/icons/light_mode.svg"));
            themeToggleBtn->setToolTip("Switch to light theme");
        } else {
            themeToggleBtn->setIcon(QIcon(":/icons/dark_mode.svg"));
            themeToggleBtn->setToolTip("Switch to dark theme");
        }
    }
}
void MainWindow::applyTheme(bool dark)
{
    QPalette palette;

    if (dark) {
        // Dark theme palette
        palette.setColor(QPalette::Window, QColor(53, 53, 53));
        palette.setColor(QPalette::WindowText, Qt::white);
        palette.setColor(QPalette::Base, QColor(25, 25, 25));
        palette.setColor(QPalette::AlternateBase, QColor(53, 53, 53));
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::white);
        palette.setColor(QPalette::Text, Qt::white);
        palette.setColor(QPalette::Button, QColor(53, 53, 53));
        palette.setColor(QPalette::ButtonText, Qt::white);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::black);
    } else {
        // Light theme palette
        palette.setColor(QPalette::Window, Qt::white);
        palette.setColor(QPalette::WindowText, Qt::black);
        palette.setColor(QPalette::Base, QColor(240, 240, 240));
        palette.setColor(QPalette::AlternateBase, Qt::white);
        palette.setColor(QPalette::ToolTipBase, Qt::white);
        palette.setColor(QPalette::ToolTipText, Qt::black);
        palette.setColor(QPalette::Text, Qt::black);
        palette.setColor(QPalette::Button, QColor(240, 240, 240));
        palette.setColor(QPalette::ButtonText, Qt::black);
        palette.setColor(QPalette::BrightText, Qt::red);
        palette.setColor(QPalette::Link, QColor(42, 130, 218));
        palette.setColor(QPalette::Highlight, QColor(42, 130, 218));
        palette.setColor(QPalette::HighlightedText, Qt::white);
    }
    // Sidebar styling
    QString sidebarStyle;
    if (dark) {
        sidebarStyle = "background: #212529;";
    } else {
        sidebarStyle = "background: #343a40;"; // Or any light color you prefer
    }
    ui->navPanel->setStyleSheet(sidebarStyle);

    // Navigation buttons styling
    QString navButtonStyle = QString(
                                 "QPushButton {"
                                 "text-align: left;"
                                 "padding: 12px 15px;"
                                 "color: white;"  // Keep text white for contrast
                                 "border: none;"
                                 "background: transparent;"
                                 "font-size: 12px;"
                                 "}"
                                 "QPushButton:hover {"
                                 "background: %1;"
                                 "}"
                                 "QPushButton[active=true] {"
                                 "background: #0d6efd;"
                                 "border-left: 4px solid #ffc107;"
                                 "}"
                                 ).arg(dark ? "#343a40" : "#495057");

    // Apply to all nav buttons
    QList<QPushButton*> navButtons = {
        ui->navMaterialsBtn, ui->navClientsBtn,
        ui->navProjectsBtn, ui->navInvoicesBtn,
        ui->navTasksBtn
    };

    for (auto btn : navButtons) {
        btn->setStyleSheet(navButtonStyle);
    }
    qApp->setPalette(palette);

    // Now apply specific stylesheets
    if (dark) {
        // Dark theme specific styles
        ui->centralwidget->setStyleSheet("background: #353535;");
        ui->materialsTable->setStyleSheet(
            "QTableWidget { background: #2d2d2d; color: white; }"
            "QHeaderView::section { background: #212529; color: white; }"
            );
    } else {
        // Light theme specific styles
        ui->centralwidget->setStyleSheet("background: white;");
        ui->materialsTable->setStyleSheet(
            "QTableWidget { background: white; color: black; }"
            "QHeaderView::section { background: #f8f9fa; color: black; }"
            );
    }

    // Force refresh
    this->style()->unpolish(this);
    this->style()->polish(this);
    update();
}
void MainWindow::on_addBtn_clicked()
{
    QDialog dialog(this);
    QFormLayout form(&dialog);

    QList<QWidget*> fields;
    fields << new QLineEdit(&dialog);  // ID
    fields << new QLineEdit(&dialog);  // Name
    QComboBox* categoryCombo = new QComboBox(&dialog);
    categoryCombo->addItems({"Bois", "Métal", "Verre", "Béton", "Plastique"});
    fields << categoryCombo;
    fields << new QLineEdit(&dialog);  // Supplier
    QDoubleSpinBox* costSpin = new QDoubleSpinBox(&dialog);
    costSpin->setRange(0, 99999);
    costSpin->setSuffix(" €");
    fields << costSpin;
    QComboBox* unitCombo = new QComboBox(&dialog);
    unitCombo->addItems({"Panneau", "Kg", "Mètre linéaire", "Unité", "Rouleau"});
    fields << unitCombo;
    QSpinBox* qtySpin = new QSpinBox(&dialog);
    qtySpin->setRange(0, 9999);
    fields << qtySpin;
    QPushButton* imageBtn = new QPushButton("Parcourir...", &dialog);
    fields << imageBtn;

    QString imagePath;
    connect(imageBtn, &QPushButton::clicked, [&]() {
        imagePath = QFileDialog::getOpenFileName(this, "Sélectionner une image", "", "Images (*.png *.jpg)");
    });

    QStringList labels = {"ID Matériel:", "Nom:", "Catégorie:", "Fournisseur:",
                          "Prix unitaire:", "Unité:", "Quantité:", "Image:"};
    for(int i = 0; i < fields.size(); ++i) {
        form.addRow(labels[i], fields[i]);
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        Material m;
        m.id = static_cast<QLineEdit*>(fields[0])->text();
        m.name = static_cast<QLineEdit*>(fields[1])->text();
        m.category = static_cast<QComboBox*>(fields[2])->currentText();
        m.supplier = static_cast<QLineEdit*>(fields[3])->text();
        m.unitCost = static_cast<QDoubleSpinBox*>(fields[4])->value();
        m.unitType = static_cast<QComboBox*>(fields[5])->currentText();
        m.stockQuantity = static_cast<QSpinBox*>(fields[6])->value();
        m.imagePath = imagePath;

        if(m.saveToDatabase()) {
            materials = Material::loadAllFromDatabase();
            setupMaterialsTable();
            updateCharts();
        } else {
            QMessageBox::warning(this, "Erreur", "Échec de l'ajout du matériel à la base de données");
        }
    }
}

void MainWindow::on_modifyBtn_clicked()
{
    int row = ui->materialsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un matériel");
        return;
    }

    Material& m = materials[row];
    QDialog dialog(this);
    QFormLayout form(&dialog);

    QList<QWidget*> fields;
    fields << new QLineEdit(m.id, &dialog);
    fields << new QLineEdit(m.name, &dialog);
    QComboBox* categoryCombo = new QComboBox(&dialog);
    categoryCombo->addItems({"Bois", "Métal", "Verre", "Béton", "Plastique"});
    categoryCombo->setCurrentText(m.category);
    fields << categoryCombo;
    fields << new QLineEdit(m.supplier, &dialog);
    QDoubleSpinBox* costSpin = new QDoubleSpinBox(&dialog);
    costSpin->setRange(0, 99999);
    costSpin->setSuffix(" €");
    costSpin->setValue(m.unitCost);
    fields << costSpin;
    QComboBox* unitCombo = new QComboBox(&dialog);
    unitCombo->addItems({"Panneau", "Kg", "Mètre linéaire", "Unité", "Rouleau"});
    unitCombo->setCurrentText(m.unitType);
    fields << unitCombo;
    QSpinBox* qtySpin = new QSpinBox(&dialog);
    qtySpin->setRange(0, 9999);
    qtySpin->setValue(m.stockQuantity);
    fields << qtySpin;
    QPushButton* imageBtn = new QPushButton(m.imagePath.isEmpty() ? "Parcourir..." : QFileInfo(m.imagePath).fileName(), &dialog);
    fields << imageBtn;

    QString newImagePath = m.imagePath;
    connect(imageBtn, &QPushButton::clicked, [&]() {
        newImagePath = QFileDialog::getOpenFileName(this, "Sélectionner une image", "", "Images (*.png *.jpg)");
        if (!newImagePath.isEmpty()) {
            imageBtn->setText(QFileInfo(newImagePath).fileName());
        }
    });

    QStringList labels = {"ID Matériel:", "Nom:", "Catégorie:", "Fournisseur:",
                          "Prix unitaire:", "Unité:", "Quantité:", "Image:"};
    for(int i = 0; i < fields.size(); ++i) {
        form.addRow(labels[i], fields[i]);
    }

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        m.id = static_cast<QLineEdit*>(fields[0])->text();
        m.name = static_cast<QLineEdit*>(fields[1])->text();
        m.category = static_cast<QComboBox*>(fields[2])->currentText();
        m.supplier = static_cast<QLineEdit*>(fields[3])->text();
        m.unitCost = static_cast<QDoubleSpinBox*>(fields[4])->value();
        m.unitType = static_cast<QComboBox*>(fields[5])->currentText();
        m.stockQuantity = static_cast<QSpinBox*>(fields[6])->value();
        m.imagePath = newImagePath;

        if(m.saveToDatabase()) {
            materials = Material::loadAllFromDatabase();
            setupMaterialsTable();
            updateCharts();
        } else {
            QMessageBox::warning(this, "Erreur", "Échec de la mise à jour du matériel dans la base de données");
        }
    }
}

void MainWindow::on_deleteBtn_clicked()
{
    int row = ui->materialsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un matériel");
        return;
    }

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this, "Confirmation", "Supprimer ce matériel?",
                                  QMessageBox::Yes|QMessageBox::No);

    if (reply == QMessageBox::Yes) {
        if(materials[row].deleteFromDatabase()) {
            materials = Material::loadAllFromDatabase();
            setupMaterialsTable();
            updateCharts();
        } else {
            QMessageBox::warning(this, "Erreur", "Échec de la suppression du matériel de la base de données");
        }
    }
}

void MainWindow::on_exportBtn_clicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Exporter PDF", "", "Fichiers PDF (*.pdf)");
    if (fileName.isEmpty()) return;

    QPrinter printer(QPrinter::HighResolution);
    printer.setOutputFormat(QPrinter::PdfFormat);
    printer.setOutputFileName(fileName);

    QTextDocument doc;
    QString html = "<h1>Rapport des Matériels</h1><table border='1'><tr>";

    // Header
    for (int col = 0; col < ui->materialsTable->columnCount(); ++col) {
        html += "<th>" + ui->materialsTable->horizontalHeaderItem(col)->text() + "</th>";
    }
    html += "</tr>";

    // Data
    for (const auto &m : materials) {
        html += "<tr>";
        html += "<td>" + m.id + "</td>";
        html += "<td>" + m.name + "</td>";
        html += "<td>" + m.category + "</td>";
        html += "<td>" + m.supplier + "</td>";
        html += "<td>" + QString::number(m.unitCost, 'f', 2) + " €</td>";
        html += "<td>" + m.unitType + "</td>";
        html += "<td>" + QString::number(m.stockQuantity) + "</td>";
        html += "<td>" + (m.imagePath.isEmpty() ? "Aucune image" : QFileInfo(m.imagePath).fileName()) + "</td>";
        html += "</tr>";
    }

    html += "</table>";
    doc.setHtml(html);
    doc.print(&printer);

    QMessageBox::information(this, "Succès", "PDF exporté avec succès");
}

void MainWindow::on_searchBtn_clicked()
{
    QString searchText = ui->searchInput->text().trimmed().toLower();

    for (int row = 0; row < ui->materialsTable->rowCount(); ++row) {
        bool matchFound = false;
        for (int col = 0; col < ui->materialsTable->columnCount(); ++col) {
            if (ui->materialsTable->item(row, col)) {
                if (ui->materialsTable->item(row, col)->text().toLower().contains(searchText)) {
                    matchFound = true;
                    break;
                }
            }
        }
        ui->materialsTable->setRowHidden(row, !matchFound);
    }
}

void MainWindow::on_importImageBtn_clicked()
{
    int row = ui->materialsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un matériel");
        return;
    }

    QString imagePath = QFileDialog::getOpenFileName(this, "Sélectionner une image", "", "Images (*.png *.jpg)");
    if (!imagePath.isEmpty()) {
        materials[row].imagePath = imagePath;
        if (materials[row].saveToDatabase()) {
            setupMaterialsTable();
        } else {
            QMessageBox::warning(this, "Erreur", "Échec de la mise à jour de l'image dans la base de données");
        }
    }
}

void MainWindow::on_orderBtn_clicked()
{
    int row = ui->materialsTable->currentRow();
    if (row < 0) {
        QMessageBox::warning(this, "Erreur", "Veuillez sélectionner un matériel");
        return;
    }

    const Material& m = materials[row];
    QDialog dialog(this);
    QFormLayout form(&dialog);

    QSpinBox* qtySpin = new QSpinBox(&dialog);
    qtySpin->setRange(1, 9999);
    qtySpin->setValue(1);
    form.addRow("Quantité à commander:", qtySpin);

    QLineEdit* supplierNote = new QLineEdit(&dialog);
    form.addRow("Note pour le fournisseur:", supplierNote);

    QDialogButtonBox buttonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dialog);
    form.addRow(&buttonBox);

    connect(&buttonBox, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(&buttonBox, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);

    if (dialog.exec() == QDialog::Accepted) {
        int orderQty = qtySpin->value();
        QString note = supplierNote->text();

        QMessageBox::information(this, "Commande passée",
                                 QString("Commande passée pour %1 %2 de %3\nFournisseur: %4\nNote: %5")
                                     .arg(orderQty)
                                     .arg(m.unitType)
                                     .arg(m.name)
                                     .arg(m.supplier)
                                     .arg(note));
    }
}

MainWindow::~MainWindow()
{
    delete ui;
    delete networkManager;
    DatabaseConnection::instance()->~DatabaseConnection();
}
