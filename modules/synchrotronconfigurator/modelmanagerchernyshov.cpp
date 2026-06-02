/***************************************************************************
                          modelmanagerchernyshov.cpp  -  description
                             -------------------
    begin                : Fri Jan 17 18:00:00 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#include <QProgressDialog>
#include <QThread>
#include "modelmanagerchernyshov.h"
#include "modelfitterchernyshov.h"
#include "noeditdelegate.h"
#include "../libXrdIO/structs.h"
#include "../libXrdIO/functions.h"
#include "../libXrdIO/bgmnfileio.h"

ModelManagerChernyshov::ModelManagerChernyshov(QCustomPlot *pFwhm, QCustomPlot *pPeaks, QTreeWidget *tSupport, QTreeWidget *tModel, QObject *parent)
    :  QObject{parent}, plotFwhm(pFwhm), plotPeaks(pPeaks), treeSupportPeaks(tSupport), treeModelPeaks(tModel)
{
    settings = SettingsManager::getInstance();
    geqExporter = nullptr;
    progressDlg = nullptr;

    fitThreadsRunning = 0;

    bool darkMode = settings->isDarkMode();
    QColor colPlotLine(darkMode ? global::Functions::colorToDarkMode(Qt::blue) : Qt::blue);
    QColor colPlotSymbols(darkMode ? global::Functions::colorToDarkMode(Qt::red) : Qt::red);
    QColor colPlotSum(darkMode ? global::Functions::colorToDarkMode(Qt::green) : Qt::green);

    // graphs plotFwhm->graph(0) and (1) must always exist. Never delete them, just clear the data.
    plotFwhm->addGraph();
    plotFwhm->addGraph();
    plotFwhm->xAxis->setLabel(QString("cos%1(%2%3)").arg(global::superTwo, "2", global::theta));
    plotFwhm->yAxis->setLabel(QString("FWHM%1 (rad%1)").arg(global::superTwo));
    plotFwhm->graph(0)->setLineStyle(QCPGraph::lsLine);
    plotFwhm->graph(0)->setPen(QPen(colPlotLine));
    plotFwhm->graph(1)->setLineStyle(QCPGraph::lsNone);
    plotFwhm->graph(1)->setScatterStyle(QCPScatterStyle(QCPScatterStyle::ssCircle, colPlotSymbols, colPlotSymbols, 6));
    plotFwhm->xAxis->setRange(0.0, 1.0);
    plotFwhm->yAxis->setRange(0.0, 1.0e-8);

    // graphs plotPeaks->graph(0), (1), and (2) must always exist. Never delete them, just clear the data.
    plotPeaks->addGraph();
    plotPeaks->addGraph();
    plotPeaks->addGraph();
    plotPeaks->xAxis->setLabel(QString("%1 %2%3%4").arg(global::Delta, global::degree, "2", global::theta));
    plotPeaks->yAxis->setLabel("Intensity (a. u.)");
    plotPeaks->xAxis->setRange(-std::numeric_limits<double>::min(), std::numeric_limits<double>::min());
    plotPeaks->yAxis->setRange(0.0, 1.0);
    plotPeaks->graph(0)->setLineStyle(QCPGraph::lsLine);
    plotPeaks->graph(0)->setPen(QPen(colPlotLine));

    peakFitPlotter = new ThreadSafePlotter();
    peakFitPlotter->setPlotter(plotPeaks, colPlotLine, colPlotSymbols, colPlotSum);

    parameterStorage = new ParameterStorage();

    QStringList headerSupportPeaks;
    QStringList headerModelPeaks;

    headerSupportPeaks << tr("No.")
                       << QString("%1%2 (%3)").arg("2").arg(global::theta).arg(global::degree)
                       << QString("FWHM (%1)").arg(global::degree)
                       << tr("Shape");

    headerModelPeaks << tr("No.")
                     << QString("%1%2 (%3)").arg("2").arg(global::theta).arg(global::degree)
                     << QString("FWHM (%1)").arg(global::degree)
                     << tr("Shape")
                     << tr("Area");

    treeSupportPeaks->setColumnCount(headerSupportPeaks.size());
    treeModelPeaks->setColumnCount(headerModelPeaks.size());

    treeSupportPeaks->setHeaderLabels(headerSupportPeaks);
    treeModelPeaks->setHeaderLabels(headerModelPeaks);

    // disable editing of column 0
    treeSupportPeaks->setItemDelegateForColumn(0, new NoEditDelegate(this));

    connect(treeSupportPeaks, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this,           SLOT(peakDataChanged(QTreeWidgetItem*,int)));
    connect(treeModelPeaks,   SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(peakItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(treeModelPeaks,   SIGNAL(sigInitCurves(QUuid)),              this,           SLOT(initSingleProfile(QUuid)));
    connect(treeModelPeaks,   SIGNAL(sigFitCurves(QUuid)),               this,           SLOT(fitSingleProfile(QUuid)));
    connect(treeModelPeaks,   SIGNAL(sigInitFromAbove(int)),             this,           SLOT(initFromAbove(int)));
    connect(treeModelPeaks,   SIGNAL(sigInitFromBelow(int)),             this,           SLOT(initFromBelow(int)));
    connect(plotPeaks,        SIGNAL(mousePress(QMouseEvent*)),          peakFitPlotter, SLOT(resetPeakPlotZoom(QMouseEvent*)));
    connect(plotPeaks->xAxis, SIGNAL(rangeChanged(QCPRange)),            peakFitPlotter, SLOT(zoomPeakPlot(QCPRange)));
}

ModelManagerChernyshov::~ModelManagerChernyshov()
{
    qDeleteAll(peakManagers);
    if (geqExporter)      delete geqExporter;
    if (peakFitPlotter)   delete peakFitPlotter;
    if (progressDlg)      delete progressDlg;
    if (parameterStorage) delete parameterStorage;
}

void ModelManagerChernyshov::exportCurves(const QString &f)
{
    QUuid uid = getCurrentUid();

    if (uid.isNull()) {
        qDebug() << QString("ModelManagerChernyshov::exportCurves(): Couldn't get current peak manager. Exiting.");
        return;
    }

    PeakManagerChernyshov *pManager = peakManagers[uid];
    if (pManager) BgmnFileIO::writeTextFile(f, pManager->getCsvString());
}

void ModelManagerChernyshov::computeFwhm()
{
    double ttStart = parameterStorage->getParameter(synchro::RANGE_MEASURED_START, -1.0);
    double ttEnd   = parameterStorage->getParameter(synchro::RANGE_MEASURED_END, -1.0);
    double ttStep  = 1.0;

    // using a new peakManager because we compute the fwhm at much finer inteval for the FWHM^2 plot than for profiles
    PeakManagerChernyshov *pManager = new PeakManagerChernyshov(peakFitPlotter);
    pManager->updateParameters(parameterStorage);

    QList<double> xval, yval;
    double tt = ttStart;

    while (tt < ttEnd) {
        pManager->getParameters()->setValue(synchro::TWOTHETA_DEG, tt);
        pManager->calculateFwhm(true);

        double fwhmRad = pManager->getParameters()->getParameter(synchro::PROFILE_FWHM_RAD, -1.0);

        if (fwhmRad > 0.0) {
            xval.append(std::pow(std::cos(qDegreesToRadians((tt))), 2.0));
            yval.append(std::pow(fwhmRad, 2.0));
        }

        tt += ttStep;
    }

    plotFwhm->graph(0)->setData(xval, yval);
    rescaleFwhmAxes(true, true);

    delete pManager;
}

void ModelManagerChernyshov::fitFundamentalParameters()
{
    ModelFitterChernyshov *modelFitter = new ModelFitterChernyshov;

    QList<double> x, y, s;
    getSupportPeakData(x, y, s);

    modelFitter->setPeakParameters(parameterStorage);
    modelFitter->setTargetData(x, y);
    modelFitter->fitModel();

    delete modelFitter;
}

int ModelManagerChernyshov::addModelPeaks()
{
    double rStart = parameterStorage->getParameter(synchro::RANGE_MEASURED_START, -1.0);
    double rEnd   = parameterStorage->getParameter(synchro::RANGE_MEASURED_END,   -1.0);
    double rStep  = parameterStorage->getParameter(synchro::RANGE_MEASURED_STEP,  -1.0);
    double rShape = parameterStorage->getParameter(synchro::CHERNYSHOV_SHAPE,     -1.0);

    if ((rStart < 0.0) || (rEnd < 0.0) || (rStep < 0.0) || (rShape < 0.0)) {
        return 0;
    }

    QList<double> ttAngles = calculatePeakPositions(rStart, rEnd, rStep);
    int n = generateModelPeaks(ttAngles, rShape);
    return n;
}

void ModelManagerChernyshov::loadSupportPeaks(const QList<double> &ttDeg, const QList<double> &hwhmDeg, const QList<double> &sh, double meanSh)
{
    generateSupportPeaks(ttDeg, hwhmDeg, sh, meanSh);
}

void ModelManagerChernyshov::generateSupportPeaks(const QList<double> &ttAngles, const QList<double> &hwhmDeg, const QList<double> &sh, double meanSh)
{
    treeSupportPeaks->clear();

    for (int i = 0; i < ttAngles.size(); ++i) {
        QTreeWidgetItem *itm = new QTreeWidgetItem;
        itm->setFlags(itm->flags() | Qt::ItemIsEditable);
        itm->setText(0, QString::number(i + 1));
        itm->setText(1, QString::number(ttAngles.at(i)));                      // in degrees
        itm->setText(2, QString::number(2.0 * hwhmDeg.at(i)));                 // in degrees
        itm->setText(3, QString::number(i >= sh.size() ? meanSh : sh.at(i)));

        itm->setData(1, Qt::UserRole, ttAngles.at(i));                         // in degrees
        itm->setData(2, Qt::UserRole, qDegreesToRadians(2.0 * hwhmDeg.at(i))); // in radians
        itm->setData(3, Qt::UserRole, i >= sh.size() ? meanSh : sh.at(i));

        treeSupportPeaks->addTopLevelItem(itm);
    }

    updateFwhmPeakPlot();
}

int ModelManagerChernyshov::generateModelPeaks(const QList<double> &ttAngles, double sh)
{
    int selectedItem = 0;
    if (treeModelPeaks->currentItem()) {
        selectedItem = treeModelPeaks->indexOfTopLevelItem(treeModelPeaks->currentItem());
    }

    peakManagers.clear();
    treeModelPeaks->clear();
    peakFitPlotter->reset();
    bool oldState = treeModelPeaks->blockSignals(true);

    for (int i = 0; i < ttAngles.size(); ++i) {
        PeakManagerChernyshov *pManager = new PeakManagerChernyshov(peakFitPlotter);
        QUuid uid = pManager->uid();

        peakManagers[uid] = pManager;
        connect(peakManagers[uid], SIGNAL(fitProcessCompleted(QUuid)), this, SLOT(fitCompleted(QUuid)));

        // set default parameters and erase potential old values
        peakManagers[uid]->updateParameters(parameterStorage);
        peakManagers[uid]->getParameters()->setValue(synchro::PROFILE_NUMBER, i + 1);
        peakManagers[uid]->getParameters()->setValue(synchro::TWOTHETA_DEG, ttAngles.at(i));
        peakManagers[uid]->getParameters()->setValue(synchro::PROFILE_FWHM_DEG, -1.0);
        peakManagers[uid]->getParameters()->setValue(synchro::PROFILE_FWHM_RAD, -1.0);

        peakManagers[uid]->generatePeak();

        double fwhmDeg  = peakManagers[uid]->getParameters()->getParameter(synchro::PROFILE_FWHM_DEG, -1.0);
        double fwhmRad  = peakManagers[uid]->getParameters()->getParameter(synchro::PROFILE_FWHM_RAD, -1.0);
        double profArea = peakManagers[uid]->getParameters()->getParameter(synchro::PROFILE_AREA, -1.0);
        int    posCor   = peakManagers[uid]->getParameters()->getParameter(synchro::POSITION_CORRECTION_MODE, 1);

        QTreeWidgetItem *itm = new QTreeWidgetItem;
        itm->setFlags(itm->flags() | Qt::ItemIsEditable);
        itm->setText(0, QString::number(i + 1));
        itm->setText(1, QString::number(ttAngles.at(i))); // in degrees
        itm->setText(2, QString::number(fwhmDeg));        // in degrees
        itm->setText(3, QString::number(posCor == 1 ? 0.0 : sh));
        itm->setText(4, QString::number(profArea));

        itm->setData(0, Qt::UserRole, peakManagers[uid]->uid());
        itm->setData(1, Qt::UserRole, ttAngles.at(i)); // in degrees
        itm->setData(2, Qt::UserRole, fwhmRad);        // in radians
        itm->setData(3, Qt::UserRole, posCor == 1 ? 0.0 : sh);
        itm->setData(4, Qt::UserRole, profArea);

        treeModelPeaks->addTopLevelItem(itm);
    }

    treeModelPeaks->blockSignals(oldState);

    if (treeModelPeaks->topLevelItemCount() > selectedItem) {
        treeModelPeaks->setCurrentItem(treeModelPeaks->topLevelItem(selectedItem));
    }

    return treeModelPeaks->topLevelItemCount();
}

void ModelManagerChernyshov::updateFwhmPeakPlot()
{
    plotFwhm->graph(1)->data()->clear();

    for (int i = 0; i < treeSupportPeaks->topLevelItemCount(); ++i) {
        QTreeWidgetItem *itm = treeSupportPeaks->topLevelItem(i);
        if (!itm) continue;

        double _ttRad     = qDegreesToRadians(itm->data(1, Qt::UserRole).toDouble());
        double _pvFwhmRad = itm->data(2, Qt::UserRole).toDouble();

        plotFwhm->graph(1)->addData(std::pow(std::cos(_ttRad), 2.0), _pvFwhmRad * _pvFwhmRad);
    }

    rescaleFwhmAxes(true, true);
}

void ModelManagerChernyshov::fitAllProfiles()
{
    fitThreadPool.clear();
    fitThreadsRunning = 0;
    fitAborted = false;

    if (peakManagers.empty()) return;

    int maxThreads = qMin(QThread::idealThreadCount(), peakManagers.size());
    qDebug() << QString("Running %1 threads").arg(maxThreads);
    qApp->setOverrideCursor(Qt::WaitCursor);

    if (!progressDlg) {
        progressDlg = new QProgressDialog();
        connect(progressDlg, SIGNAL(aborted()), this, SLOT(abortFit()));
    }

    progressDlg->setMaximum(peakManagers.size());
    progressDlg->show();

    QThreadPool *globalPool = QThreadPool::globalInstance();
    QMapIterator<QUuid, PeakManagerChernyshov*> iter(peakManagers);

    while (iter.hasNext()) {
        iter.next();
        fitThreadPool.append(iter.key());
    }

    for (int i = 0; i < maxThreads; ++i) {
        QUuid uid = fitThreadPool.takeFirst();
        peakManagers[uid]->fitCurves(globalPool); // Pass the global thread pool
        ++fitThreadsRunning;
    }
}

void ModelManagerChernyshov::initSingleProfile(QUuid uid)
{
    if (!peakManagers.contains(uid)) return;
    peakManagers[uid]->initCurves();
}

void ModelManagerChernyshov::initFromAbove(int idx)
{
    if (idx < 1) return;
    QTreeWidgetItem *itmSource = treeModelPeaks->topLevelItem(idx - 1);
    QTreeWidgetItem *itmTarget = treeModelPeaks->topLevelItem(idx);

    if (!itmSource || !itmTarget) return;

    QUuid uidSource = itmSource->data(0, Qt::UserRole).toUuid();
    QUuid uidTarget = itmTarget->data(0, Qt::UserRole).toUuid();

    const PeakManagerChernyshov *pmSource = peakManagers.value(uidSource);
    PeakManagerChernyshov *pmTarget = peakManagers[uidTarget];

    pmTarget->setL2CurveParameters(pmSource->getL2CurveParameters());
}

void ModelManagerChernyshov::initFromBelow(int idx)
{
    if (idx > treeModelPeaks->topLevelItemCount() - 2) return;
    QTreeWidgetItem *itmSource = treeModelPeaks->topLevelItem(idx + 1);
    QTreeWidgetItem *itmTarget = treeModelPeaks->topLevelItem(idx);

    if (!itmSource || !itmTarget) return;

    QUuid uidSource = itmSource->data(0, Qt::UserRole).toUuid();
    QUuid uidTarget = itmTarget->data(0, Qt::UserRole).toUuid();

    const PeakManagerChernyshov *pmSource = peakManagers.value(uidSource);
    PeakManagerChernyshov *pmTarget = peakManagers[uidTarget];

    pmTarget->setL2CurveParameters(pmSource->getL2CurveParameters());
}

void ModelManagerChernyshov::fitSingleProfile(QUuid uid)
{
    if (!peakManagers.contains(uid)) return;
    qApp->setOverrideCursor(Qt::WaitCursor);

    QThreadPool *globalPool = QThreadPool::globalInstance();

    peakManagers[uid]->fitCurves(globalPool); // Pass to the global thread pool
    ++fitThreadsRunning;
}

void ModelManagerChernyshov::fitCompleted(QUuid)
{
    --fitThreadsRunning;

    if (progressDlg) {
        progressDlg->setValue(peakManagers.size() - fitThreadPool.size() - fitThreadsRunning);
    }

    if (fitThreadPool.size() && !fitAborted) {
        QUuid uid = fitThreadPool.takeFirst();

        if (peakManagers.contains(uid)) {
            QThreadPool *globalPool = QThreadPool::globalInstance();
            peakManagers[uid]->fitCurves(globalPool); // Pass to the global thread pool
            ++fitThreadsRunning;
            return;
        }
    }

    if (fitThreadsRunning < 1) {
        QUuid uid = getCurrentUid();
        if (!uid.isNull()) peakManagers[uid]->updatePlot();

        qApp->restoreOverrideCursor();
        if (progressDlg) progressDlg->hide();
        emit sigFitsComplete();
    } else {
        // waiting for running threads to terminate
    }
}

QUuid ModelManagerChernyshov::getCurrentUid(bool *ok) const
{
    QUuid uid;
    QTreeWidgetItem *itm = treeModelPeaks->currentItem();

    if (itm) uid = itm->data(0, Qt::UserRole).toUuid();
    if (ok) *ok = peakManagers.contains(uid);
    return uid;
}

void ModelManagerChernyshov::saveOutputFiles(const QString &workingDir, const QString &baseName)
{
    QFileInfo geqFileName(workingDir + "/" + baseName + ".geq");
    QFileInfo tplFileName(workingDir + "/" + baseName + ".tpl");

    qDebug() << QString("Synchrotron Configurator: ModelManagerChernyshov::saveOutputFiles(): Saving file %1").arg(geqFileName.absoluteFilePath());
    qDebug() << QString("Synchrotron Configurator: ModelManagerChernyshov::saveOutputFiles(): Saving file %1").arg(tplFileName.absoluteFilePath());

    saveGeqFile(geqFileName.absoluteFilePath());
    saveTplFile(tplFileName.absoluteFilePath());
}

void ModelManagerChernyshov::saveGeqFile(const QString &f)
{
    if (!geqExporter) geqExporter = new BgmnGeqExport;

    QList<LorentzParams> params;
    QMapIterator<QUuid, PeakManagerChernyshov*> iter(peakManagers);

    while (iter.hasNext()) {
        iter.next();
        params.append(iter.value()->getL2CurveParameters());
    }

    double d = 0.0; // 1/LAC
    double r = parameterStorage->getParameter(synchro::CHERNYSHOV_D, -1.0); // BGMN (R) = radius = Chernyshov (D)
    double t = parameterStorage->getParameter(synchro::CHERNYSHOV_C, -1.0); // BGMN (T) = sample size = Chernyshov (c)

    GEOMETRY geo = GEOMETRY::CAPILLARY;
    if (parameterStorage->getParameter(synchro::GEOMETRY, QString("CAPILLARY")) == "TRANSMISSION") geo = GEOMETRY::TRANSMISSION;

    geqExporter->setGeometry(geo);
    geqExporter->setInstrumentParameters(d, r, t);
    geqExporter->setCurveData(params);
    geqExporter->setGerFileName(f);

    QByteArray geqBinary = geqExporter->getBinaryFileContent();

    if (geqBinary.isEmpty()) {
        qDebug() << QString("ModelManagerChernyshov::saveGeqFile(): geq file content is empty.");
        return;
    }

    saveGerFile(f);
    BgmnFileIO::writeBinaryFile(f, geqBinary);
}

void ModelManagerChernyshov::saveGerFile(const QString &f)
{
    QStringList out("GEOMETRY=CAPILLARY TubeTails=0.0000");
    QMapIterator<QUuid, PeakManagerChernyshov*> iter(peakManagers);

    while (iter.hasNext()) {
        iter.next();
        LorentzParams p = iter.value()->getL2CurveParameters();
        double tt = iter.value()->getParameters()->getParameter(synchro::TWOTHETA_DEG, -1.0);
        out.append(QString("THETA=%1 N=%2 GSUM=1.0").arg(0.5 * tt, 0, 'f', 4).arg(p.size()));

        for (int j = 0; j < p.size(); ++j) {
            out.append(QString("%1 %2 %3").arg(p.at(j).g(), 0, 'f', 5).arg(p.at(j).e(), 0, 'f', 7).arg(p.at(j).q(), 0, 'f', 8));
        }
    }

    BgmnFileIO::writeTextFile(f, out.join("\n"));
}

void ModelManagerChernyshov::saveTplFile(const QString &f)
{
    double rStart = parameterStorage->getParameter(synchro::RANGE_MEASURED_START, -1.0);
    double rEnd   = parameterStorage->getParameter(synchro::RANGE_MEASURED_END, -1.0);
    double wl     = parameterStorage->getParameter(synchro::WAVELENGTH_NM, -1.0);

    QString output("SampleID:\n% Theoretical instrumental function\nVERZERR=\n% Wavelength\nSYNCHROTRON=");

    output += QString("%1\n").arg(wl);
    output += QString("% Phases\n% Measured data\nVAL[1]=\n% Minimum Angle (2theta)\n");

    if (rStart < 1.0) {
        output += QString("WMIN=1.0\n");
    } else {
        output += QString("% WMIN=%1\n").arg(rStart);
    }

    output += QString("% Maximum Angle (2theta)\n");
    output += QString("% WMAX=%1\n").arg(rEnd);

    output += QString("% Result list output\nLIST=\n% Peak list output\nOUTPUT=\n");
    output += QString("% Diagram output\nDIAGRAMM=\n% Global parameters for zero point and sample displacement\n");
    output += QString("PARAM[1]=EPS1=0_-0.01^0.01\nEPS2=0\nEPS3=0\nNTHREADS=2\nPROTOKOLL=Y");

    BgmnFileIO::writeTextFile(f, output);
}

void ModelManagerChernyshov::peakItemChanged(QTreeWidgetItem* current, QTreeWidgetItem*)
{
    if (!current) return;
    QUuid uid = current->data(0, Qt::UserRole).toUuid();
    if (!peakManagers.contains(uid)) return;
    peakManagers[uid]->updatePlot();
}

void ModelManagerChernyshov::peakDataChanged(QTreeWidgetItem* itm, int col)
{
    if (!itm) return;
    QUuid uid = itm->data(0, Qt::UserRole).toUuid();
    if (!peakManagers.contains(uid)) return;

    bool oldState = treeSupportPeaks->blockSignals(true);
    ParameterStorage *_peakParams = peakManagers[uid]->getParameters();
    double _value = itm->text(col).toDouble();

    switch (col) {
    case 0:
        break;
    case 1:
        _peakParams->setValue(synchro::TWOTHETA_DEG, _value);
        break;
    case 2:
        _peakParams->setValue(synchro::PROFILE_FWHM_DEG, _value);
        _value = qDegreesToRadians(_value);
        _peakParams->setValue(synchro::PROFILE_FWHM_RAD, _value);
        break;
    case 3:
        _peakParams->setValue(synchro::CHERNYSHOV_SHAPE, _value);
        break;
    default:
        break;
    }

    itm->setData(col, Qt::UserRole, _value);
    peakManagers[uid]->generatePeak();
    treeSupportPeaks->blockSignals(oldState);

    updateFwhmPeakPlot();
}

QList<double> ModelManagerChernyshov::calculatePeakPositions(double start, double end, double interval)
{
    QList<double> out;
    if (qFuzzyIsNull(interval)) return out;
    if (interval < 0.0)         return out;

    out << start;
    double d = 0.0;

    while (d < end) {
        if (d > start) out << d;
        d += interval;
    }

    if ((out.constLast() < end) && !qFuzzyCompare(out.constLast(), end)) out << end;
    return out;
}

int ModelManagerChernyshov::getSupportPeakData(QList<double> &ttDeg, QList<double> &fwhmRad, QList<double> &shape)
{
    ttDeg.clear();
    fwhmRad.clear();
    shape.clear();

    for (int i = 0; i < treeSupportPeaks->topLevelItemCount(); ++i) {
        ttDeg.append(treeSupportPeaks->topLevelItem(i)->data(1, Qt::UserRole).toDouble());
        fwhmRad.append(treeSupportPeaks->topLevelItem(i)->data(2, Qt::UserRole).toDouble());
        shape.append(treeSupportPeaks->topLevelItem(i)->data(3, Qt::UserRole).toDouble());
    }

    return treeSupportPeaks->topLevelItemCount();
}

void ModelManagerChernyshov::abortFit()
{
    fitAborted = true;
}

void ModelManagerChernyshov::rescaleFwhmAxes(bool x, bool y)
{
    double minX = std::numeric_limits<double>::max();
    double maxX = 0.0;
    double minY = 0.0;
    double maxY = 0.0;
    int validRanges = 0;

    for (int i = 0; i < plotFwhm->graphCount(); ++i) {
        if (!plotFwhm->graph(i)->data()->size()) continue;

        bool xok, yok;
        QCPRange xrange = plotFwhm->graph(i)->getKeyRange(xok);
        QCPRange yrange = plotFwhm->graph(i)->getValueRange(yok);

        if (xok && yok) ++validRanges;
        else            continue;

        minX = qMin(minX, xrange.lower);
        maxX = qMax(maxX, xrange.upper);
        maxY = qMax(maxY, yrange.upper);
    }

    if (validRanges <= 0) return;

    double rx = 0.025 * (maxX - minX);
    if (x) plotFwhm->xAxis->setRange(minX - rx, maxX + rx);
    if (y) plotFwhm->yAxis->setRange(minY, maxY * 1.025);
    plotFwhm->replot();
}

QList<synchro::SupportPeak> ModelManagerChernyshov::getSupportPeaks() const
{
    QList<synchro::SupportPeak> sPeaks;

    for (int i = 0; i < treeSupportPeaks->topLevelItemCount(); ++i) {
        synchro::SupportPeak supPeak;
        QTreeWidgetItem *itm = treeSupportPeaks->topLevelItem(i);
        if (!itm) continue;

        supPeak.number = itm->text(0).toInt();
        supPeak.position = itm->data(1, Qt::UserRole).toDouble();
        supPeak.fwhm = qRadiansToDegrees(itm->data(2, Qt::UserRole).toDouble());
        supPeak.shape = itm->data(3, Qt::UserRole).toDouble();

        sPeaks.append(supPeak);
    }

    return sPeaks;
}
void ModelManagerChernyshov::setSupportPeaks(const QList<synchro::SupportPeak> &l)
{
    QList<double> ttAngles;
    QList<double> hwhmDeg;
    QList<double> sh;
    double meanSh = 0.0;

    for (int i = 0; i < l.size(); ++i) {
        ttAngles.append(l.at(i).position);
        hwhmDeg.append(l.at(i).fwhm / 2.0);
        sh.append(l.at(i).shape);
        meanSh += l.at(i).shape;
    }

    meanSh /= double(l.size());
    generateSupportPeaks(ttAngles, hwhmDeg, sh, meanSh);
}

QList<synchro::Profile> ModelManagerChernyshov::getProfiles() const
{
    QMap<int, synchro::Profile> profiles;

    for (auto i = peakManagers.cbegin(); i != peakManagers.cend(); ++i) {
        ParameterStorage *_peakParams = i.value()->getParameters();

        int    num = _peakParams->getParameter(synchro::PROFILE_NUMBER, -1);
        double pos = _peakParams->getParameter(synchro::TWOTHETA_DEG, -1.0);
        double fwhm = _peakParams->getParameter(synchro::PROFILE_FWHM_DEG, -1.0);
        double shape = _peakParams->getParameter(synchro::CHERNYSHOV_SHAPE, -1.0);
        double area = _peakParams->getParameter(synchro::PROFILE_AREA, -1.0);

        QList<double> pX, pY, cX, cY;
        if (settings->value("Data/saveCurveData", false).toBool()) {
            i.value()->getRawProfilePv(pX, pY);
            i.value()->getRawProfileConv(cX, cY);
        }

        LorentzParams l2params = i.value()->getL2CurveParameters();
        QList<synchro::L2Curve> l2curves;

        for (int j = 0; j < l2params.size(); ++j) {
            l2curves.append(synchro::L2Curve(l2params.at(j).g(), l2params.at(j).e(), l2params.at(j).q()));
        }

        profiles[num] = synchro::Profile(num, pos, fwhm, shape, area, pX, pY, cX, cY, l2curves);
    }

    return profiles.values();
}

void ModelManagerChernyshov::setProfiles(const QList<synchro::Profile> &l)
{
    peakManagers.clear();
    treeModelPeaks->clear();
    peakFitPlotter->reset();

    bool oldState = treeModelPeaks->blockSignals(true);

    for (int i = 0; i < l.size(); ++i) {
        const synchro::Profile profile = l.at(i);
        double fwhmDeg = profile.fwhm;
        double fwhmRad = qDegreesToRadians(fwhmDeg);

        PeakManagerChernyshov *pManager = new PeakManagerChernyshov(peakFitPlotter);
        QUuid uid = pManager->uid();

        peakManagers[uid] = pManager;
        connect(peakManagers[uid], SIGNAL(fitProcessCompleted(QUuid)), this, SLOT(fitCompleted(QUuid)));

        // set default parameters and erase potential old values
        peakManagers[uid]->updateParameters(parameterStorage);
        peakManagers[uid]->getParameters()->setValue(synchro::TWOTHETA_DEG, profile.position);
        peakManagers[uid]->getParameters()->setValue(synchro::PROFILE_FWHM_DEG, fwhmDeg);
        peakManagers[uid]->getParameters()->setValue(synchro::PROFILE_FWHM_RAD, fwhmRad);
        peakManagers[uid]->getParameters()->setValue(synchro::PROFILE_AREA, profile.area);

        peakManagers[uid]->setRawProfiles(profile.pseudoVoigtX, profile.pseudoVoigtY,
                                         QList<double>(), QList<double>(),
                                         profile.convolvedX, profile.convolvedY);

        LorentzParams lps;

        for (int j = 0; j < profile.l2curves.size(); ++j) {
            synchro::L2Curve l2c = profile.l2curves.at(j);
            LorentzParam lp(l2c.g, l2c.e, l2c.q);
            lps.append(lp);
        }

        peakManagers[uid]->setL2CurveParameters(lps);

        QTreeWidgetItem *itm = new QTreeWidgetItem;
        itm->setFlags(itm->flags() | Qt::ItemIsEditable);
        itm->setText(0, QString::number(i + 1));
        itm->setText(1, QString::number(profile.position)); // in degrees
        itm->setText(2, QString::number(fwhmDeg));          // in degrees
        itm->setText(3, QString::number(profile.shape));
        itm->setText(4, QString::number(profile.area));

        itm->setData(0, Qt::UserRole, peakManagers[uid]->uid());
        itm->setData(1, Qt::UserRole, profile.position); // in degrees
        itm->setData(2, Qt::UserRole, fwhmRad);        // in radians
        itm->setData(3, Qt::UserRole, profile.shape);
        itm->setData(4, Qt::UserRole, profile.area);

        treeModelPeaks->addTopLevelItem(itm);
    }

    treeModelPeaks->blockSignals(oldState);

    if (treeModelPeaks->topLevelItemCount()) {
        treeModelPeaks->setCurrentItem(treeModelPeaks->topLevelItem(0));
    }
}

bool ModelManagerChernyshov::hasAllFittedCurves() const
{
    QMapIterator<QUuid, PeakManagerChernyshov*> iter(peakManagers);

    while (iter.hasNext()) {
        iter.next();
        LorentzParams lp = iter.value()->getL2CurveParameters();
        if (!lp.size()) return false;
    }

    return true;
}
