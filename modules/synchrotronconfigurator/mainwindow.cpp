/***************************************************************************
                          mainwindow.cpp  -  description
                             -------------------
    begin                : Wed Dec 11 18:00:00 CEST 2024
    copyright            : (C) 2024 by Nicola Doebelin
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

#include "mainwindow.h"
#include "structssc.h"
#include "ui_mainwindow.h"

#include <QGuiApplication>
#include <QMainWindow>
#include <QCloseEvent>
#include <QDir>
#include <QFileInfo>
#include <QFileDialog>
#include <QSvgWidget>
#include <QSvgRenderer>
#include "helpaboutdialog.h"
#include "synchrotronpreferencesdialog.h"
#include "instrumentgraphicsview.h"
#include "synchrotronxmlio.h"
#include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/functions.h"
#include "../libXrdIO/absorptioncoefficientcalculator.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    settings = SettingsManager::getInstance();
    settings->initialize();

    QString iconTheme = settings->value("Gui/iconTheme", QString()).toString();

    if (iconTheme.isEmpty()) {
        if (settings->isDarkMode()) iconTheme = QString("profex-dark");
        else                        iconTheme = QString("profex-light-colored");
    }

    QIcon::setThemeName(iconTheme);

    ui->setupUi(this);
    setWindowTitle(QString("Profex Synchrotron Configurator"));

    bool dm = settings->isDarkMode();
    scene = new InstrumentScene(dm);
    if (dm) scene->setBackgroundBrush(QBrush(QGuiApplication::palette().color(QPalette::Base)));
    ui->graphicsViewInstrument->setScene(scene);

    labelStatus = new QLabel(statusBar());
    labelStatus->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    statusBar()->addPermanentWidget(labelStatus, 0);

    modelManagerChernyshov = new ModelManagerChernyshov(ui->plotFwhm,
                                                        ui->plotPeakFit,
                                                        ui->treeWidgetSupportPeaks,
                                                        ui->treeWidgetModelPeaks,
                                                        this);

    ui->actionFit_Fundamental_Parameters->setEnabled(false); // enable when support peaks were loaded

    ui->plotPeakFit->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->plotPeakFit->xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
    ui->plotPeakFit->xAxis->axisRect()->setRangeZoom(Qt::Horizontal);
    ui->plotPeakFit->setMouseTracking(true);
    ui->plotFwhm->setMouseTracking(true);

    initSettings();
    initTreeWidgetItems();
    initConnections();
}

MainWindow::~MainWindow()
{
    delete ui;
    settings->destroy();
}

bool MainWindow::doShowMaximized()
{
    return settings->value("Gui/maximized", false).toBool();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    saveSettings();
    settings->sync();
    e->accept();
}

void MainWindow::initSettings()
{
    restoreGeometry(settings->value("Gui/geometry", QByteArray()).toByteArray());
    ui->splitterConfig->restoreState(settings->value("Gui/splitterConfig", QByteArray()).toByteArray());
    ui->splitterPlots->restoreState(settings->value("Gui/splitterSim", QByteArray()).toByteArray());
    ui->splitter->restoreState(settings->value("Gui/splitterMain", QByteArray()).toByteArray());
    if (_workingDir.isEmpty()) _workingDir = settings->value("Gui/workingDir", QDir::homePath()).toString();

    bool expertOptions = settings->value("Data/showExpertOptions", false).toBool();
    ui->groupBoxDetectorExpertOptions->setHidden(!expertOptions);

    ui->doubleSpinBoxRangeStart->setValue(settings->value("Data/twoThetaStart", 1.0).toDouble());
    ui->doubleSpinBoxRangeEnd->setValue(settings->value("Data/twoThetaEnd", 60.0).toDouble());
    ui->doubleSpinBoxRangeInterval->setValue(settings->value("Data/twoThetaInterval", 5.0).toDouble());

    ui->doubleSpinBoxD->setValue(settings->value("Data/Chernyshov/paramD", 300.0).toDouble());
    ui->doubleSpinBoxP->setValue(settings->value("Data/Chernyshov/paramP", 0.172).toDouble());
    ui->doubleSpinBoxT->setValue(settings->value("Data/Chernyshov/paramT", 0.177).toDouble());
    ui->doubleSpinBoxC->setValue(settings->value("Data/Chernyshov/paramC", 0.2).toDouble());
    ui->doubleSpinBoxPhi->setValue(settings->value("Data/Chernyshov/paramPhi", 0.0).toDouble());
    ui->checkBoxPhiFocused->setChecked(settings->value("Data/Chernyshov/paramPhiFocused", false).toBool());
    ui->doubleSpinBoxDetectorTilt->setValue(settings->value("Data/Chernyshov/paramAlpha", 0.0).toDouble());

    ui->comboBoxPosCorrectionMode->addItem("None");
    ui->comboBoxPosCorrectionMode->addItem("exGaussian");
    ui->comboBoxPosCorrectionMode->addItem("Pseudo-Voigt Convolution");

    if (ui->groupBoxDetectorExpertOptions->isHidden()) {
        ui->comboBoxPosCorrectionMode->setCurrentIndex(0);
        ui->checkBoxTransparencyCutoff->setChecked(true);
    } else {
        ui->comboBoxPosCorrectionMode->setCurrentIndex(settings->value("Data/Chernyshov/positionalCorrectionMode", 1).toInt());
        ui->checkBoxTransparencyCutoff->setChecked(settings->value("Data/Chernyshov/transparencyCutoff", true).toBool());
    }

    ui->doubleSpinBoxWavelength->setValue(settings->value("Data/Chernyshov/wavelength", 0.01).toDouble());
    customLac = settings->value("Data/Chernyshov/customLac", 0.0).toDouble();
    ui->doubleSpinBoxPeakShape->setValue(settings->value("Data/Chernyshov/peakShape", 0.5).toDouble());

    ui->treeWidgetSupportPeaks->header()->restoreState(settings->value("Gui/treeSupportPeaksHeader", QByteArray()).toByteArray());
    ui->treeWidgetModelPeaks->header()->restoreState(settings->value("Gui/treeModelPeaksHeader", QByteArray()).toByteArray());

    ui->comboBoxGeometry->addItem("CAPILLARY");
    ui->comboBoxGeometry->addItem("TRANSMISSION");
    ui->comboBoxGeometry->setCurrentIndex(settings->value("Data/Geometry", 0).toInt());

    initDetectors();
    int curDetector = settings->value("Data/Chernyshov/currentDetector", 0).toInt();
    ui->comboBoxDetectorMaterial->setCurrentIndex(curDetector);

    bool darkMode = settings->isDarkMode();
    QColor bgCol(darkMode ? QGuiApplication::palette().color(QPalette::Base) : Qt::white);
    QPen penAxes(darkMode ? global::Functions::colorToDarkMode(Qt::black) : Qt::black);

    ui->plotFwhm->setBackground(QBrush(bgCol));
    ui->plotFwhm->legend->setBrush(bgCol);

    ui->plotFwhm->xAxis->setBasePen(penAxes);
    ui->plotFwhm->yAxis->setBasePen(penAxes);

    ui->plotFwhm->xAxis->setTickPen(penAxes);
    ui->plotFwhm->yAxis->setTickPen(penAxes);

    ui->plotFwhm->xAxis->setSubTickPen(penAxes);
    ui->plotFwhm->yAxis->setSubTickPen(penAxes);

    ui->plotFwhm->xAxis->setLabelColor(darkMode ? Qt::white : Qt::black);
    ui->plotFwhm->yAxis->setLabelColor(darkMode ? Qt::white : Qt::black);
    ui->plotFwhm->xAxis->setTickLabelColor(darkMode ? Qt::white : Qt::black);
    ui->plotFwhm->yAxis->setTickLabelColor(darkMode ? Qt::white : Qt::black);

    ui->plotPeakFit->setBackground(QBrush(bgCol));
    ui->plotPeakFit->legend->setBrush(bgCol);

    ui->plotPeakFit->xAxis->setBasePen(penAxes);
    ui->plotPeakFit->yAxis->setBasePen(penAxes);

    ui->plotPeakFit->xAxis->setTickPen(penAxes);
    ui->plotPeakFit->yAxis->setTickPen(penAxes);

    ui->plotPeakFit->xAxis->setSubTickPen(penAxes);
    ui->plotPeakFit->yAxis->setSubTickPen(penAxes);

    ui->plotPeakFit->xAxis->setLabelColor(darkMode ? Qt::white : Qt::black);
    ui->plotPeakFit->yAxis->setLabelColor(darkMode ? Qt::white : Qt::black);
    ui->plotPeakFit->xAxis->setTickLabelColor(darkMode ? Qt::white : Qt::black);
    ui->plotPeakFit->yAxis->setTickLabelColor(darkMode ? Qt::white : Qt::black);

    ui->textEditHelp->document()->setDefaultStyleSheet(BgmnFileIO::readTextFile("://resources/contextHelp.css"));
    updateAbsorptionCoefficient();
    updateProfileParameters();
    updateFwhmParametersChernyshov();
}

void MainWindow::initTreeWidgetItems()
{
    QTreeWidgetItem *itBlHeader = new QTreeWidgetItem(ui->treeWidgetModules, QStringList("Beamline"));
    itBlHeader->setData(0, Qt::UserRole, "BEAMLINE");
    configItemMap["BEAMLINE"] = itBlHeader;

    QTreeWidgetItem *itBlPrimary = new QTreeWidgetItem(itBlHeader, QStringList("Primary Beam"));
    itBlPrimary->setData(0, Qt::UserRole, "PRIMARYBEAM");
    configItemMap["PRIMARYBEAM"] = itBlPrimary;

    QTreeWidgetItem *itBlSample = new QTreeWidgetItem(itBlHeader, QStringList("Sample"));
    itBlSample->setData(0, Qt::UserRole, "SAMPLE");
    configItemMap["SAMPLE"] = itBlSample;

    QTreeWidgetItem *itBlDetector = new QTreeWidgetItem(itBlHeader, QStringList("Detector"));
    itBlDetector->setData(0, Qt::UserRole, "DETECTOR");
    configItemMap["DETECTOR"] = itBlDetector;

    QTreeWidgetItem *itBlSupportPeaks = new QTreeWidgetItem(itBlHeader, QStringList("Support Peaks"));
    itBlSupportPeaks->setData(0, Qt::UserRole, "SUPPORTPEAKS");
    configItemMap["SUPPORTPEAKS"] = itBlSupportPeaks;

    QTreeWidgetItem *itProfileHeader = new QTreeWidgetItem(ui->treeWidgetModules, QStringList("Simulated Profiles"));
    itProfileHeader->setData(0, Qt::UserRole, "SIMULATEDPROFILES");
    configItemMap["SIMULATEDPROFILES"] = itProfileHeader;

    QTreeWidgetItem *itProfileRange = new QTreeWidgetItem(itProfileHeader, QStringList("Computation Range"));
    itProfileRange->setData(0, Qt::UserRole, "RANGE");
    configItemMap["RANGE"] = itProfileRange;

    QTreeWidgetItem *itProfileFits = new QTreeWidgetItem(itProfileHeader, QStringList("Profile Fit"));
    itProfileFits->setData(0, Qt::UserRole, "PROFILEFIT");
    configItemMap["PROFILEFIT"] = itProfileFits;

    ui->treeWidgetModules->expandAll();
}

void MainWindow::saveSettings()
{
    settings->setValue("Gui/geometry", saveGeometry());
    settings->setValue("Gui/splitterConfig", ui->splitterConfig->saveState());
    settings->setValue("Gui/splitterSim", ui->splitterPlots->saveState());
    settings->setValue("Gui/splitterMain", ui->splitter->saveState());
    settings->setValue("Gui/workingDir", _workingDir);

    settings->setValue("Data/twoThetaStart", ui->doubleSpinBoxRangeStart->value());
    settings->setValue("Data/twoThetaEnd", ui->doubleSpinBoxRangeEnd->value());
    settings->setValue("Data/twoThetaInterval", ui->doubleSpinBoxRangeInterval->value());
    settings->setValue("Data/Geometry", ui->comboBoxGeometry->currentIndex());

    settings->setValue("Data/Chernyshov/paramD", ui->doubleSpinBoxD->value());
    settings->setValue("Data/Chernyshov/paramP", ui->doubleSpinBoxP->value());
    settings->setValue("Data/Chernyshov/paramT", ui->doubleSpinBoxT->value());
    settings->setValue("Data/Chernyshov/paramC", ui->doubleSpinBoxC->value());
    settings->setValue("Data/Chernyshov/paramPhi", ui->doubleSpinBoxPhi->value());
    settings->setValue("Data/Chernyshov/paramPhiFocused", ui->checkBoxPhiFocused->isChecked());
    settings->setValue("Data/Chernyshov/paramAlpha", ui->doubleSpinBoxDetectorTilt->value());
    settings->setValue("Data/Chernyshov/wavelength", ui->doubleSpinBoxWavelength->value());
    settings->setValue("Data/Chernyshov/positionalCorrectionMode", ui->comboBoxPosCorrectionMode->currentIndex());
    settings->setValue("Data/Chernyshov/linAbsCoefficient", ui->doubleSpinBoxLAC->value());
    settings->setValue("Data/Chernyshov/customLac", customLac);
    settings->setValue("Data/Chernyshov/currentDetector", ui->comboBoxDetectorMaterial->currentIndex());
    settings->setValue("Data/Chernyshov/peakShape", ui->doubleSpinBoxPeakShape->value());
    settings->setValue("Data/Chernyshov/transparencyCutoff", ui->checkBoxTransparencyCutoff->isChecked());

    settings->setValue("Gui/treeSupportPeaksHeader", ui->treeWidgetSupportPeaks->header()->saveState());
    settings->setValue("Gui/treeModelPeaksHeader", ui->treeWidgetModelPeaks->header()->saveState());
}

void MainWindow::initConnections()
{
    connect(scene, SIGNAL(itemSelected(QString)), this, SLOT(instrumentItemSelected(QString)));

    connect(ui->doubleSpinBoxD,          SIGNAL(valueChanged(double)), this, SLOT(updateFwhmParametersChernyshov()));
    connect(ui->doubleSpinBoxP,          SIGNAL(valueChanged(double)), this, SLOT(updateFwhmParametersChernyshov()));
    connect(ui->doubleSpinBoxC,          SIGNAL(valueChanged(double)), this, SLOT(updateFwhmParametersChernyshov()));
    connect(ui->doubleSpinBoxT,          SIGNAL(valueChanged(double)), this, SLOT(updateFwhmParametersChernyshov()));
    connect(ui->doubleSpinBoxPhi,        SIGNAL(valueChanged(double)), this, SLOT(updateFwhmParametersChernyshov()));
    connect(ui->doubleSpinBoxRangeStart, SIGNAL(valueChanged(double)), this, SLOT(updateFwhmParametersChernyshov()));
    connect(ui->doubleSpinBoxRangeEnd,   SIGNAL(valueChanged(double)), this, SLOT(updateFwhmParametersChernyshov()));
    connect(ui->checkBoxPhiFocused,      SIGNAL(toggled(bool)),        this, SLOT(updateFwhmParametersChernyshov()));
    connect(ui->comboBoxGeometry,        SIGNAL(currentIndexChanged(int)), this, SLOT(updateFwhmParametersChernyshov()));
    connect(ui->doubleSpinBoxDetectorTilt, SIGNAL(valueChanged(double)), this, SLOT(detectorTiltChanged(double)));

    connect(ui->doubleSpinBoxPeakShape,     SIGNAL(valueChanged(double)), this, SLOT(updateProfileParameters()));
    connect(ui->doubleSpinBoxRangeInterval, SIGNAL(valueChanged(double)), this, SLOT(updateProfileParameters()));
    connect(ui->comboBoxPosCorrectionMode,  SIGNAL(currentIndexChanged(int)), this, SLOT(updateProfileParameters()));
    connect(ui->comboBoxPosCorrectionMode, SIGNAL(currentIndexChanged(int)), this, SLOT(toggleTransparencyCutoffEnabled(int)));
    connect(ui->checkBoxTransparencyCutoff, SIGNAL(toggled(bool)), this, SLOT(updateProfileParameters()));

    connect(ui->doubleSpinBoxRangeStart,    SIGNAL(valueChanged(double)), this, SLOT(updateRangeParameters()));
    connect(ui->doubleSpinBoxRangeEnd,      SIGNAL(valueChanged(double)), this, SLOT(updateRangeParameters()));

    connect(ui->comboBoxDetectorMaterial, SIGNAL(currentIndexChanged(int)), this, SLOT(updateAbsorptionCoefficient()));
    connect(ui->doubleSpinBoxWavelength,  SIGNAL(valueChanged(double)),     this, SLOT(updateAbsorptionCoefficient()));
    connect(ui->doubleSpinBoxLAC,         SIGNAL(valueChanged(double)),     this, SLOT(customAbsorptionCoefficient()));

    connect(ui->treeWidgetModules, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(raiseParametersPage(QTreeWidgetItem*,QTreeWidgetItem*)));

    connect(ui->plotFwhm, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(coordinatesFwhm(QMouseEvent*)));
    connect(ui->plotPeakFit, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(coordinatesProfile(QMouseEvent*)));

    if (modelManagerChernyshov) {
        connect(modelManagerChernyshov, SIGNAL(sigFitsComplete()), this, SLOT(fitComplete()));
    }
}

void MainWindow::initDetectors()
{
    int cur = ui->comboBoxDetectorMaterial->currentIndex();
    ui->comboBoxDetectorMaterial->clear();

    QList<QVariant> defaultDetectors;
    defaultDetectors << "Unknown" << -1.0 << "Si" << 2.3296 << "Cd Te" << 5.86;

    QList<QVariant> detectors = settings->value("Data/detectorList", defaultDetectors).toList();

    for (int i = 0; i < detectors.size() - 1; i += 2) {
        ui->comboBoxDetectorMaterial->addItem(detectors.at(i).toString(), detectors.at(i+1));
    }

    ui->comboBoxDetectorMaterial->setCurrentIndex(cur);
}

void MainWindow::loadFile()
{
    // the static function of QFileDialog was buggy for some reason.
    QStringList filters;
    filters << tr("Configuration File (*.PXSML *.pxsml)");

    QFileDialog fdlg(this, tr("Open Configuration File"), _workingDir);
    fdlg.setAcceptMode(QFileDialog::AcceptOpen);
    fdlg.setNameFilters(filters);
    fdlg.setFileMode(QFileDialog::ExistingFile);
    fdlg.setOption(QFileDialog::HideNameFilterDetails, false);

    if (fdlg.exec() != QDialog::Accepted) {
        return;
    }

    QStringList fn = fdlg.selectedFiles();
    if (fn.isEmpty()) return;

    QFileInfo fi(fn.constFirst());
    _configFileName = fi.absoluteFilePath();
    _workingDir = fi.absolutePath();
    qDebug() << QString("Synchrotron Configurator: MainWindow::loadFile(): Loading file %1").arg(_configFileName);
    setWindowTitle(QString("Profex Synchrotron Configurator - %1").arg(_configFileName));

    ParameterStorage *parameters = new ParameterStorage;
    QList<synchro::SupportPeak> supportPeaks;
    QList<synchro::Profile> profiles;

    SynchrotronXmlIO *xmlIo = new SynchrotronXmlIO(fi.absoluteFilePath());
    xmlIo->readFile(parameters, supportPeaks, profiles);
    delete xmlIo;

    for (auto child : ui->stackedWidgetParameters->findChildren<QWidget *>()) {
        child->blockSignals(true);
    }

    ui->doubleSpinBoxD->setValue(parameters->getParameter(synchro::CHERNYSHOV_D, -1.0));
    ui->checkBoxD->setChecked(parameters->getParameter(synchro::CHERNYSHOV_D_CHECKED, false));
    ui->doubleSpinBoxP->setValue(parameters->getParameter(synchro::CHERNYSHOV_P, -1.0));
    ui->checkBoxP->setChecked(parameters->getParameter(synchro::CHERNYSHOV_P_CHECKED, false));
    ui->doubleSpinBoxT->setValue(parameters->getParameter(synchro::CHERNYSHOV_T, -1.0));
    ui->checkBoxT->setChecked(parameters->getParameter(synchro::CHERNYSHOV_T_CHECKED, false));
    ui->doubleSpinBoxC->setValue(parameters->getParameter(synchro::CHERNYSHOV_C, -1.0));
    ui->checkBoxC->setChecked(parameters->getParameter(synchro::CHERNYSHOV_C_CHECKED, false));
    ui->doubleSpinBoxPhi->setValue(parameters->getParameter(synchro::CHERNYSHOV_PHI, -1.0));
    ui->checkBoxPhi->setChecked(parameters->getParameter(synchro::CHERNYSHOV_PHI_CHECKED, false));
    ui->checkBoxPhiFocused->setChecked(parameters->getParameter(synchro::CHERNYSHOV_PHI_FOCUSED, false));
    ui->doubleSpinBoxDetectorTilt->setValue(parameters->getParameter(synchro::CHERNYSHOV_ALPHA, 0.0));

    int posCorrMode = parameters->getParameter(synchro::POSITION_CORRECTION_MODE, 1);
    ui->comboBoxPosCorrectionMode->setCurrentIndex(posCorrMode);
    ui->doubleSpinBoxWavelength->setValue(parameters->getParameter(synchro::WAVELENGTH_NM, 0.01));
    ui->doubleSpinBoxPeakShape->setValue(parameters->getParameter(synchro::CHERNYSHOV_SHAPE, 0.5));
    ui->comboBoxDetectorMaterial->setCurrentText(parameters->getParameter(synchro::DETECTOR_MATERIAL, QString("unknown")));
    ui->doubleSpinBoxLAC->setValue(parameters->getParameter(synchro::DETECTOR_LAC_MM, 0.0) * 10.0);

    ui->checkBoxTransparencyCutoff->setEnabled(posCorrMode == 2);
    ui->checkBoxTransparencyCutoff->setChecked(parameters->getParameter(synchro::DETECTOR_TRANSPARENCY_CUTOFF, false));

    ui->doubleSpinBoxRangeStart->setValue(parameters->getParameter(synchro::RANGE_MEASURED_START, -1.0));
    ui->doubleSpinBoxRangeEnd->setValue(parameters->getParameter(synchro::RANGE_MEASURED_END, -1.0));
    ui->doubleSpinBoxRangeInterval->setValue(parameters->getParameter(synchro::RANGE_MEASURED_STEP, -1.0));

    ui->comboBoxGeometry->setCurrentText(parameters->getParameter(synchro::GEOMETRY, QString("CAPILLARY")));

    for (auto child : ui->stackedWidgetParameters->findChildren<QWidget *>()) {
        child->blockSignals(false);
    }

    modelManagerChernyshov->getParameters()->update(parameters);
    updateFwhmParametersChernyshov();
    modelManagerChernyshov->setSupportPeaks(supportPeaks);
    modelManagerChernyshov->setProfiles(profiles);

    if (!supportPeaks.isEmpty()) ui->actionFit_Fundamental_Parameters->setEnabled(true);
}

/*
 * reads a csv file with the following content (Pseudo-Voigt curve parameters):
 *
 * 2thetaDeg;hwhmDeg;shape
 *
 * splitter ; tabs and spaces are supported
 * comment lines starting with # and ! are supported
 */
void MainWindow::importPeakList()
{
    QString f = QFileDialog::getOpenFileName(this, tr("Import Peak List"), QDir::homePath(), "CSV Files (*.csv *.CSV)");
    if (f.isEmpty()) return;

    qDebug() << QString("Synchrotron Configurator: MainWindow::importPeakList(): Loading file %1").arg(f);

    QStringList content = BgmnFileIO::readTextFile(f).split("\n");
    QList<double> lTtDeg, lHwhmDeg, lShape;
    double meanShape = 0.0;

    static QRegularExpression rxSplit("[;\\s\\t]+");
    static QRegularExpression rxComment("^[#!]");

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).contains(rxComment)) continue;

        QStringList line = content.at(i).split(rxSplit);

        if (line.size() < 3) continue;
        lTtDeg.append(line.at(0).toDouble());
        lHwhmDeg.append(line.at(1).toDouble());
        lShape.append(line.at(2).toDouble());
        meanShape += lShape.constLast();
    }

    meanShape /= content.size() > 0 ? content.size() : 1.0;
    // if (!qFuzzyIsNull(meanShape)) ui->doubleSpinBoxPeakShape->setValue(meanShape);

    if (lTtDeg.size() > 0 && lHwhmDeg.size() > 0 && lShape.size() > 0) {
        modelManagerChernyshov->loadSupportPeaks(lTtDeg, lHwhmDeg, lShape, meanShape);
        ui->actionFit_Fundamental_Parameters->setEnabled(true);
        instrumentItemSelected("SUPPORTPEAKS");
    }
}

void MainWindow::saveAs()
{
    QString f = QFileDialog::getSaveFileName(this, tr("Save configuration"), QDir::homePath(), "Configuration files (*.pxsml *.PXSML)");
    if (f.isEmpty()) return;

    QFileInfo fi(f);
    if (fi.suffix().isEmpty()) {
        _configFileName = fi.absoluteFilePath() + ".pxsml";
    } else {
        _configFileName = fi.absoluteFilePath();
    }

    setWindowTitle(QString("Profex Synchrotron Configurator - %1").arg(_configFileName));
    qDebug() << QString("Synchrotron Configurator: MainWindow::saveAs(): Saving file %1").arg(_configFileName);

    SynchrotronXmlIO *xmlIo = new SynchrotronXmlIO(_configFileName);

    xmlIo->setParameters(modelManagerChernyshov->getParameters());
    xmlIo->setSupportPeaks(modelManagerChernyshov->getSupportPeaks());
    xmlIo->setProfiles(modelManagerChernyshov->getProfiles());
    xmlIo->writeFile();

    delete xmlIo;
}

void MainWindow::save()
{
    if (_configFileName.isEmpty()) {
        saveAs();
    } else {
        SynchrotronXmlIO *xmlIo = new SynchrotronXmlIO(_configFileName);

        xmlIo->setParameters(modelManagerChernyshov->getParameters());
        xmlIo->setSupportPeaks(modelManagerChernyshov->getSupportPeaks());
        xmlIo->setProfiles(modelManagerChernyshov->getProfiles());
        xmlIo->writeFile();

        delete xmlIo;
    }
}

void MainWindow::fitFundamentalParameters()
{
    updateFwhmParametersChernyshov();
    modelManagerChernyshov->fitFundamentalParameters();
    const ParameterStorage *_params = modelManagerChernyshov->getParameters();

    bool oldStateD = ui->doubleSpinBoxD->blockSignals(true);
    bool oldStateP = ui->doubleSpinBoxP->blockSignals(true);
    bool oldStateT = ui->doubleSpinBoxT->blockSignals(true);
    bool oldStateC = ui->doubleSpinBoxC->blockSignals(true);
    bool oldStatePhi = ui->doubleSpinBoxPhi->blockSignals(true);

    ui->doubleSpinBoxD->setValue(_params->getParameter(synchro::CHERNYSHOV_D, -1.0));
    ui->doubleSpinBoxP->setValue(_params->getParameter(synchro::CHERNYSHOV_P, -1.0));
    ui->doubleSpinBoxT->setValue(_params->getParameter(synchro::CHERNYSHOV_T, -1.0));
    ui->doubleSpinBoxC->setValue(_params->getParameter(synchro::CHERNYSHOV_C, -1.0));
    ui->doubleSpinBoxPhi->setValue(_params->getParameter(synchro::CHERNYSHOV_PHI, -1.0));

    ui->doubleSpinBoxD->blockSignals(oldStateD);
    ui->doubleSpinBoxP->blockSignals(oldStateP);
    ui->doubleSpinBoxT->blockSignals(oldStateT);
    ui->doubleSpinBoxC->blockSignals(oldStateC);
    ui->doubleSpinBoxPhi->blockSignals(oldStatePhi);

    updateFwhmParametersChernyshov();
}

void MainWindow::runProfileCalculation()
{
    save();
    generateAllProfiles();
    fitAllProfiles();
}

void MainWindow::generateAllProfiles()
{
    updateFwhmParametersChernyshov();
    int n = modelManagerChernyshov->addModelPeaks();

    if (n > 0) instrumentItemSelected("PROFILEFIT");
}

void MainWindow::fitAllProfiles()
{
    modelManagerChernyshov->fitAllProfiles();
    instrumentItemSelected("PROFILEFIT");
}

void MainWindow::saveGeqFile()
{
    QFileInfo fi(_configFileName);
    modelManagerChernyshov->saveOutputFiles(fi.absolutePath(), fi.completeBaseName());
}

void MainWindow::helpAbout()
{
    HelpAboutDialog *hdlg = new HelpAboutDialog(this);
    hdlg->setVersion(QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD));
    hdlg->setLogDestination(logDest);
    hdlg->exec();
    delete hdlg;
}

void MainWindow::updateFwhmParametersChernyshov()
{
    modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_D, ui->doubleSpinBoxD->value());
    modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_D_CHECKED, ui->checkBoxD->isChecked());
    modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_P, ui->doubleSpinBoxP->value());
    modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_P_CHECKED, ui->checkBoxP->isChecked());
    modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_C, ui->doubleSpinBoxC->value());
    modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_C_CHECKED, ui->checkBoxC->isChecked());
    modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_T, ui->doubleSpinBoxT->value());
    modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_T_CHECKED, ui->checkBoxT->isChecked());

    if (ui->checkBoxPhiFocused->isChecked()) {
        modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_PHI, -ui->doubleSpinBoxPhi->value());
        modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_PHI_CHECKED, false);
        modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_PHI_FOCUSED, true);
    } else {
        modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_PHI, ui->doubleSpinBoxPhi->value());
        modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_PHI_CHECKED, ui->checkBoxPhi->isChecked());
        modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_PHI_FOCUSED, false);
    }

    modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_ALPHA, ui->doubleSpinBoxDetectorTilt->value());
    modelManagerChernyshov->getParameters()->setValue(synchro::GEOMETRY, ui->comboBoxGeometry->currentText());

    modelManagerChernyshov->computeFwhm(); // realtime update of plot expected
}

void MainWindow::updateRangeParameters()
{
    updateProfileParameters();
    modelManagerChernyshov->computeFwhm(); // realtime update of plot expected
}

void MainWindow::updateProfileParameters()
{
    modelManagerChernyshov->getParameters()->setValue(synchro::RANGE_MEASURED_START, ui->doubleSpinBoxRangeStart->value());
    modelManagerChernyshov->getParameters()->setValue(synchro::RANGE_MEASURED_END, ui->doubleSpinBoxRangeEnd->value());
    modelManagerChernyshov->getParameters()->setValue(synchro::RANGE_MEASURED_STEP, ui->doubleSpinBoxRangeInterval->value());

    modelManagerChernyshov->getParameters()->setValue(synchro::CHERNYSHOV_SHAPE, ui->doubleSpinBoxPeakShape->value());
    modelManagerChernyshov->getParameters()->setValue(synchro::POSITION_CORRECTION_MODE, ui->comboBoxPosCorrectionMode->currentIndex());
    modelManagerChernyshov->getParameters()->setValue(synchro::DETECTOR_TRANSPARENCY_CUTOFF, ui->checkBoxTransparencyCutoff->isChecked());

    double profileRange = settings->value("Data/profileRange", 25.0).toDouble();
    modelManagerChernyshov->getParameters()->setValue(synchro::RANGE_PROFILE_WFACTOR, profileRange);
}

void MainWindow::updateAbsorptionCoefficient()
{
    double lac = customLac;
    double density = ui->comboBoxDetectorMaterial->currentData(Qt::UserRole).toDouble();

    if (density > 0.0) {
        QString material = ui->comboBoxDetectorMaterial->currentText();
        double wavelength = ui->doubleSpinBoxWavelength->value();

        lac = AbsorptionCoefficientCalculator::getLacFromFormula(material, wavelength, density);

        // just in case something went wrong with LAC calculation, fall back
        if (lac < 0.0) lac = customLac;
    }

    bool oldState = ui->doubleSpinBoxLAC->blockSignals(true);
    ui->doubleSpinBoxLAC->setValue(lac);
    ui->doubleSpinBoxLAC->blockSignals(oldState);

    modelManagerChernyshov->getParameters()->setValue(synchro::WAVELENGTH_NM, ui->doubleSpinBoxWavelength->value());
    modelManagerChernyshov->getParameters()->setValue(synchro::DETECTOR_LAC_MM, 0.1 * lac);
    modelManagerChernyshov->getParameters()->setValue(synchro::DETECTOR_MATERIAL, ui->comboBoxDetectorMaterial->currentText());
    modelManagerChernyshov->getParameters()->setValue(synchro::DETECTOR_DENSITY, ui->comboBoxDetectorMaterial->currentData(Qt::UserRole).toDouble());
}

void MainWindow::customAbsorptionCoefficient()
{
    customLac = ui->doubleSpinBoxLAC->value();
    modelManagerChernyshov->getParameters()->setValue(synchro::DETECTOR_LAC_MM, 0.1 * customLac);
}

void MainWindow::preferences()
{
    SynchrotronPreferencesDialog *prefDlg = new SynchrotronPreferencesDialog(this);
    prefDlg->exec();
    delete prefDlg;

    if (settings->value("Data/showExpertOptions", false).toBool()) {
        ui->groupBoxDetectorExpertOptions->setHidden(false);
    } else {
        ui->groupBoxDetectorExpertOptions->setHidden(true);
        ui->comboBoxPosCorrectionMode->setCurrentIndex(0);
        ui->checkBoxTransparencyCutoff->setChecked(true);
    }

    initDetectors();
    updateProfileParameters();

    QTreeWidgetItem *cur = ui->treeWidgetModules->currentItem();
    if (cur) loadHelpText(cur->data(0, Qt::UserRole).toString());
}

void MainWindow::exportCurves()
{
    QString f = QFileDialog::getSaveFileName(this, tr("Export Curves"), QDir::homePath(), "CSV files (*.csv *.CSV)");
    if (f.isEmpty()) return;

    modelManagerChernyshov->exportCurves(f);
}

void MainWindow::raiseParametersPage(QTreeWidgetItem *cur, QTreeWidgetItem *)
{
    if (!cur) return;
    QString str = cur->data(0, Qt::UserRole).toString();
    bool oldState = scene->blockSignals(true);

    loadHelpText(str);

    if (str == "BEAMLINE") {
        ui->stackedWidgetParameters->setCurrentIndex(0);
        ui->tabWidgetPlots->setCurrentIndex(0);
        scene->setItemSelected(QString());
    } else if (str == "PRIMARYBEAM") {
        ui->stackedWidgetParameters->setCurrentIndex(1);
        ui->tabWidgetPlots->setCurrentIndex(0);
        scene->setItemSelected("PRIMARYBEAM");
    } else if (str == "SAMPLE") {
        ui->stackedWidgetParameters->setCurrentIndex(2);
        ui->tabWidgetPlots->setCurrentIndex(0);
        scene->setItemSelected("SAMPLE");
    } else if (str == "DETECTOR") {
        ui->stackedWidgetParameters->setCurrentIndex(3);
        ui->tabWidgetPlots->setCurrentIndex(0);
        scene->setItemSelected("DETECTOR");
    } else if (str == "SUPPORTPEAKS") {
        ui->stackedWidgetParameters->setCurrentIndex(4);
        ui->tabWidgetPlots->setCurrentIndex(0);
        scene->setItemSelected(QString());
    } else if (str == "SIMULATEDPROFILES") {
        ui->stackedWidgetParameters->setCurrentIndex(5);
        ui->tabWidgetPlots->setCurrentIndex(1);
        scene->setItemSelected(QString());
    } else if (str == "RANGE") {
        ui->stackedWidgetParameters->setCurrentIndex(6);
        ui->tabWidgetPlots->setCurrentIndex(1);
    } else if (str == "PROFILEFIT") {
        ui->stackedWidgetParameters->setCurrentIndex(7);
        ui->tabWidgetPlots->setCurrentIndex(1);
        scene->setItemSelected(QString());
    }

    scene->blockSignals(oldState);
}

void MainWindow::loadHelpText(const QString &str)
{
    bool expertOptions = settings->value("Data/showExpertOptions", false).toBool();
    QString help;
    QString file;

    if (str == "BEAMLINE") {
        // no help text for beamline parent item
    } else if (str == "PRIMARYBEAM") {
        file = ":/synchrotronconfigurator/helpFiles/primaryBeamConfig.html";
    } else if (str == "SAMPLE") {
        file = ":/synchrotronconfigurator/helpFiles/sampleConfig.html";
    } else if (str == "DETECTOR") {
        if (expertOptions) file = ":/synchrotronconfigurator/helpFiles/detectorConfigExpert.html";
        else               file = ":/synchrotronconfigurator/helpFiles/detectorConfig.html";
    } else if (str == "SUPPORTPEAKS") {
        file = ":/synchrotronconfigurator/helpFiles/supportPeaks.html";
    } else if (str == "SIMULATEDPROFILES") {
        // no help text for profile parent item
    } else if (str == "RANGE") {
        file = ":/synchrotronconfigurator/helpFiles/computationRange.html";
    } else if (str == "PROFILEFIT") {
        file = ":/synchrotronconfigurator/helpFiles/computedProfiles.html";
    }

    if (!file.isEmpty()) {
        help = BgmnFileIO::readTextFile(file);
    }

    ui->textEditHelp->setText(help);
}

void MainWindow::instrumentItemSelected(QString s)
{
    if (s.isEmpty() || !configItemMap.contains(s)) return;

    bool oldState = ui->treeWidgetModules->blockSignals(true);

    scene->setItemSelected(s);
    QTreeWidgetItem *itm = configItemMap.value(s);

    if (itm) {
        ui->treeWidgetModules->setCurrentItem(itm);
        raiseParametersPage(itm, nullptr);
    }

    ui->treeWidgetModules->blockSignals(oldState);
}

void MainWindow::fitComplete()
{
    saveGeqFile();
}

void MainWindow::coordinatesFwhm(QMouseEvent *event)
{
    double x = ui->plotFwhm->xAxis->pixelToCoord(event->pos().x());
    double y = ui->plotFwhm->yAxis->pixelToCoord(event->pos().y());

    if ((ui->plotFwhm->xAxis->range().contains(x))
        && (ui->plotFwhm->yAxis->range().contains(y))) {
        double tt = std::acos(std::sqrt(x)) * 180.0 / M_PI;
        double fw = std::sqrt(y) * 180.0 / M_PI;
        statusBar()->showMessage(QString("2%1 = %2%3 FWHM = %4%3").arg(global::theta).arg(tt, 0, 'f', 4).arg(global::degree).arg(fw, 0, 'f', 6));
    }
}

void MainWindow::coordinatesProfile(QMouseEvent *event)
{
    double x = ui->plotPeakFit->xAxis->pixelToCoord(event->pos().x());
    double y = ui->plotPeakFit->yAxis->pixelToCoord(event->pos().y());

    if ((ui->plotPeakFit->xAxis->range().contains(x))
        && (ui->plotPeakFit->yAxis->range().contains(y))) {
        statusBar()->showMessage(QString("2%1 = %2%3 I = %4").arg(global::theta).arg(x, 0, 'f', 4).arg(global::degree).arg(y, 0, 'f', 2));
    }

}

void MainWindow::toggleTransparencyCutoffEnabled(int n)
{
    ui->checkBoxTransparencyCutoff->setEnabled(n == 2);
}

void MainWindow::detectorTiltChanged(double d)
{
    scene->setDetectorTilt(d);
    updateFwhmParametersChernyshov();
}

/* EOF */
