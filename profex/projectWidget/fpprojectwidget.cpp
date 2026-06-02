/***************************************************************************
                          fpprojectwidget.cpp  -  description
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

#include "fpprojectwidget.h"
#include "../libXrdIO/parser/fpsumparser.h"
#include "fpaddremovedialog.h"
#include "../libXrdIO/import/importhandler.h"
#include "../libXrdIO/export/exporthandler.h"
#include "../libXrdIO/scanops.h"
#include "../libXrdIO/structs.h"
#include "syntaxHighlighter/fphighlighter.h"

#include <QMessageBox>
#include <QDebug>

#include <vector>

FpProjectWidget::FpProjectWidget(QWidget *parent) :
        ProjectWidget(parent)
{
    // init the fullprof process handler
    fpHandler = new FpHandler(this);
    connect(fpHandler, SIGNAL(cycleComplete(int, int)), this, SLOT(cycleComplete(int, int)));
    connect(fpHandler, SIGNAL(aborted()), this, SLOT(processAborted()));

    controlFileExtension = "pcr";

    // these files will be loaded automatically
    textFormats << "pcr" << "sum";

    resultsTree = new FpResultsTreeWidget(this);

    ImportHandler iHandler;
    graphFormats = iHandler.extensions();
    // make sure the prf file is first in the list
    // because it's our preferred file format to auto-load
    graphFormats.prepend("prf");

    // file formats directly supported by the Rietveld backend
    nativeGraphFormats << "dat";

    pSelectItem->setProjectType("FullProf");

    // hide some widgets until the functionality is implemented
    //comboBoxRefStructures->hide();
    //labelRefStr->hide();

    initFpSettings();
}

/*
 * virtual function, do not call from constructor
 */
void FpProjectWidget::initSettings()
{
    initFpSettings();
}

/*
 * non-virtual function, can be called from constructor
 */
void FpProjectWidget::initFpSettings()
{
    initGlobalSettings();
}

void FpProjectWidget::runPeakDetection()
{
    refOutput->appendPlainText("Peak detection is not available for Fullprof projects.");
}

// executes the Fullprof command
void FpProjectWidget::runRefinement()
{
    qDebug() << QString("FpProjectWidget::runRefinement(): Running refinement");
    saveAll();

    // check if the dat file exists. If not, try to create it from the XRDML file.
    // if this fails, return, because fullprof can't run without a dat file
    if (!QFile::exists(QDir::fromNativeSeparators(projectDir + "/" + projectBasename + ".dat"))) {
        if (convertRawDataFile() <= 0) {
            return;
        }
    }

    // erase old *.sub files
    QStringList subFilter;
    subFilter << QString("%1*.sub").arg(projectBasename) << QString("%1*.SUB").arg(projectBasename);
    QDir subDir(projectDir);
    QFileInfoList subFiles = subDir.entryInfoList(subFilter, QDir::Files);

    for (int sf = 0; sf < subFiles.size(); ++sf) {
        if (QFile::exists(subFiles.at(sf).absoluteFilePath())) QFile::remove(subFiles.at(sf).absoluteFilePath());
    }

    controlFile = QDir::fromNativeSeparators(projectDir + "/" + projectBasename + "." + controlFileExtension);
    refinedGraphFile = QDir::fromNativeSeparators(projectDir + "/" + projectBasename + ".prf");

    QPlainTextEdit *e = fileEditor(controlFile, false);

    if (e) {
        previousControlContent.append(e->toPlainText());
    } else {
        QMessageBox::information(this, tr("Create Control File"), tr("Please create a control file using \"Add Phase (+)\" first."));
        return;
    }

    refOutput->clear();

    // initialize the process handler
    if (!fpHandler->init(controlFile)) {
        QMessageBox::information(this, tr("Fullprof.2k Error"), tr("Fullprof.2k executable not found.\n\n"
                 "Please check the Fullprof configuration in the preferences."));
        return;
    }

    // start the process
    if (fpHandler->run()) {
        setStatus(global::RefinementStatus::RUNNING);
        pSelectItem->setRefinementStats(1, fpHandler->totalCycles(), -1.0, -1.0);
        setCurrentIndex(0); // switch to the graph page
    } else {
        setStatus(global::RefinementStatus::FAILURE);
        QMessageBox::information(this, tr("Error starting Fullprof"),
            QString(tr("Refinement could not be started.\nThe following command was aborted:\n\n%1 %2")).arg(refinerExec).arg(controlFile));
    }
}

/*
   this slot is called by the finish() signal of fpHandler's QProcess.
   it reads the output files and starts the process again (fpHandler will
   know when to stop).
*/
void FpProjectWidget::cycleComplete(int cycle, int total)
{
    // pSelectItem->setStatus(QString(tr("%1 of %2").arg(cycle).arg(total)));
    pSelectItem->setRefinementStats(cycle, total, 0.0, 0.0);

    if (graphLiveUpdate) {
        if (refinedGraphFile == loadedGraphFile) {
            graphControl->loadScanFile(refinedGraphFile, QStringLiteral("FPPRF3_PRF"), true, graphControl->getSampleId(), true);
        } else {
            if (graphControl->loadScanFile(refinedGraphFile, QStringLiteral("FPPRF3_PRF"), true, graphControl->getSampleId(), true) > 0) {
                QFileInfo fi(refinedGraphFile);
                setTabText(0, fi.fileName());
                loadedGraphFile = refinedGraphFile;
                updateResults();
            }
        }
    }

    // read the fullprof output
    QString output = fpHandler->readOutput();
    refOutput->appendPlainText(output);
    refOutput->ensureCursorVisible();

    // parse output and check if a problem occurred
    if (!output.simplified().contains(QString("=> Normal end, final calculations and writing...").simplified(), Qt::CaseSensitive)) {
        fpHandler->abort();
        pSelectItem->setStatus(global::RefinementStatus::FAILURE);
        QMessageBox::information(this, tr("Error running Fullprof"),
            tr("The refinement was aborted.\nPlease check the Fullprof output."));

        return;
    }

    // parse output and stop if convergence has been reached.
    if (output.contains("Convergence reached at this CYCLE !!!!", Qt::CaseSensitive) && stopOnConvergence) {
        fpHandler->abort();
        pSelectItem->setStatus(global::RefinementStatus::COMPLETED);
        emit completed(this);
        updateGui();

        return;
    }

    // restart the process, fpHandler returns false if no more cycles are left
    if (!fpHandler->run()) {
        pSelectItem->setStatus(global::RefinementStatus::COMPLETED);
        emit completed(this);
        updateGui();

        return;
    }
}

bool FpProjectWidget::isRunning()
{
    if (fpHandler->isRunning()) return true;
    if (peakFitWidget->isRunning()) return true;
    return false;
}

/*
   updates all GUI widgets, e.g. when the refinement is complete
*/
void FpProjectWidget::updateGui()
{
    graphView->setComplete();
    loadTextFile(controlFile);
}

/*
   Asks fpHandler to abort the running process
*/
void FpProjectWidget::abort()
{
    int n = 0;

    if (fpHandler->isRunning()) {
        fpHandler->abort();
        ++n;
    }

    if (peakFitWidget->isRunning()) {
        peakFitWidget->abortFit();
        ++n;
    }

    if (n) pSelectItem->setStatus(global::RefinementStatus::ABORTED);
}

QString FpProjectWidget::getSampleID()
{
    FpPcrParser pparser(controlFile);
    return pparser.sampleID();
}

bool FpProjectWidget::hasControlFile()
{
    QPlainTextEdit *e = fileEditor(controlFile, false);

    if (!e) return false;
    return true;
}

void FpProjectWidget::addRemovePhase()
{
    QPlainTextEdit *e = fileEditor(controlFile, true);

    if (!e) {
        qDebug() << QString("FpProjectWidget::appendPhase(): Could not create control file editor for file %1").arg(controlFile);
        return;
    }

    FpAddRemoveDialog *adlg = new FpAddRemoveDialog();

    adlg->setProjectBasename(projectBasename);
    adlg->setProjectDir(projectDir);
    adlg->setProjectScanFile(loadedGraphFile);
    adlg->setText(e->toPlainText());

    if (adlg->exec() == QDialog::Accepted) {
        e->setPlainText(adlg->getString());
    }

    delete adlg;
}

void FpProjectWidget::load(const QString &txt, const QString &grph, const QString &uid)
{
    if (!QFile::exists(txt) && !QFile::exists(grph)) return;

    bool autoDetectGraph = true;

    if (!grph.isEmpty()) {
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
        loadGraph(autoDetermineGraphFile(), QString());
    }

    setCurrentIndex(0);
    parseProjectFiles();
    updateResults();
}

/*
  returns the phase quantities as a ; separated string
*/
QStringList FpProjectWidget::getQuantitiesCsv()
{
    QStringList s;
    QList<phase> lst = compileQuantities();

    if (!lst.size()) {
        return s;
    }

    s.append(QString("Sample;%1\n").arg(lst.at(0).sample));
    s.append("Phase Name;R_Bragg [%];Quantity [rel. wt-%];Avg. Crystallite Size [nm];mP [a.u.]\n");

    for (int i = 0; i < lst.size(); ++i) {
        phase p = lst.at(i);
        s.append(QString("%1;%2;%3;%4;%5\n").arg(p.name).arg(p.rbragg, 0, 'f', 1).arg(p.fraction, 0, 'f', 2).arg(p.crystsize, 0, 'f', 0).arg(p.mp, 0, 'f', 2));
    }

    return s;
}

QList<phase> FpProjectWidget::compileQuantities()
{
    qDebug() << QString("FpProjectWidget::compileQuantities(): Compiling results for project %1/%2").arg(projectDir).arg(projectBasename);

    if (controlFile == "") {
        controlFile = QDir::fromNativeSeparators(projectDir + "/" + projectBasename + "." + controlFileExtension);
    }

    QString sumFile = QDir::fromNativeSeparators(projectDir + "/" + projectBasename + ".sum");

    FpSumParser sparser(sumFile);
    QList<phase> lst = sparser.getPhases();

    if (!lst.size()) {
        return lst;
    }

    FpPcrParser pparser(controlFile);
    QVector<double> v = pparser.getCrystSizes(pparser.getWavelength(0));
    QVector<double> w = pparser.getScales();

    int midx = qMin(lst.size(), v.size());

    for (int i = 0; i < midx; ++i) {
            lst[i].crystsize = v.at(i);
            lst[i].scale = w.at(i);
            lst[i].mp = lst.at(i).scale * lst.at(i).atz * lst.at(i).volume;
    }

    return lst;
}

void FpProjectWidget::applyTextBlock(const QString &s)
{
    ControlFileEdit *e = getCurrentEditor();
    if (!e) return;
    e->textCursor().insertText(s);
}

void FpProjectWidget::setHighlighting(ControlFileEdit *ed)
{
    FpHighlighter *fhl = new FpHighlighter(this);
    fhl->setDocument(ed->document());
    highlighters.insert(ed, fhl);
}

void FpProjectWidget::saveCifFiles(bool complete, const QMap<QString, QVariant> &auxDataGlobal)
{
    Q_UNUSED(complete);
    Q_UNUSED(auxDataGlobal);
    refOutput->appendPlainText(tr("Not available for Fullprof projects\n"));
}

void FpProjectWidget::saveCastepCellFiles()
{
    refOutput->appendPlainText(tr("Not available for Fullprof projects\n"));
}

void FpProjectWidget::setInternalStandard()
{
    refOutput->appendPlainText(tr("Not available for Fullprof projects\n"));
}

void FpProjectWidget::unsetInternalStandard()
{
    refOutput->appendPlainText(tr("Not available for Fullprof projects\n"));
}

void FpProjectWidget::generateReport()
{
    refOutput->appendPlainText(tr("Not available for Fullprof projects\n"));
}

int FpProjectWidget::convertRawDataFile()
{
    QString datFile(QDir::fromNativeSeparators(projectDir + "/" + projectBasename + ".dat"));
    QString inFile(graphControl->fileName());

    QMap<QString, QVariant> flags;
    flags["fieldSeparator"] = settings->value("config/asciiFieldSeparator", QVariant(QString(" ")));

    QVector<Scan> scanHeap;
    ImportHandler iHandler;
    ExportHandler eHandler;

    const Scan *cScan = graphControl->firstActiveScan();
    if (!cScan) cScan = graphControl->getScan(0);

    // load the file
    if (iHandler.load(inFile, cScan->uid().toString(), scanHeap) <= 0) {
        qDebug() << QString("FpProjectWidget::convertRawDataFile(): Could not read file %1").arg(inFile);
        QMessageBox::information(this, tr("Problem converting file"), QString(tr("There was a problem converting file\n%1\nto\n%2")).arg(inFile).arg(datFile));
        return 0;
    }

    // make sure we loaded something
    if (!scanHeap.size()) {
        qDebug() << QString("FpProjectWidget::convertRawDataFile(): No scans found in file %1").arg(inFile);
        QMessageBox::information(this, tr("Problem converting file"), QString(tr("There was a problem converting file\n%1\nto\n%2")).arg(inFile).arg(datFile));
        return 0;
    }

    Scan &scan = scanHeap[0];

    // check if we have to convert the divergence slit to FDS
    bool ok;
    bool aiOk;

    if (scan.auxInfo("incidentBeamPath:divergenceSlit:type", aiOk).toString() == "automaticDivergenceSlitType") {
        double irrl = scan.auxInfo("incidentBeamPath:divergenceSlit:irradiatedLength", aiOk).toDouble(&ok);
        qDebug() << QString("FpProjectWidget::runRefinement(): Automatic divergence slit detected.");

        if (ok)  {
            double radius = scan.auxInfo("incidentBeamPath:radius", aiOk).toDouble(&ok);
            qDebug() << QString("FpProjectWidget::runRefinement(): Using radius %1.").arg(radius);

            if (ok) {
                double dang = settings->value("config/dsAngle", "0.25").toDouble(&ok);
                qDebug() << QString("FpProjectWidget::runRefinement(): Converting divergence slit to fixed in file %1").arg(inFile);
                scan = ScanOps::convertDivSlitToFDS(scan, irrl, dang, radius);
            }
        }
    } else {
        qDebug() << QString("FpProjectWidget::runRefinement(): Unknown divergence slit mode. Asking for user input.");

        if (QMessageBox::question(this, tr("Divergence slit conversion"), tr("Do you want to convert the scan from automatic to fixed divergence slit?"),
                                   QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes)
        {
            double dang = settings->value("config/dsAngle", "0.25").toDouble(&ok);
            double irrl = QInputDialog::getDouble(this, tr("Divergence slit conversion"), tr("Irradiated Length [mm]:"), 10.0, 0.0, 50.0);
            double radius = QInputDialog::getDouble(this, tr("Divergence slit conversion"), tr("Goniometer Radius [mm]:"), 250.0, 0.0, 500.0);
            qDebug() << QString("FpProjectWidget::runRefinement(): Converting divergence slit to fixed in file %1").arg(inFile);
            scan = ScanOps::convertDivSlitToFDS(scan, irrl, dang, radius);
        } else {
            qDebug() << QString("FpProjectWidget::runRefinement(): Divergence slit conversion skipped");
        }
    }

    // write the dat file
    if (eHandler.save("FPDAT10_DAT", datFile, scanHeap[0], flags) == -1) {
        qDebug() << QString("FpProjectWidget::runRefinement(): Could not save DAT file %1").arg(datFile);
        QMessageBox::information(this, tr("Problem converting file"), QString(tr("There was a problem converting file\n%1\nto\n%2")).arg(inFile).arg(datFile));
        return 0;
    }

    return 1;
}

void FpProjectWidget::updateResults()
{
}
