/***************************************************************************
                          strucimportdialog.cpp  -  description
                             -------------------
    begin                : Fri Mar 13 14:02:00 CEST 2015
    copyright            : (C) 2015 by Nicola Doebelin
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


#include "strucimportdialog.h"
#include "ui_strucimportdialog.h"
#include "../libXrdIO/hkl.h"
#include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/structs.h"
#include "../libXrdIO/functions.h"
#include "projectWidget/syntaxHighlighter/bgmnhighlighter.h"
#include "imageresolutiondialog.h"
#include "projectWidget/bgmnbackendconfig.h"
#include "codcifretrievedialog.h"

#include <QDebug>
#include <QStringList>
#include <QListWidget>
#include <QFileInfo>
#include <QFileDialog>
#include <QInputDialog>
#include <QJSEngine>
#include <QtMath>
#include <QProgressDialog>

StrucImportDialog::StrucImportDialog(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::StrucImportDialog)
{
    ui->setupUi(this);

    settings = SettingsManager::getInstance();
    codManager = CodDbManager::getInstance();

    textEditMessages = new QPlainTextEdit(this);
    ui->dockWidgetOutput->setWidget(textEditMessages);
    processWasCancelled = false;

    progressBar = new QProgressBar(this);
    labelNumberOfFiles = new QLabel(QString(), this);
    ui->statusbar->addWidget(labelNumberOfFiles);
    ui->statusbar->addPermanentWidget(progressBar);

    progressBar->reset();
    progressBar->setEnabled(false);
    labelNumberOfFiles->clear();
    ui->action_Abort->setEnabled(false);

    comboBoxWavelength = new WaveLengthComboBox(this);
    comboBoxWavelength->showKa2(false);
    comboBoxWavelength->showKb(false);
    comboBoxWavelength->initData();
    ui->toolBarWaveLength->addWidget(comboBoxWavelength);

    codSearchDialog = nullptr;
    codOnlineDownloader = nullptr;
    codLocalDownloader = nullptr;

    strIndexRequired = false;
    nVerifiedFiles = 0;

    hklPlot = new QCustomPlot(this);
    hklPlot->xAxis->setLabel(tr("Diffraction Angle [%1 2%2]").arg(global::degree).arg(global::theta));
    hklPlot->yAxis->setLabel(tr("Intensity [%]"));
    hklPlot->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    hklPlot->xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
    hklPlot->xAxis->axisRect()->setRangeZoom(Qt::Horizontal);
    ui->dockWidgetGraph->setWidget(hklPlot);
    hklPlot->legend->setVisible(true);

    tabifyDockWidget(ui->dockWidgetOutput, ui->dockWidgetGraph);

    toggleActionsEnabled();
    initSettings();
    initConnections();
    sgParser.init();
}

StrucImportDialog::~StrucImportDialog()
{
    if (codSearchDialog)     delete codSearchDialog;
    if (codOnlineDownloader) delete codOnlineDownloader;
    if (codLocalDownloader)  delete codLocalDownloader;
    delete ui;
}

void StrucImportDialog::closeEvent(QCloseEvent *e)
{
    if (strIndexRequired) emit emitRunIndexing();

    saveSettings();
    QMainWindow::closeEvent(e);
}

void StrucImportDialog::initSettings()
{
    restoreGeometry(settings->value("strucimportdlg/geometry", QByteArray()).toByteArray());
    restoreState(settings->value("strucimportdlg/windowState", QByteArray()).toByteArray());

    comboBoxWavelength->setCurrentIndex(settings->value("strucimportdlg/wavelength", 4).toInt());
    msgFadeColor = settings->value("strucimportdlg/fadecol", "#888888").toString();

    selectedFilter = settings->value("strucimportdlg/filter", QString()).toString();

    BgmnHighlighter *bhl = new BgmnHighlighter(settings->getSyntaxHighlightingMode(), this);
    bhl->setDocument(ui->textEditStr->document());

    QFont edFont;
    edFont.fromString(settings->value("config/editorFont", font().toString()).toString());
    ui->textEditSource->setFont(edFont);
    textEditMessages->setFont(edFont);
    ui->textEditStr->setFont(edFont);

    hklPlot->setBackground(QBrush(QGuiApplication::palette().color(QPalette::Base)));

    QPen penAxis(settings->isDarkMode() ? global::Functions::colorToDarkMode(QColor(Qt::black)) : Qt::black);
    QPen penGrid(settings->isDarkMode() ? global::Functions::colorToDarkMode(QColor(Qt::lightGray)) : Qt::lightGray, 0, Qt::DotLine);
    hklPlot->xAxis->setBasePen(penAxis);
    hklPlot->xAxis->setTickPen(penAxis);
    hklPlot->xAxis->setSubTickPen(penAxis);
    hklPlot->xAxis->setLabelColor(penAxis.color());
    hklPlot->xAxis->setTickLabelColor(penAxis.color());
    hklPlot->xAxis->grid()->setPen(penGrid);
    hklPlot->xAxis->grid()->setSubGridPen(penGrid);
    hklPlot->xAxis->grid()->setVisible(false);
    hklPlot->xAxis->grid()->setSubGridVisible(false);

    hklPlot->yAxis->setBasePen(penAxis);
    hklPlot->yAxis->setTickPen(penAxis);
    hklPlot->yAxis->setSubTickPen(penAxis);
    hklPlot->yAxis->setLabelColor(penAxis.color());
    hklPlot->yAxis->setTickLabelColor(penAxis.color());
    hklPlot->yAxis->grid()->setPen(penGrid);
    hklPlot->yAxis->grid()->setSubGridPen(penGrid);
    hklPlot->yAxis->grid()->setVisible(false);
    hklPlot->yAxis->grid()->setSubGridVisible(false);

    hklPlot->legend->setBrush(hklPlot->background());
    hklPlot->legend->setTextColor(penAxis.color());
    hklPlot->legend->setBorderPen(Qt::NoPen);
}

void StrucImportDialog::saveSettings()
{
    settings->setValue("strucimportdlg/geometry", saveGeometry());
    settings->setValue("strucimportdlg/windowState", saveState());
    settings->setValue("strucimportdlg/filter", selectedFilter);
    settings->setValue("strucimportdlg/wavelength", comboBoxWavelength->currentIndex());
}

void StrucImportDialog::initConnections()
{
    connect(hklPlot->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(zoomHklPlot(QCPRange)));
    connect(hklPlot, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(resetHklPlotZoom(QMouseEvent*)));
    connect(ui->listWidgetFiles, SIGNAL(currentItemChanged(QListWidgetItem*,QListWidgetItem*)), this, SLOT(selectionChanged(QListWidgetItem*,QListWidgetItem*)));

    connect(ui->action_Load_source_files, SIGNAL(triggered()), this, SLOT(addFiles()));
    connect(ui->action_Save, SIGNAL(triggered()), this, SLOT(saveAs()));
    connect(ui->actionSave_all, SIGNAL(triggered()), this, SLOT(saveAll()));
    connect(ui->action_Export_messages, SIGNAL(triggered()), this, SLOT(exportMessages()));
    connect(ui->actionExport_graphs, SIGNAL(triggered()), this, SLOT(exportGraphs()));
    connect(ui->action_Retrieve_COD_records, SIGNAL(triggered()), this, SLOT(enterCodCodes()));
    connect(ui->action_Search_COD, SIGNAL(triggered()), this, SLOT(searchCodDb()));
    connect(ui->action_Verify_structure, SIGNAL(triggered()), this, SLOT(verify()));
    connect(ui->actionVerify_all_structures, SIGNAL(triggered()), this, SLOT(verifyAll()));
    connect(ui->action_Abort, SIGNAL(triggered()), this, SLOT(cancelProcess()));
    connect(ui->actionRemove_file, SIGNAL(triggered()), this, SLOT(removeFile()));
    connect(ui->actionClose_all_files, SIGNAL(triggered()), this, SLOT(closeAll()));
    connect(ui->actionClose_verified_files, SIGNAL(triggered()), this, SLOT(closeVerified()));
    connect(ui->actionConvert_source_file, SIGNAL(triggered()), this, SLOT(convertStructure()));

    connect(comboBoxWavelength, SIGNAL(currentIndexChanged(int)), this, SLOT(redrawHklPlot()));
}

/*
 * raises the dialog and immediately shows the "add file" file dialog
 */
void StrucImportDialog::showAndAdd()
{
    QMainWindow::show();
    addFiles();
}

/*
 * public function, receives a structure filename to populate the file list
 */
void StrucImportDialog::setFile(const QString &s)
{
    setFileNames(QStringList(s));
}

/*
 * public function, receives a list of structure filenames to populate the file list
 */
void StrucImportDialog::setFile(const QStringList &l)
{
    setFileNames(l);
}

/*
 * receives a list of cif filenames, populates the listwidget and displays the first item
 */
void StrucImportDialog::setFileNames(const QStringList &l)
{
    if (!l.size()) return;

    verifyList.clear();
    setGuiState(false);
    processWasCancelled = false;
    progressBar->setMaximum(l.size());
    progressBar->setValue(0);
    progressBar->setEnabled(true);
    ui->action_Abort->setEnabled(true);

    clearData();
    bool oldState = ui->listWidgetFiles->blockSignals(true);
    int n = ui->listWidgetFiles->count();

    for (int i = 0; i < l.size(); ++i) {
        if (processWasCancelled) break;

        QFileInfo fi(l.at(i));
        qDebug() << QString("StrucImportDialog::setFileNames(): Importing file %1").arg(fi.absoluteFilePath());

        QListWidgetItem *it = new QListWidgetItem(ui->listWidgetFiles);
        it->setText(QString("*%1").arg(fi.fileName()));
        ui->listWidgetFiles->addItem(it);

        StrucImportItemData *itData = new StrucImportItemData();
        connect(itData, SIGNAL(sigVerifyComplete(QUuid)), this, SLOT(verifyComplete(QUuid)));
        itData->setUiPointers(&sgParser);
        itData->setSourceFileName(fi.absoluteFilePath());
        itData->loadData();

        listData[itData->uid()] = itData;
        listItems[it] = itData->uid();
        verifyList.append(itData->uid());

        progressBar->setValue(i);
        qApp->processEvents();
    }

    ui->listWidgetFiles->setCurrentRow(n);
    displayItemData(itemData(n));

    progressBar->reset();
    progressBar->setValue(0);
    progressBar->setMaximum(verifyList.size());
    progressBar->setEnabled(true);
    ui->action_Abort->setEnabled(true);

    ui->listWidgetFiles->blockSignals(oldState);

    verifyNext();
}

void StrucImportDialog::selectionChanged(QListWidgetItem *curr, QListWidgetItem *prev)
{
    StrucImportItemData *itp = itemData(prev);
    StrucImportItemData *itc = itemData(curr);

    if (itp) itp->saveData(ui->textEditStr->toPlainText(), textEditMessages->document()->toHtml());
    if (itc) displayItemData(itc);
}

/*
 * slot causing the current source file to be converted to BGMN STR again
 */
void StrucImportDialog::convertStructure()
{
    StrucImportItemData *it = currentItemData();
    if (!it) return;

    it->setStrString(QString());
    it->setSourceString(ui->textEditSource->toPlainText());
    it->parseSourceString();
    setVerifiedOKStatus(it->uid(), false);
    displayItemData(it);
    ui->tabWidget->setCurrentIndex(0);
    verify();
}

void StrucImportDialog::clearData()
{
    ui->textEditSource->clear();
    ui->textEditStr->clear();
    textEditMessages->clear();
    hklPlot->clearPlottables();
    hklPlot->replot();
}

void StrucImportDialog::displayItemData(const StrucImportItemData *it)
{
    if (!it) {
        clearData();
        return;
    }

    ui->textEditStr->setPlainText(it->strString());
    ui->textEditSource->setPlainText(it->sourceString());
    setMessage(it);

    if (it->verifiedOk()) {
        ui->textEditStr->setErrorLineIndices(it->errorLines());
        plotHklLines(it);
    } else {
        hklPlot->clearPlottables();
        hklPlot->replot();
    }

    ui->actionExport_graphs->setEnabled(it->verifiedOk());
    updateNumberOfFiles();
}

/*
 * Opens a file dialog to select one or more structure files (CIF or XML)
 */
void StrucImportDialog::addFiles()
{
    StrucImportItemData *it = currentItemData();
    if (it) it->saveData(ui->textEditStr->toPlainText(), textEditMessages->document()->toHtml());

    if (workingDir.isEmpty()) workingDir = settings->value("strucimportdlg/workingdir", QDir::homePath()).toString();

    QStringList flist = QFileDialog::getOpenFileNames(this,
                                                      tr("Open crystal structure data files"),
                                                      workingDir,
                                                      tr("CIF Files (*.cif *.CIF)"
                                                         ";;ICDD XML Files (*.xml *.XML)"
                                                         ";;Rruff DIF files (*.txt *.TXT *.dif *.DIF)"
                                                         ";;BGMN STR Files (*.str *.STR)"),
                                                      &selectedFilter);

    if (!flist.isEmpty()) {
        QFileInfo fi(flist.last());
        workingDir = fi.absolutePath();
        settings->setValue("strucimportdlg/workingdir", workingDir);
        setFileNames(flist);
    }
}

void StrucImportDialog::searchCodDb()
{
    if (!codManager->isConnected()) {
        QMessageBox::information(this,
                                 tr("COD database not connected"),
                                 tr("<p>The COD database is not connected.</p>"
                                    "<p>To download and install the COD:</p>"
                                    "<p><ol><li>Download the COD file from the Profex website<br>(https://www.profex-xrd.org).</li>"
                                    "<li>Extract the ZIP file to the hard drive.</li>"
                                    "<li>Open \"Edit -> Preferences -> COD Database\" and connect to the extracted file.</li></ol></p>"));

        return;
    }

    QStringList records;

    if (!codSearchDialog) {
        codSearchDialog = new CodCifRetrieveDialog(this);
    }

    if (codSearchDialog->exec() == QDialog::Accepted) {
        records = codSearchDialog->getRecords();
    } else {
        return;
    }

    qDebug() << QString("StrucImportDialog::retrieveDbFiles(): Selected %1 records for download (%2)")
                .arg(records.size()).arg(records.join(", "));

    if (!records.size()) {
        return;
    }

    processWasCancelled = false;
    progressBar->setMaximum(records.size());
    progressBar->setValue(0);
    progressBar->setEnabled(true);
    ui->action_Abort->setEnabled(true);

    bool onlineCif = settings->value("config/useOnlineCifDir", true).toBool();

    if (onlineCif) {
        if (!codOnlineDownloader) {
            codOnlineDownloader = new CodDownloadManager(this);
            connect(codOnlineDownloader, SIGNAL(cifDownloadComplete(QList<CifFile>&, QStringList&)), this, SLOT(loadCodCifs(QList<CifFile>&, QStringList&)));
            connect(codOnlineDownloader, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
        }

        QString codUrl = settings->value("strucimportdlg/codserver", "http://www.crystallography.net/cod").toString();
        if (!codUrl.isEmpty()) codOnlineDownloader->startCifDownload(records, codUrl);
    } else {
        if (!codLocalDownloader) {
            codLocalDownloader = new CodLocalCifManager(this);
            connect(codLocalDownloader, SIGNAL(cifDownloadComplete(QList<CifFile>&, QStringList&)), this, SLOT(loadCodCifs(QList<CifFile>&, QStringList&)));
            connect(codLocalDownloader, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
        }

        QString cifUrl = settings->value("config/localCodCifDir", "").toString();
        if (!cifUrl.isEmpty()) codLocalDownloader->startCifDownload(records, cifUrl);
    }
}

void StrucImportDialog::loadCodCifs(QList<CifFile> &cifFileContent, QStringList &failed)
{
    QString dir = settings->getTempLocation();
    qDebug() << QString("StrucImportDialog::loadCodCifs(): Temp location set to %1").arg(dir);

    static QRegularExpression rxMineral("_chemical_name_mineral\\s+([^\\r\\n]+)");
    static QRegularExpression rxReplaceSpaces("[\\s,\\.]+");
    QRegularExpressionMatch rm;
    QStringList localFiles;

    for (int i = 0; i < cifFileContent.size(); ++i) {
        QString fname = cifFileContent.at(i).record + ".cif";
        rm = rxMineral.match(cifFileContent.at(i).content);

        if (rm.hasMatch()) {
            QString minName = rm.captured(1);
            minName = minName.replace(rxReplaceSpaces, "_");
            fname = minName + "_COD_" + fname;
        }

        fname = dir + QDir::separator() + fname;
        qDebug() << QString("StrucImportDialog::loadCodCifs(): Writing file %1").arg(fname);
        BgmnFileIO::writeTextFile(fname, cifFileContent.at(i).content);
        localFiles.append(fname);
    }

    progressBar->reset();
    progressBar->setEnabled(false);
    ui->action_Abort->setEnabled(false);
    setFileNames(localFiles);

    if (failed.size()) {
        QMessageBox::information(this, tr("Download failed"),
                                 QString(tr("CIF files of the following records could not be downloaded:\n%1"))
                                     .arg(failed.join(", ")));
    }
}

void StrucImportDialog::enterCodCodes()
{
    bool ok;
    QString s = QInputDialog::getMultiLineText(this, tr("CIF file download"), tr("Enter COD IDs"), QString(), &ok);
    if (s.isEmpty() || !ok) return;

    static QRegularExpression rxSep("[\\s,;]+");
    QStringList records = s.split(rxSep);

    bool onlineCif = settings->value("config/useOnlineCifDir", true).toBool();
    if (onlineCif) {
        if (!codOnlineDownloader) {
            codOnlineDownloader = new CodDownloadManager(this);
            connect(codOnlineDownloader, SIGNAL(cifDownloadComplete(QList<CifFile>&, QStringList&)), this, SLOT(loadCodCifs(QList<CifFile>&, QStringList&)));
            connect(codOnlineDownloader, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
        }

        QString codUrl = settings->value("strucimportdlg/codserver", "http://www.crystallography.net/cod").toString();
        if (!codUrl.isEmpty()) codOnlineDownloader->startCifDownload(records, codUrl);
    } else {
        if (!codLocalDownloader) {
            codLocalDownloader = new CodLocalCifManager(this);
            connect(codLocalDownloader, SIGNAL(cifDownloadComplete(QList<CifFile>&, QStringList&)), this, SLOT(loadCodCifs(QList<CifFile>&, QStringList&)));
            connect(codLocalDownloader, SIGNAL(progress(int)), progressBar, SLOT(setValue(int)));
        }

        QString cifUrl = settings->value("config/localCodCifDir", "").toString();
        if (!cifUrl.isEmpty()) codLocalDownloader->startCifDownload(records, cifUrl);
    }
}

void StrucImportDialog::removeFile()
{
    QListWidgetItem *it = ui->listWidgetFiles->takeItem(ui->listWidgetFiles->currentRow());
    if (!it) return;

    StrucImportItemData *dt = itemData(it);

    if (dt) {
        listData.remove(dt->uid());
        delete dt;
    }

    delete it;
    displayItemData(currentItemData());
    toggleActionsEnabled();
}

void StrucImportDialog::closeAll()
{
    if (QMessageBox::question(this,
                              tr("Close all files"),
                              tr("Do you want to close all files?\n"
                                 "Unsaved files will be lost.")) == QMessageBox::No) {
        return;
    }

    ui->listWidgetFiles->clear();
    labelNumberOfFiles->clear();
    listData.clear();
    listItems.clear();
    clearData();
    toggleActionsEnabled();
}

void StrucImportDialog::closeVerified()
{
    if (QMessageBox::question(this,
                              tr("Close all verified files"),
                              tr("Do you want to close all verified files?\n"
                                 "Unsaved files will be lost.")) == QMessageBox::No) {
        return;
    }

    for (int i = ui->listWidgetFiles->count() - 1; i >= 0; --i) {
        StrucImportItemData *dt = itemData(i);
        if (!dt) continue;

        if (dt->verifiedOk()) {
            QListWidgetItem *dl = ui->listWidgetFiles->takeItem(i);

            if (dl) {
                listData.remove(listItems.value(dl));
                listItems.remove(dl);
            }

            delete dl;
        }
    }

    displayItemData(currentItemData());
    toggleActionsEnabled();
}

StrucImportItemData * StrucImportDialog::currentItemData()
{
    return listData.value(listItems.value(ui->listWidgetFiles->currentItem(), QUuid()), nullptr);
}

StrucImportItemData * StrucImportDialog::itemData(int n)
{
    return listData.value(listItems.value(ui->listWidgetFiles->item(n), QUuid()), nullptr);
}

StrucImportItemData * StrucImportDialog::itemData(QListWidgetItem *it)
{
    return listData.value(listItems.value(it, QUuid()), nullptr);
}

QUuid StrucImportDialog::currentItemUid() const
{
    return listItems.value(ui->listWidgetFiles->currentItem(), QUuid());
}

/*
 * Save the current strfilecontent to a file
 */
void StrucImportDialog::saveAs()
{
    StrucImportItemData *it = currentItemData();
    if (!it) return;

    it->saveData(ui->textEditStr->toPlainText(), textEditMessages->document()->toHtml());

    if (!it->verifiedOk()) {
        QMessageBox::warning(this, tr("Save STR file"), tr("Warning: The STR file has not been verified."));
    }

    BgmnBackendConfig bkgConfig;
    QString dir = bkgConfig.getUserStuctureRepo();
    QString selFilter;
    QStringList filters;

    filters << QString("BGMN Structure Files (*.str *.STR)");

    // note: we only allow export to CIF if a CrystalStructure object is found in the crystalStructureMap,
    //       because the CIF export from there is used. Not all imported formats can create a CrystalStructure
    //       object.
    if (crystalStructureMap.contains(it->uid())) {
        filters << QString("CIF Files (*.cif *.CIF)");
    }

    QString f = QFileDialog::getSaveFileName(this, tr("Save STR File"), dir, filters.join(";;"), &selFilter);

    if (f.isEmpty()) return;

    saveFile(it, f, filters.indexOf(selFilter));
    textEditMessages->appendPlainText(QString("File saved as %1").arg(f));
}

void StrucImportDialog::saveAll()
{
    QMessageBox::information(this, tr("Save all structure files"), tr("Only verified structures will be saved."));

    StrucImportItemData *it = currentItemData();
    if (it) it->saveData(ui->textEditStr->toPlainText(), textEditMessages->document()->toHtml());

    BgmnBackendConfig bkgConfig;
    QString dir = bkgConfig.getUserStuctureRepo();
    dir = QFileDialog::getExistingDirectory(this, tr("Output Directory"), dir);

    if (!QFile::exists(dir)) return;

    textEditMessages->clear();

    for (int i = 0; i < ui->listWidgetFiles->count(); ++i) {
        StrucImportItemData *it = itemData(i);

        if (!it) continue;
        if (!it->verifiedOk()) continue;

        QFileInfo fiSource(it->sourceFileName());
        QFileInfo fiSave(dir + "/" + fiSource.baseName() + ".str");
        saveFile(it, fiSave.absoluteFilePath(), 0);
        textEditMessages->appendPlainText(QString("File saved as %1").arg(fiSave.absoluteFilePath()));
    }
}

void StrucImportDialog::saveFile(const StrucImportItemData *it, const QString &fileName, int format)
{
    if (!it) return;

    if (format == 0) {
        BgmnFileIO::writeTextFile(fileName, it->strString());
        strIndexRequired = true;
    }

    if (format == 1) {
        CrystalStructure struc = crystalStructureMap[it->uid()];
        BgmnFileIO::writeTextFile(fileName, struc.toCif(true, it->sourceFileName()));
    }
}

/*
 * slot called by the UI toolbutton
 */
void StrucImportDialog::verify()
{
    setGuiState(false);
    StrucImportItemData *it = currentItemData();
    if (it) it->saveData(ui->textEditStr->toPlainText(), textEditMessages->document()->toHtml());

    processWasCancelled = false;
    verifyList.clear();
    verifyList.append(currentItemUid());
    verifyNext();
}

/*
 * slot called by the UI toolbutton
 */
void StrucImportDialog::verifyAll()
{
    StrucImportItemData *it = currentItemData();
    if (it) it->saveData(ui->textEditStr->toPlainText(), textEditMessages->document()->toHtml());

    progressBar->setMaximum(ui->listWidgetFiles->count());
    progressBar->setValue(0);
    progressBar->setEnabled(true);
    ui->action_Abort->setEnabled(true);

    setGuiState(false);

    processWasCancelled = false;
    verifyList = listData.keys();
    verifyNext();
}

void StrucImportDialog::verifyNext()
{
    if (verifyList.size()) {
        StrucImportItemData *it = listData.value(verifyList.takeFirst(), nullptr);

        if (it) {
            it->verify();
        } else {
            verifyComplete(QUuid());
        }
    } else {
        verifyComplete(QUuid());
    }
}

void StrucImportDialog::verifyComplete(QUuid uid)
{
    StrucImportItemData *verifiedItem = listData.value(uid, nullptr);

    if (verifiedItem) {
        setVerifiedOKStatus(uid, verifiedItem->verifiedOk());
    }

    if (verifyList.size() && !processWasCancelled) {
        verifyNext();
    } else {
        StrucImportItemData *displayedItem = currentItemData();

        if (displayedItem) {
            displayItemData(displayedItem);
        }

        progressBar->reset();
        progressBar->setEnabled(false);
        ui->action_Abort->setEnabled(false);
        setGuiState(true);
        toggleActionsEnabled();
    }
}

void StrucImportDialog::plotHklLines(const StrucImportItemData *it)
{
    if (!it) return;

    hklPlot->clearPlottables();
    double wl = comboBoxWavelength->currentData(Qt::UserRole).toDouble();

    if (it->hklData().hklData().isEmpty()) {
        hklPlot->replot();
        return;
    }

    QVector<double> xdata(hklToTwoTheta(it->hklData().hklData(), wl));
    QVector<double> ydata(hklToIntensity(it->hklData().hklData()));

    QCPBars *hklBars = new QCPBars(hklPlot->xAxis, hklPlot->yAxis);
    hklBars->setData(xdata, ydata);
    hklBars->setWidthType(QCPBars::wtAbsolute);
    hklBars->setPen(QPen(settings->isDarkMode() ? global::Functions::colorToDarkMode(QColor(Qt::blue)) : Qt::blue, 0.0));
    hklBars->setName(QString("%1 = %2 g/cm%3").arg(global::rho).arg(it->hklData().density(), 0, 'f', 4).arg(global::superThree));

    resetHklPlotZoom();
}

void StrucImportDialog::redrawHklPlot()
{
    plotHklLines(currentItemData());
}

QVector<double> StrucImportDialog::hklToTwoTheta(const QVector<Hkl> &vec, double wl)
{
    QVector<double> dvec;

    for (int i = 0; i < vec.size(); ++i) {
        double tth = 2.0 * qRadiansToDegrees(qAsin(wl / (2.0 * vec.at(i).position())));
        dvec.append(tth);
    }

    return dvec;
}

QVector<double> StrucImportDialog::hklToIntensity(const QVector<Hkl> &vec)
{
    QVector<double> dvec;
    double imax = 0.0;

    for (int i = 0; i < vec.size(); ++i) {
        imax = qMax(imax, vec.at(i).intensity());
    }

    for (int i = 0; i < vec.size(); ++i) {
        dvec.append(100.0 * vec.at(i).intensity() / imax);
    }

    return dvec;
}

void StrucImportDialog::resetHklPlotZoom(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) resetHklPlotZoom();
}

void StrucImportDialog::resetHklPlotZoom()
{
    double dmax = settings->value("config/hklUpperRangeD", 0.1540598).toDouble();
    double wmax = global::Functions::dToTwoTheta(dmax, comboBoxWavelength->currentData(Qt::UserRole).toDouble());

    hklPlot->xAxis->setRange(0.0, wmax);
    hklPlot->yAxis->setRange(0.0, 100.0);
    hklPlot->replot();
}

void StrucImportDialog::zoomHklPlot(const QCPRange &newRange)
{
    double wmin = 0.0;
    double wmax = 180.0;

    QCPRange fixedRange(newRange);

    if (fixedRange.lower < wmin)
    {
        fixedRange.lower = wmin;
        fixedRange.upper = wmin + newRange.size();

        if (fixedRange.upper > wmax || qFuzzyCompare(newRange.size(), wmax-wmin)) {
            fixedRange.upper = wmax;
        }

        hklPlot->xAxis->setRange(fixedRange);
    } else if (fixedRange.upper > wmax)
    {
        fixedRange.upper = wmax;
        fixedRange.lower = wmax - newRange.size();

        if (fixedRange.lower < wmin || qFuzzyCompare(newRange.size(), wmax-wmin)) {
            fixedRange.lower = wmin;
        }

        hklPlot->xAxis->setRange(fixedRange);
    }

    hklPlot->replot();
}

void StrucImportDialog::exportGraphs()
{
    StrucImportItemData *it = currentItemData();
    if (!it) return;

    QString selectedFilter;
    QFileInfo fiSource(it->sourceFileName());
    QString fn = QFileDialog::getSaveFileName(this,
                                              tr("CSV File Name"),
                                              settings->value("config/workingdir", QDir::homePath()).toString() + fiSource.completeBaseName(),
                                              tr("CSV File (*.csv *.CSV);;PDF Graph (*.pdf *.PDF);;PNG Image (*.png *.PNG)"),
                                              &selectedFilter);

    if (fn.isEmpty()) return;

    QFileInfo fi(fn);
    bool addSuffix = fi.suffix().isEmpty();

    if (selectedFilter.startsWith("CSV")) {
        QString f = fi.absoluteFilePath() + (addSuffix ? ".csv" : QString());
        exportHklCsv(f, it);
    } else if (selectedFilter.startsWith("PDF")) {
        QString f = fi.absoluteFilePath() + (addSuffix ? ".pdf" : QString());
        exportHklPdf(f, it);
    } else if (selectedFilter.startsWith("PNG")) {
        QString f = fi.absoluteFilePath() + (addSuffix ? ".png" : QString());
        exportHklPng(f, it);
    }
}

void StrucImportDialog::exportHklCsv(const QString &fi, const StrucImportItemData *it)
{
    qApp->setOverrideCursor(Qt::WaitCursor);

    double lambda = 10.0 * comboBoxWavelength->currentData(Qt::UserRole).toDouble();

    QString out = QString("HKL data generated from file %1\n").arg(fi);
    out += QString("Wavelength;%1;Angstrom\n\n").arg(lambda, 0, 'f', 6);
    out += "h;k;l;d [Angstrom];2theta [degrees];Intensity [%]\n";

    QVector<Hkl> vec = it->hklData().hklData();
    QVector<double> ttheta = hklToTwoTheta(vec, 0.1 * lambda);
    QVector<double> intens = hklToIntensity(vec);

    int n = qMin(vec.size(), qMin(ttheta.size(), intens.size()));

    for (int i = 0; i < n; ++i) {
        static QRegularExpression rxSep("\\s+");
        QStringList hklStrings = vec.at(i).hkl().split(rxSep);
        out += hklStrings.join(";") + ";";
        out += QString("%1;").arg(10.0 * vec.at(i).position(), 0, 'f', 6);
        out += QString("%1;").arg(ttheta.at(i), 0, 'f', 6);
        out += QString("%1\n").arg(intens.at(i), 0, 'f', 6);
    }

    BgmnFileIO::writeTextFile(fi, out);

    qApp->restoreOverrideCursor();
}

void StrucImportDialog::exportHklPdf(const QString &fi, const StrucImportItemData *it)
{
    Q_UNUSED(it);
    qApp->setOverrideCursor(Qt::WaitCursor);
    hklPlot->savePdf(fi);
    qApp->restoreOverrideCursor();
}

void StrucImportDialog::exportHklPng(const QString &fi, const StrucImportItemData *it)
{
    Q_UNUSED(it);
    int w = 1280;
    int h = 1024;

    ImageResolutionDialog *irdlg = new ImageResolutionDialog(this);

    irdlg->setPixelWidth(w);
    irdlg->setPixelHeight(h);

    if (irdlg->exec() == QDialog::Accepted) {
        w = irdlg->pixelWidth();
        h = irdlg->pixelHeight();
        delete irdlg;
    } else {
        delete irdlg;
        return;
    }

    qApp->setOverrideCursor(Qt::WaitCursor);
    hklPlot->savePng(fi, w, h);
    qApp->restoreOverrideCursor();
}

void StrucImportDialog::exportMessages()
{
    QString filter;
    QString fn = QFileDialog::getSaveFileName(this,
                                              tr("File Name"),
                                              settings->value("config/workingdir", QDir::homePath()).toString(),
                                              tr("HTML File (*.html *.HTML);;Text file (*.txt *.TXT)"), &filter);

    if (fn.isEmpty()) return;

    QString outStr("************************************************************************************<br>");

    for (int i = 0; i < ui->listWidgetFiles->count(); ++i) {
        StrucImportItemData *it = dynamic_cast<StrucImportItemData*>(ui->listWidgetFiles->item(i));
        if (it) outStr += it->msgString() + "<br>";
        outStr += "************************************************************************************<br>";
    }

    if (filter.startsWith("Text")) {
        BgmnFileIO::writeTextFile(fn, QTextDocumentFragment::fromHtml(outStr).toPlainText());
    } else if (filter.startsWith("HTML")) {
        BgmnFileIO::writeTextFile(fn, outStr);
    }
}

void StrucImportDialog::setVerifiedOKStatus(const QUuid &uid, bool ok)
{
    QListWidgetItem *it = listItems.key(uid, nullptr);
    if (!it) return;

    QString itemText = it->text();
    static QRegularExpression rx("\\*?(.+)");
    QRegularExpressionMatch rm = rx.match(itemText);

    if (rm.hasMatch()) {
        itemText = QString("%1%2").arg(ok ? "" : "*", rm.captured(1));
        it->setText(itemText);
    }

    qDebug() << QString("StrucImportDialog::setVerifiedOKStatus(): Setting item status to %1 (%2)").arg(itemText, ok ? "true" : "false");
}

void StrucImportDialog::updateNumberOfFiles()
{
    nVerifiedFiles = 0;

    for (int i = 0; i < ui->listWidgetFiles->count(); ++i) {
        StrucImportItemData *it = itemData(i);
        if (!it) continue;
        if (it->verifiedOk()) ++nVerifiedFiles;
    }

    labelNumberOfFiles->setText(QString("%1/%2").arg(nVerifiedFiles).arg(ui->listWidgetFiles->count()));
}

void StrucImportDialog::setMessage(const StrucImportItemData *it)
{
    textEditMessages->clear();
    if (!it) return;
    textEditMessages->appendHtml(it->msgString());

}

void StrucImportDialog::toggleActionsEnabled()
{
    bool saveActionsState = nVerifiedFiles > 0;
    bool closeActionsState = ui->listWidgetFiles->count() > 0;
    bool exportGraphsState = hklPlot->graphCount() > 0;
    bool exportMessagesState = !textEditMessages->toPlainText().isEmpty();

    ui->actionRemove_file->setEnabled(closeActionsState);
    ui->actionClose_all_files->setEnabled(closeActionsState);
    ui->actionClose_verified_files->setEnabled(closeActionsState);

    ui->action_Save->setEnabled(saveActionsState);
    ui->actionSave_all->setEnabled(saveActionsState);

    ui->actionExport_graphs->setEnabled(exportGraphsState);
    ui->action_Export_messages->setEnabled(exportMessagesState);
}

void StrucImportDialog::setGuiState(bool b)
{
    ui->listWidgetFiles->setEnabled(b);
    ui->textEditSource->setEnabled(b);
    ui->textEditStr->setEnabled(b);
    textEditMessages->setEnabled(b);
    hklPlot->setEnabled(b);
}

void StrucImportDialog::cancelProcess()
{
    processWasCancelled = true;
}
