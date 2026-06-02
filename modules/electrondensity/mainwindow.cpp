/***************************************************************************
                          mainwindow.cpp  -  description
                             -------------------
    begin                : Mon Sep 22 09:00:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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
#include "ui_mainwindow.h"

#include <QGuiApplication>
#include <QMainWindow>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileDialog>
#include <QMutex>
#include <QSplitter>
#include <QMessageBox>

#include "threadfouriersynth.h"
#include "threadrenderemap.h"
#include "emapimporthandler.h"
#include "emapsettingsdialog.h"
#include "helpaboutdialog.h"

#include "../libXrdIO/colorMaps/lutgenerator.h"
#include "../libXrdIO/structs.h"
#include "../libXrdIO/bgmnfileio.h"

#include "math.h"

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

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
    setWindowTitle(QString("Profex Electron Density Maps"));

    comboBoxSynthesis = new QComboBox(this);
    comboBoxProjection = new QComboBox(this);
    comboBoxColorMap = new QComboBox(this);
    checkBoxAutoRange = new QCheckBox(tr("Automatic range"), this);
    doubleSpinBoxRange = new QDoubleSpinBox(this);

    comboBoxSynthesis->setToolTip(tr("Synthesis"));
    comboBoxProjection->setToolTip(tr("Projection"));
    comboBoxColorMap->setToolTip(tr("Color Map"));
    checkBoxAutoRange->setToolTip(tr("Set electron density range automatically"));
    doubleSpinBoxRange->setToolTip(tr("Range"));

    QAction *actSynth  = ui->parameterToolBar->addWidget(comboBoxSynthesis);
    QAction *actProjct = ui->parameterToolBar->addWidget(comboBoxProjection);
    QAction *actCmap   = ui->parameterToolBar->addWidget(comboBoxColorMap);
    QAction *actAutoRange = ui->parameterToolBar->addWidget(checkBoxAutoRange);
    QAction *actRange  = ui->parameterToolBar->addWidget(doubleSpinBoxRange);

    actSynth->setIconText(tr("Synthesis"));
    actProjct->setIconText(tr("Projection"));
    actCmap->setIconText(tr("Color Map"));
    actAutoRange->setIconText(tr("Automatic Range"));
    actRange->setIconText(tr("Range"));

    cell.a = 1.0;
    cell.b = 1.0;
    cell.c = 1.0;
    cell.alpha = 90.0;
    cell.beta  = 90.0;
    cell.gamma = 90.0;
    cell.volume = 1.0;

    dmin = 0.0; // minimum density for display
    dmax = 0.0; // maximum density for display
    dmaxStored = 0.0; // qMax(dmax, qAbs(dmin)) stored for automatic range
    activeThreads = 0;
    synthesis = UNDEFINED;
    workingDir = QString();

    LutGenerator lgen(256, true);
    lutList = lgen.getLuts();
    exportCoordinateDialog = nullptr;
    pdlg = nullptr;

    statusCoordinates = new QLabel(statusBar());
    statusCoordinates->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    statusCoordinates->setMinimumWidth(statusCoordinates->fontMetrics().horizontalAdvance("MMMMMMMMMM.MMM 000.00 MMM"));
    statusBar()->addPermanentWidget(statusCoordinates, 0);

    setAcceptDrops(true);

    initConnections();
    initGuiElements();
    initSettings();
}

MainWindow::~MainWindow()
{
    if (exportCoordinateDialog) delete exportCoordinateDialog;
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

void MainWindow::initGuiElements()
{
    comboBoxSynthesis->addItem("Fcalc");
    comboBoxSynthesis->addItem("Fobs");
    comboBoxSynthesis->addItem("Fobs - Fcalc");

    comboBoxProjection->addItem("AB");
    comboBoxProjection->addItem("AC");
    comboBoxProjection->addItem("BC");

    comboBoxColorMap->addItems(lutList.keys());

    doubleSpinBoxRange->setMinimum(0.0);
    doubleSpinBoxRange->setMaximum(99.9);
    doubleSpinBoxRange->setSingleStep(0.1);
    doubleSpinBoxRange->setDecimals(2);
    doubleSpinBoxRange->setValue(0.0);

    ui->drawingWidget2D->showDefaultText();
}

void MainWindow::initSettings()
{
    restoreGeometry(settings->value("Gui/geometry", QByteArray()).toByteArray());
    if (workingDir.isEmpty()) workingDir = settings->value("Gui/workingDir", QDir::homePath()).toString();
    nCpus = QThread::idealThreadCount();

    comboBoxSynthesis->setCurrentIndex(settings->value("Map/fourierType", 1).toInt());

    QString lut = settings->value("Map/lookupTable", QString()).toString();
    if (lut.isEmpty()) {
        comboBoxColorMap->setCurrentIndex(0);
        ui->scaleWidget2D->setLut(lutList.value(comboBoxColorMap->currentText()));
    } else {
        comboBoxColorMap->setCurrentText(lut);
        ui->scaleWidget2D->setLut(lutList.value(lut));
    }

    ui->drawingWidget2D->setImageSize(settings->value("Map/imageHeight", 768).toInt());

    projection = static_cast<projection_t>(settings->value("Map/projectionPlane", 0).toInt());
    comboBoxProjection->setCurrentIndex(static_cast<int>(projection));
    ui->drawingWidget2D->setProjection(projection);

    bool daxes = settings->value("Map/drawAxes", true).toBool();
    ui->actionDraw_Axes->setChecked(daxes);
    ui->drawingWidget2D->toggleAxes(daxes);

    bool datms = settings->value("Map/drawAtoms", true).toBool();
    ui->actionDraw_Atoms->setChecked(datms);
    ui->drawingWidget2D->toggleAtoms(datms);

    bool datlbls = settings->value("Map/drawAtomLabels", true).toBool();
    ui->actionDraw_Atom_Labels->setChecked(datlbls);
    ui->drawingWidget2D->toggleAtomLabels(datlbls);

    bool preRender = settings->value("Map/prerender", true).toBool();
    ui->actionPre_render_Images->setChecked(preRender);

    checkBoxAutoRange->setChecked(settings->value("Gui/autoRange", true).toBool());

    //ui->splitter->setStretchFactor(0, 1);
    //ui->splitter->setStretchFactor(1, 5);
    //ui->splitter->restoreState(settings->value("Gui/splitter", QByteArray()).toByteArray());
}

void MainWindow::initConnections()
{
    connect(ui->drawingWidget2D, SIGNAL(coordinates(float,float,float)), this, SLOT(coordinates(float,float,float)));
    connect(comboBoxProjection, SIGNAL(currentIndexChanged(int)), this, SLOT(setProjection(int)));
    connect(ui->sliderLevel, SIGNAL(valueChanged(int)), this, SLOT(currentLevelChanged(int)));
    connect(doubleSpinBoxRange, SIGNAL(valueChanged(double)), this, SLOT(dmaxChanged(double)));
    connect(comboBoxColorMap, SIGNAL(currentTextChanged(QString)), this, SLOT(lutChanged(QString)));
    connect(checkBoxAutoRange, SIGNAL(toggled(bool)), this, SLOT(autoRangeToggled(bool)));
}

void MainWindow::saveSettings()
{
    settings->setValue("Gui/geometry", saveGeometry());
    settings->setValue("Gui/workingDir", workingDir);

    settings->setValue("Map/fourierType", comboBoxSynthesis->currentIndex());
    settings->setValue("Map/lookupTable", comboBoxColorMap->currentText());

    settings->setValue("Map/projectionPlane", static_cast<int>(projection));
    settings->setValue("Map/drawAxes", ui->actionDraw_Axes->isChecked());
    settings->setValue("Map/drawAtoms", ui->actionDraw_Atoms->isChecked());
    settings->setValue("Map/drawAtomLabels", ui->actionDraw_Atom_Labels->isChecked());
    settings->setValue("Map/prerender", ui->actionPre_render_Images->isChecked());
    //settings->setValue("Gui/splitter", ui->splitter->saveState());
}

/*
 * drag and drop events to load fcf files
 */
void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasFormat("text/plain")) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    QList<QUrl> urls = e->mimeData()->urls();
    if (urls.size()) {
        setFileName(urls.at(0).toLocalFile());
        e->acceptProposedAction();
    }
}

void MainWindow::loadFile()
{
    QString s(QFileDialog::getOpenFileName(this,
                                           tr("FCF file"),
                                           workingDir,
                                           "FCF file (*.fcf *.FCF);;FOU file (*.fou *.FOU)"));

    if (!QFileInfo::exists(s)) {
        qDebug() << QString("MainWindow::fileLoad(): File does not exist: %1").arg(hklFileName.absoluteFilePath());
        return;
    }

    setFileName(s);
}

void MainWindow::saveCurrent()
{
    QString fn = QFileDialog::getSaveFileName(this, tr("Save image as..."), workingDir, "PNG (*.png *.PNG)");

    if (fn.isEmpty()) {
        return;
    }

    QImage img = ui->drawingWidget2D->getImage();
    QImage scl = ui->scaleWidget2D->getScale(int(img.height() / (1.02*1.02)));
    int gap = int(0.2 * float(scl.width()));
    QImage out(img.width() + scl.width() + gap, img.height() + 2, img.format());
    out.fill(Qt::white);

    QPainter p(&out);
    p.drawImage(QPoint(1 + int(0.02 * img.height()), 1 + int(0.02 * img.height())), scl);
    p.drawImage(QPoint(scl.width() + gap, 1), img);

    out.save(fn);
}

void MainWindow::saveAll()
{
    QString fn(QFileDialog::getSaveFileName(this, tr("Save All images as..."), workingDir, "PNG (*.png *.PNG)"));

    if (fn.isEmpty()) {
        return;
    }

    // block the GUI
    setGuiState(false);

    // set up the progress dialog
    if (!pdlg) {
        pdlg = new QProgressDialog(this);
        connect(pdlg, SIGNAL(canceled()), this, SLOT(abortCalculation()));
    }

    pdlg->reset();
    pdlg->setMinimumDuration(0);
    pdlg->setMaximum(ui->sliderLevel->maximum());
    pdlg->setLabelText("Saving buffered images to disk");
    pdlg->show();
    qApp->processEvents();

    // dissect the file name into path, basename, and suffix
    QFileInfo fi(fn);
    QString path = fi.absolutePath();
    QString bn = fi.completeBaseName();
    QString ext = fi.suffix().isEmpty() ? "png" : fi.suffix();
    int h = 0;

    // loop over all levels and try to get the pre-rendered image. If successful, save it, if not, store
    // the level number for rendering
    for (int i = 0; i <= ui->sliderLevel->maximum(); ++i) {
        QString fname = QString("%1/%2_%3.%4").arg(path).arg(bn).arg(i, 4, 10, QChar('0')).arg(ext);

        qDebug() << QString("Rendering and saving image %1").arg(fname);

        ui->drawingWidget2D->drawLevel(i);
        QImage img = ui->drawingWidget2D->getImage();
        img.save(fname);
        h = img.height();

        pdlg->setValue(i);
        qApp->processEvents();
    }

    QImage scl = ui->scaleWidget2D->getScale(h);
    QString fname = QString("%1/%2_%3.%4").arg(path).arg(bn).arg("scale").arg(ext);
    scl.save(fname);

    // hide the progress dialog and unblock the GUI
    pdlg->hide();
    pdlg->reset();
    setGuiState(true);
}

/*
 * set new file names for input files, either from a QFileDialog, or from drag&drop events,
 * or from a command line argument. Determines all necessary file names and calls
 * ::calculateMap()
 */
void MainWindow::setFileName(const QString &s)
{
    hklFileName = QFileInfo(s);
    workingDir = hklFileName.absolutePath();
    QString bName = workingDir + "/" + hklFileName.completeBaseName();

    QString mode = "UNKNOWN";
    if (hklFileName.suffix().toLower() == "fcf") mode = "BGMN";
    if (hklFileName.suffix().toLower() == "fou") mode = "FP";

    // bgmn output files
    if (mode == "BGMN") {
        cellFileName.setFile(bName + ".pdb"); // try this first
        cellFileName.setFile(bName + ".res"); // but prefer this
    }

    // fullprof output files
    if (mode == "FP") {
        cellFileName.setFile(bName + ".hkl"); // but prefer this
    }

    calculateMap();
}

/*
 * starts the calculation of the electron density map.
 * the fcf and res files must have been set before.
 *
 * This slot can also be called directly from the GUI to
 * repeat the calculation. If no file names have been set,
 * it will call ::fileLoad() first. Else it will just
 * re-read the files specified before.
 */
void MainWindow::calculateMap()
{
    // no file name yet? Call slot fileLoad()
    if (hklFileName.fileName().isEmpty()) {
        loadFile();
        return;
    }

    EmapImportHandler iHandler;
    atoms.clear();

    // read the data files from disk, return if something goes wrong
    if (hklFileName.suffix().toLower() == "fcf") {
        if (!iHandler.readFcf(hklFileName.absoluteFilePath(), cell)) return;

        if (cellFileName.suffix().toLower() == "res") {
            if (!iHandler.readRes(cellFileName.absoluteFilePath(), cell, atoms)) return;
        }

        if (cellFileName.suffix().toLower() == "pdb") {
            if (!iHandler.readPdb(cellFileName.absoluteFilePath(), cell)) return;
        }
    }

    // read the data files from disk, return if something goes wrong
    if (hklFileName.suffix().toLower() == "fou") {
        if (!iHandler.readFou(hklFileName.absoluteFilePath(), cell)) return;
        if (!iHandler.readHkl(cellFileName.absoluteFilePath(), cell)) return;
    }

    // check if there is hkl and unit cell data available
    if (!cell.hkl.size()) {
        qDebug() << QString("MainWindow::calculateMap(): No hkl data available");
        return;
    }

    if ((cell.a <= 0.0) || (cell.b <= 0.0) || (cell.c <= 0.0)) {
        qDebug() << QString("MainWindow::calculateMap(): No unit cell data available");
        return;
    }

    double res = settings->value("Map/resolution", 0.25).toDouble();

    levelsX = int(cell.a / res);
    levelsY = int(cell.b / res);
    levelsZ = int(cell.c / res);

    // create the transformation matrix from the unit cell
    mf2c = EMapDataHandler::createF2CMatrix(cell, projection);

    // disable the GUI
    setGuiState(false);

    // set the size of the vector array to the number of levels, filled with empty vectors
    vmap.clear();
    for (int i = 0; i < levelsZ; ++i) {
        vmap.append(QVector<QVector<float> >());
    }

    // reset some thread counting variables
    nextZLevel = 0;
    activeThreads = 0;
    abort = false;

    // reset the minimum and maximum electron values
    dmax = dmin = dmaxStored = 0.0;

    // start the timer for profiling
    startTime.start();

    // prepare and show the progress dialog
    if (!pdlg) {
        pdlg = new QProgressDialog(this);
        connect(pdlg, SIGNAL(canceled()), this, SLOT(abortCalculation()));
    }

    pdlg->setMinimumDuration(0);
    pdlg->setMaximum(levelsZ);
    pdlg->setValue(0);
    pdlg->setLabelText(QString("Computing electron density map"));
    pdlg->show();

    // start computation threads
    for (int i = 0; i < qMin(nCpus, levelsZ); ++i) {
        startFourierSynth();
    }
}

/*
 * creates a new thread to process one level (nextZLevel)
 */
void MainWindow::startFourierSynth()
{
    ThreadFourierSynth *fsThread = new ThreadFourierSynth(this);
    if (pdlg) pdlg->setValue(nextZLevel);

    connect(fsThread, SIGNAL(newDmax(float)), this, SLOT(newDmax(float)));
    connect(fsThread, SIGNAL(newDmin(float)), this, SLOT(newDmin(float)));
    connect(fsThread, SIGNAL(finished()), this, SLOT(fourierSyntThreadCompleted()));
    connect(fsThread, SIGNAL(finished()), fsThread, SLOT(deleteLater()));

    synthesis = static_cast<synthesis_t>(comboBoxSynthesis->currentIndex());  // 0 = Fcalc, 1 = Fobs, 2 = Fobs - Fcalc
    fsThread->setMode(synthesis);
    fsThread->setResolutionLevel(levelsX, levelsY, levelsZ, nextZLevel);
    fsThread->setVoxelMap(&vmap);
    fsThread->setHklList(cell.hkl);
    fsThread->setVolume(cell.volume);

    ++activeThreads;
    ++nextZLevel;
    fsThread->start();
}

/*
 * A fourier synthesis thread has terminated.
 * Checks if:
 *
 * - abort was called
 * - more levels have to be processed
 * - the electron density array is complete
 */
void MainWindow::fourierSyntThreadCompleted()
{
    --activeThreads;

    if (abort) {
        if (!activeThreads) {
            pdlg->hide();
            qDebug() << QString("All threads terminated");
        }

        setGuiState(true);
        return;
    }

    if (nextZLevel < levelsZ) {
        startFourierSynth();
    } else {
        if (!activeThreads) {
            qDebug() << QString("%1 seconds needed for fourier synthesis").arg(double(startTime.elapsed())/1000.0, 0, 'f', 2);

            if (pdlg) {
                pdlg->hide();
                pdlg->reset();
            }

            if (qFuzzyIsNull(doubleSpinBoxRange->value()) || checkBoxAutoRange->isChecked()) {
                double m = double(qMax(dmax, fabs(dmin)));
                doubleSpinBoxRange->setValue(m);
                doubleSpinBoxRange->setSingleStep(m < 0.5 ? 0.01 : 0.1);
            } else {
                double m = doubleSpinBoxRange->value();
                dmax = m;
                dmin = -m;
                doubleSpinBoxRange->setSingleStep(m < 0.5 ? 0.01 : 0.1);
            }

            renderImages();
        }
    }
}

void MainWindow::renderImages()
{
    ui->scaleWidget2D->setRange(dmin, dmax);
    ui->scaleWidget2D->setLut(lutList.value(comboBoxColorMap->currentText()));

    if (!EMapDataHandler::checkEMapSize(vmap)) {
        qDebug() << QString("MainWindow::renderImages(): EMap is empty, exiting.");
        setGuiState(true);
        return;
    }

    int levels = 0;
    if ((projection == AB_PLANE) || (projection == C_AXIS)) levels = levelsZ - 1;
    if ((projection == AC_PLANE) || (projection == B_AXIS)) levels = levelsY - 1;
    if ((projection == BC_PLANE) || (projection == A_AXIS)) levels = levelsX - 1;

    ui->sliderLevel->setMaximum(levels);
    ui->sliderLevel->setValue(qMin(ui->sliderLevel->value(), ui->sliderLevel->maximum()));

    setGuiState(false);

    bool preRender = ui->actionPre_render_Images->isChecked();

    ui->drawingWidget2D->setLut(lutList.value(comboBoxColorMap->currentText()), dmax, dmin);
    ui->drawingWidget2D->setProjection(projection);
    ui->drawingWidget2D->setVmap(&vmap);
    ui->drawingWidget2D->setImageSize(settings->value("Map/imageHeight", 768).toInt());
    ui->drawingWidget2D->setSuperSamplingFactor(settings->value("Map/superSampling", 1).toInt());
    ui->drawingWidget2D->setInterpolation(settings->value("Map/interpolation", 1).toInt());
    ui->drawingWidget2D->setMatrix(mf2c);
    ui->drawingWidget2D->setNCpus(nCpus);
    ui->drawingWidget2D->initLevel(ui->sliderLevel->value());
    ui->drawingWidget2D->setAtomList(atoms);

    if (preRender) {
        pdlg->setLabelText(QString("Rendering images"));
        pdlg->setMaximum(levels);
        pdlg->setValue(0);
        pdlg->show();

        for (int i = 0; i <= levels; ++i) {
            if (abort) break;
            ui->drawingWidget2D->preRenderImage(i, i == 0);
            pdlg->setValue(i);
            qApp->processEvents();
        }

        pdlg->hide();
    }

    ui->drawingWidget2D->drawLevel(ui->sliderLevel->value());
    ui->drawingWidget2D->zoomToContent();
    setGuiState(true);
}

void MainWindow::abortCalculation()
{
    abort = true;

    if (pdlg) {
        pdlg->setLabelText(QString("Waiting for threads to finish..."));
    }

    qDebug() << QString("Aborting...");
}

/*
 * a new maximum electron density was reported by one of the
 * fourier synth threads. Check if it is greater than the previous
 * one and if yes, store it in a global variable
 */
void MainWindow::newDmax(float f)
{
    dmax = qMax(dmax, f);
    dmaxStored = qMax(dmax, dmaxStored);
}

/*
 * a new minimum electron density was reported by one of the
 * fourier synth threads. Check if it is smaller than the previous
 * one and if yes, store it in a global variable
 */
void MainWindow::newDmin(float f)
{
    dmin = qMin(dmin, f);
    dmaxStored = qMax(qAbs(dmin), dmaxStored);
}

void MainWindow::dmaxChanged(double d)
{
    dmax = float(d);
    dmin = float(-d);
}

void MainWindow::currentLevelChanged(int i)
{
    if (vmap.size()) {
        ui->drawingWidget2D->drawLevel(i);
    }
}

void MainWindow::setProjection(int i)
{
    projection = static_cast<projection_t>(i);
    mf2c = EMapDataHandler::createF2CMatrix(cell, projection);
}

void MainWindow::lutChanged(QString s)
{
    ui->scaleWidget2D->setLut(lutList.value(s));
}

void MainWindow::coordinates(float x, float y, float z)
{
    statusCoordinates->setText(QString("x=%1 y=%2 z=%3").arg(x, 0, 'f', 4).arg(y, 0, 'f', 4).arg(z, 0, 'f', 4));
}

/*
 * b = false: disables the GUI during long calculations
 * b = true:  enables the GUI
 */
void MainWindow::setGuiState(bool b)
{
    ui->actionCalculate_Map->setEnabled(b);
    ui->actionLoad_File->setEnabled(b);
    ui->actionRender_Images->setEnabled(b);
    ui->actionSave_Current_Frame->setEnabled(b);
    ui->actionSave_All_Frames->setEnabled(b);

    if (!b) {
        qApp->setOverrideCursor(Qt::WaitCursor);
    } else {
        // a bit ugly, but the only way to make sure all
        // override cursors are reverted
        while (qApp->overrideCursor()) qApp->restoreOverrideCursor();
    }
}

void MainWindow::mousePressEvent(QMouseEvent *e)
{
    e->ignore();
}

void MainWindow::preferences()
{
    EmapSettingsDialog *sdlg = new EmapSettingsDialog(this);

    sdlg->setResolution(settings->value("Map/resolution", 0.25).toDouble());
    sdlg->setInterpolation(settings->value("Map/interpolation", 2).toInt());
    sdlg->setImageHeight(settings->value("Map/imageHeight", 768).toInt());
    sdlg->setSuperSampling(settings->value("Map/superSampling", 1).toInt());

    if (sdlg->exec() == QDialog::Accepted) {
        settings->setValue("Map/resolution", sdlg->getResolution());
        settings->setValue("Map/interpolation", sdlg->getInterpolation());
        settings->setValue("Map/imageHeight", sdlg->getImageHeight());
        settings->setValue("Map/superSampling", sdlg->getSuperSampling());
    }

    delete sdlg;
}

void MainWindow::helpAbout()
{
    HelpAboutDialog *hdlg = new HelpAboutDialog(this);
    hdlg->setVersion(QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD));
    hdlg->setLogDestination(logDest);
    hdlg->exec();
    delete hdlg;
}

void MainWindow::saveRawMap()
{
    if (!EMapDataHandler::checkEMapSize(vmap)) {
        QMessageBox::information(this, tr("Export map data"), tr("No electron density data available. Calculate the map first."));
        return;
    }

    if (!exportCoordinateDialog) {
        exportCoordinateDialog = new EMapExportCoordinateDialog(this);
    }

    if (exportCoordinateDialog->exec() != QDialog::Accepted) {
        return;
    }

    int dataMode = exportCoordinateDialog->dataMode();
    int coordMode = exportCoordinateDialog->coordinateMode();

    QStringList filters = QStringList() << QString(tr("CSV File (*.csv *.CSV)")) << QString(tr("GNUplot script (*.gpl *.GPL)"));
    QString selectedFilter;
    QString f = QFileDialog::getSaveFileName(this, tr("Export map data"), workingDir, filters.join(";;"), &selectedFilter);

    if (f.isEmpty()) return;

    if (dataMode == 0) exportRawMapVolume(coordMode, f, filters.indexOf(selectedFilter));
    else               exportRawMapLevels(coordMode, f, filters.indexOf(selectedFilter), projection, ui->sliderLevel->value());
}

void MainWindow::exportRawMapVolume(int mode, const QString &fname, int format)
{
    setGuiState(false);

    QString out;
    QString title;

    if      (synthesis == FOBS)  title = "Fobs";
    else if (synthesis == FCALC) title = "Fcalc";
    else if (synthesis == FDIFF) title = "Fobs - Fcalc";

    if (format == 0) { // CSV
        out += QString("# Exported by Profex\n");
        out += QString("# %1 synthesis of file %2\n").arg(title, hklFileName.fileName());
        out += QString("# %1 coordinates\n").arg(mode == 0 ? "Fractional" : "Cartesian");
        out += QString("# x y z e\n");
    } else if (format == 1) { // GNUplot
        out = QString("# Exported by Profex\n\nreset\nset termoption dashed\nset key opaque\nset autoscale\n");
        out += QString("set xtic auto\nset ytic auto\nset ztic auto\n");
        out += QString("set title \"%1 (%2 coordinates)\"\n").arg(hklFileName.fileName(), mode == 0 ? "Fractional" : "Cartesian");
        out += QString("set xlabel \"x\"\nset ylabel \"y\"\nset zlabel \"z\"\nset xyplane at 0\n");
        out += QString("splot \'-\' using 1:2:3:4 with points pt 7 ps 0.3 lc palette title '%1'\n").arg(title);
        out += QString("\n\n#inline data x y z e\n");
    }

    bool ok;
    if (mode == 0) out += EMapDataHandler::mapToCsvFractional(vmap, &ok);
    else           out += EMapDataHandler::mapToCsvCartesian(vmap, cell, &ok);

    if (ok) BgmnFileIO::writeTextFile(fname, out);
    setGuiState(true);
}

void MainWindow::exportRawMapLevels(int mode, const QString &fname, int format, projection_t prj, int lvl)
{
    setGuiState(false);
    QString out;
    QString title;

    if      (synthesis == FOBS)  title = "Fobs";
    else if (synthesis == FCALC) title = "Fcalc";
    else if (synthesis == FDIFF) title = "Fobs - Fcalc";

    if (format == 0) { // CSV
        out += QString("# Exported by Profex\n");
        out += QString("# %1 synthesis of file %2\n").arg(title, hklFileName.fileName());
        out += QString("# %1 coordinates\n").arg(mode == 0 ? "Fractional" : "Cartesian");
        if      (prj == AB_PLANE) out += QString("# x y e\n");
        else if (prj == AC_PLANE) out += QString("# x z e\n");
        else                      out += QString("# y z e\n");
    } else if (format == 1) { // GNUplot
        out = QString("# Exported by Profex\n\nreset\nset termoption dashed\nset key opaque\nset autoscale\n");
        out += QString("set xtic auto\nset ytic auto\nset ztic auto\n");
        out += QString("set title \"%1 (%2 coordinates)\"\n").arg(hklFileName.fileName(), mode == 0 ? "Fractional" : "Cartesian");

        if      (prj == AB_PLANE) out += QString("set xlabel \"x\"\nset ylabel \"y\"\nset zlabel \"e\"\nset xyplane at 0\n");
        else if (prj == AC_PLANE) out += QString("set xlabel \"x\"\nset ylabel \"z\"\nset zlabel \"e\"\nset xyplane at 0\n");
        else                      out += QString("set xlabel \"y\"\nset ylabel \"z\"\nset zlabel \"e\"\nset xyplane at 0\n");

        out += QString("splot \'-\' using 1:2:3 with points pt 7 ps 0.3 lc palette title '%1'\n").arg(title);
        if      (prj == AB_PLANE) out += QString("\n\n#inline data x y e\n");
        else if (prj == AC_PLANE) out += QString("\n\n#inline data x z e\n");
        else                      out += QString("\n\n#inline data y z e\n");
    }

    bool ok;
    if (mode == 0) out += EMapDataHandler::layerToCsvFractional(vmap, prj, lvl, &ok);
    else           out += EMapDataHandler::layerToCsvCartesian(vmap, cell, prj, lvl, &ok);

    if (ok) BgmnFileIO::writeTextFile(fname, out);

    setGuiState(true);
}

void MainWindow::autoRangeToggled(bool b)
{
    if (b) doubleSpinBoxRange->setValue(dmaxStored);
    doubleSpinBoxRange->setEnabled(!b);
}

/* EOF */




