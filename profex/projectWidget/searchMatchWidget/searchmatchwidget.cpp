/***************************************************************************
                          searchmatchwidget.cpp  -  description
                             -------------------
    begin                : Mon May 20 18:00:00 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
    email                : ndoebelin@gmx.ch
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "ui_searchmatchwidget.h"
#include "searchmatchwidget.h"
#include "strfilefilter.h"
#include "periodictablewidget.h"
#include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/parser/bgmnlstparser.h"
#include "../libXrdIO/parser/bgmnsavparser.h"
#include "../libXrdIO/parser/bgmnstrparser.h"
#include "../libXrdIO/import/importhandler.h"
#include "../libXrdIO/export/exporthandler.h"

#include <QMenu>
#include <QMessageBox>
#include <QFileDialog>
#include <QLineEdit>

SearchMatchWidget::SearchMatchWidget(GraphDataController *c, QWidget *parent) :
    AbstractGraphView(c, parent),
    ui(new Ui::SearchMatchWidget)
{
    ui->setupUi(this);

    refStrManager = BgmnRefStructureManager::getInstance();
    refOutput = nullptr;

    projectLam = QString();
    projectGeqFile = QString();
    defaultEps2 = 0.0;
    ui->labelInstrumentMessage->setText(QString());

    QStringList sheader;
    sheader << "Repository" << "Number of phases";
    ui->treeWidgetDirectories->setColumnCount(sheader.size());
    ui->treeWidgetDirectories->setHeaderLabels(sheader);

    QStringList theader;
    theader << "File" << "Phase" << "Fraction" << "FoM";
    ui->treeWidgetResults->setColumnCount(theader.size());
    ui->treeWidgetResults->setHeaderLabels(theader);

    QStringList pheader;
    pheader << "File" << "Phase" << "Fraction" << "Source";
    ui->treeWidgetPinned->setColumnCount(pheader.size());
    ui->treeWidgetPinned->setHeaderLabels(pheader);

    ui->comboBoxCrystallinity->addItem(tr("Infinite"), 0.0);
    ui->comboBoxCrystallinity->addItem(tr("High"), 0.0025);
    ui->comboBoxCrystallinity->addItem(tr("Medium"), 0.01);
    ui->comboBoxCrystallinity->addItem(tr("Low"), 0.025);
    ui->comboBoxCrystallinity->addItem(tr("Nano"), 0.1);

    ui->comboBoxUnitCellRefinement->addItem(tr("Fix"), 0.0);
    ui->comboBoxUnitCellRefinement->addItem(tr("Strict"), 0.001);
    ui->comboBoxUnitCellRefinement->addItem(tr("Medium"), 0.005);
    ui->comboBoxUnitCellRefinement->addItem(tr("Loose"), 0.01);

    ui->comboBoxInstrument->lineEdit()->setPlaceholderText(QString("<select instrument>"));
    ui->comboBoxLambda->lineEdit()->setPlaceholderText(QString("<select radiation>"));

    nativeFormats << "xy" << "val";

    ui->toolButtonSave->setEnabled(false);
    ui->toolButtonPin->setEnabled(false);
    ui->toolButtonUnpin->setEnabled(false);

    ui->progressBar->setEnabled(false);
    ui->progressBar->reset();
    ui->progressBar->hide();

    buttonToggleAllOptional     = new PeriodicTableToggleAllButton(0, this);
    buttonToggleAllMandatory    = new PeriodicTableToggleAllButton(1, this);
    buttonToggleAllOne          = new PeriodicTableToggleAllButton(2, this);
    buttonToggleAllDiscard      = new PeriodicTableToggleAllButton(3, this);
    buttonToggleAllDiscOptional = new PeriodicTableToggleAllButton(4, this);
    buttonToggleAllDiscOptional->setText(QChar(0x25CF));

    ui->strFileEditor->setLineWrapMode(QPlainTextEdit::NoWrap);
    bhl = new BgmnHighlighter(settings->getSyntaxHighlightingMode(), this);

    // set contents margins of results tab widget to 0 (could this be done in the designer???)
    for (int i = 0; i < ui->tabWidgetResults->count(); ++i) {
        QWidget *w = ui->tabWidgetResults->widget(i);
        if (w) w->layout()->setContentsMargins(0, 0, 0, 0);
    }

    statistics.n = 0;
    statistics.sum = 0.0;
    statistics.mean = 0.0;
    statistics.stdev = 0.0;

    bgmnHandler = new BgmnHandler(QUuid::createUuid());
    diaHandler = new BgmnDiaHandler2();


    ui->treeWidgetResults->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeWidgetPinned->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    initGui();
    initSettings();
    initConnections();
}

SearchMatchWidget::~SearchMatchWidget()
{
    if (bgmnHandler)  delete bgmnHandler;
    if (diaHandler)   delete diaHandler;
    if (bhl)          delete bhl;
    delete ui;
}

void SearchMatchWidget::updateView()
{
    if (!isShown) return;
}

void SearchMatchWidget::customMenuRequestedResults(QPoint p)
{
    contextMenuHeaderResults->popup(ui->treeWidgetResults->header()->viewport()->mapToGlobal(p));
}

void SearchMatchWidget::customMenuRequestedPinned(QPoint p)
{
    contextMenuHeaderPinned->popup(ui->treeWidgetPinned->header()->viewport()->mapToGlobal(p));
}

void SearchMatchWidget::initGui()
{
    contextMenuHeaderResults = new QMenu();
    contextMenuHeaderPinned = new QMenu();

    for (int i = 1; i < ui->treeWidgetResults->header()->count() - 1; ++i) {
        QAction *act = new QAction(ui->treeWidgetResults->headerItem()->text(i), contextMenuHeaderResults);
        act->setCheckable(true);
        act->setData(i);
        contextMenuHeaderResults->addAction(act);
    }

    for (int i = 1; i < ui->treeWidgetPinned->header()->count(); ++i) {
        QAction *act = new QAction(ui->treeWidgetPinned->headerItem()->text(i), contextMenuHeaderPinned);
        act->setCheckable(true);
        act->setData(i);
        contextMenuHeaderPinned->addAction(act);
    }

    QGridLayout *periodicTableToggleLayout = new QGridLayout;

    periodicTableToggleLayout->addWidget(buttonToggleAllOptional,     0, 0);
    periodicTableToggleLayout->addWidget(buttonToggleAllMandatory,    1, 0);
    periodicTableToggleLayout->addWidget(buttonToggleAllOne,          0, 2);
    periodicTableToggleLayout->addWidget(buttonToggleAllDiscard,      1, 2);
    periodicTableToggleLayout->addWidget(buttonToggleAllDiscOptional, 0, 4);

    periodicTableToggleLayout->addWidget(new QLabel(tr("Optional")),         0, 1);
    periodicTableToggleLayout->addWidget(new QLabel(tr("Mandatory")),        1, 1);
    periodicTableToggleLayout->addWidget(new QLabel(tr("At least one")),     0, 3);
    periodicTableToggleLayout->addWidget(new QLabel(tr("Discarded")),        1, 3);
    periodicTableToggleLayout->addWidget(new QLabel(tr("Discard optional")), 0, 5);

    ui->groupBoxToggleAllRestrictions->setLayout(periodicTableToggleLayout);
}

void SearchMatchWidget::initSettings()
{
    ui->radioButtonFav->setChecked(settings->value("searchMatch/databaseFavsOnly", true).toBool());
    ui->radioButtonDir->setChecked(!ui->radioButtonFav->isChecked());
    ui->treeWidgetDirectories->setEnabled(ui->radioButtonDir->isChecked());

    ui->spinBoxItMax->setValue(settings->value("searchMatch/itMax", 10).toInt());
    ui->checkBoxAniso->setChecked(settings->value("searchMatch/allowAniso", false).toBool());
    ui->doubleSpinBoxWmax->setValue(settings->value("searchMatch/wmax", 60.0).toDouble());
    ui->checkBoxSetWmax->setChecked(settings->value("searchMatch/setWmax", true).toBool());
    ui->doubleSpinBoxWmin->setValue(settings->value("searchMatch/wmin", 5.0).toDouble());
    ui->checkBoxSetWmin->setChecked(settings->value("searchMatch/setWmin", true).toBool());
    ui->checkBoxEps2->setChecked(settings->value("searchMatch/eps2", false).toBool());
    ui->checkBoxRU->setChecked(settings->value("searchMatch/setRU", false).toBool());
    ui->spinBoxRU->setEnabled(ui->checkBoxRU->isChecked());
    ui->spinBoxRU->setValue(settings->value("searchMatch/valueRU", 10).toInt());

    ui->splitter->restoreState(settings->value("searchMatch/splitter", QByteArray()).toByteArray());
    ui->treeWidgetDirectories->header()->restoreState(settings->value("searchMatch/drHeader", QByteArray()).toByteArray());
    ui->treeWidgetResults->header()->restoreState(settings->value("searchMatch/twHeader", QByteArray()).toByteArray());
    ui->treeWidgetPinned->header()->restoreState(settings->value("searchMatch/pwHeader", QByteArray()).toByteArray());

    ui->comboBoxCrystallinity->setCurrentIndex(settings->value("searchMatch/crystallinity", 1).toInt());
    ui->comboBoxUnitCellRefinement->setCurrentIndex(settings->value("searchMatch/unitCellRefinement", 1).toInt());

    // do this AFTER restoring the header state. Else sorting is not enabled after updating from a previous version
    ui->treeWidgetResults->setSortingEnabled(true);
    ui->treeWidgetPinned->setSortingEnabled(true);

/* hidden columns seem to be managed by QHeaderView::restoreState
    QList<QVariant> hiddenColsResults;
    QList<QVariant> hiddenColsPinned;
    hiddenColsResults = settings->value("searchMatch/hiddenColumnsResults", hiddenColsResults).toList();
    hiddenColsPinned = settings->value("searchMatch/hiddenColumnsPinned",  hiddenColsPinned).toList();

    int n = qMin(ui->treeWidgetResults->header()->count() - 1, contextMenuHeaderResults->actions().count() + 1);

    for (int i = 1; i < n; ++i) {
        bool b = hiddenColsResults.contains(QVariant(i));
        ui->treeWidgetResults->setColumnHidden(i, b);
        contextMenuHeaderResults->actions().at(i - 1)->setChecked(!b);
    }

    n = qMin(ui->treeWidgetPinned->header()->count(), contextMenuHeaderPinned->actions().count() + 1);

    for (int i = 1; i < n; ++i) {
        bool b = hiddenColsPinned.contains(QVariant(i));
        ui->treeWidgetPinned->setColumnHidden(i, b);
        contextMenuHeaderPinned->actions().at(i - 1)->setChecked(!b);
    }
*/
}

void SearchMatchWidget::initConnections()
{
    connect(bgmnHandler, SIGNAL(allComplete(QUuid,global::RefinementStatus)), this, SLOT(complete(QUuid,global::RefinementStatus)));
    connect(ui->treeWidgetResults, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(itemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(ui->treeWidgetResults, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(showQuantities(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(ui->treeWidgetResults, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(itemClicked(QTreeWidgetItem*,int)));
    connect(ui->treeWidgetResults, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(itemDoubleClicked(QTreeWidgetItem*,int)));
    connect(ui->treeWidgetResults, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(showQuantities(QTreeWidgetItem*,int)));
    connect(ui->treeWidgetPinned, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(itemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(ui->treeWidgetPinned, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(itemClicked(QTreeWidgetItem*,int)));

    connect(contextMenuHeaderResults, SIGNAL(triggered(QAction*)), this, SLOT(toggleColumnHiddenResults(QAction*)));
    connect(contextMenuHeaderPinned, SIGNAL(triggered(QAction*)), this, SLOT(toggleColumnHiddenPinned(QAction*)));
    connect(ui->treeWidgetResults->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuRequestedResults(QPoint)));
    connect(ui->treeWidgetPinned->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuRequestedPinned(QPoint)));

    connect(buttonToggleAllOptional, SIGNAL(clicked(bool)), ui->periodicTable, SLOT(allOptional()));
    connect(buttonToggleAllMandatory, SIGNAL(clicked(bool)), ui->periodicTable, SLOT(allMandatory()));
    connect(buttonToggleAllOne, SIGNAL(clicked(bool)), ui->periodicTable, SLOT(allOne()));
    connect(buttonToggleAllDiscard, SIGNAL(clicked(bool)), ui->periodicTable, SLOT(allDisabled()));
    connect(buttonToggleAllDiscOptional, SIGNAL(clicked(bool)), ui->periodicTable, SLOT(disableOptionals()));

    // instant saving of settings to avoid conflicts with multiple search-match widgets.
    // each change instantly becomes the default for next time
    connect(ui->treeWidgetResults->header(),     SIGNAL(sectionResized(int,int,int)), this, SLOT(headerResultsChanged()));
    connect(ui->treeWidgetPinned->header(),      SIGNAL(sectionResized(int,int,int)), this, SLOT(headerPinnedChanged()));
    connect(ui->treeWidgetDirectories->header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(headerDirectoriesChanged()));

    connect(ui->radioButtonFav,           SIGNAL(toggled(bool)),            this, SLOT(favButtonToggled(bool)));
    connect(ui->spinBoxItMax,             SIGNAL(valueChanged(int)),        this, SLOT(itMaxChanged(int)));
    connect(ui->comboBoxLambda,           SIGNAL(currentTextChanged(QString)), this, SLOT(lambdaChanged(QString)));
    connect(ui->doubleSpinBoxSynchrotron, SIGNAL(valueChanged(double)),     this, SLOT(synchrotronValueChanged(double)));
    connect(ui->comboBoxInstrument,       SIGNAL(currentTextChanged(QString)), this, SLOT(instrumentChanged(QString)));
    connect(ui->checkBoxAniso,            SIGNAL(toggled(bool)),            this, SLOT(anisoToggled(bool)));
    connect(ui->doubleSpinBoxWmax,        SIGNAL(valueChanged(double)),     this, SLOT(wmaxChanged(double)));
    connect(ui->checkBoxSetWmax,          SIGNAL(toggled(bool)),            this, SLOT(wmaxToggled(bool)));
    connect(ui->doubleSpinBoxWmin,        SIGNAL(valueChanged(double)),     this, SLOT(wminChanged(double)));
    connect(ui->checkBoxSetWmin,          SIGNAL(toggled(bool)),            this, SLOT(wminToggled(bool)));
    connect(ui->checkBoxEps2,             SIGNAL(toggled(bool)),            this, SLOT(eps2Toggled(bool)));

    connect(ui->checkBoxRU,               SIGNAL(toggled(bool)),            this, SLOT(ruToggled(bool)));
    connect(ui->spinBoxRU,                SIGNAL(valueChanged(int)),        this, SLOT(ruChanged(int)));

    connect(ui->splitter,                 SIGNAL(splitterMoved(int,int)),   this, SLOT(splitterChanged(int,int)));

    connect(ui->comboBoxCrystallinity,      SIGNAL(currentIndexChanged(int)),          this, SLOT(crystallinityChanged(int)));
    connect(ui->comboBoxUnitCellRefinement, SIGNAL(currentIndexChanged(int)),          this, SLOT(ucRefineementChanged(int)));
    connect(ui->treeWidgetDirectories,      SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(directorySelectionChanged(QTreeWidgetItem*,int)));
}

void SearchMatchWidget::initData()
{
    int errors = 0;
    if (!initInstruments()) errors++;
    if (!initLam())         errors++;
    if (!initDbDirs())      errors++;

    hasValidBgmnConfig = errors == 0;
}

bool SearchMatchWidget::initInstruments()
{
    QStringList devDir = settings->value("bgmnProject/deviceDatabase", QStringList()).toStringList();
    if (devDir.isEmpty()) return false;

    QStringList devExt;
    devExt << "geq" << "GEQ";

    bool oldState = ui->comboBoxInstrument->blockSignals(true);

    ui->comboBoxInstrument->addItem(QString(), QVariant());

    for (int d = 0; d < devDir.size(); ++d) {
        QMap<QString,QString> devFiles = BgmnFileIO::getFileList(devDir.at(d), devExt);
        QMap<QString,QString>::const_iterator i = devFiles.constBegin();

        while (i != devFiles.constEnd()) {
            QFileInfo fi(i.value());
            ui->comboBoxInstrument->addItem(fi.baseName(), i.value());
            ++i;
        }
    }

    ui->comboBoxInstrument->blockSignals(oldState);
    QString lastDevice = settings->value("searchMatch/defaultInstrument", QString()).toString();
    if (lastDevice.isEmpty()) lastDevice = settings->value("bgmnProject/defaultInstrument", QString()).toString();
    int n = ui->comboBoxInstrument->findText(lastDevice);
    ui->comboBoxInstrument->setCurrentIndex(n < 0 ? 0 : n);
    return true;
}

bool SearchMatchWidget::initLam()
{
    QFileInfo fiBgmnExec(settings->value("bgmnProject/bgmnExec", "").toString());
    if (!fiBgmnExec.exists()) return false;
    QString lamDir = fiBgmnExec.absolutePath();

    QStringList lamExt;
    lamExt << "lam" << "LAM";

    QMap<QString,QString> lamFiles = BgmnFileIO::getFileList(lamDir, lamExt);
    QMap<QString,QString>::const_iterator i = lamFiles.constBegin();

    bool oldState = ui->comboBoxLambda->blockSignals(true);

    ui->comboBoxLambda->addItem(QString(), QVariant());

    while (i != lamFiles.constEnd()) {
        QFileInfo fi(i.value());
        ui->comboBoxLambda->addItem(fi.baseName(), i.value());
        ++i;
    }

    ui->comboBoxLambda->blockSignals(oldState);

    int n = ui->comboBoxLambda->findText(settings->value("searchMatch/lamFile", QString()).toString());
    ui->comboBoxLambda->setCurrentIndex(n < 0 ? 0 : n);
    ui->doubleSpinBoxSynchrotron->setValue(settings->value("searchMatch/synchrotron", 0.05).toDouble());
    return true;
}

bool SearchMatchWidget::initDbDirs()
{
    QStringList repositories = settings->value("bgmnProject/structureDatabaseIndexed", QStringList()).toStringList();
    repositories.append(settings->value("bgmnProject/structureDatabaseNonIndexed", QStringList()).toStringList());

    QStringList checkedDirs = settings->value("searchMatch/databaseCheckedDirs", QStringList()).toStringList();

    ui->labelNumPhasesFavs->setText(QString("%1 phases").arg(refStrManager->countFavPhasesIndexed()));
    ui->labelNumPhasesAll->setText(QString("%1 phases").arg(refStrManager->countAllPhasesIndexed()));

    QMap<QString, QTreeWidgetItem *> items = refStrManager->createDirTree(ui->treeWidgetDirectories,
                                                                          repositories,
                                                                          false,
                                                                          Qt::ItemFlags(Qt::ItemIsEnabled
                                                                                        | Qt::ItemIsAutoTristate
                                                                                        | Qt::ItemIsUserCheckable));

    QMapIterator<QString, QTreeWidgetItem*> it(items);

    bool oldState = ui->treeWidgetDirectories->blockSignals(true);
    while (it.hasNext()) {
        it.next();
        it.value()->setText(1, QString("%1").arg(refStrManager->countPhasesInDirectoryIndexed(it.value()->data(0, Qt::UserRole).toString())));

        if (checkedDirs.contains(it.value()->data(0, Qt::UserRole).toString())) {
            it.value()->setCheckState(0, Qt::Checked);
        } else {
            it.value()->setCheckState(0, Qt::Unchecked);
        }
    }
    ui->treeWidgetDirectories->blockSignals(oldState);

    return true;
}

bool SearchMatchWidget::hasValidInstrumentSelection()
{
    if (projectGeqFile.isEmpty()) {
        return ui->comboBoxInstrument->currentIndex() > 0;
    }

    return true;
}

bool SearchMatchWidget::hasValidWavelengthSelection()
{
    if (projectLam.isEmpty()) {
        if (ui->radioButtonLambda->isChecked()) {
            return ui->comboBoxLambda->currentIndex() > 0;
        } else if (ui->radioButtonSynchrotron->isChecked()) {
            return true;
        } else {
            return false;
        }
    }

    return true;
}

void SearchMatchWidget::setWidgets(QPlainTextEdit *o)
{
    refOutput = o;
}

/*
 * files in l must be given with absolute file path
 */
void SearchMatchWidget::setPinnedPhases(const QList<QPair<QString, QString> > &l)
{
    for (int i = 0; i < l.size(); ++i) {
        QFileInfo fi(l.at(i).first);
        bool skip = false;

        // check if the phase is already pinned
        QList<QTreeWidgetItem*> pPhase = ui->treeWidgetPinned->findItems(fi.fileName(), Qt::MatchExactly, FILENAME);

        if (pPhase.size()) {
            for (int j = 0; j < pPhase.size(); ++j) {
                if (pPhase.at(j)->data(FILENAME, Qt::UserRole).toString() == fi.absoluteFilePath()) {
                    skip = true;
                }
            }
        }

        if (skip) continue;

        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetPinned);
        it->setText(FILENAME,    fi.fileName());
        it->setText(PHASE,       l.at(i).second);
        it->setText(GEWICHT,     QString());
        it->setText(FOMSOURCE,   "project");
        it->setData(FILENAME,    Qt::UserRole, fi.absoluteFilePath());
        it->setData(FOMSOURCE,   Qt::UserRole, false);
        it->setToolTip(FILENAME, fi.absoluteFilePath());
        ui->treeWidgetPinned->addTopLevelItem(it);
    }
}

void SearchMatchWidget::setInstrumentFile(const QString &s)
{
    ui->comboBoxInstrument->setEnabled(false);
    ui->labelInstrumentMessage->setText(tr("using project instrument"));

    QFileInfo fi(s);
    if (fi.suffix().toLower() == "geq") {
        projectGeqFile = s;
    } else {
        if (QFile::exists(valFile.absolutePath() + "/" + s + ".geq")) {
            projectGeqFile = valFile.absolutePath() + "/" + s + ".geq";
        } else if (QFile::exists(valFile.absolutePath() + "/" + s + ".GEQ")) {
            projectGeqFile = valFile.absolutePath() + "/" + s + ".GEQ";
        } else {
            qDebug() << QString("SearchMatchWidget::setInstrumentFile(): Instrument file not found: %1")
                        .arg(valFile.absolutePath() + "/" + s);
        }
    }

    ui->comboBoxInstrument->setCurrentText(projectGeqFile);
}

void SearchMatchWidget::setLambdaFile(const QString &s)
{
    ui->comboBoxLambda->setEnabled(false);
    ui->doubleSpinBoxSynchrotron->setEnabled(false);
    ui->radioButtonLambda->setEnabled(false);
    ui->radioButtonSynchrotron->setEnabled(false);
    ui->comboBoxLambda->setCurrentText(s);
    projectLam = s;
}

void SearchMatchWidget::setSynchrotron(double d)
{
    if (d > 0.0) {
        ui->radioButtonSynchrotron->setChecked(true);
        ui->doubleSpinBoxSynchrotron->setValue(d);
    }
}

void SearchMatchWidget::pin()
{
    QList< QTreeWidgetItem*> selected = ui->treeWidgetResults->selectedItems();
    ui->treeWidgetResults->clearSelection();

    for (int i = 0; i < selected.size(); ++i) {
        QTreeWidgetItem *tit = selected.at(i);

        QTreeWidgetItem *lit = new QTreeWidgetItem(ui->treeWidgetPinned);
        lit->setText(FILENAME,   tit->text(FILENAME));
        lit->setText(PHASE,      tit->text(PHASE));
        lit->setText(GEWICHT,    QString());
        lit->setText(FOMSOURCE,  "matched");
        lit->setData(FILENAME,   Qt::UserRole, tit->data(FILENAME, Qt::UserRole));
        lit->setData(PHASE,      Qt::UserRole, tit->data(PHASE, Qt::UserRole));
        lit->setData(GEWICHT,    Qt::UserRole, 0.0);
        lit->setData(FOMSOURCE,  Qt::UserRole, true);
        lit->setToolTip(FILENAME, tit->toolTip(FILENAME));
        ui->treeWidgetPinned->addTopLevelItem(lit);
    }

    bool oldState = ui->treeWidgetResults->blockSignals(true);
    qDeleteAll(selected);
    emit refStructureReset();
    ui->treeWidgetResults->blockSignals(oldState);
}

void SearchMatchWidget::unpin()
{
    qDeleteAll(ui->treeWidgetPinned->selectedItems());
}

void SearchMatchWidget::run()
{
    if (!hasValidBgmnConfig) {
        QMessageBox::warning(this, tr("Search-Match"), tr("No valid BGMN backend configuration found. "
                                                          "Please check your program preferences."));
        qDebug() << QString("SearchMatchWidget::run(): BGMN backend configuration is not valid. Exiting.");
        return;
    }

    if (!hasValidInstrumentSelection()) {
        QMessageBox::information(this, tr("Search-Match"), tr("Please select your instrument configuration on the \"Controls\" page."));
        qDebug() << QString("SearchMatchWidget::run(): User did not specify a valid instrument configuration. Exiting.");
        ui->tabWidget->setCurrentWidget(ui->tabControls);
        return;
    }

    if (!hasValidWavelengthSelection()) {
        QMessageBox::information(this, tr("Search-Match"), tr("Please select a valid wavelength on the \"Controls\" page."));
        qDebug() << QString("SearchMatchWidget::run(): User did not specify a valid wavelength. Exiting.");
        ui->tabWidget->setCurrentWidget(ui->tabControls);
        return;
    }

    aborted = false;

    statistics.n = 0;
    statistics.sum = 0.0;
    statistics.mean = 0.0;
    statistics.stdev = 0.0;

    if (bgmnHandler->isRunning()) {
        abort();
        return;
    }

    if (!scanControl) {
        QMessageBox::warning(this, tr("Search-Match"), tr("Cannot access internal scan data.\nSearch-match aborted."));
        qDebug() << QString("SearchMatchWidget::run(): Cannot access scanControl. Exiting.");
        return;
    }

    if (!scanControl->count()) {
        QMessageBox::warning(this, tr("Search-Match"), tr("No scans found.\nSearch-match aborted."));
        qDebug() << QString("SearchMatchWidget::run(): No scans found. Exiting.");
        return;
    }

    if (!convertRawFile()) {
        QMessageBox::warning(this, tr("Search-Match"), tr("Could not convert raw data file \nto native format.\nSearch-match aborted."));
        qDebug() << QString("SearchMatchWidget::run(): Could not convert raw file to native format.");
        return;
    }

    pinnedPhases.clear();
    fomList.clear();

    for (int i = 0; i < ui->treeWidgetPinned->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetPinned->topLevelItem(i);
        it->setText(GEWICHT, QString());
        pinnedPhases.append(it->data(FILENAME, Qt::UserRole).toString());
        it->setData(GEWICHT,  Qt::UserRole, QList<QVariant>());
    }

    ui->toolButtonSave->setEnabled(false);
    ui->treeWidgetResults->clear();
    ui->tabWidget->setCurrentIndex(3);

    wDir = valFile.absolutePath();
    diaFile = QFileInfo(wDir + "/" + valFile.baseName() + ".dia");
    lstFile = QFileInfo(wDir + "/" + valFile.baseName() + ".lst");
    parFile = QFileInfo(wDir + "/" + valFile.baseName() + ".par");

    if (!projectGeqFile.isEmpty()) {
        geqFile = QFileInfo(wDir + "/" + projectGeqFile);
    } else {
        geqFile = QFileInfo(ui->comboBoxInstrument->currentData(Qt::UserRole).toString());
    }

    strFiles = getStrFiles();

    if (strFiles.isEmpty()) {
        if (ui->radioButtonFav->isChecked()) {
            qDebug() << QString("SearchMatchWidget::run(): No structure files found in favorites. Exiting.");
            QMessageBox::information(this, tr("Search-Match"),
                                     tr("No favorite phases found.\nPlease define "
                                        "favorites or\ndisable favorites-only search."));
            return;
        }

        qDebug() << QString("SearchMatchWidget::run(): No structure files selected. Exiting.");
        QMessageBox::information(this, tr("Search-Match"),
                                 tr("No phases retrieved from database.\n\n"
                                    "Please check the following:\n"
                                    "- At least one structure repository is checked on the \"Phases\" page\n"
                                    "- At least one phase fulfills the restrictions on the \"Restrictions\" page"));
        return;
    }

    if (pinnedPhases.size()) {
        double thAxis  = settings->value("searchMatch/compareAxis", 0.02).toDouble();
        double thAngle = settings->value("searchMatch/compareAngle", 0.1).toDouble();

        strFiles = StrFileFilter::eliminatePinnedFiles(strFiles, pinnedPhases);

        if (strFiles.isEmpty()) {
            qDebug() << QString("SearchMatchWidget::run(): No structure files left after filtering pinned ones. Exiting.");
            QMessageBox::information(this, tr("Search-Match"), tr("All structure files in the search directories are already pinned."));
            return;
        }

        strFiles = StrFileFilter::eliminateDuplicates(strFiles, pinnedPhases, thAxis, thAngle);
        strFiles = StrFileFilter::eliminateUnindexed(strFiles);

        if (strFiles.isEmpty()) {
            qDebug() << QString("SearchMatchWidget::run(): No structure files left after filtering duplicates of pinned ones. Exiting.");
            QMessageBox::information(this, tr("Search-Match"), tr("No structure files left after filtering pinned ones and duplicates of pinned phases."));
            return;
        }
    }

    if (!geqFile.exists()) {
        qDebug() << QString("SearchMatchWidget::run(): Instrument file %1 not found. Exiting.").arg(geqFile.absoluteFilePath());
        return;
    }

    if (!bgmnHandler->init()) {
        qDebug() << QString("SearchMatchWidget::run(): Initializing BgmnHandler failed. Exiting.");
        return;
    }

    QString instrSrcPath = geqFile.absolutePath();
    QString instrDstPath = valFile.absolutePath();
    QString geqDst = instrDstPath + "/" + geqFile.fileName();

    if (instrDstPath != instrSrcPath) {
        QString geqSrc = geqFile.absoluteFilePath();
        QString savSrc = instrSrcPath + "/" + geqFile.baseName() + ".sav";
        QString gerSrc = instrSrcPath + "/" + geqFile.baseName() + ".ger";
        QString tplSrc = instrSrcPath + "/" + geqFile.baseName() + ".tpl";
        QString savDst = instrDstPath + "/" + geqFile.baseName() + ".sav";
        QString gerDst = instrDstPath + "/" + geqFile.baseName() + ".ger";
        QString tplDst = instrDstPath + "/" + geqFile.baseName() + ".tpl";
        QString bkgDst;
        QString bkgSrc;

        BgmnSavParser sparser(tplSrc);

        if (!sparser.untFile().isEmpty()) {
            untFile = QFileInfo(instrSrcPath + "/" + sparser.untFile());

            bkgSrc = instrSrcPath + "/" + untFile.fileName();
            bkgDst = instrDstPath + "/" + untFile.fileName();
            BgmnFileIO::copyFile(bkgSrc, bkgDst); // text
        } else {
            untFile = QFileInfo();
        }

        QFile::copy(geqSrc, geqDst);          // binary
        BgmnFileIO::copyFile(savSrc, savDst); // text
        BgmnFileIO::copyFile(gerSrc, gerDst); // text
        BgmnFileIO::copyFile(tplSrc, tplDst); // text
    }

    diaHandler->init(scanControl, diaFile.absoluteFilePath());

    strFile = QFileInfo(strFiles.takeFirst().file());
    ui->progressBar->setEnabled(true);
    ui->progressBar->setMaximum(strFiles.size());
    ui->progressBar->show();
    emit exitStatus(1);
    startProcess();
}

QList<HklPhaseData> SearchMatchWidget::getStrFiles()
{
    QStringList restrAll  = ui->periodicTable->getAll();
    QStringList restrOne  = ui->periodicTable->getOne();
    QStringList restrNone = ui->periodicTable->getNone();

    qDebug() << QString("SearchMatchWidget::getStrFiles(): Restrictions \"All\":  %1").arg(restrAll.join(";"));
    qDebug() << QString("SearchMatchWidget::getStrFiles(): Restrictions \"One\":  %1").arg(restrOne.join(";"));
    qDebug() << QString("SearchMatchWidget::getStrFiles(): Restrictions \"None\": %1").arg(restrNone.join(";"));

    QList<HklPhaseData> phases;

    if (ui->radioButtonFav->isChecked()) {
        phases = refStrManager->getFavPhases(restrAll, restrOne, restrNone);
        qDebug() << QString("SearchMatchWidget::getStrFiles(): %1 favorite phases match the restrictions.").arg(phases.size());
        return phases;
    }

    QTreeWidgetItemIterator it(ui->treeWidgetDirectories);

    while (*it) {
        if ((*it)->checkState(0) == Qt::Checked) {
            QString dir((*it)->data(0, Qt::UserRole).toString());
            QList<HklPhaseData> p = refStrManager->getSubDirPhases(dir, restrAll, restrOne, restrNone);
            qDebug() << QString("SearchMatchWidget::getStrFiles(): %1 phases match the restrictions in %2").arg(p.size()).arg(dir);
            phases.append(p);
        }

        ++it;
    }

    return phases;
}


bool SearchMatchWidget::convertRawFile()
{
    rawFile = scanControl->fileInfo();

    QMap<QString, QVariant> flags;
    flags["fieldSeparator"] = QVariant(QString(" "));
    flags["fixBgmnZero"]    = QVariant(true);

    valFile = QFileInfo(rawFile.absolutePath() + "/" + rawFile.baseName() + ".xy");

    Scan *scan = scanControl->firstActiveScan();

    if (!scan) {
        qDebug() << QString("SearchMatchWidget::convertRawFile(): No Scan available");
        return false;
    }

    ExportHandler exHandler;

    if (exHandler.save("ASCII_XY", valFile.absoluteFilePath(), *scan, flags) <= 0) {
        qDebug() << QString("SearchMatchWidget::convertRawFile(): Could not convert scan to native format: %1 to %2")
                    .arg(rawFile.absoluteFilePath(), valFile.absoluteFilePath());
        return false;
    }

    return true;
}

void SearchMatchWidget::startProcess()
{
    // create a random string for the local str file name, because there might already be a local
    // copy with the original name from a different refinement project. We don't want to overwrite it.

    QUuid uid = QUuid::createUuid();
    localStrFile = QFileInfo(valFile.absolutePath() + "/" + uid.toString(QUuid::WithoutBraces) + "-" + strFile.fileName());

    BgmnFileIO::copyFile(strFile.absoluteFilePath(), localStrFile.absoluteFilePath());

    QStringList args;

    args << QString("VERZERR=%1").arg(geqFile.fileName());
    args << QString("VAL[1]=%1").arg(valFile.fileName());

    if (ui->radioButtonLambda->isChecked()) {
        args << QString("LAMBDA=%1").arg(projectLam.isEmpty() ? ui->comboBoxLambda->currentText() : projectLam);
    } else {
        args << QString("SYNCHROTRON=%1").arg(ui->doubleSpinBoxSynchrotron->value());
    }

    for (int i = 0; i < pinnedPhases.size(); ++i) {
        args << QString("STRUC[%1]=%2").arg(i+1).arg(pinnedPhases.at(i));
    }

    args << QString("STRUC[%1]=%2").arg(pinnedPhases.size() + 1).arg(localStrFile.absoluteFilePath());
    args << QString("ITMAX=%1").arg(ui->spinBoxItMax->value());

    if (!ui->checkBoxAniso->isChecked()) {
        args << QString("ONLYISO=Y");
    }

    int n = settings->value("bgmnProject/nThreads", QThread::idealThreadCount()).toInt();
    if (n == 0) n = QThread::idealThreadCount();

    args << QString("NTHREADS=%1").arg(n);

    if (ui->checkBoxSetWmin->isChecked()) {
        args << QString("WMIN=%1").arg(ui->doubleSpinBoxWmin->value());
    }

    if (ui->checkBoxSetWmax->isChecked()) {
        args << QString("WMAX=%1").arg(ui->doubleSpinBoxWmax->value());
    }

    if (ui->checkBoxRU->isChecked()) {
        args << QString("RU=%1").arg(ui->spinBoxRU->value());
    }

    changeParameterState(localStrFile.absoluteFilePath());

    if (untFile.isFile()) {
        args << QString("UNT=%1").arg(untFile.fileName());
    }

    args << QString("DIAGRAMM=%1").arg(diaFile.fileName());
    args << QString("LIST=%1").arg(lstFile.fileName());
    args << QString("OUTPUT=%1").arg(parFile.fileName());
    args << QString("PROTOKOLL=Y");
    args << QString("SAVE=N");

    if (ui->checkBoxEps2->isChecked()) {
        args << QString("PARAM[1]=EPS2=%1_-0.1^0.1").arg(defaultEps2, 0, 'f', 6);
    } else {
        args << QString("EPS2=%1").arg(defaultEps2, 0, 'f', 6);
    }

    scanControl->setViewStatus(global::RefinementStatus::MATCHING);

    if (bgmnHandler->run(valFile.absolutePath(), args)) {
        diaHandler->startPolling();
    }
}

void SearchMatchWidget::pollOutput()
{
    QString output(bgmnHandler->readOutput().trimmed());

    if (refOutput) {
        if (output == ".") {
            refOutput->moveCursor(QTextCursor::End);
            refOutput->insertPlainText(output);
            refOutput->moveCursor(QTextCursor::End);
        } else {
            refOutput->appendPlainText(output + " ");
        }

        refOutput->ensureCursorVisible();
    }
}

void SearchMatchWidget::complete(QUuid u, global::RefinementStatus st)
{
    Q_UNUSED(u);
    pollOutput();

    ui->progressBar->setValue(ui->progressBar->maximum() - strFiles.size() + 1);

    parseLst();

    if (strFiles.size()) {
        // QFile::copy(lstFile.absoluteFilePath(), lstFile.absolutePath() + QDir::separator() + strFile.baseName() + ".lst");
        QFile::remove(localStrFile.absoluteFilePath());
        strFile = QFileInfo(strFiles.takeFirst().file());
        diaHandler->init(scanControl, diaFile.absoluteFilePath());
        startProcess();
    } else {
        scanControl->setViewStatus(st);
        emit exitStatus(aborted ? 0 : 2);

        ImportHandler ihandler;
        QString uid = ihandler.uidByFileName(rawFile.absoluteFilePath());

        scanControl->loadScanFile(rawFile.absoluteFilePath(), uid, false, scanControl->getSampleId(), true);
        QFile::remove(diaFile.absoluteFilePath());
        QFile::remove(parFile.absoluteFilePath());
        QFile::remove(lstFile.absoluteFilePath());
        QFile::remove(localStrFile.absoluteFilePath());

        ui->toolButtonSave->setEnabled(true);
        ui->progressBar->reset();
        ui->progressBar->setEnabled(false);
        ui->progressBar->hide();
    }
}

void SearchMatchWidget::abort()
{
    aborted = true;
    strFiles.clear();
    bgmnHandler->abort();
}

bool SearchMatchWidget::isRunning()
{
    return bgmnHandler->isRunning();
}

void SearchMatchWidget::parseLst()
{
    bool ok;
    BgmnLstParser lparser(lstFile.absoluteFilePath(), ok);
    if (!ok) return;

    QStringList phases = lparser.getPhaseNames();
    QList<QMap<QString, QVariant> > params;

    for (int i = 0; i < phases.size(); ++i) {
        params.append(lparser.getAllLocalParameters(phases.at(i), true));
    }

    double rwp = lparser.getRwp();
    // double rexp = lparser.getRexp();
    // double chi2 = lparser.getChi2();

    PhaseStats stats = calculateFoM(rwp, params.last());
    fomList.append(stats.fom);

    QStringList text;
    text << strFile.fileName()
         << phases.last()
         << QString()
         << QString("%1").arg(stats.fom, 0, 'f', 6, '0');

    QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetResults, text);
    it->setData(FILENAME,  Qt::UserRole, strFile.absoluteFilePath());
    it->setData(PHASE,     Qt::UserRole, phases.last());
    it->setData(GEWICHT,   Qt::UserRole, stats.gewicht);
    it->setData(FOMSOURCE, Qt::UserRole, stats.fom);
    it->setToolTip(FILENAME, strFile.absoluteFilePath());
    ui->treeWidgetResults->addTopLevelItem(it);
    ui->treeWidgetResults->sortByColumn(3, Qt::DescendingOrder);

    updateQuantities(params, it);
    setItemColors();
}

PhaseStats SearchMatchWidget::calculateFoM(double rwp, QMap<QString, QVariant> localGoals)
{
    double gw = getPhaseValue("GEWICHT", localGoals);
    double b1 = getPhaseValue("B1", localGoals);
    double k2 = getPhaseValue("k2", localGoals);

    // get the starting values of a, b, c from the str file
    double az, bz, cz;
    cellZero(az, bz, cz);

    double ar = getPhaseValue("A", localGoals);
    double br = getPhaseValue("B", localGoals);
    double cr = getPhaseValue("C", localGoals);

    if (qFuzzyIsNull(ar)) ar = az;
    if (qFuzzyIsNull(br)) br = bz;
    if (qFuzzyIsNull(cr)) cr = cz;

    double tb1 = settings->value("seachMatch/weightB1", 0.02).toDouble();
    double tk2 = settings->value("seachMatch/weightK2", 0.00001).toDouble();

    double a = settings->value("seachMatch/weightA", 1.0).toDouble();
    double b = settings->value("seachMatch/weightB", 1.0).toDouble();
    double c = b1 > tb1 ? settings->value("seachMatch/weightC", 1.0).toDouble() : 0.0;
    double d = k2 > tk2 ? settings->value("seachMatch/weightD", 1.0).toDouble() : 0.0;

    // multiplied by 100 to increase the weight by default
    double aterm = az <= 0.0 ? 0.0 : 100.0 * a * ((qAbs(ar - az) / az)
                                        + (qAbs(br - bz) / bz)
                                        + (qAbs(cr - cz) / cz));

    double bterm = b * gw;
    double delta = 1.0 + (qFuzzyIsNull(b1) ? 0.0 : c/b1) + d*k2;

    double fom = ((1.0 / (rwp + aterm)) + bterm) / delta;

    PhaseStats stats;

    if (localGoals.contains("Phase")) stats.name = localGoals.value("Phase", QString()).toStringList().first();
    else stats.name = QString();

    stats.fom = fom;
    stats.gewicht = gw;
    stats.azero = az;
    stats.bzero = bz;
    stats.czero = cz;
    stats.arefined = ar;
    stats.brefined = br;
    stats.crefined = cr;

    return stats;
}

double SearchMatchWidget::getPhaseValue(const QString &p, const QMap<QString, QVariant> &m)
{
    bool ok;
    QStringList l = m.value(p).toStringList();
    if (!l.size()) return 0.0;

    double d = l.first().toDouble(&ok);
    if (!ok) return 0.0;
    return d;
}

void SearchMatchWidget::cellZero(double &a, double &b, double &c)
{
    QString content = BgmnFileIO::readTextFile(strFile.absoluteFilePath());

    static QRegularExpression rxA("(?:PARAM=)?A=(\\d+\\.\\d+)(?:_\\d+\\.\\d+)?(?:\\^\\d+\\.\\d+)?");
    static QRegularExpression rxB("(?:PARAM=)?B=(\\d+\\.\\d+)(?:_\\d+\\.\\d+)?(?:\\^\\d+\\.\\d+)?");
    static QRegularExpression rxC("(?:PARAM=)?C=(\\d+\\.\\d+)(?:_\\d+\\.\\d+)?(?:\\^\\d+\\.\\d+)?");

    QRegularExpressionMatch rmA = rxA.match(content);
    QRegularExpressionMatch rmB = rxB.match(content);
    QRegularExpressionMatch rmC = rxC.match(content);

    if (rmA.hasMatch()) a = rmA.captured(1).toDouble();
    else a = -1.0;
    if (rmB.hasMatch()) b = rmB.captured(1).toDouble();
    else b = a;
    if (rmC.hasMatch()) c = rmC.captured(1).toDouble();
    else c = a;
}

void SearchMatchWidget::setItemColors()
{
    updateMeanAndSd();

    for (int i = 0; i < ui->treeWidgetResults->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetResults->topLevelItem(i);
        double fom = it->data(FOMSOURCE, Qt::UserRole).toDouble();

        if (statistics.stdev > 0.0) {
            QColor c = calcFoMColor((fom - statistics.mean) / statistics.stdev);
            it->setBackground(FOMSOURCE, c);
        }
    }

}

void SearchMatchWidget::updateMeanAndSd()
{
    statistics.stdev = 0.0;

    if (fomList.size() == 0) {
        return;
    }

    statistics.n = fomList.size();
    statistics.sum += fomList.last();
    statistics.mean = statistics.sum / double(statistics.n);

    if (fomList.size() == 1) {
        return;
    }

    // calculate mean and std dev of all FoM
    double sd = 0.0;

    for (int i = 0; i < fomList.size(); ++i) {
        sd += pow(fomList.at(i) - statistics.mean, 2.0);
    }

    statistics.stdev = qSqrt(sd / double(statistics.n));
}

/*
 * d = (fom - mean) / sd
 */
QColor SearchMatchWidget::calcFoMColor(double d)
{
    double g = 512.0 * d / 3.0;
    double r = 512.0 - g;

    if (r > 255.0) r = 255.0;
    if (r < 0.0)   r = 0.0;

    if (g > 255.0) g = 255.0;
    if (g < 0.0)   g = 0.0;

    return QColor(int(r), int(g), 0);
}

void SearchMatchWidget::itemClicked(QTreeWidgetItem *i, int)
{
    itemChanged(i, nullptr);
}

void SearchMatchWidget::itemDoubleClicked(QTreeWidgetItem *, int)
{
    pin();
}

void SearchMatchWidget::itemChanged(QTreeWidgetItem *cur, QTreeWidgetItem *)
{
    if (!cur) {
        emit refStructureReset();
        ui->toolButtonPin->setEnabled(false);
        ui->toolButtonUnpin->setEnabled(false);
        ui->toolButtonClearMatchSelection->setEnabled(false);
        ui->strFileEditor->clear();
    } else {
        QFileInfo fi(cur->text(0));
        emit refStructureSelected(fi.completeBaseName());
        ui->toolButtonPin->setEnabled(true);
        ui->toolButtonUnpin->setEnabled(true);
        ui->toolButtonClearMatchSelection->setEnabled(true);
        ui->strFileEditor->setPlainText(BgmnFileIO::readTextFile(cur->data(FILENAME, Qt::UserRole).toString()));
        bhl->setDocument(ui->strFileEditor->document());
    }
}

QStringList SearchMatchWidget::getPinnedPhases()
{
    QStringList l;

    for (int i = 0; i < ui->treeWidgetPinned->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetPinned->topLevelItem(i);
        if (it->data(FOMSOURCE, Qt::UserRole).toDouble()) {
            l.append(it->data(FILENAME, Qt::UserRole).toString());
        }
    }

    return l;
}

void SearchMatchWidget::updateQuantities(const QList<QMap<QString, QVariant> > &l, QTreeWidgetItem *mit)
{
    QMap<QString, QTreeWidgetItem*> pinnedItems;

    // store pointers to all pinned items for quick access
    for (int i = 0; i < ui->treeWidgetPinned->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetPinned->topLevelItem(i);
        pinnedItems[it->text(1)] = it;
    }

    double gwMatch = getPhaseValue("GEWICHT", l.last());
    mit->setData(GEWICHT, Qt::UserRole, gwMatch);

    double gwSum = gwMatch;

    // loop over all pinned phases (all except the last one). Each pinned phase
    // stores its GEWICHT at the time it was refined together with the phase of "mit"
    // in a map associated with the phase name of "mit"
    for (int i = 0; i < l.size() - 1; ++i) {
        QString ph = l.at(i).value("Phase", QString()).toStringList().first();
        double gw = getPhaseValue("GEWICHT", l.at(i));

        if (pinnedItems.contains(ph)) {
            QMap<QString, QVariant> gl = pinnedItems[ph]->data(GEWICHT, Qt::UserRole).toMap();
            gl[mit->text(1)] = gw;
            pinnedItems[ph]->setData(GEWICHT, Qt::UserRole, gl);
        }

        gwSum += gw;
    }

    double quant = 0.0;
    if (!qFuzzyIsNull(gwSum)) quant = gwMatch / gwSum;

    mit->setText(GEWICHT, QString("%1").arg(100.0 * quant, 0, 'f', 2));
}

void SearchMatchWidget::showQuantities(QTreeWidgetItem *mit, int)
{
    showQuantities(mit, Q_NULLPTR);
}

void SearchMatchWidget::showQuantities(QTreeWidgetItem *mit, QTreeWidgetItem *)
{
    if (!mit) return;

    QString matched = mit->text(PHASE);
    double gwMatch = mit->data(GEWICHT, Qt::UserRole).toDouble();
    double gwSum = gwMatch;

    for (int i = 0; i < ui->treeWidgetPinned->topLevelItemCount(); ++i) {
        QMap<QString, QVariant> l = ui->treeWidgetPinned->topLevelItem(i)->data(GEWICHT, Qt::UserRole).toMap();
        gwSum += l.value(matched, 0.0).toDouble();
    }

    mit->setText(GEWICHT, QString("%1").arg(100.0 * gwMatch / gwSum, 0, 'f', 2));


    for (int i = 0; i < ui->treeWidgetPinned->topLevelItemCount(); ++i) {
        QMap<QString, QVariant> l = ui->treeWidgetPinned->topLevelItem(i)->data(GEWICHT, Qt::UserRole).toMap();
        double gwPinned = l.value(matched, 0.0).toDouble();
        ui->treeWidgetPinned->topLevelItem(i)->setText(GEWICHT, QString("%1").arg(100.0 * gwPinned / gwSum, 0, 'f', 2));
    }
}

QString SearchMatchWidget::getDeviceFile()
{
    QFileInfo fi(ui->comboBoxInstrument->currentData(Qt::UserRole).toString());
    return fi.absoluteFilePath();
}

void SearchMatchWidget::clearResults()
{
    ui->toolButtonPin->setEnabled(false);
    ui->toolButtonUnpin->setEnabled(false);
    ui->toolButtonSave->setEnabled(false);
    ui->treeWidgetPinned->clear();
    ui->treeWidgetResults->clear();
}

void SearchMatchWidget::clearPinned()
{
    ui->treeWidgetPinned->clear();
}

void SearchMatchWidget::toggleColumnHiddenResults(QAction *a)
{
    int c = a->data().toInt();
    ui->treeWidgetResults->setColumnHidden(c, !a->isChecked());

    QList<QVariant> hiddenColsResults;
    for (int i = 1; i < ui->treeWidgetResults->header()->count(); ++i) {
        if (ui->treeWidgetResults->isColumnHidden(i)) hiddenColsResults.append(QVariant(i));
    }

    settings->setValue("searchMatch/hiddenColumnsResults", hiddenColsResults);
}


void SearchMatchWidget::toggleColumnHiddenPinned(QAction *a)
{
    int c = a->data().toInt();
    ui->treeWidgetPinned->setColumnHidden(c, !a->isChecked());

    QList<QVariant> hiddenColsPinned;
    for (int i = 1; i < ui->treeWidgetPinned->header()->count(); ++i) {
        if (ui->treeWidgetPinned->isColumnHidden(i)) hiddenColsPinned.append(QVariant(i));
    }

    settings->setValue("searchMatch/hiddenColumnsPinned", hiddenColsPinned);
}

void SearchMatchWidget::save()
{
    QString fname = QFileDialog::getSaveFileName(this, tr("Save Score List"), valFile.absolutePath(), tr("CSV File (*.CSV *.csv)"));
    if (fname.isEmpty()) return;
    QStringList content("Pinned phases\nFile;Phase;Source");

    QStringList pinnedNames;
    QList<QMap<QString, QVariant> > pinnedGw;

    for (int i = 0; i < ui->treeWidgetPinned->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetPinned->topLevelItem(i);
        QString f = it->data(FILENAME,  Qt::UserRole).toString();
        QString p = it->data(GEWICHT,   Qt::UserRole).toString();
        bool    o = it->data(FOMSOURCE, Qt::UserRole).toBool();

        pinnedNames << p;
        pinnedGw << it->data(GEWICHT, Qt::UserRole).toMap();

        content << QString("%1;%2;%3").arg(f, p, o ? "matched" : "project");
    }

    QStringList header;
    header << QString("\n\nMatched phases\nFile;Phase;GEWICHT;Figure of Merit");

    for (int j = 0; j < pinnedNames.size(); ++j) {
        header << QString("GEWICHT %1").arg(pinnedNames.at(j));
    }

    content << header.join(";");

    for (int i = 0; i < ui->treeWidgetResults->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetResults->topLevelItem(i);
        QString f = it->data(FILENAME,  Qt::UserRole).toString();
        QString p = it->data(PHASE,     Qt::UserRole).toString();
        double g  = it->data(GEWICHT,   Qt::UserRole).toDouble();
        double o  = it->data(FOMSOURCE, Qt::UserRole).toDouble();

        QStringList line;
        line << f << p << QString("%1;%2").arg(g, 0, 'f', 6).arg(o, 0, 'f', 6);

        for (int j = 0; j < pinnedNames.size(); ++j) {
            line << QString("%1").arg(pinnedGw.at(j)[p].toDouble(), 0, 'f', 6);
        }

        content << line.join(";");
    }

    BgmnFileIO::writeTextFile(fname, content.join("\n"));
}

void SearchMatchWidget::changeParameterState(const QString &strFile)
{
    BgmnStrParser strParser(strFile);

    double cellLim = ui->comboBoxUnitCellRefinement->currentData(Qt::UserRole).toDouble();
    double b1Lim = ui->comboBoxCrystallinity->currentData(Qt::UserRole).toDouble();

    int cellStat = qFuzzyIsNull(cellLim) ? 0 : 1;
    int b1Stat = qFuzzyIsNull(b1Lim) ? 0 : 1;

    strParser.setRefinementState(QString("UC:A:%1").arg(cellLim, 0, 'f', 4), cellStat);
    strParser.setRefinementState(QString("UC:B:%1").arg(cellLim, 0, 'f', 4), cellStat);
    strParser.setRefinementState(QString("UC:C:%1").arg(cellLim, 0, 'f', 4), cellStat);
    strParser.setRefinementState(QString("UC:ALPHA:%1").arg(cellLim, 0, 'f', 4), cellStat);
    strParser.setRefinementState(QString("UC:BETA:%1").arg(cellLim, 0, 'f', 4), cellStat);
    strParser.setRefinementState(QString("UC:GAMMA:%1").arg(cellLim, 0, 'f', 4), cellStat);
    strParser.setRefinementState(QString("PR:B1:%1").arg(b1Lim, 0, 'f', 4), b1Stat);
    strParser.setRefinementState(QString("PR:k2:"), 0);

    strParser.writeToFile(strFile);
}

void SearchMatchWidget::addReferencePhases(const QList<PeakFile> &fmap)
{
    ui->treeWidgetResults->clear();

    for (int i = 0; i < fmap.size(); ++i) {
        QStringList text;
        text << fmap.at(i).fileInfo.fileName()
             << fmap.at(i).phase
             << QString()
             << QString("%1").arg(1.0 - fmap.at(i).offset, 0, 'f', 6, '0');

        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetResults, text);
        it->setData(FILENAME,  Qt::UserRole, fmap.at(i).fileInfo.absoluteFilePath());
        it->setData(PHASE,     Qt::UserRole, fmap.at(i).phase);
        it->setData(GEWICHT,   Qt::UserRole, 1.0);
        it->setData(FOMSOURCE, Qt::UserRole, 1.0 - fmap.at(i).offset);
        it->setToolTip(FILENAME, fmap.at(i).fileInfo.absoluteFilePath());
        ui->treeWidgetResults->addTopLevelItem(it);
        ui->treeWidgetResults->sortByColumn(3, Qt::DescendingOrder);
    }

    setItemColors();
}

void SearchMatchWidget::headerResultsChanged()
{
    settings->setValue("searchMatch/twHeader", ui->treeWidgetResults->header()->saveState());
}

void SearchMatchWidget::headerPinnedChanged()
{
    settings->setValue("searchMatch/pwHeader", ui->treeWidgetPinned->header()->saveState());
}

void SearchMatchWidget::headerDirectoriesChanged()
{
    settings->setValue("searchMatch/drHeader", ui->treeWidgetDirectories->header()->saveState());
}

void SearchMatchWidget::favButtonToggled(bool)
{
    settings->setValue("searchMatch/databaseFavsOnly", ui->radioButtonFav->isChecked());
}

void SearchMatchWidget::itMaxChanged(int)
{
    settings->setValue("searchMatch/itMax", ui->spinBoxItMax->value());
}

void SearchMatchWidget::lambdaChanged(QString)
{
    settings->setValue("searchMatch/lamFile", ui->comboBoxLambda->currentText());
}

void SearchMatchWidget::synchrotronValueChanged(double)
{
    settings->setValue("searchMatch/synchrotron", ui->doubleSpinBoxSynchrotron->value());
}

void SearchMatchWidget::instrumentChanged(QString)
{
    settings->setValue("searchMatch/defaultInstrument", ui->comboBoxInstrument->currentText());
}

void SearchMatchWidget::anisoToggled(bool)
{
    settings->setValue("searchMatch/allowAniso", ui->checkBoxAniso->isChecked());
}

void SearchMatchWidget::wmaxToggled(bool)
{
    settings->setValue("searchMatch/setWmax", ui->checkBoxSetWmax->isChecked());
}

void SearchMatchWidget::wmaxChanged(double)
{
    settings->setValue("searchMatch/wmax", ui->doubleSpinBoxWmax->value());
}

void SearchMatchWidget::wminToggled(bool)
{
    settings->setValue("searchMatch/setWmin", ui->checkBoxSetWmin->isChecked());
}

void SearchMatchWidget::wminChanged(double)
{
    settings->setValue("searchMatch/wmin", ui->doubleSpinBoxWmin->value());
}

void SearchMatchWidget::eps2Toggled(bool)
{
    settings->setValue("searchMatch/eps2", ui->checkBoxEps2->isChecked());
}

void SearchMatchWidget::ruToggled(bool)
{
    settings->setValue("searchMatch/setRU", ui->checkBoxRU->isChecked());
}

void SearchMatchWidget::ruChanged(int)
{
    settings->setValue("searchMatch/valueRU", ui->spinBoxRU->value());
}

void SearchMatchWidget::splitterChanged(int, int)
{
    settings->setValue("searchMatch/splitter", ui->splitter->saveState());
}

void SearchMatchWidget::crystallinityChanged(int)
{
    settings->setValue("searchMatch/crystallinity", ui->comboBoxCrystallinity->currentIndex());
}

void SearchMatchWidget::ucRefineementChanged(int)
{
    settings->setValue("searchMatch/unitCellRefinement", ui->comboBoxUnitCellRefinement->currentIndex());
}

void SearchMatchWidget::directorySelectionChanged(QTreeWidgetItem*,int)
{
    QStringList databaseCheckedDirs;

    QTreeWidgetItemIterator it(ui->treeWidgetDirectories);
    while (*it) {
        if ((*it)->checkState(0) == Qt::Checked) {
            databaseCheckedDirs.append((*it)->data(0, Qt::UserRole).toString());
        }
        ++it;
    }

    settings->setValue("searchMatch/databaseCheckedDirs", databaseCheckedDirs);
}

void SearchMatchWidget::clearMatchSelection()
{
    ui->treeWidgetResults->clearSelection();
    ui->treeWidgetResults->setCurrentItem(nullptr);
    itemChanged(nullptr, nullptr);
}

void SearchMatchWidget::showHelp()
{
    emit helpText("searchMatchWidget");
}

