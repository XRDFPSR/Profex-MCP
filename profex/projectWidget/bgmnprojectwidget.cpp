/***************************************************************************
                          bgmnprojectwidget.cpp  -  description
                             -------------------
    begin                : Mon Jan 30 11:00:00 CEST 2012
    copyright            : (C) 2012 by Nicola Doebelin
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

#include "bgmnprojectwidget.h"
#include "projectWidget/bgmnpresethandlergui.h"
#include "syntaxHighlighter/bgmnhighlighter.h"
#include "gentemplatedialog.h"
#include "internalstandarddialog.h"
#include "../quazip/JlCompress.h"

#include "../libXrdIO/parser/bgmnlstparser.h"
#include "../libXrdIO/parser/bgmnsavparser.h"
#include "../libXrdIO/parser/bgmnstrparser.h"
#include "../libXrdIO/parser/bgmngeqparser.h"
#include "../libXrdIO/parser/bgmnresparser.h"
#include "../libXrdIO/parser/bgmnlamparser.h"
#include "../libXrdIO/import/importhandler.h"
#include "../libXrdIO/export/exporthandler.h"
#include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/crystal/crystalstructure.h"
#include "../libXrdIO/scan.h"
#include "../libXrdIO/scanops.h"
#include "../libXrdIO/structs.h"
#include "../libXrdIO/parser/bgmninstrumentsavparser.h"
#include "../libXrdIO/parser/bgmnhtmlreportgenerator.h"
#include "bgmnbackendconfig.h"
#include "bgmninstrumentselectdialog.h"
#include "bgmnsinglepeakrefinementdialog.h"

#include <QFileDialog>
#include <QMessageBox>
#include <QApplication>
#include <QProgressDialog>
#include <QDomDocument>
#include <QDomNode>
#include <QDebug>
#include <QDirIterator>
#include <QPair>
#include <QFont>

BgmnProjectWidget::BgmnProjectWidget(QWidget *parent) :
        ProjectWidget(parent)
{
    // init the BGMN process handler
    bgmnHandler = new BgmnHandler(QUuid::createUuid(), this);
    eflechHandler = new EflechHandler(QUuid::createUuid(), this);
    diaHandler = new BgmnDiaHandler2(this);

    controlFileExtension = QStringLiteral("sav");
    currentIteration = 0;

    // these files will be loaded automatically
    textFormats << QStringLiteral("sav") << QStringLiteral("SAV") << QStringLiteral("lst") << QStringLiteral("LST");

    ImportHandler iHandler;
    graphFormats = iHandler.extensions();

    // make sure the dia file is first in the list
    // because it's our preferred file format to auto-load
    graphFormats.prepend("dia");

    // file formats directly supported by the Rietveld backend
    nativeGraphFormats << "xy";

    // on non-win platforms, *.val files cannot be treated as native because
    // bgmn doesn't accept win line endings
#ifdef Q_OS_WIN
    nativeGraphFormats << "val";
#endif

    // create the add/remove phase dialog
    arPhaseDialog = nullptr;

    // create strfilebatchedit dialog
    strFileEditDlg = nullptr;

    pSelectItem->setProjectType("BGMN");
    refStrManager = BgmnRefStructureManager::getInstance();

    excelExporter = new MsoExportExcel(graphControl, this);

    QStringList resultsHeaders;
    resultsHeaders << tr("Parameter") << tr("Value") << tr("ESD");

    resultsTree = new BgmnResultsTreeWidget(this);
    resultsTree->setColumnCount(resultsHeaders.count());
    resultsTree->setHeaderLabels(resultsHeaders);
    resultsTree->setLstParser(&lparser);
    resultsTree->initSettings();

    connect(bgmnHandler, SIGNAL(pollOutput()), this, SLOT(bgmnPollOutput()));
    connect(bgmnHandler, SIGNAL(allComplete(QUuid,global::RefinementStatus)), this, SLOT(bgmnComplete(QUuid,global::RefinementStatus)));
    connect(bgmnHandler, SIGNAL(aborted()), this, SLOT(processAborted()));
    connect(eflechHandler, SIGNAL(aborted()), this, SLOT(processAborted()));
    connect(searchMatchWidget, SIGNAL(refStructureSelected(QString)), this, SLOT(setReferenceStructure(QString)));
    connect(searchMatchWidget, SIGNAL(refStructureReset()), this, SLOT(resetRefStructure()));
    connect(searchMatchWidget, SIGNAL(exitStatus(int)), this, SLOT(searchMatchStatus(int)));
    connect(scanList, SIGNAL(currentScanChanged(int)), this, SLOT(currentScanChanged(int)));

    initBgmnSettings();
}

BgmnProjectWidget::~BgmnProjectWidget()
{
    // do not delete QObjects owned by this, will be done automatically
}

/*
 * virtual function, do not call from constructor
 */
void BgmnProjectWidget::initSettings()
{
    initBgmnSettings();
}

/*
 * non-virtual function, can be called from constructor
 */
void BgmnProjectWidget::initBgmnSettings()
{
    initGlobalSettings();

    QMapIterator<QString, ControlFileEdit*> it(editors);

    while (it.hasNext()) {
        it.next();
        ControlFileEdit *ed = it.value();
        bool oldState = ed->blockSignals(true);
        setHighlighting(ed);
        ed->blockSignals(oldState);
    }

    QString currentRepo = settings->value("bgmnProject/currentSelectedStrRepository", QString()).toString();
    setReferenceRepoList(currentRepo);
    setReferenceStructureFileList();
}

/*
 * Saving settings: Settings are saved instantly in the functions making
 * the change. This avoids conflicts between multiple open projects
 * as much as possible.
 */

/*
 * ***********************************************************************
 * Public process control API
 * ***********************************************************************
 */
void BgmnProjectWidget::runRefinement()
{
    bgmnRunProcess();
}

void BgmnProjectWidget::runPeakDetection()
{
    teilRunProcess();
}

bool BgmnProjectWidget::isRunning()
{
    if (bgmnHandler->isRunning()) return true;
    if (eflechHandler->isRunning()) return true;
    if (searchMatchWidget->isRunning()) return true;
    if (peakFitWidget->isRunning()) return true;
    return false;
}

void BgmnProjectWidget::abort()
{
    int n = 0;

    if (bgmnHandler->isRunning()) {
        bgmnAbortProcess();
        ++n;
    }

    if (eflechHandler->isRunning()) {
        eflechAbortProcess();
        ++n;
    }

    if (searchMatchWidget->isRunning()) {
        searchMatchWidget->abort();
        ++n;
    }

    if (peakFitWidget->isRunning()) {
        peakFitWidget->abortFit();
        ++n;
    }

    if (n) setStatus(global::RefinementStatus::ABORTED);
}

/*
 * ***********************************************************************
 * BGMN process communication
 * ***********************************************************************
 */
void BgmnProjectWidget::bgmnRunProcess()
{
    saveAll();
    currentIteration = 0;
    controlFile = getControlFileName();

    if (!fileEditor(controlFile, false)) {
        QMessageBox::information(this, tr("Create Control File"), tr("Please create a control file using \"Add Phase (+)\" first."));
        return;
    }

    BgmnSavParser sparser = getSavParser();

    // check if the VAL[n] file exists. If not, try to create it from the loaded scan file.
    // if this fails, return, because BGMN can't run without a VAL file
    int numValFiles = convertRawDataFile(sparser.valFile());

    if (numValFiles < 1) {
        // if conversion reported an error, we will abort
        if (numValFiles < 0) {
            // if it reported an error code of -1, we will also display a message before aborting
            QMessageBox::warning(this,
                                 tr("Problem creating XY file"),
                                 tr("Profex could not convert the scan file to a format supported by BGMN.\n"
                                    "Please convert it and run the refinement with the converted file."));
        }

        return;
    }

    if (settings->value("bgmnProject/readLamFile", false).toBool()) {
        setCustomWavelengt(sparser.getLambda());
    }

    QString diaFile(sparser.diagramFile());
    hasTubeTails = checkTubeTails(sparser);

    if (diaFile.isEmpty()) {
        qDebug() << QString("BgmnProjectWidget::runRefinement(): No graph file specified. Aborting.");
        return;
    }

    refinedGraphFile = getAbsoluteFilePath(diaFile);
    QFileInfo fi(refinedGraphFile);
    QString tempGraphFile = QDir::toNativeSeparators(QString("%1/%2").arg(settings->getTempLocation(), fi.fileName()));

    QFile parFile(getAbsoluteFilePath(sparser.outputFile()));
    QFile lstFile(getAbsoluteFilePath(sparser.listFile()));

    parFile.remove();
    lstFile.remove();
    deleteReport();
    clearGui();

    diaHandler->init(graphControl, tempGraphFile);
    setRexpDenom(sparser);

    // initialize the process handler
    if (!bgmnHandler->init()) {
        qDebug() << QString("BgmnProjectWidget::runRefinement(): BGMN executable not found: %1").arg(refinerExec);
        QMessageBox::information(this, tr("BGMN executable not found"), tr("The BGMN executable was not found.\n\nPlease check your BGMN configuration in the preferences."));
        return;
    }

    refOutput->appendPlainText(QString("Running process: %1 %2")
                               .arg(QDir::toNativeSeparators(settings->value("bgmnProject/bgmnExec", "").toString()),
                               QDir::toNativeSeparators(controlFile)));
    processTimer.start();

    QStringList args;
    args << controlFile;
    args << QString("DIAGRAMM=%1").arg(tempGraphFile);
    args << "SAVE=N";

    if (refStatus == global::RefinementStatus::SCHEDULED) {
        int npar = settings->value("config/batchParallelJobs", 1).toInt();

        if (npar > 1) {
            int nthr = sparser.numberOfThreads();
            if (nthr < 0) nthr = settings->value("bgmnProject/nThreads", 0).toInt();
            if (nthr == 0) nthr = QThread::idealThreadCount();
            nthr /= npar;
            args << QString("NTHREADS=%1").arg(nthr == 0 ? 1 : nthr);
        }
    }

    // start the process
    if (bgmnHandler->run(projectDir, args)) {
        diaHandler->startPolling(); // start listening to timer events
        setStatus(global::RefinementStatus::RUNNING);
        setCurrentIndex(0); // switch to the graph page
    } else {
        setStatus(global::RefinementStatus::FAILURE);
        QMessageBox::information(this, tr("Error starting BGMN"),
            QString(tr("Refinement could not be started.\nThe following command was aborted:\n\n%1 %2")).arg(refinerExec, controlFile));
    }
}

void BgmnProjectWidget::bgmnAbortProcess()
{
    bgmnHandler->abort();
}

void BgmnProjectWidget::bgmnPollOutput()
{
    QString output(bgmnHandler->readOutput().trimmed());
    int nIter = protocolParser.parse(output);

    if (nIter > 0) {
        double rwp = protocolParser.getRwp();
        double rex = protocolParser.getRexp();
        double rfs = protocolParser.getFirst();
        currentIteration += nIter;

        pSelectItem->setRefinementStats(currentIteration, -1, rwp, rex);
        convDisplay->setValues(rwp, rex, rfs);
        resultsTree->setRvalues(rwp, rex);
    }

    if (output == ".") {
        refOutput->moveCursor(QTextCursor::End);
        refOutput->insertPlainText(output);
        refOutput->moveCursor(QTextCursor::End);
    } else {
        static QRegularExpression rxConfig("unable to write config-file (?:bgmn|BGMN)\\.(?:cfg|CFG)");
        QString s = output.replace(rxConfig, QString()).trimmed();
        if (!s.isEmpty()) refOutput->appendPlainText(s + " ");
    }

    refOutput->ensureCursorVisible();

    if (!currentIndex()) {
        emit currentTabChanged(static_cast<ProjectWidget *>(this), currentIndex());
    }
}

void BgmnProjectWidget::bgmnComplete(QUuid _uid, global::RefinementStatus _status)
{
    Q_UNUSED(_uid);
    diaHandler->stopPolling();
    bgmnPollOutput();

    if (refOutput->toPlainText().contains("(E) ")) {
        setStatus(global::RefinementStatus::FAILURE);
    } else {
        setStatus(_status);
    }

    if (lparser.getFileName().isEmpty()) {
        parseProjectFiles();
    } else {
        lparser.reload();
    }

    BgmnFileIO::copyFile(diaHandler->monitoredFile(), refinedGraphFile);

    if (QFile::remove(diaHandler->monitoredFile())) {
        qDebug() << QString("BgmnProjectWidget::bgmnComplete(): Temporary file deleted: %1").arg(diaHandler->monitoredFile());
    } else {
        qDebug() << QString("BgmnProjectWidget::bgmnComplete(): Could not delete temporary file: %1").arg(diaHandler->monitoredFile());
    }

    double eps1 = 0.0;
    double eps2 = 0.0;
    double eps3 = 0.0;

    getEpsValues(eps1, eps2, eps3);

    graphControl->setAngularCorrections(eps1, eps2, eps3);
    graphControl->setViewStatus(refStatus);
    load(QString(), refinedGraphFile, "BGMN_DIA");

    if (searchMatchWidget)     searchMatchWidget->setEps2(eps2);
    if (peakIntegrationWidget) peakIntegrationWidget->setSampleDisplacements(eps1, eps2, eps3);

    if (!currentIndex()) {
        // we have to fire this signal, else the file name in the status bar will not be updated
        emit currentTabChanged(static_cast<ProjectWidget *>(this), currentIndex());
    }

    updateResults();

    if ((refStatus == global::RefinementStatus::COMPLETED)
        && (settings->value("bgmnProject/createReport", false).toBool())) {
        writeReport();
    }

    QString outputMsg;
    QString timeStr = elapsedTimeFormatted(processTimer.elapsed());

    switch (refStatus) {
    case global::RefinementStatus::COMPLETED:
        outputMsg = QString(tr("Refinement completed in %1 hh:mm:ss.ms").arg(timeStr));
        break;
    case global::RefinementStatus::ABORTED:
        outputMsg = QObject::tr("Refinement was aborted by the user");
        break;
    case global::RefinementStatus::FAILURE:
        outputMsg = QString(tr("Refinement exited with errors"));
        break;
    case global::RefinementStatus::CRASH:
        outputMsg = QString(tr("BGMN crashed after %1 hh:mm:ss.ms").arg(timeStr));
        break;
    case global::IDLE:
        break; // nothing to do, just added to eliminate compiler warnings
    case global::RUNNING:
        break; // nothing to do, just added to eliminate compiler warnings
    case global::MATCHING:
        break; // nothing to do, just added to eliminate compiler warnings
    case global::SCHEDULED:
        break; // nothing to do, just added to eliminate compiler warnings
    case global::FITSCHEDULED:
        break; // nothing to do, just added to eliminate compiler warnings
    case global::FITRUNNING:
        break; // nothing to do, just added to eliminate compiler warnings
    }

    refOutput->appendPlainText(outputMsg);
    refOutput->ensureCursorVisible();

    emit completed(this);
    qDebug() << QString("BgmnProjectWidget::complete(): %1").arg(outputMsg);
}

/*
 * ***********************************************************************
 * SearchMatchWidget process communication
 * ***********************************************************************
 */

void BgmnProjectWidget::runSearchMatch()
{
    setupSearchMatchWidget(false, false);
    searchMatchWidget->run();
}

void BgmnProjectWidget::setupSearchMatchWidget(bool instr, bool clearPinned)
{
    if (clearPinned) {
        searchMatchWidget->clearPinned();
    }

    bool hasProject;
    BgmnSavParser sparser = getSavParser(&hasProject);
    if (!hasProject) return;

    QStringList fn = sparser.getStruc(projectDir);
    QList<QPair<QString, QString> > phases;

    for (int i = 0; i < fn.size(); ++i) {
        BgmnStrParser strParser(fn.at(i));

        QPair<QString, QString> data;
        data.first = fn.at(i);
        data.second = strParser.getPhaseName();
        phases.append(data);
    }

    searchMatchWidget->setPinnedPhases(phases);

    if (instr) {
        searchMatchWidget->setInstrumentFile(sparser.deviceFile());
        searchMatchWidget->setEps2(refHdispValue->value());
        searchMatchWidget->setLambdaFile(sparser.getLambda());
        searchMatchWidget->setSynchrotron(sparser.getSynchrotron());
    }
}

void BgmnProjectWidget::searchMatchStatus(int i)
{
    if (i == 0) setStatus(global::RefinementStatus::ABORTED);
    if (i == 1) setStatus(global::RefinementStatus::MATCHING);
    if (i == 2) setStatus(global::RefinementStatus::COMPLETED);
}

/*
 * ***********************************************************************
 * TEIL process communication
 * ***********************************************************************
 */
void BgmnProjectWidget::teilRunProcess()
{
    refOutput->clear();

    if (!graphModel->count()) {
        qDebug() << QString("BgmnProjectWidget::teilRunProcess(): No scan loaded. Exiting.");
        return;
    }

    QString wmin;
    QString wmax;

    bool ok;
    BgmnSavParser sparser(getSavParser(&ok));

    if (ok) {
        double wmi = sparser.getWmin();
        double wma = sparser.getWmax();

        if (wmi > 0.0) wmin = QString("WMIN=%1").arg(wmi, 0, 'f', 4);
        if (wma > 0.0) wmax = QString("WMAX=%1").arg(wma, 0, 'f', 4);
    }

    QStringList errors;
    QString euid = eflechHandler->getStrId();

    QString sdev;
    QString lam;

    if (!eflechSourceDevFile(sdev, lam)) {
        qDebug() << QString("BgmnProjectWidget::teilRunProcess(): User aborted. Exiting.");
        return;
    }

    QFileInfo devFi(sdev);
    QStringList devFilter;
    devFilter << QString("%1.geq").arg(devFi.completeBaseName());
    devFilter << QString("%1.GEQ").arg(devFi.completeBaseName());

    QList<QFileInfo> tempFiles = devFi.absoluteDir().entryInfoList(devFilter);

    // from here on we need to remove the temporary files when aborting the process

    if (!eflechHandler->gatherTemporaryFiles(tempFiles, projectDir, euid)) {
        errors << QString("Copying files to temporary location %1 failed.").arg(projectDir);
    }

    if (!eflechActiveScanToTemp(euid)) {
        errors << QString("Writing active scan to temporary location %1 failed.").arg(projectDir);
    }

    const Scan *ascan = graphControl->firstActiveScan();
    QString aScanName = graphControl->scanName(ascan->uid());

    if (ascan) {
        if (!eflechHandler->init(aScanName, ascan->minAngle(), ascan->maxAngle())) {
            errors << QString("Error initializing eflechHandler");
        }
    } else {
        errors << QString("Could not get pointer to active scan.");
    }

    // bail out here if errors occured above
    if (errors.size()) {
        eflechHandler->removeTemporaryFiles(projectDir, euid);

        for (int i = 0; i < errors.size(); ++i) {
            qDebug() << QString("BgmnProjectWidget::teilRunProcess(): %1").arg(errors.at(i));
        }

        return;
    }

    QStringList args;
    args << euid;
    args << QString("VERZERR=%1.geq").arg(euid);
    args << QString("OUTPUTMASK=%1-$").arg(euid);
    args << QString("TITELMASK=%1-part-$").arg(euid);
    args << QString("TEST=%1").arg(eflechTestString());
    args << QString("VAL[1]=%1.xy").arg(euid);
    args << lam;
    args << QString("NTHREADS=%1").arg(QThread::idealThreadCount());
    args << QString("SAVE=Y");
    if (!wmin.isEmpty()) args << wmin;
    if (!wmax.isEmpty()) args << wmax;

    if (settings->value("eflech/overrideEpsilon", false).toBool()) {
        args << QString("EPSILON=%1").arg(settings->value("eflech/epsilon", 0.05).toDouble(), 0, 'f', 4);
    }

    disconnect(eflechHandler, SIGNAL(allComplete(QUuid)), nullptr, nullptr);
    disconnect(eflechHandler, SIGNAL(pollOutput()), nullptr, nullptr);
    connect(eflechHandler, SIGNAL(allComplete(QUuid)), this, SLOT(teilComplete()));
    connect(eflechHandler, SIGNAL(pollOutput()), this, SLOT(teilPollOutput()));

    setStatus(global::RefinementStatus::RUNNING);

    processTimer.start();
    eflechHandler->run(0, projectDir, args);
}

void BgmnProjectWidget::teilAbortProcess()
{
    eflechHandler->abort();
    setStatus(global::RefinementStatus::ABORTED);
}

void BgmnProjectWidget::teilPollOutput()
{
    refOutput->appendPlainText(eflechHandler->readOutput().trimmed());
}

void BgmnProjectWidget::teilComplete()
{
    disconnect(eflechHandler, SIGNAL(allComplete(QUuid)), nullptr, nullptr);
    disconnect(eflechHandler, SIGNAL(pollOutput()), nullptr, nullptr);
    connect(eflechHandler, SIGNAL(allComplete(QUuid)), this, SLOT(eflechComplete()));
    connect(eflechHandler, SIGNAL(pollOutput()), this, SLOT(eflechPollOutput()));
    eflechRunProcess();
}

/*
 * ***********************************************************************
 * EFLECH process communication
 * ***********************************************************************
 */
void BgmnProjectWidget::eflechRunProcess()
{
    eflechHandler->run(1, projectDir, QStringList(eflechHandler->getStrId()));
}

void BgmnProjectWidget::eflechAbortProcess()
{
    eflechHandler->abort();
    setStatus(global::RefinementStatus::ABORTED);
    eflechHandler->removeTemporaryFiles(projectDir, eflechHandler->getStrId());
}

void BgmnProjectWidget::eflechPollOutput()
{
    refOutput->appendPlainText(eflechHandler->readOutput().trimmed());
    eflechAppendHklData(eflechHandler, true);
}

void BgmnProjectWidget::eflechComplete()
{
    QString euid = eflechHandler->getStrId();
    disconnect(eflechHandler, SIGNAL(allComplete(QUuid)), nullptr, nullptr);
    disconnect(eflechHandler, SIGNAL(pollOutput()), nullptr, nullptr);

    eflechAppendHklData(eflechHandler, false);

    eflechHandler->removeTemporaryFiles(projectDir, euid);
    setStatus(global::RefinementStatus::COMPLETED);

    QString timeString = QString(tr("Peak detection completed in %1 hh:mm:ss.ms").arg(elapsedTimeFormatted(processTimer.elapsed())));
    refOutput->appendPlainText(timeString);
    qDebug() << QString("BgmnProjectWidget::eflechComplete(): %1").arg(timeString);

    emit completed(this);
}

bool BgmnProjectWidget::eflechSourceDevFile(QString &dev, QString &lam)
{
    bool ok;
    BgmnSavParser sparser(getSavParser(&ok));

    if (ok) {
        QFileInfo fiSDev(projectDir + QDir::separator() + sparser.deviceFile());
        if (fiSDev.suffix().toLower() != "geq") {
            fiSDev = QFileInfo(fiSDev.absoluteFilePath() + ".geq");
        }

        dev = fiSDev.absoluteFilePath();
        lam = QString("LAMBDA=%1").arg(sparser.getLambda());
        return true;
    }

    BgmnInstrumentSelectDialog *biDlg = new BgmnInstrumentSelectDialog(this);

    if (biDlg->exec() != QDialog::Accepted) {
        return false;
    }

    QFileInfo fiDev(biDlg->getDevFile());
    dev = fiDev.absoluteFilePath();

    if (biDlg->isLamSelected()) {
        QFileInfo fiLam(biDlg->getLamFile());
        lam = QString("LAMBDA=%1").arg(fiLam.completeBaseName());
    } else {
        lam = QString("SYNCHROTRON=%1").arg(biDlg->getSynchrotronValue(), 0, 'f', 6);
    }

    return true;
}

bool BgmnProjectWidget::eflechActiveScanToTemp(const QString &uid)
{
    if (!graphControl->hasData()) return false;

    Scan *scan = graphControl->firstActiveScan();
    if (!scan) return false;

    if (!scan->hasScanData()) {
        QString m(QString(tr("Scan has no scan data. Select a scan with measured data and run the process again.")));
        refOutput->appendPlainText(m);
        return false;
    }

    QMap<QString, QVariant> flags;
    flags["fieldSeparator"] = QVariant(QString(" "));
    flags["fixBgmnZero"]    = QVariant(true);

    QString outName(projectDir + "/" + uid + ".xy");

    ExportHandler exHandler;
    if (exHandler.save("ASCII_XY", outName, *scan, flags) > 0) return true;

    return false;
}

QString BgmnProjectWidget::eflechTestString()
{
    QString s;
    if (settings->value("eflech/testN", true).toBool())  s += "N";
    if (settings->value("eflech/testD", true).toBool())  s += "D";
    if (settings->value("eflech/testP", false).toBool()) s += "+";
    if (settings->value("eflech/testM", false).toBool()) s += "-";
    if (settings->value("eflech/test2", true).toBool())  s += "2";
    if (settings->value("eflech/test3", true).toBool())  s += "3";
    if (settings->value("eflech/test4", true).toBool())  s += "4";
    if (settings->value("eflech/testU", true).toBool())  s += "U";
    return s;
}

void BgmnProjectWidget::eflechAppendHklData(EflechHandler *ehandler, bool isTemporary)
{
    bool ok = false;
    bool isLastTemporaryPeakData = graphControl->count() > 0
                                   ? graphControl->last()->auxInfo(tempAuxString, ok).toBool()
                                   : false;

    if (!(isLastTemporaryPeakData && ok)) {
        graphControl->appendHklScan(Scan(QString("%1 peak data").arg(ehandler->getSourceName()), QColor(), 1));
    }

    double l = graphView->getWaveLength();
    if (qFuzzyIsNull(l)) l = settings->defaultWavelength();

    Scan *hklScan = graphControl->getLast();
    if (!hklScan) return;

    hklScan->setWaveLength(l);
    hklScan->setAuxInfo(tempAuxString, QVariant(isTemporary));

    QVector<QVector<double> > peakData = ehandler->parseEflechParFiles(projectDir, ehandler->getStrId());
    hklScan->pDataHkl().clear();

    for (int i = 0; i < qMin(peakData[0].size(), peakData[1].size()); ++i) {
        hklScan->pDataHkl().append(Hkl(peakData[0][i], QString(), 0.0, QString(), QColor(), peakData[1][i], 1.0, 1, peakData[2][i], peakData[3][i]));
    }

    graphControl->normalizeHklScan(hklScan);
    graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);
    peakListWidget->updateData();
}

void BgmnProjectWidget::clearGui()
{
    refOutput->clear();
    chemOutput->clearAll();
    convDisplay->clear();
    peakListWidget->clearAll();
    resultsTree->resetStats();
    resultsTree->clearGlobal();
    resultsTree->clearLocal();
    protocolParser.reset();
    // searchMatchWidget->clearResults();
}

/*
 * returns the absolute file path of a file with a relative path (or no path) relative
 * to projectDir
 */
QString BgmnProjectWidget::getAbsoluteFilePath(const QString &file)
{
    QFileInfo fi(file);

    if (fi.absoluteDir() == QDir(projectDir)) {
        return fi.absoluteFilePath();
    }

    QFileInfo absFileName(projectDir + "/" + fi.fileName());
    return absFileName.absoluteFilePath();
}

void BgmnProjectWidget::setRexpDenom(const BgmnSavParser &sparser)
{
    double wmin = sparser.getWmin();
    double wmax = sparser.getWmax();

    QString valFile = QDir::fromNativeSeparators(projectDir + "/" + sparser.valFile().constFirst());
    QPair<int, double> rexpDenom;

    QVector<Scan> scans;
    ImportHandler ihandler;
    QString uid = ihandler.uidByFileName(valFile);
    ihandler.load(valFile, uid, scans, true);

    if (scans.isEmpty()) return;

    bool tt = false;
    QFileInfo fGeq(projectDir + "/" + sparser.deviceFile());

    // VERZERR= can be entered without file extension
    if (!fGeq.exists()) {
        if (fGeq.suffix().toLower() != "geq") {
            if (QFile::exists(fGeq.absoluteFilePath() + ".geq")) {
                fGeq = QFileInfo(fGeq.absoluteFilePath() + ".geq");
            } else if (QFile::exists(fGeq.absoluteFilePath() + ".GEQ")) {
                fGeq = QFileInfo(fGeq.absoluteFilePath() + ".GEQ");
            }
        }
    }

    if (fGeq.exists()) {
        BgmnGeqParser gparser(fGeq.absoluteFilePath());
        tt = gparser.hasTubeTails();
    }

    rexpDenom = scans.first().getRexpDenom(wmin, wmax, tt);

    convDisplay->setRexpDenom(rexpDenom.first, rexpDenom.second);
    protocolParser.setRexpDenom(rexpDenom.first, rexpDenom.second);
}

QVector<double> BgmnProjectWidget::getScanWeighing()
{
    if (graphControl->count() == 0) return QVector<double>();
    return graphControl->getFirst()->getScanWeighing(hasTubeTails);
}

bool BgmnProjectWidget::checkTubeTails(const BgmnSavParser &sparser)
{
    QFileInfo fGeq(projectDir + "/" + sparser.deviceFile());

    // VERZERR= can be entered without file extension
    if (!fGeq.exists()) {
        if (fGeq.suffix().toLower() != "geq") {
            if (QFile::exists(fGeq.absoluteFilePath() + ".geq")) {
                fGeq = QFileInfo(fGeq.absoluteFilePath() + ".geq");
            } else if (QFile::exists(fGeq.absoluteFilePath() + ".GEQ")) {
                fGeq = QFileInfo(fGeq.absoluteFilePath() + ".GEQ");
            }
        }
    }

    if (fGeq.exists()) {
        BgmnGeqParser gparser(fGeq.absoluteFilePath());
        return gparser.hasTubeTails();
    }

    return false;
}

QString BgmnProjectWidget::elapsedTimeFormatted(int mstotal)
{
    QTime t(0, 0, 0, 0);
    t = t.addMSecs(mstotal);
    return t.toString("hh:mm:ss.zzz");
}

/*
 * uses the add/remove phase dialog to append or remove structures
 * to/from the SAV file
 */
void BgmnProjectWidget::addRemovePhase()
{
    if (!arPhaseDialog) {
        arPhaseDialog = new BgmnAddRemoveDialog(this);
        arPhaseDialog->initSettings();
    }

    showAddRemovePhases();
}

void BgmnProjectWidget::showAddRemovePhases()
{
    QString controlFileContent;
    QPlainTextEdit *e = fileEditor(controlFile, false);
    if (e) controlFileContent = e->toPlainText();

    // if the search-match widget has pinned phases, we preselect them.
    // if not, we check if a reference structure is active, and preselect it.
    // else, nothing is preselected.
    QStringList selectedAddFiles = searchMatchWidget->getPinnedPhases();
    bool createFromSearchMatchResults = selectedAddFiles.size() > 0 ? true : false;

    if (!createFromSearchMatchResults) {
        if (comboBoxRefStructures->currentIndex() > 0) {
            selectedAddFiles = QStringList(comboBoxRefStructures->currentData(Qt::UserRole).toString());
        }
    }

    BgmnSavParser sparser(controlFileContent, controlFile);
    QStringList strucFilesPaths = sparser.getStruc(projectDir);
    QString devFile = sparser.deviceFile();
    arPhaseDialog->updateDialog(devFile, selectedAddFiles, strucFilesPaths);

    if (devFile.isEmpty()) {
        if (createFromSearchMatchResults) {
            QFileInfo fi(searchMatchWidget->getDeviceFile());
            arPhaseDialog->setDeviceFile(true, fi.fileName());
        }
    } else {
        arPhaseDialog->setDeviceFile(false, devFile);
    }

    if (sampleID.isEmpty()) {
        sampleID = sparser.sampleId();
    }

    if (arPhaseDialog->exec() == QDialog::Rejected) {
        qDebug() << QString("BgmnProjectWidget::addRemovePhase(): Dialog cancelled. Exiting.");
    } else {
        applyAddRemovePhases();
    }
}

void BgmnProjectWidget::applyAddRemovePhases()
{
    resetRefStructure();
    if (!arPhaseDialog) return;

    // get or create the control file editor
    QPlainTextEdit *e = fileEditor(controlFile, true);
    BgmnSavParser sparser(e->toPlainText(), controlFile);

    // check if there is anything to do
    bool cf = arPhaseDialog->createDefaultControlFile();
    bool af = arPhaseDialog->getCheckedAddFiles().size() ? true : false;
    bool rf = arPhaseDialog->getCheckedDeleteFiles().size() ? true : false;

    if ((!cf) && (!af) && (!rf)) {
        qDebug() << QString("BgmnProjectWidget::addRemovePhase(): Dialog accepted, but nothing to do. Exiting.");
        return;
    }

    // adding files can take a long time when working on network shared drives
    qApp->setOverrideCursor(Qt::WaitCursor);

    // store the previous val files for later
    QStringList vfiles = sparser.valFile();

    // if requested, create a new control file from the template file
    if (cf) {
        if (!createControlFileFromTemplate(e, sparser, arPhaseDialog->deviceFile(), arPhaseDialog->overwriteFiles())) {
            qApp->restoreOverrideCursor();
            return;
        }
    }

    QStringList addStr = arPhaseDialog->getCheckedAddFiles();     // new str files at the source location
    QStringList remStr = arPhaseDialog->getCheckedDeleteFiles();  // str files to remove at project location

    qDebug() << QString("BgmnProjectWidget::addRemovePhase(): New structures to add: %1").arg(addStr.join(", "));
    qDebug() << QString("BgmnProjectWidget::addRemovePhase(): Structures to remove: %1").arg(remStr.join(", "));

    removeStrucFiles(sparser, remStr, arPhaseDialog->deleteStrFiles());
    QStringList newStr = insertStrucFiles(sparser, addStr, arPhaseDialog->overwriteFiles()); // new str files at project location

    // write back the VAL[]= files from the original text if they were not empty
    if (!vfiles.isEmpty()) {
        sparser.setValFile(vfiles);
    }

    // get the complete sav file text from the sav parser
    // and write it to the text editor
    e->setPlainText(adjustControlOutputFiles(sparser.getContent(), false));

    // update the search match widget with project information
    setupSearchMatchWidget(cf, true);

    if (settings->value("bgmnProject/openAddedStrFiles", true).toBool()) {
        openTextFileEditors(newStr);
    }

    qApp->restoreOverrideCursor();
}

bool BgmnProjectWidget::createControlFileFromTemplate(QPlainTextEdit *e, BgmnSavParser &sparser, const QString &devFile, bool overwrite)
{
    if (!e) {
        QMessageBox::warning(this, tr("Create Control File"), tr("Could not create a control file."));
        qDebug() << QString("BgmnProjectWidget::createControlFileFromTemplate(): Could not create a control file. Exiting.");
        return false;
    }

    bool ok = true;
    QFileInfo fiCtrlFile(controlFile);
    QFileInfo df(devFile);
    QFileInfo dfname(BgmnFileIO::gatherDevFiles(df.absoluteFilePath(), fiCtrlFile.absolutePath(), overwrite, &ok));
    if (!ok) {
        qApp->restoreOverrideCursor();
        QMessageBox::warning(this, tr("Error copying the device files"), tr("An error occurred when copying the device files.\n"
                                                                            "Check the log file for more information."));
        qDebug() << QString("BgmnProjectWidget::createControlFileFromTemplate(): Could not create a control file. Exiting.");
        return false;
    }

    sparser.setContent(defaultControlFile(df.absoluteFilePath()));
    sparser.setSampleId(sampleID);
    sparser.setDeviceFile(dfname.fileName());
    sparser.setNumberOfThreads(settings->value("bgmnProject/nThreads", 0).toInt());

    QString untFile(sparser.untFile());
    if (!untFile.isEmpty()) {
        QFileInfo uFiSource(df.absolutePath() + "/" + untFile);
        QFileInfo uFiDest(projectDir + "/" + uFiSource.fileName());

        if (uFiSource.exists() && !uFiDest.exists()) {
            BgmnFileIO::copyFile(uFiSource.absoluteFilePath(), uFiDest.absoluteFilePath());
            sparser.setUntFile(uFiDest.fileName());
        }
    }

    QString untcFile(sparser.untcFile());
    if (!untcFile.isEmpty()) {
        QFileInfo uFiSource(df.absolutePath() + "/" + untcFile);
        QFileInfo uFiDest(projectDir + "/" + uFiSource.fileName());

        if (uFiSource.exists() && !uFiDest.exists()) {
            BgmnFileIO::copyFile(uFiSource.absoluteFilePath(), uFiDest.absoluteFilePath());
            sparser.setUntcFile(uFiDest.fileName());
        }
    }

    return true;
}

QStringList BgmnProjectWidget::removeStrucFiles(BgmnSavParser &sparser, const QStringList &removeStrFiles, bool deleteFiles)
{
    QStringList l;
    bool mgeGoals = settings->value("bgmnProject/manageGoals", true).toBool();

    for (int i = 0; i < removeStrFiles.size(); ++i) {
        sparser.removeStructureFile(removeStrFiles.at(i), projectDir, mgeGoals);

        QString sourceFile(QString("%1/%2").arg(projectDir, removeStrFiles.at(i)));
        l.append(sourceFile);

        // if requested, delete the file
        // but check if there is another entry in the sav file. Don't delete it
        // if there is, because the refinement would not work anymore
        if (deleteFiles && (!sparser.hasPhase(removeStrFiles.at(i)))) {
            if (QFile::exists(sourceFile)) {
                QFile::remove(sourceFile);
            }
        }
    }

    return l;
}

QStringList BgmnProjectWidget::insertStrucFiles(BgmnSavParser &sparser, const QStringList &insertStrFiles, bool overwriteFiles)
{
    bool mgeGoals = settings->value("bgmnProject/manageGoals", true).toBool();
    QFileInfo fiCtrlFile(controlFile);
    bool ok;
    QStringList strFiles = BgmnFileIO::gatherStrFiles(insertStrFiles, fiCtrlFile.absolutePath(), overwriteFiles, 3, &ok);

    if (ok) {
        sparser.addStructureFiles(strFiles, mgeGoals, false);
    } else {
        QMessageBox::warning(this, tr("Error adding structure files"),
                             tr("An error occurred when adding the structure files.\n"
                                "Please check the log file for more information"));
    }
    return strFiles;
}

void BgmnProjectWidget::load(const QString &txt, const QString &grph, const QString &uid)
{
    if (!QFile::exists(txt) && !QFile::exists(grph)) return;

    int tabToRaise = 0;
    bool autoDetectGraph = true;

    if (grph.isEmpty()) {
        tabToRaise = count();
    } else {
        loadGraph(grph, uid);
        autoDetectGraph = false;
    }

    if (txt.isEmpty()) {
        autoLoadTextFiles(grph);
    } else {
        loadTextFile(txt); // load manually because the auto-load function ignores the caller file
        autoLoadTextFiles(txt);
    }

    if (autoDetectGraph) {
        QString autoGraph = autoDetermineGraphFile();

        if (QFile::exists(autoGraph)) {
            loadGraph(autoGraph, QString());
        } else {
            QFileInfo fi(txt);
            projectBasename = fi.completeBaseName();
            projectDir = QDir::fromNativeSeparators(fi.absolutePath());
            controlFile = QString();
            sampleID = QString();

            pSelectItem->setProjectName(projectBasename);
            pSelectItem->setProjectFileName(fi.fileName());
            pSelectItem->setProjectFilePath(projectDir);
            pSelectItem->setProjectSampleId(sampleID);

        }
    }

    int pf = parseProjectFiles();

    double eps1 = 0.0;
    double eps2 = 0.0;
    double eps3 = 0.0;

    if (pf > 1) getSampleDisplacements(eps1, eps2, eps3);
    sampleDisplacementsChanged(eps1, eps2, eps3);
    matchStrongestPeak();
    setupSearchMatchWidget(true, true);

    setCurrentIndex(tabToRaise);
    updateResults();
}

void BgmnProjectWidget::applyTextBlock(const QString &s)
{
    ControlFileEdit *e = getCurrentEditor();
    if (!e) return;
    e->textCursor().insertText(s);

    QFileInfo fi(e->getFileName());
    if (fi.suffix().toLower() == "sav") {
        BgmnSavParser sparser(e->toPlainText(), controlFile);
        sparser.writeGoals();
        e->setPlainText(sparser.getContent());
    }
}

void BgmnProjectWidget::matchStrongestPeak()
{
    if (!settings->value("config/autoSelRefPhase", false).toBool()) return;
    if (!graphControl->hasData()) return;

    double maxIntPos = graphControl->first()->angleOfMaxIntensity();
    referencePosClicked(0.0, 0.0, global::Functions::twoThetaToD(maxIntPos, graphControl->getWaveLength(settings->defaultWavelength())));
}

/*
 * returns the phase quantities as a ; separated string
 */
QStringList BgmnProjectWidget::getQuantitiesCsv()
{
    if (!lparser.hasData()) {
        parseProjectFiles();
    }

    sampleID = getSampleID();

    QStringList goals = lparser.getGoalsCsv(getGlobalIncludeList(), sampleID);
    goals.append(lparser.getRValues(sampleID));
    return goals;
}

/*
 * returns refined parameters as a ; separated string
 */
QStringList BgmnProjectWidget::getResultsCsv()
{
    if (!lparser.hasData()) {
        parseProjectFiles();
    }

    sampleID = getSampleID();

    QString localParams(settings->value("bgmnProject/reportedLocalParameters", global::defaultBgmnLocalGoals).toString());

    return lparser.getLocalParametersCsv(localParams.split("\n"), sampleID);
}

QStringList BgmnProjectWidget::getChemistry(bool header)
{
    sampleID = getSampleID();

    BgmnSavParser sparser(controlFile);
    ChemistryMode mode = static_cast<ChemistryMode>(settings->value("chemistry/mode", 0).toInt());
    ChemTableData chemData;
    chemData.setData(sparser, lparser, true);

    return chemData.getCsvList(mode, header, sampleID);
}

void BgmnProjectWidget::updateResults()
{
    if (!lparser.hasData()) {
        parseProjectFiles();
    }

    int lastIter = currentIteration > 0 ? currentIteration : -1;
    pSelectItem->setRefinementStats(lastIter, -1, lparser.getRwp(), lparser.getRexp());

    bool ok;
    BgmnSavParser savParser = getSavParser(&ok);

    if (ok) {
        resultsTree->updateGoals(savParser);
        chemOutput->setData(savParser, lparser);
    }
    peakListWidget->updateData();
}

/*
 * check if all val files exist. If not, try to generate them from the scans currently loaded in "graph"
 */
int BgmnProjectWidget::convertRawDataFile(const QStringList &val)
{
    QMap<QString, QVariant> flags;
    flags["fieldSeparator"] = QVariant(QString(" "));
    flags["fixBgmnZero"]    = QVariant(true);

    int n = 0;
    QStringList errors;

    // compile a list of graphHeap indices with scans containing scan data (skip pure hkl scans)
    QList<int> scanIndices;
    for (int i = 0; i < graphControl->count(); ++i) {
        if (graphControl->at(i)->scanTypes().testFlag(Scan::MEASURED) && graphControl->at(i)->hasScanData()) {
            scanIndices.append(i);
        }
    }

    QStringList valFiles = val;

    if (valFiles.isEmpty()) {
        BgmnSavParser sparser = getSavParser();
        valFiles = sparser.valFile();
    }

    for (int i = 0; i < valFiles.size(); ++i) {
        QFileInfo fiOut(QDir::fromNativeSeparators(projectDir + "/" + valFiles.at(i)));

        if (fiOut.exists()) {
            qDebug() << QString("BgmnProjectWidget::generateValFiles(): Scan file %1 exists. No conversion needed.").arg(fiOut.fileName());
            ++n;
            continue;
        }

        if (i < scanIndices.size()) {
            Scan *scan = graphControl->getScan(scanIndices.at(i));

            ExportHandler exHandler;
            n += exHandler.save("ASCII_XY", fiOut.absoluteFilePath(), *scan, flags) > 0 ? 1 : 0;
        } else {
            errors << fiOut.absoluteFilePath();
        }
    }

    if (errors.size()) {
        qDebug() << QString("BgmnProjectWidget::convertRawDataFile(): The following files could not be converted: %1").arg(errors.join(", "));
    }

    if (n == 0) return -1;
    return n;
}

QString BgmnProjectWidget::adjustControlOutputFiles(const QString &cont, bool clearFirst)
{
    BgmnSavParser sparser(cont, controlFile);

    if (clearFirst) {
        sparser.setDiagramFile(QString());
        sparser.setOutputFile(QString());
        sparser.setListFile(QString());
        sparser.setValFile(QStringList());
    }

    // hard code all output filenames to "projectBasename".xxx, except for the
    // val files (these will be handled later)
    sparser.setDiagramFile(QString("%1.dia").arg(projectBasename));
    sparser.setOutputFile(QString("%1.par").arg(projectBasename));
    sparser.setListFile(QString("%1.lst").arg(projectBasename));
    sparser.setStrucOutBaseName(projectBasename);
    sparser.setSampleId(sampleID);

    // count the number of scans that actually contain scan data (skip pure hkl scans)
    int numberOfDataScans = 0;

    for (int i = 0; i < graphControl->count(); ++i) {
        bool isXYData   = graphControl->at(i)->scanTypes().testFlag(Scan::XY);
        bool isMeasured = graphControl->at(i)->scanTypes().testFlag(Scan::MEASURED);
        if (isXYData && isMeasured) ++numberOfDataScans;
    }

    if (sparser.valFile().isEmpty()) {
        QStringList skipConvert = settings->value("bgmnProject/skipConvertToXy", "val xye").toStringList();
        skipConvert << nativeGraphFormats;
        sparser.generateValFiles(loadedGraphFile, skipConvert, numberOfDataScans);
    }

    return sparser.getContent();
}

/*
 * makes sure every STRUC line has a matching STRUCOUT line
 */
void BgmnProjectWidget::prepareForSequenceOrigin()
{
    QPlainTextEdit *e = fileEditor(controlFile, false);
    if (!e) return;

    BgmnSavParser sparser(e->toPlainText(), controlFile);
    sparser.generateStrucOutFiles(projectBasename);
    e->setPlainText(sparser.getContent());
}

bool BgmnProjectWidget::setSequenceInputFiles(const QString &cont, const QString &prev)
{
    QString adjContent = adjustControlOutputFiles(cont, true);
    BgmnSavParser cParser(adjContent, controlFile);
    BgmnSavParser pParser(prev, "previous");

    if (cParser.getStruc().size() == pParser.getStrucOut().size()) {
        cParser.overrideStrucFileNames(pParser.getStrucOut());

        // count the number of scans that actually contain scan data (skip pure hkl scans)
        int numberOfDataScans = 0;
        for (int i = 0; i < graphControl->count(); ++i) {
            numberOfDataScans += graphControl->at(i)->hasScanData() ? 1 : 0;
        }

        if (cParser.valFile().isEmpty()) {
            QStringList skipConvert = settings->value("bgmnProject/skipConvertToXy", "val xye").toStringList();
            skipConvert << nativeGraphFormats;
            cParser.generateValFiles(loadedGraphFile, skipConvert, numberOfDataScans);
        }

        setControlFileContent(cParser.getContent());
        return true;
    }

    qDebug() << QString("BgmnProjectWidget::setSequenceInputFiles(): Number of STRUC and STRUCOUT entries do not match. Exiting.");
    setControlFileContent(adjContent);
    return false;
}

void BgmnProjectWidget::setHighlighting(ControlFileEdit *ed)
{
    BgmnHighlighter *bhl = new BgmnHighlighter(settings->getSyntaxHighlightingMode(), this);
    bhl->setDocument(ed->document());
    highlighters.insert(ed, bhl);
}

QStringList BgmnProjectWidget::contextHelp()
{
    QStringList list;
    ControlFileEdit *e = getCurrentEditor();

    if (!e) {
        qDebug() << QString("BgmnProjectWidget::contextHelp(): Current widget is not a QTextEditor.");
        return list;
    }

    QString word = e->blockUnderSelection();
    QString keyword;
    QString dummy;

    if (!e->splitParameterBlock(word, dummy, keyword, dummy, dummy, dummy)) {
        qDebug() << QString("BgmnProjectWidget::contextHelp(): Could not grep the keyword");
        return list;
    }

    list << editors.key(e) << keyword;
    qDebug() << QString("BgmnProjectWidget::contextHelp(): Requesting context help for %1").arg(keyword);
    return list;
}

void BgmnProjectWidget::displayReferenceStructure(const QString &s)
{
    if (s.isEmpty()) {
        graphView->setReferenceReflections(Scan());
        return;
    }

    QVector<QVector<double> > xy = getReferenceXyPattern(s);

    Scan rScan;
    rScan.pDataHkl() = getReferenceHklLines(s);
    rScan.pDataAngle()     = xy.at(0);
    rScan.pDataIntensity() = xy.at(1);
    rScan.setWaveLength(graphView->getWaveLength());
    rScan.setColor(settings->value("graph/hklLineColor", "#009900").toString());

    if (xy.at(0).size() &&  xy.at(1).size()) {
        rScan.setTypes(Scan::XY | Scan::SYNTHETIC);
    } else {
        rScan.setTypes(Scan::HKL | Scan::SYNTHETIC);
    }

    if (rScan.pDataHkl().size() || rScan.pDataAngle().size()) {
        graphView->setReferenceReflections(rScan);
    } else {
        refOutput->appendPlainText(QString("\nNo hkl data found for file %1.").arg(s));
        refOutput->appendPlainText(QString("Please index your reference structure database."));
    }
}

QVector<Hkl> BgmnProjectWidget::getReferenceHklLines(const QString &s)
{
    return refStrManager->getReferenceLines(s);
}

QVector<QVector<double> > BgmnProjectWidget::getReferenceXyPattern(const QString &s)
{
    return refStrManager->getReferencePattern(s);
}

Scan BgmnProjectWidget::getReferenceScan(const QString &s)
{
    return refStrManager->getReferenceScan(s);
}

/*
 * A double click on the graph searches the phase with the strongest peak
 * closest to the double click position and selects it from the
 * reference structure combo box
 */
void BgmnProjectWidget::referencePosClicked(double x, double y, double d)
{
    Q_UNUSED(x);
    Q_UNUSED(y);

    bool fav = refStrFavoritesButton->isChecked();
    QMultiMap<double, PeakFile> nearestMatches;
    refStrManager->referencePosClicked(d,
                                       nearestMatches,
                                       comboBoxRefStrucRepos->currentData(Qt::UserRole).toString(),
                                       fav,
                                       settings->value("bgmnProject/doubleClickNearestPeaks", 1).toInt());

    if (!nearestMatches.size()) return;

    QMultiMapIterator<double, PeakFile> it(nearestMatches);

    refOutput->appendPlainText(QString("Clicked at d=%1").arg(d));
    if (fav) refOutput->appendPlainText(QString("Searching in favorites only!"));
    refOutput->appendPlainText(QString("Best matching phases:"));

    int n = 0;
    QList<PeakFile> refFiles;
    int nmax = settings->value("bgmnProject/doubleClickNearestMatches", 5).toInt();

    while (it.hasNext()) {
        it.next();

        refOutput->appendPlainText(QString("%1: %2 (%3)").arg(n+1).arg(it.value().fileInfo.fileName()).arg(it.key(), 0, 'f', 4));
        refFiles.append(it.value());

        ++n;
        if (n >= nmax) break;
    }

    searchMatchWidget->addReferencePhases(refFiles);
    setReferenceStructure(nearestMatches.values().constFirst().fileInfo.completeBaseName());
}

void BgmnProjectWidget::setReferenceStructure(QString s)
{
    int idx = comboBoxRefStructures->findText(s);
    comboBoxRefStructures->setCurrentIndex(idx > 0 ? idx : 0);
}

void BgmnProjectWidget::toggleRefStrFavorites(bool b)
{
    // receives a signal if this or any other project
    // toggled the ref str favorites button
    bool oldState = refStrFavoritesButton->blockSignals(true);
    refStrFavoritesButton->setChecked(b);
    refStrFavoritesButton->blockSignals(oldState);
    comboBoxRefStrucRepos->setEnabled(!b);

    setReferenceStructureFileList();
}

void BgmnProjectWidget::setReferenceRepoList(const QString &currentRepo)
{
    bool oldState = comboBoxRefStrucRepos->blockSignals(true);

    comboBoxRefStrucRepos->clear();
    comboBoxRefStrucRepos->addItem(QString(), QVariant());
    comboBoxRefStrucRepos->setItemData(0, QVariant(QString("All repositories")), Qt::ToolTipRole);
    QStringList dirs = refStrManager->getAllSourceDirs(false);

    for (int i = 0; i < dirs.size(); ++i) {
        QFileInfo fi(dirs.at(i));
        comboBoxRefStrucRepos->addItem(fi.fileName(), QVariant(fi.absoluteFilePath()));
        comboBoxRefStrucRepos->setItemData(i+1, QVariant(fi.absoluteFilePath()), Qt::ToolTipRole);
    }

    int n = comboBoxRefStrucRepos->findData(QVariant(currentRepo), Qt::UserRole);
    if (n > 0) comboBoxRefStrucRepos->setCurrentIndex(n);

    comboBoxRefStrucRepos->blockSignals(oldState);
}

/*
 * Populates the combo box with reference structures.
 */
void BgmnProjectWidget::setReferenceStructureFileList()
{
    resetRefStructure();
    bool oldState = comboBoxRefStructures->blockSignals(true);

    QString repo = comboBoxRefStrucRepos->currentData(Qt::UserRole).toString();
    settings->setValue("bgmnProject/currentSelectedStrRepository", repo);

    comboBoxRefStructures->clear();
    comboBoxRefStructures->addItem(QString(), QVariant());

    QStringList files;
    if (isRefStrFavorites()) files = refStrManager->getIndexedFavoriteFileNames();
    else                     files = refStrManager->getIndexedFileNames(repo);

    // we'll temporarily store the file infos in a QMap, because it will be sorted alphabetically
    // by file name
    QMap<QString, QFileInfo> map;

    for (int i = 0; i < files.size(); ++i) {
        QFileInfo fi(files.at(i));
        map.insert(fi.fileName().toLower(), fi);
    }

    QMap<QString, QFileInfo>::const_iterator it = map.constBegin();
    int n = 1;

    while (it != map.constEnd()) {
        QVariant absFpath(it.value().absoluteFilePath());
        comboBoxRefStructures->addItem(it.value().completeBaseName(), absFpath);
        comboBoxRefStructures->setItemData(n, absFpath, Qt::ToolTipRole);
        ++it;
        ++n;
    }

    comboBoxRefStructures->setCurrentIndex(0);
    comboBoxRefStructures->blockSignals(oldState);
    searchMatchWidget->initData();
}

/*
 * compiles and returns a stringlist with global parameters to be
 * shown in the output tables
 */
QStringList BgmnProjectWidget::getGlobalIncludeList()
{
    QString globalGoals = settings->value("bgmnProject/reportedGlobalGoals", global::defaultBgmnGlobalGoals).toString();
    if (globalGoals.isEmpty()) globalGoals = global::defaultBgmnGlobalGoals;
    return globalGoals.isEmpty() ? QStringList(".*") : globalGoals.split("\n");
}

/*
 * compiles and returns a stringlist with local parameters to be
 * shown in the output tables
 */
QStringList BgmnProjectWidget::getLocalIncludeList()
{
    QString localGoals = settings->value("bgmnProject/reportedLocalParameters", global::defaultBgmnLocalGoals).toString();
    if (localGoals.isEmpty()) localGoals = global::defaultBgmnLocalGoals;
    return localGoals.isEmpty() ? global::defaultBgmnLocalGoals.split("\n") : localGoals.split("\n");
}

/*
 * opens all structure files referenced in the control file in a text editor
 */
QStringList BgmnProjectWidget::openProjectStrFiles()
{
    QStringList strFiles = getProjectStrFiles();
    openTextFileEditors(strFiles);
    return strFiles;
}

QStringList BgmnProjectWidget::getProjectStrFiles()
{
    BgmnSavParser sparser = getSavParser();
    QStringList rel = sparser.getStruc();
    QStringList abs;

    for (int i = 0; i < rel.size(); ++i) {
        abs.append(QString(QDir::fromNativeSeparators(projectDir + "/" + rel.at(i))));
    }

    return abs;
}

BgmnSavParser BgmnProjectWidget::getSavParser(bool *ok)
{
    BgmnSavParser sparser(QString(), controlFile);
    QPlainTextEdit *e = fileEditor(controlFile, false);

    if (e) {
        // get text from text editor, so it's up to date even if the sav file has not been
        // saved after modification. It is also more efficient than loading the file from
        // disk
        sparser.setContent(e->toPlainText());
        if (ok) *ok = true;
    } else {
        // load the sav file from the disk
        bool b;
        if (QFile::exists(controlFile)) {
            b = sparser.loadFile(controlFile);
        } else {
            b = false;
        }
        if (ok) *ok = b;
    }

    return sparser;
}

QString BgmnProjectWidget::getSampleID()
{
    bool ok;
    BgmnSavParser sparser = getSavParser(&ok);

    if (ok) return sparser.sampleId();
    return sampleID;
}

/*
 * closes all structure files referenced in the control file in a text editor
 */
void BgmnProjectWidget::closeProjectStrFiles()
{
    QList<int> tabList;

    QMap<QString, ControlFileEdit*>::const_iterator it = editors.constBegin();

    // loop over all tabs and check if it is a str file. If yes, store the tab index
    // in a list
    while (it != editors.constEnd()) {
        QFileInfo fi(it.key());

        if (fi.suffix().toLower() == "str") {
            tabList.append(editorsTabIndex(it.value()));
        }

        ++it;
    }

    // close all tabs stored in the index list.
    // Note: it is important to start at the end of a sorted list, because removing
    // elements changes the index of the following elements.
    std::sort(tabList.begin(), tabList.end());

    while (tabList.size()) {
        closeTab(tabList.takeLast());
    }
}

void BgmnProjectWidget::closeProjectStrFiles(const QStringList &files)
{
    for (int i = 0; i < files.size(); ++i) {
        closeTab(files.at(i));
    }
}

void BgmnProjectWidget::compileSumFormula(const QMap<QString, CrystalStructure> &phases)
{
    QMap<QString, CrystalStructure>::const_iterator it = phases.constBegin();

    while (it != phases.constEnd()) {
        CrystalStructure struc = it.value();
        ++it;
    }
}

int BgmnProjectWidget::applyPreset(const QString &s, bool overwrite)
{
    if (s.isEmpty()) {
        qDebug() << QString("BgmnProjectWidget::applyPreset(): No preset name specified, exiting.");
        return -1;
    } else {
        qDebug() << QString("BgmnProjectWidget::applyPreset(): Applying preset %1 to project %2").arg(s, projectBasename);
    }

    ControlFileEdit *e = fileEditor(controlFile, true);
    BgmnPresetHandlerGui pHandler(this, e);
    int r = pHandler.applyPreset(s, overwrite);

    resetRefStructure();
    return r;
}

bool BgmnProjectWidget::hasControlFile()
{
    QPlainTextEdit *e = fileEditor(controlFile, false);

    if (!e) return false;
    return true;
}

/*
 * creates and returns the content (as a QString) of a default
 * control file, either read from a *.tpl file for the provided
 * *.geq file, or by showing the GenTemplateDialog
 */
QString BgmnProjectWidget::defaultControlFile(const QString &geq)
{
    QString savFileContent;
    QFileInfo bgmnExec(settings->value("bgmnProject/bgmnExec", "").toString());

    QFileInfo df(geq);
    QFileInfo tf(df.absolutePath() + "/" + df.completeBaseName() + ".tpl");

    if (!tf.exists()) {
        qDebug() << QString("BgmnProjectWidget::defaultControlFile(): No template file found for device %1.tpl").arg(df.completeBaseName());
        qDebug() << QString("BgmnProjectWidget::defaultControlFile(): Showing GenTemplateDialog");

        GenTemplateDialog *gtdlg = new GenTemplateDialog(this);
        gtdlg->setBgmnDir(bgmnExec.absolutePath());

        if (gtdlg->exec()) {
            savFileContent = gtdlg->getString();
        }

        delete gtdlg;
    } else {
        savFileContent = BgmnFileIO::readTextFile(tf.absoluteFilePath());
    }

    return savFileContent;
}

/*
 * creates a refinement template from the current refinement
 */
void BgmnProjectWidget::createPreset()
{
    QString presetFile = getPresetName();

    if (presetFile.isEmpty()) return;

    BgmnPresetHandlerGui pHandler(this, fileEditor(controlFile, false));
    pHandler.savePreset(presetFile);
}

QString BgmnProjectWidget::getPresetName()
{
    QString pFile;
    QString pName;

    BgmnBackendConfig bkgConfig;
    QString pRepo = bkgConfig.getUserPresetRepo();

    if (pRepo.isEmpty()) {
        QMessageBox::information(this, tr("No Preset Repositore configures"), tr("Please specify a directory to store presets in\nEdit -> Preferences -> BGMN -> Respositories"));
        return QString();
    }

    // endless loop, until an empty or valid file name is selected
    while (true) {
        pName = QInputDialog::getText(this, tr("Preset Name"), tr("Preset Name"));

        if (pName.isEmpty()) {
            // input dialog was aborted or closed without entering a name
            return QString();
        }

        pFile = QString(pRepo + "/" + pName + "/" + pName + ".pfp");

        if (QFile::exists(pFile)) {
            QMessageBox::information(this, tr("Preset exists"),
                                     QString(tr("A preset named %1 already exists.\n"
                                                "Please enter a different name.").arg(pName)));
            // stay in the loop
        } else {
            // a valid name was provided
            return pFile;
        }
    }

    return QString();
}

void BgmnProjectWidget::getBaselinePreset(QDomDocument &doc)
{
    QList<int> bls = graphControl->baseLines();
    if (bls.size() == 0) return;

    QList<global::BaseLineSnip> baseLines;

    for (int i = 0; i < bls.size(); ++i) {
        const Scan *blSc = graphControl->at(bls.at(i));

        bool ok;
        QList<QVariant> blPars = blSc->auxInfo("baseLineParameters", ok).toList();
        if (blPars.size() >= 2) baseLines.append(global::BaseLineSnip(blPars.at(0).toInt(), blPars.at(1).toInt()));
    }

    QList<QDomElement> baseLineList;

    for (int i = 0; i < baseLines.size(); ++i) {
        QDomElement baselineEl = doc.createElement("baseLine");
        baselineEl.setAttribute("Mode", "SNIP");
        baselineEl.setAttribute("m", baseLines.at(i).m);
        baselineEl.setAttribute("w", baseLines.at(i).mode);
        baseLineList.append(baselineEl);
    }

    for (int i = 0; i < baseLineList.size(); ++i) {
        doc.documentElement().appendChild(baseLineList.at(i));
    }
}

void BgmnProjectWidget::applyPresetBaseLine(const QDomElement &element)
{
    if (element.attribute("Mode") != "SNIP") return;

    int m = element.attribute("m", "60").toInt();
    int w = element.attribute("w", "1").toInt();

    graphControl->appendScan(ScanOps::baseLineSNIP(*(graphModel->first()), m, w));
}

void BgmnProjectWidget::applyXmlSettings(const QString &pfpFile)
{
    BgmnPresetHandlerGui pHandler(this, fileEditor(controlFile, false));
    pHandler.restoreBackup(pfpFile);
}

/*
 * creates a zip archive with all files referenced in the control file
 * or loaded in the current project. overwrites existing zip files.
 * Abort before calling this function if overwriting is not desired
 */
void BgmnProjectWidget::createZipArchive(const QString &zf)
{
    if (QFile::exists(zf)) QFile::remove(zf);
    qDebug() << QString("BgmnProjectWidget::createZipArchive(): Creating project backup");

    // get a list of all files used in the project
    QFileInfoList flist;
    if (!controlFile.isEmpty()) flist.append(getAllProjectFilesList());
    flist.append(getAllLoadedScanFilesList());

    // write the project xml file with status of the other widgets
    QFileInfo fiCtrl(controlFile);
    QFileInfo fiXml(fiCtrl.absolutePath() + QDir::separator() + fiCtrl.completeBaseName() + ".pfp");;

    BgmnPresetHandlerGui pHandler(this, fileEditor(controlFile, false));
    QDomDocument doc = pHandler.createBackupXmlFile(fiXml.absoluteFilePath());
    BgmnFileIO::writeTextFile(fiXml.absoluteFilePath(), doc.toString());
    flist.append(fiXml);

    // extract file names
    QStringList clist;

    for (int i = 0; i < flist.size(); ++i) {
        clist.append(flist.at(i).absoluteFilePath());
        qDebug() << QString("    Adding %1").arg(clist.last());
    }

    // create zip archive
    if (JlCompress::compressFiles(zf, clist)) {
        refOutput->appendPlainText(QString("Backup saved to %1").arg(QDir::toNativeSeparators(zf)));
        qDebug() << QString("    Archive created in %1").arg(QDir::toNativeSeparators(zf));
    } else {
        refOutput->appendPlainText(QString("Could not write backup file %1").arg(zf));
        qDebug() << QString("BgmnProjectWidget::createZipArchive(): Problem creating %1").arg(QDir::toNativeSeparators(zf));
    }

    // QFile::remove(fiXml.absoluteFilePath());
}

/*
 * returns a list with all files belonging to a project.
 */
QFileInfoList BgmnProjectWidget::getAllProjectFilesList() const
{
    QFileInfoList flist;
    BgmnSavParser sparser(controlFile);

    // add all files referenced in the sav file
    checkFileExists(flist, controlFile);
    checkFileExists(flist, sparser.outputFile(),  projectDir);
    checkFileExists(flist, sparser.listFile(),    projectDir);
    checkFileExists(flist, sparser.diagramFile(), projectDir);
    checkFileExists(flist, sparser.untFile(),     projectDir);
    checkFileExists(flist, sparser.untcFile(),    projectDir);

    checkFilesExist(flist, sparser.getStruc(),   projectDir);
    checkFilesExist(flist, sparser.getStrucOut(), projectDir);
    checkFilesExist(flist, sparser.getSimpleStrucOut(), projectDir);

    checkFilesExist(flist, sparser.getFcfOut(), projectDir);
    checkFilesExist(flist, sparser.getResOut(), projectDir);
    checkFilesExist(flist, sparser.getPdbOut(), projectDir);

    QStringList vfiles = sparser.valFile();
    checkFilesExist(flist, vfiles, projectDir);

    ImportHandler ihandler;
    QStringList rdExtensions = ihandler.extensions();

    // add raw data files with the same name as the project basename (e.g. x.raw belonging to x.xy)
    for (int i = 0; i < rdExtensions.size(); ++i) {
        checkFileExists(flist, projectBasename + "." + rdExtensions.at(i).toLower(), projectDir);
        checkFileExists(flist, projectBasename + "." + rdExtensions.at(i).toUpper(), projectDir);
    }

    // also add raw data files with the same name as the val files (e.g. x.raw belonging to x.xy)
    // note: in case of multiple val files, the file names do not match the projectBasename. Here
    // we use the val file names to locate additional raw data files.
    for (int i = 0; i < vfiles.size(); ++i) {
        QFileInfo fiVf(vfiles.at(i));

        for (int j = 0; j < rdExtensions.size(); ++j) {
            checkFileExists(flist, fiVf.completeBaseName() + "." + rdExtensions.at(j).toLower(), projectDir);
            checkFileExists(flist, fiVf.completeBaseName() + "." + rdExtensions.at(j).toUpper(), projectDir);
        }
    }

    // add all device files
    QFileInfo fiDf(sparser.deviceFile());
    checkFileExists(flist, fiDf.fileName(), projectDir);

    // in case the suffix .geq is not part of the device file, add it and check again
    if (fiDf.suffix().toLower() != "geq") {
        checkFileExists(flist, fiDf.completeBaseName() + ".geq", projectDir);
        checkFileExists(flist, fiDf.completeBaseName() + ".GEQ", projectDir);
    }

    // add the other device files
    checkFileExists(flist, fiDf.completeBaseName() + ".sav", projectDir);
    checkFileExists(flist, fiDf.completeBaseName() + ".ger", projectDir);
    checkFileExists(flist, fiDf.completeBaseName() + ".tpl", projectDir);
    checkFileExists(flist, fiDf.completeBaseName() + ".pxsml", projectDir);
    checkFileExists(flist, fiDf.completeBaseName() + ".SAV", projectDir);
    checkFileExists(flist, fiDf.completeBaseName() + ".GER", projectDir);
    checkFileExists(flist, fiDf.completeBaseName() + ".TPL", projectDir);
    checkFileExists(flist, fiDf.completeBaseName() + ".PXSML", projectDir);

    // add tube tails file if used
    bool ok;
    BgmnInstrumentSavParser isParser(projectDir + QDir::separator() + fiDf.completeBaseName() + ".sav", &ok);

    if (ok) {
        checkFileExists(flist, isParser.getTubeTailsFile(), projectDir);
    } else if (isParser.loadFile(projectDir + QDir::separator() + fiDf.completeBaseName() + ".SAV")) {
        checkFileExists(flist, isParser.getTubeTailsFile(), projectDir);
    }

    // add lam file (is a bit complicated, because lam file names can be lower/upper case or mixed)
    QString lamFile(sparser.getLambda());
    QStringList lamFilters;
    lamFilters << "*.lam" << "*.LAM" << "*.ano" << "*.ANO" << "*.mdr" << "*.MDR";
    QFileInfo fiBgmn(settings->value("bgmnProject/bgmnExec", QString()).toString());
    QFileInfoList fiAllLam = fiBgmn.absoluteDir().entryInfoList(lamFilters, QDir::Files);

    for (int i = 0; i < fiAllLam.size(); ++i) {
        if (fiAllLam.at(i).completeBaseName().toLower() == lamFile.toLower()) {
            flist << fiAllLam.at(i);
        }
    }

    return flist;
}

QFileInfoList BgmnProjectWidget::getAllLoadedScanFilesList() const
{
    QFileInfoList flist;

    // in case of inserted scans, check for the files of those, too
    for (int i = 0; i < graphControl->count(); ++i) {
        checkFileExists(flist, graphControl->at(i)->sourceFileName(), QString());
    }

    return flist;
}

bool BgmnProjectWidget::checkFilesExist(QFileInfoList &fil, const QStringList &flist, const QString &dir) const
{
    int n = 0;

    for (int i = 0; i < flist.size(); ++i) {
        if (checkFileExists(fil, flist.at(i), dir)) ++n;
    }

    return n > 0;
}

bool BgmnProjectWidget::checkFileExists(QFileInfoList &fil, const QString &file, const QString &dir) const
{
    QString path = dir.isEmpty() ? QString() : dir + QDir::separator();

    if (QFileInfo::exists(path + file)) { // is faster than QFileInfo(path+fil).exists() according to docu
        QFileInfo fi(path + file);

        if (fi.isFile() && !fil.contains(fi)) {
            fil.append(fi);
            return true;
        }
    }

    return false;
}

/*
 * returns a list with all input files belonging to a project. This is very much dependent on
 * the type of project (BGMN, FP). Input files are structure, device, and background files.
 * Not included are: DIA, PAR, VAL, LIST, STRUCOUT, SimpleSTRUCOUT, FCFOUT, RESOUT, and PDBOUT
 */
QFileInfoList BgmnProjectWidget::getSharedInputProjectFilesList() const
{
    // get references to all files
    BgmnSavParser sparser(controlFile);

    QFileInfoList flist;
    flist.append(QFileInfo(projectDir + "/" + sparser.untFile()));

    // add structures
    QStringList strfiles = sparser.getStruc();
    for (int i = 0; i < strfiles.size(); ++i) {
        flist.append(QFileInfo(projectDir + "/" + strfiles.at(i)));
    }

    // add device files
    QFileInfo fiDf(projectDir + "/" + sparser.deviceFile());
    flist.append(fiDf);
    flist.append(QFileInfo(projectDir + "/" + fiDf.completeBaseName() + ".ger"));
    flist.append(QFileInfo(projectDir + "/" + fiDf.completeBaseName() + ".geq"));
    flist.append(QFileInfo(projectDir + "/" + fiDf.completeBaseName() + ".sav"));
    flist.append(QFileInfo(projectDir + "/" + fiDf.completeBaseName() + ".tpl"));

    // test which files exist, skip the ones that don't exist
    QFileInfoList clist;

    for (int i = 0; i < flist.size(); ++i) {
        if (flist.at(i).isFile() && flist.at(i).exists()) {
            clist.append(flist.at(i));
        }
    }

    return clist;
}

/*
 * copes the input files shared with other projects to the current directory.
 */
bool BgmnProjectWidget::gatherSharedInputFiles(const QString &sourceDir, const QFileInfoList &sourceFiles, bool forceOverwrite)
{
    QDir sdir(sourceDir);
    QFileInfoList destFiles;

    // create a list with destination file names
    for (int i = 0; i < sourceFiles.size(); ++i) {
        QString sourceRelPath = sdir.relativeFilePath(sourceFiles.at(i).absoluteFilePath());
        QFileInfo destFi(projectDir + "/" + sourceRelPath);

        // check if source and dest are identical
        if (destFi.absolutePath() != sourceFiles.at(i).absolutePath()) {
            destFiles.append(destFi);
        }
    }

    // check if any of the dest files exists, but ignore *.geq, *.ger, *.sav
    QStringList ex;
    for (int i = 0; i < destFiles.size(); ++i) {
        if (destFiles.at(i).suffix().toLower() == "geq") continue;
        if (destFiles.at(i).suffix().toLower() == "ger") continue;
        if (destFiles.at(i).suffix().toLower() == "sav") continue;

        if (destFiles.at(i).exists()) ex.append(destFiles.at(i).absoluteFilePath());
    }

    // ask for permission to overwrite files
    if (ex.size()) {
        if (!forceOverwrite) {
            if (QMessageBox::question(this,
                                      tr("Overwriting files"),
                                      QString(tr("The following files exists and may be used by other projects.\nDo you want to overwrite them?\n\n%1")).arg(ex.join("\n")))
                    == QMessageBox::No) return false;
        }
    }

    // delete existing files
    for (int i = 0; i < ex.size(); ++i) {
        QFile::remove(ex.at(i));
    }

    // copy the files
    for (int i = 0; i < qMin(sourceFiles.size(), destFiles.size()); ++i) {
        qDebug() << QString("Copying %1 to %2").arg(sourceFiles.at(i).absoluteFilePath(), destFiles.at(i).absoluteFilePath());
        QFile fi(sourceFiles.at(i).absoluteFilePath());
        fi.copy(destFiles.at(i).absoluteFilePath());
    }

    return true;
}

/*
 * returns the EPS values (sample height displacement) from the list file.
 */
void BgmnProjectWidget::getSampleDisplacements(double &e1, double &e2, double &e3)
{
    if (!lparser.hasData()) {
        parseProjectFiles();
    }

    getEpsValues(e1, e2, e3);
}

/*
 * Saves all structures found in the list file as cif files.
 * File names are assigned automatically:
 *
 * projectbasename-phasename.cif
 */
void BgmnProjectWidget::saveCifFiles(bool complete, const QMap<QString, QVariant> &auxDataGlobal)
{
    if (!lparser.hasData()) {
        parseProjectFiles();
    }

    QStringList phases = lparser.getPhaseNames();
    QMap<QString, BgmnStrParser> phaseMap = getStrParserMapByPhase();

    if (phases.isEmpty()) {
        qDebug() << QString("BgmnProjectWidget::saveCifFiles(): No phases read from file %1, exiting").arg(lparser.getFileName());
        return;
    }

    QFileInfo bname(controlFile);
    QStringList fnames;

    QMap<QString, QVariant> auxData(auxDataGlobal);

    if (graphControl->hasData()) {
        QList<QVariant> wls;
        wls.append(graphControl->getWaveLength(settings->defaultWavelength()));
        auxData["_diffrn_radiation_wavelength"] = QVariant(wls);
    }

    for (int i = 0; i < phases.size(); ++i) {
        CrystalStructure structure = lparser.getCrystalStructure(phases.at(i));
        structure.unitCell().setSettingNumber(phaseMap.value(phases.at(i)).getSetting());
        QString fn(bname.absolutePath() + "/" + bname.completeBaseName() + "-" + phases.at(i) + ".cif");

        qDebug() << QString("BgmnProjectWidget::saveCifFiles(): Saving phase %1 to file %2").arg(phases.at(i), fn);

        QFile f(fn);

        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << QString("BgmnProjectWidget::saveCifFiles(): Could not open file for writing %1, continuing").arg(fn);
            continue;
        }

        QTextStream out(&f);
        out << structure.toCif(complete, controlFile, auxData);
        f.close();

        fnames.append(fn);
    }

    refOutput->appendPlainText(QString("Exported CIF files:\n%1\n").arg(fnames.join("\n")));
}

QList<phaseData> BgmnProjectWidget::getCifData(const QMap<QString, QVariant> &auxDataGlobal)
{
    QList<phaseData> list;

    if (!lparser.hasData()) {
        parseProjectFiles();
    }

    QStringList phases = lparser.getPhaseNames();

    if (phases.isEmpty()) {
        qDebug() << QString("BgmnProjectWidget::getCifData(): No phases read from file %1, exiting").arg(lparser.getFileName());
        return list;
    }

    QMap<QString, QVariant> auxData(auxDataGlobal);

    if (graphControl->hasData()) {
        QList<QVariant> wls;
        wls.append(graphControl->getWaveLength(settings->defaultWavelength()));
        auxData["_diffrn_radiation_wavelength"] = QVariant(wls);
    }

    for (int i = 0; i < phases.size(); ++i) {
        CrystalStructure structure = lparser.getCrystalStructure(phases.at(i));

        phaseData d;
        d.project = projectBasename;
        d.phase = phases.at(i);
        d.data = QVariant(structure.toCif(false, controlFile, auxData));
        list.append(d);
    }

    return list;
}

/*
 * Saves all structures found in the list file as cif files.
 * File names are assigned automatically:
 *
 * projectbasename-phasename.cell
 */
void BgmnProjectWidget::saveCastepCellFiles()
{
    bool ok;
    BgmnSavParser sparser(controlFile, &ok);

    if (!ok) {
        qDebug() << QString("BgmnProjectWidget::saveCastepCellFiles(): Could not read control file %1, exiting").arg(controlFile);
        return;
    }

    QStringList resFileNames = sparser.getResOut();
    QStringList resFiles;

    // store the res files that really exist in resFiles
    for (int i = 0; i < resFileNames.size(); ++i) {
        if (QFileInfo::exists(projectDir + "/" + resFileNames.at(i))) {
            resFiles.append(projectDir + "/" + resFileNames.at(i));
        }
    }

    if (!resFiles.size()) {
        qDebug() << QString("BgmnProjectWidget::saveCastepCellFiles(): None of the referenced res files exists");
        refOutput->appendPlainText(QString(tr("CELL file export: No RES files available")));
        refOutput->appendPlainText(QString(tr("  CELL files are converted from RES files")));
        refOutput->appendPlainText(QString(tr("  Please add \"RESOUT[n]=mystructure.res\" to your control file")));
        refOutput->appendPlainText(QString(tr("  and re-run the refinement.")));
        return;
    }

    QFileInfo bname(controlFile);
    refOutput->appendPlainText(QString(tr("Exported CELL files:")));

    for (int r = 0; r < resFiles.size(); ++r) {
        BgmnResParser rparser(resFiles.at(r));
        CrystalStructure structure = rparser.structure();

        QString fn(bname.absolutePath() + "/" + bname.baseName() + "-" + structure.name() + ".cell");

        qDebug() << QString("BgmnProjectWidget::saveCastepCellFiles(): Saving phase %1 to file %2")
                    .arg(structure.name(), fn);

        QFile f(fn);

        if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
            qDebug() << QString("BgmnProjectWidget::saveCastepCellFiles(): Could not open file for reading %1, continuing").arg(fn);
            continue;
        }

        QTextStream out(&f);
        out << structure.toCastepCell();
        f.close();

        refOutput->appendPlainText(fn);
    }
}

/*
 * returns 1 if the SAV file was found
 * returns 2 if also the LST file was found
 * else returns 0
 */
int BgmnProjectWidget::parseProjectFiles()
{
    int n = 0;
    // if the control file doesn't exist yet, just do nothing
    if (!QFile::exists(controlFile)) return n;

    ++n;

    QFileInfo fiControl(controlFile);
    BgmnSavParser sparser(fiControl.absoluteFilePath());

    QFileInfo fiLst(fiControl.absolutePath() + "/" + sparser.listFile());
    QString lam  = sparser.getLambda();
    double synch = sparser.getSynchrotron();
    double neutr = sparser.getNeutron();

    if (!lam.isEmpty()) {
        graphView->setWavelengthMode(Scan::WavelengthMode::CHARACTERISTIC);
    } else if (synch > 0.0) {
        graphView->setWavelengthMode(Scan::WavelengthMode::SYNCHROTRON);
    } else if (neutr > 0.0) {
        graphView->setWavelengthMode(Scan::WavelengthMode::NEUTRON);
    } else {
        graphView->setWavelengthMode(Scan::WavelengthMode::UNKNOWN);
    }

    if (fiLst.exists()) {
        lparser.load(fiLst.absoluteFilePath());
        ++n;
    }

    if (settings->value("bgmnProject/readLamFile", false).toBool()) {
        setCustomWavelengt(sparser.getLambda());
    }

    return n;
}

void BgmnProjectWidget::setInternalStandard()
{
    // get the text editor for the controlFile
    QPlainTextEdit *e = fileEditor(controlFile, false);

    if (!e) {
        qDebug() << QString("BgmnProjectWidget::setInternalStandard(): No control file available");
        return;
    }

    // create a savparser to access and modify the control file content
    BgmnSavParser sparser(e->toPlainText(), controlFile);

    // populate the dialog with phases read from the savparser
    QStringList strucFiles = sparser.getStruc();
    QStringList phaseNames;

    for (int i = 0; i < strucFiles.size(); ++i) {
        QString absStrucFile = QString("%1/%2").arg(projectDir, strucFiles.at(i));
        BgmnStrParser strParser(absStrucFile);
        phaseNames.append(strParser.getQuantGoal());
    }

    if (phaseNames.isEmpty()) {
        QMessageBox::information(this, tr("No phases found"), tr("Please add at least one phase to the project."));
        return;
    }

    InternalStandardDialog *idlg = new InternalStandardDialog(this);
    idlg->setPhases(phaseNames);
    idlg->setCurrentPhase(sparser.internalStandard());
    idlg->setCurrentQuantity(sparser.internalStandardQuantity());

    if (idlg->exec() == QDialog::Accepted) {
        sparser.setInternalStandard(idlg->getPhase(), idlg->getQuantity());
        e->setPlainText(sparser.getContent());
    }

    delete idlg;
}

void BgmnProjectWidget::unsetInternalStandard()
{
    // get the text editor for the controlFile
    QPlainTextEdit *e = fileEditor(controlFile, false);

    if (!e) {
        qDebug() << QString("BgmnProjectWidget::unsetInternalStandard(): No control file available");
        return;
    }

    // create a savparser to access and modify the control file content
    BgmnSavParser sparser(e->toPlainText(), controlFile);
    sparser.unsetInternalStandard();
    e->setPlainText(sparser.getContent());
}

void BgmnProjectWidget::generateReport()
{
    QString fileName = writeReport();
    refOutput->appendPlainText("Report saved to " + fileName);
    QDesktopServices::openUrl(QUrl("File:" + fileName, QUrl::TolerantMode));
}

QString BgmnProjectWidget::writeReport()
{
    QString fileName(projectDir + "/" + projectBasename + ".html");

    BgmnHtmlReportGenerator reportGen(lparser);
    reportGen.setDocumentStructure(settings->value("bgmnProject/report/documentStructure", BgmnFileIO::readTextFile(":/resources/report-structure.xml")).toString());

    BgmnSavParser sparser(controlFile);

    QMap<QString, QVariant> projectData;
    projectData.insert("rawFileName", loadedGraphFile);
    projectData.insert("workingDir", workingDir());
    projectData.insert("sampleId", QVariant(sampleID));
    projectData.insert("geqFile", sparser.deviceFile());
    projectData.insert("wavelength", graphView->getWaveLength());
    projectData.insert("wavelengthFile", sparser.getLambda());
    projectData.insert("globalGoalsIncludes", QVariant(getGlobalIncludeList()));
    projectData.insert("localGoalsIncludes", QVariant(getLocalIncludeList()));
    projectData.insert("quantGoals100percent", QVariant(settings->value("bgmnProject/quantGoals100percent", QVariant(false))));
    projectData.insert("parFileName", getAbsoluteFilePath(sparser.outputFile()));

    ChemTableData chemData;
    chemData.setData(sparser, lparser, true);
    projectData.insert("chemistryData", chemData.getHtmlTable(reportGen.chemistryTableMode()));

    projectData.insert("diffpatternSvg", graphView->getSvg(reportGen.patternAspectRatio()));

    QFont reportFont;
    reportFont.setFamily("Helvetica");
    reportFont.setPointSize(10);

    reportGen.setProjectData(projectData);

    reportGen.generateDocument();
    BgmnFileIO::writeTextFile(fileName, reportGen.getContents());
    return fileName;
}

void BgmnProjectWidget::deleteReport()
{
    QString fileName(projectDir + "/" + projectBasename + ".html");
    if (QFile::exists(fileName)) QFile::remove(fileName);
}

void BgmnProjectWidget::resetMarginColor()
{
    if (!bgmnHandler->isRunning()) {
        setStatus(global::RefinementStatus::IDLE);
    }
}

void BgmnProjectWidget::setCustomWavelengt(const QString &lambda)
{
    if (!settings->value("bgmnProject/readLamFile", false).toBool()) return;

    // we must determine the lam file name case insensitively (often the case in the control
    // file doesn't match the actual file name's case). That's why the following routine
    // is a bit complicated.
    QFileInfo fibgmn(settings->value("bgmnProject/bgmnExec", QString()).toString());
    QFileInfo filambda(fibgmn.absolutePath() + "/" + lambda + ".lam");
    QDir ldir(filambda.absolutePath());

    QStringList filters;
    filters << "*.lam" << "*.LAM";

    QStringList files = ldir.entryList(filters, QDir::Files | QDir::Readable).filter(filambda.fileName(), Qt::CaseInsensitive);

    if (files.size()) {
        QString foundLamFile(filambda.absolutePath() + "/" + files.first());
        qDebug() << QString("BgmnProjectWidget::setCustomWavelengt(): Parsing lam file %1").arg(foundLamFile);
        BgmnLamParser lamparser(foundLamFile);
        // graph->setCustomWaveLengths(lamparser.getWavelengths());
        graphControl->setCustomWaveLength(lamparser.getWavelengths());
    } else {
        qDebug() << QString("BgmnProjectWidget::setCustomWavelengt(): Lam file %1 does not exist").arg(filambda.absoluteFilePath());
    }
}

QString BgmnProjectWidget::getResultsFile()
{
    if (!lparser.hasData()) {
        parseProjectFiles();
    }

    return lparser.getFileName();
}

/*
 * returns the file name (and absolute path) of the device SAV file.
 *
 * - If the device SAV file is found in the project dir, its full path and
 *   name will be returned.
 *
 * - If it is not found in the project dir (only GEQ and GER must be present),
 *   it will search for the file in the device file repository. If found, the
 *   return value will depend on the state of the flag "copy":
 *
 *   - copy = true:  The SAV file will be copied from the device repo to the
 *                   project dir, and the destination file path and name
 *                   will be returned.
 *   - copy = false: The SAV file will not be copied, the source file path
 *                   and name will be returned.
 *
 * - If the SAV file is neither found in the project dir, nor in the device repo,
 *   an empty string is returned.
 */
QString BgmnProjectWidget::getInstrumentConfigFile(bool copy)
{
    BgmnSavParser sparser(controlFile);
    QFileInfo fiGeq(sparser.deviceFile());

    QStringList filters;
    filters << fiGeq.completeBaseName() + ".sav" << fiGeq.completeBaseName() + ".SAV";

    QDir pdir(projectDir);
    QFileInfoList savList = pdir.entryInfoList(filters, QDir::Files);

    if (savList.size()) {
        return savList.first().absoluteFilePath();
    }

    QStringList devRepos = settings->value("bgmnProject/deviceDatabase", QStringList()).toStringList();

    for (int i = 0; i < devRepos.size(); ++i)  {
        QDir repo(devRepos.at(i));
        savList = repo.entryInfoList(filters, QDir::Files);

        if (!savList.size()) continue;

        QFileInfo fiSrc = savList.first();
        QFileInfo fiDst = QFileInfo(projectDir + "/" + savList.first().fileName());

        if (copy) {
            if (!BgmnFileIO::copyFile(fiSrc.absoluteFilePath(), fiDst.absoluteFilePath())) {
                return QString();
            }

            return fiDst.absoluteFilePath();
        }
        return fiSrc.absoluteFilePath();
    }

    return QString();
}

void BgmnProjectWidget::editProjectStrFiles()
{
    if (!strFileEditDlg) {
        strFileEditDlg = new StrFileBatchEditDialog(this);
        connect(strFileEditDlg, SIGNAL(accepted()), this, SLOT(applyEditProjectStrFiles()));
        connect(strFileEditDlg, SIGNAL(sigApply()), this, SLOT(applyEditProjectStrFiles()));
    }

    strFileEditDlg->reset();


    QFileInfo cfi(currentFileName());
    QString absCurFileName;

    if (cfi.suffix().toLower() == "str") {
        absCurFileName = cfi.absoluteFilePath();
    }

    strFileEditDlg->setCurrentFile(absCurFileName);
    strFileEditDlg->setOpenFileList(getOpenFileNames("str"));
    strFileEditDlg->setStrFileList(getProjectStrFiles());

    strFileEditDlg->show();
}

void BgmnProjectWidget::applyEditProjectStrFiles()
{
    // read values from dialog
    QMap<QString, QVariant> values = strFileEditDlg->getParameters();
    QStringList modFiles = values.value("files").toStringList();

    // save all open str files
    for (int i = 0; i < modFiles.size(); ++i) {
        saveEditorFile(modFiles.at(i));
    }

    // apply changes to files on disk
    BgmnStrParser strParser;

    for (int i = 0; i < modFiles.size(); ++i) {
        strParser.loadFile(modFiles.at(i));

        QMapIterator<QString, QVariant> it(values);

        while (it.hasNext()) {
            it.next();
            strParser.setRefinementState(it.key(), it.value().toInt());
        }

        strParser.writeToFile(modFiles.at(i));
        // reload all open str files
        loadTextFile(modFiles.at(i));
    }
}

/*
 * sets the current scan as reference scan in the currently shown reference structure
 */
void BgmnProjectWidget::setReferenceXY()
{
    if (!graphModel->hasData()) return;

    int n = scanList->currentIndex();

    if (n < 0) {
        qDebug() << QString("ProjectWidget::setReferenceXY(): No selected scan found, index was %1").arg(n);
        return;
    }

    Scan *scan = graphModel->getScan(n);
    QString rFile = comboBoxRefStructures->currentData(Qt::UserRole).toString();
    if (rFile.isEmpty()) {
        refOutput->appendPlainText("Select a reference structure first.");
        return;
    }

    if (QMessageBox::question(this,
                              tr("Overwrite reference pattern"),
                              tr("Do you really want to overwrite the current reference structure's pattern?"))
            == QMessageBox::Yes) {
        refStrManager->setXyData(rFile, *scan);
    }
}

void BgmnProjectWidget::addAmorphousPeak()
{
    QPlainTextEdit *e = fileEditor(controlFile, false);

    if (!e) return;

    bool ok;
    double tt = settings->value("bgmnProject/amorphPeak2t", 30.0).toDouble();

    tt = QInputDialog::getDouble(this, tr("Amorphous Peak"), QString(tr("Center position (°2%1)").arg(global::theta)), tt, 1.0, 180.0, 2, &ok);
    if (!ok) return;

    settings->setValue("bgmnProject/amorphPeak2t", tt);

    QString baseName("amorphous");
    QFileInfo fi(workingDir() + "/" + baseName + ".str");

    int n = 1;

    while (fi.exists()) {
        baseName = QString("amorphous%1").arg(n);
        fi = QFileInfo(QString("%1/%2.str").arg(workingDir(), baseName));
        ++n;
    }

    double wl = graphView->getWaveLength(); // Angstrom
    double d = global::Functions::twoThetaToD(tt, 0.1 * wl);
    double dmax = global::Functions::twoThetaToD(qMax(graphView->getTmin(),  tt - 2.0), 0.1 * wl);
    double dmin = global::Functions::twoThetaToD(qMin(graphView->getTmax(),  tt + 2.0), 0.1 * wl);;

    QString str = QString("PHASE=%1\n").arg(baseName);
    str += QString("SpacegroupNo=195 GeneralCondition=eq(1,sqr(h)+sqr(k)+sqr(l))\n");
    str += QString("PARAM=B1=0.1_0^1 B2=0 PARAM=A=%1_%2^%3").arg(d, 0, 'f', 6).arg(dmin, 0, 'f', 6).arg(dmax, 0, 'f', 6);

    BgmnFileIO::writeTextFile(fi.absoluteFilePath(), str);
    BgmnSavParser sparser = getSavParser();
    sparser.addAmorphous(fi.fileName());
    e->setPlainText(sparser.getContent());
}

void BgmnProjectWidget::createSinglePeakRefinement()
{
    int total = 0;
    int selected = 0;

    peakListWidget->hasPeakSelection(total, selected);

    if (selected == 0) {
        if (total == 0) QMessageBox::information(this, tr("Single Peak Refinement"), tr("No peaks found. Run peak detection first."));
        else            QMessageBox::information(this, tr("Single Peak Refinement"), tr("No peaks selected. Select peaks from the peak list first."));
        return;
    }

    if (hasControlFile()) {
        if (QMessageBox::question(this,
                                  tr("Overwrite existing project"),
                                  tr("This will overwrite the existing project.\nDo you want to continue?")
                                  ) == QMessageBox::No) {
            return;
        }
    }

    BgmnSinglePeakRefinementDialog *spRefDlg = new BgmnSinglePeakRefinementDialog(this);

    if (spRefDlg->exec() != QDialog::Accepted) {
        delete spRefDlg;
        return;
    }

    QString devFile = spRefDlg->deviceFile();
    QString phName = spRefDlg->usePhaseName() ? QString() : spRefDlg->useOtherName();

    QPlainTextEdit *e = fileEditor(controlFile, true);
    BgmnSavParser sparser(e->toPlainText(), controlFile);

    QList<Hkl> selectedPeaks = peakListWidget->getDataSelected(global::PositionUnit::TWOTHETA);

    // adding files can take a long time when working on network shared drives
    qApp->setOverrideCursor(Qt::WaitCursor);

    // store the previous val files for later
    QStringList vfiles = sparser.valFile();

    // if requested, create a new control file from the template file
    if (sparser.deviceFile().isEmpty()) {
        if (!createControlFileFromTemplate(e, sparser, devFile, true)) {
            delete spRefDlg;
            qApp->restoreOverrideCursor();
            return;
        }
    }

    QStringList peakFiles = createSinglePeakStrFiles(selectedPeaks, phName, spRefDlg->overwrite());
    sparser.addStructureFiles(peakFiles, false, false);

    // write back the VAL[]= files from the original text if they were not empty
    if (!vfiles.isEmpty()) {
        sparser.setValFile(vfiles);
    }

    // get the complete sav file text from the sav parser
    // and write it to the text editor
    e->setPlainText(adjustControlOutputFiles(sparser.getContent(), false));

    // update the search match widget with project information
    setupSearchMatchWidget(true, true);

    if (settings->value("bgmnProject/openAddedStrFiles", true).toBool()) {
        openTextFileEditors(peakFiles);
    }

    delete spRefDlg;
    qApp->restoreOverrideCursor();
}

/*
 * if baseName is empty, the file name will be composed of hkl::phase,
 * else from baseName
 */
QStringList BgmnProjectWidget::createSinglePeakStrFiles(const QList<Hkl> &hkl, const QString &baseName, bool overwrite)
{
    QStringList l;
    static QRegularExpression rx("[\\s_\\-\\(\\)\\?\\+]|peak data");
    double wl = 0.1 * graphView->getWaveLength();

    for (int i = 0; i < hkl.size(); ++i) {
        double tt = hkl.at(i).position();
        double d = global::Functions::twoThetaToD(tt, wl);
        double dmin = global::Functions::twoThetaToD(tt + 0.5, wl);
        double dmax = global::Functions::twoThetaToD(tt - 0.5, wl);

        QString phName = baseName.isEmpty()
                             ? QString("%1%2").arg(hkl.at(i).phase()).arg(i)
                             : QString("%1%2").arg(baseName).arg(i);

        phName = phName.replace(rx, QString());

        QFileInfo pfName(QDir::fromNativeSeparators(QString("%1/%2.str").arg(projectDir, phName)));

        QString pfContent = QString("PHASE=%1\n").arg(phName);
        pfContent += "SpacegroupNo=195 GeneralCondition=eq(1,sqr(h)+sqr(k)+sqr(l))\n";
        pfContent += QString("PARAM=B1=0_0^0.01 B2=0 PARAM=A=%1_%2^%3\n").arg(d, 0, 'f', 6).arg(dmin, 0, 'f', 6).arg(dmax, 0, 'f', 6);
        pfContent += QString("GOAL:P%1=(rad*asin(0.05*sk(1,0,0)*lambda))*2").arg(phName);

        if (!pfName.exists() || overwrite) {
            BgmnFileIO::writeTextFile(pfName.absoluteFilePath(), pfContent);
        }

        l.append(pfName.absoluteFilePath());
    }

    return l;
}

void BgmnProjectWidget::exportToExcel()
{
    bool ok;
    BgmnSavParser sparser = getSavParser(&ok);
    if (!ok) return;
    excelExporter->exportData(controlFile, projectDir + "/" + sparser.listFile());
}

void BgmnProjectWidget::editExcelExport()
{
    bool ok;
    BgmnSavParser sparser = getSavParser(&ok);
    if (!ok) return;
    excelExporter->showEditor(controlFile, projectDir + "/" + sparser.listFile());
}

void BgmnProjectWidget::getEpsValues(double &eps1, double &eps2, double &eps3)
{
    BgmnSavParser sparser = getSavParser();

    if (lparser.hasEpsN(1))      eps1 = lparser.getEpsN(1);
    else if (sparser.hasEpsN(1)) eps1 = sparser.getEpsN(1);
    else                         eps1 = 0.0;

    if (lparser.hasEpsN(2))      eps2 = lparser.getEpsN(2);
    else if (sparser.hasEpsN(2)) eps2 = sparser.getEpsN(2);
    else                         eps2 = 0.0;

    if (lparser.hasEpsN(3))      eps3 = lparser.getEpsN(3);
    else if (sparser.hasEpsN(3)) eps3 = sparser.getEpsN(3);
    else                         eps3 = 0.0;
}

QStringList BgmnProjectWidget::openStructureFileUnderCursor()
{
    if (qobject_cast<GraphWindow*>(currentWidget()) == graphView) {
        return openStructureFileOfActiveGraph();
    } else {
        ControlFileEdit *e = getCurrentEditor();
        if (!e) return QStringList();

        QStringList files;

        if (e->getFileType() == ControlFileEdit::ControlFileEditFileType::BGMN_SAV) {
            files = openStructureFileOfSavTextBlock(e);
        } else if (e->getFileType() == ControlFileEdit::ControlFileEditFileType::BGMN_LST) {
            files = openStructureFileOfLstTextBlock(e);
        } else {
            files = openStructureFileOfSelectItem();
        }

        return files;
    }

    return QStringList();
}

QStringList BgmnProjectWidget::openStructureFileOfActiveGraph()
{
    QStringList pNames;
    QVector<Scan *> scnLst = graphControl->activeScans();

    for (int i = 0; i < scnLst.size(); ++i) {
        pNames.append(strFileFromPhaseName(scnLst.at(i)->name()));
    }

    if (!pNames.isEmpty()) {
        openTextFileEditors(pNames);
    }

    return pNames;
}

QStringList BgmnProjectWidget::openStructureFileOfSelectItem()
{
    QStringList sNames = scanList->getActiveScanNames();
    QStringList pNames;

    for (int i = 0; i < sNames.size(); ++i) {
        pNames.append(strFileFromPhaseName(sNames.at(i)));
    }

    if (!pNames.isEmpty()) {
        openTextFileEditors(pNames);
    }

    return pNames;
}

QStringList BgmnProjectWidget::openStructureFileOfSavTextBlock(ControlFileEdit *e)
{
    QString sfnl = e->blockUnderSelection();

    static QRegularExpression rx("^STRUC\\[\\d+\\]=(.*)$");
    QRegularExpressionMatch rm;

    if (!sfnl.contains(rx, &rm)) return QStringList();

    QFileInfo fi(projectDir + QDir::separator() + rm.captured(1));
    QStringList lstFi(fi.absoluteFilePath());
    openTextFileEditors(lstFi);
    return lstFi;
}

QStringList BgmnProjectWidget::openStructureFileOfLstTextBlock(ControlFileEdit *e)
{
    QString pName = lparser.getPhaseNameFromLineNumber(e->textCursor().blockNumber());

    if (pName.isEmpty()) return QStringList();

    QStringList fiLst(strFileFromPhaseName(pName));
    openTextFileEditors(fiLst);
    return fiLst;
}

QString BgmnProjectWidget::strFileFromPhaseName(const QString &p)
{
    QStringList strFiles = getProjectStrFiles();

    // If BGMN finds a PHASE parameter in the STR file, it uses
    // it to label the phase in the lst file.
    // Try to find a STR file with PHASE=p

    for (int i = 0; i < strFiles.size(); ++i) {
        BgmnStrParser strParser(strFiles.at(i));
        if (strParser.getPhaseName() == p) {
            return strFiles.at(i);
        }
    }

    // If no PHASE variable is given in the STR file, BGMN
    // falls back to using the file name (without extension)
    // to label the phase. Try to locate a file with name p.str

    QString fnSmall   = QString("%1/%2.%3").arg(projectDir, p, "str");
    QString fnCapital = QString("%1/%2.%3").arg(projectDir, p, "STR");

    if (QFile::exists(fnSmall))   return fnSmall;
    if (QFile::exists(fnCapital)) return fnCapital;

    return QString();
}

QList<BgmnStrParser> BgmnProjectWidget::getStrParserList()
{
    QStringList strFiles = getProjectStrFiles();
    QList<BgmnStrParser> lst;

    for (int i = 0; i < strFiles.size(); ++i) {
        lst.append(BgmnStrParser(strFiles.at(i)));
    }

    return lst;
}

QMap<QString, BgmnStrParser> BgmnProjectWidget::getStrParserMapByFile()
{
    QStringList strFiles = getProjectStrFiles();
    QMap<QString, BgmnStrParser> map;

    for (int i = 0; i < strFiles.size(); ++i) {
        map[strFiles.at(i)] = BgmnStrParser(strFiles.at(i));
    }

    return map;
}

QMap<QString, BgmnStrParser> BgmnProjectWidget::getStrParserMapByPhase()
{
    QStringList strFiles = getProjectStrFiles();
    QMap<QString, BgmnStrParser> map;

    for (int i = 0; i < strFiles.size(); ++i) {
        BgmnStrParser p(strFiles.at(i));
        map[p.getPhaseName()] = p;
    }

    return map;
}

void BgmnProjectWidget::currentScanChanged(int n)
{
    ControlFileEdit *e = getCurrentEditor();
    if (!e) return;
    if (e->getFileType() != ControlFileEdit::ControlFileEditFileType::BGMN_LST) return;

    QString sName = graphControl->scanName(graphControl->getScan(n)->uid());

    QList<BgmnStrParser> lst = getStrParserList();
    for (int i = 0; i < lst.size(); ++i) {
        if (lst.at(i).getPhaseName() == sName) {
            e->setCurrentLine(lparser.lineOfPhase(sName));
            return;
        }
    }

    e->setCurrentLine(9);
}

QHash<QString, CrystalStructure> BgmnProjectWidget::getStructuresModels()
{
    QMap<QString, BgmnStrParser> strParsers = getStrParserMapByPhase();
    QHash<QString, CrystalStructure> hash;

    for (auto it = strParsers.cbegin(); it != strParsers.cend(); ++it) {
        hash[it.key()] = it.value().getCrystalStructure();
    }

    return hash;
}

QHash<QString, CrystalStructure> BgmnProjectWidget::getStructuresRefined()
{
    QStringList phaseNames = lparser.getPhaseNames();
    QMap<QString, BgmnStrParser> strParsers = getStrParserMapByPhase();
    QHash<QString, CrystalStructure> hash;

    for (int i = 0; i < phaseNames.size(); ++i) {
        QString phase = phaseNames.at(i);
        CrystalStructure struc = lparser.getCrystalStructure(phaseNames.at(i));

        if (!struc.unitCell().isValid()) {
            if (strParsers.contains(phase)) {
                const CrystalUnitCell strCell = strParsers.value(phase).getCrystalStructure().unitCell();

                double th = 0.001;
                if (struc.unitCell().a()     < th) struc.unitCell().setA(strCell.a());
                if (struc.unitCell().b()     < th) struc.unitCell().setB(strCell.b());
                if (struc.unitCell().c()     < th) struc.unitCell().setC(strCell.c());
                if (struc.unitCell().alpha() < th) struc.unitCell().setAlpha(strCell.alpha());
                if (struc.unitCell().beta()  < th) struc.unitCell().setBeta(strCell.beta());
                if (struc.unitCell().gamma() < th) struc.unitCell().setGamma(strCell.gamma());
            }
        }

        hash[phase] = struc;
    }

    return hash;
}

QStringList BgmnProjectWidget::getPhaseNames()
{
    QStringList strFiles = getProjectStrFiles();
    QStringList phases;

    for (int i = 0; i < strFiles.size(); ++i) {
        BgmnStrParser p(strFiles.at(i));
        phases << p.getPhaseName();
    }

    return phases;
}

/* EOF */

