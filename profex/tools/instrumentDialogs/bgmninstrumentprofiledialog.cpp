/***************************************************************************
                          bgmninstrumentprofiledialog.cpp  -  description
                             -------------------
    begin                : Mon May 21 09:30:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#include "bgmninstrumentprofiledialog.h"
#include "ui_bgmninstrumentprofiledialog.h"
#include "imageresolutiondialog.h"
#include "../libXrdIO/bgmnfileio.h"

BgmnInstrumentProfileDialog::BgmnInstrumentProfileDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BgmnInstrumentProfileDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Peak shape function"));
    settings = SettingsManager::getInstance();

    lamDisplay = new BgmnLamDisplay(this);
    geqDisplay = new BgmnGeqDisplay(this);
    sampDisplay = new BgmnSampleDisplay(this);
    convDisplay = new BgmnConvolutionDisplay(this);

    QWidget *sampWidget = new QWidget(this);
    QGridLayout *layoutSamp = new QGridLayout(this);
    layoutSamp->setContentsMargins(0, 0, 0, 0);
    layoutSamp->addWidget(sampDisplay, 0, 0, 1, 1);

    QWidget *sampCtrlWidget = new QWidget(this);
    sampControlPanel = new Ui::SampDisplayForm;
    sampControlPanel->setupUi(sampCtrlWidget);
    layoutSamp->addWidget(sampCtrlWidget, 1, 0, 1, 1);

    sampWidget->setLayout(layoutSamp);

    sampControlPanel->doubleSpinBoxB1->setRange(0.0, 0.5);
    sampControlPanel->doubleSpinBoxk1->setRange(0.0, 1.0);
    sampControlPanel->doubleSpinBoxk2->setRange(0.0, 0.001);

    sampControlPanel->doubleSpinBoxB1->setDecimals(8);
    sampControlPanel->doubleSpinBoxk1->setDecimals(2);
    sampControlPanel->doubleSpinBoxk2->setDecimals(8);

    sampControlPanel->doubleSpinBoxB1->setSingleStep(0.0001);
    sampControlPanel->doubleSpinBoxk1->setSingleStep(0.1);
    sampControlPanel->doubleSpinBoxk2->setSingleStep(0.0000005);

    sampControlPanel->doubleSpinBoxB1->setValue(0.0);
    sampControlPanel->doubleSpinBoxk1->setValue(0.0);
    sampControlPanel->doubleSpinBoxk2->setValue(0.0);

    sampControlPanel->doubleSpinBoxB1->setToolTip(tr("Crystallite size broadening B1"));
    sampControlPanel->doubleSpinBoxk1->setToolTip(tr("Crystallite size distribution k1"));
    sampControlPanel->doubleSpinBoxk2->setToolTip(tr("Micro-strain k2"));

    sampDisplay->setB1(sampControlPanel->doubleSpinBoxB1->value());
    sampDisplay->setk1(sampControlPanel->doubleSpinBoxk1->value());
    sampDisplay->setk2(sampControlPanel->doubleSpinBoxk2->value());
    convDisplay->setB1(sampControlPanel->doubleSpinBoxB1->value());
    convDisplay->setk1(sampControlPanel->doubleSpinBoxk1->value());
    convDisplay->setk2(sampControlPanel->doubleSpinBoxk2->value());

    ui->tabWidgetMain->addTab(lamDisplay,  tr("Wavelength Distribution D"));
    ui->tabWidgetMain->addTab(geqDisplay,  tr("Instrumental Function G"));
    ui->tabWidgetMain->addTab(sampWidget,  tr("Sample Function P"));
    ui->tabWidgetMain->addTab(convDisplay, tr("Convolution D*G*P"));
    ui->tabWidgetMain->setTabEnabled(3, false);

    ui->sliderTwoTheta->setEnabled(false);
    ui->progressBar->hide();

    connect(convDisplay, SIGNAL(setProgress(int)), ui->progressBar, SLOT(setValue(int)));
    connect(ui->tabWidgetMain, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    connect(ui->sliderTwoTheta, SIGNAL(valueChanged(int)), this, SLOT(twoThetaChanged(int)));

    connect(sampControlPanel->doubleSpinBoxB1, SIGNAL(valueChanged(double)), this, SLOT(b1Changed(double)));
    connect(sampControlPanel->doubleSpinBoxk1, SIGNAL(valueChanged(double)), this, SLOT(k1Changed(double)));
    connect(sampControlPanel->doubleSpinBoxk2, SIGNAL(valueChanged(double)), this, SLOT(k2Changed(double)));
}

BgmnInstrumentProfileDialog::~BgmnInstrumentProfileDialog()
{
    delete ui;
}

void BgmnInstrumentProfileDialog::loadFile()
{
    if (ui->tabWidgetMain->currentIndex() > 1) {
        return;
    }

    QString path;
    QString title;
    QString text;

    if (ui->tabWidgetMain->currentIndex() == 0) {
        path = settings->value("bgmnProject/bgmnExec", QDir::homePath()).toString();
        title = tr("Open wavelength distribution file");
        text = tr("Wavelength distribution file (*.lam *.LAM)");
    }

    if (ui->tabWidgetMain->currentIndex() == 1) {
        path = settings->value("bgmnProject/deviceDatabase", QStringList(QDir::homePath())).toStringList().first();
        title = tr("Open instrument configuration file");
        text = tr("Instrument configuration file (*.geq *.GEQ)");
    }

    QString f(QFileDialog::getOpenFileName(this, title, path, text));

    if (!QFile::exists(f)) return;

    if (ui->tabWidgetMain->currentIndex() == 0) {
        currentLamFile = f;
        lamDisplay->loadFile(currentLamFile);
        geqDisplay->clearData();
        sampDisplay->clearData();
        convDisplay->clearData();
        convDisplay->setLamFile(currentLamFile);
    }

    if (ui->tabWidgetMain->currentIndex() == 1) {
        currentGeqFile = f;
        geqDisplay->loadFile(currentGeqFile);
        sampDisplay->clearData();
        convDisplay->clearData();
        sampDisplay->setPeakPositionsTwoTheta(geqDisplay->peakPositions(), lamDisplay->getWavelength());
        convDisplay->setGeqFile(currentGeqFile);
        ui->sliderTwoTheta->setMaximum(geqDisplay->count() - 1);
        ui->progressBar->setMaximum(geqDisplay->count() - 1);
        ui->sliderTwoTheta->setEnabled(true);
    }

    if (lamDisplay->hasData() && geqDisplay->hasData() && sampDisplay->hasData()) {
        ui->tabWidgetMain->setTabEnabled(3, true);
    }
}

void BgmnInstrumentProfileDialog::reloadFile()
{
    qApp->setOverrideCursor(Qt::WaitCursor);

    if (ui->tabWidgetMain->currentIndex() == 0) {
        lamDisplay->reload();
        geqDisplay->clearData();
        sampDisplay->clearData();
        convDisplay->clearData();
    }

    if (ui->tabWidgetMain->currentIndex() == 1) {
        geqDisplay->reload();
        sampDisplay->clearData();
        convDisplay->clearData();
    }

    if (ui->tabWidgetMain->currentIndex() == 2) {
        sampDisplay->reload();
        convDisplay->clearData();
    }

    if (ui->tabWidgetMain->currentIndex() == 3) {
        ui->progressBar->reset();
        ui->progressBar->show();
        ui->sliderTwoTheta->hide();

        convDisplay->reload();

        ui->sliderTwoTheta->show();
        ui->progressBar->hide();
    }

    if (lamDisplay->hasData() && geqDisplay->hasData() && sampDisplay->hasData()) {
        ui->tabWidgetMain->setTabEnabled(3, true);
    }

    qApp->restoreOverrideCursor();
}

void BgmnInstrumentProfileDialog::exportCsv()
{
    QString fn = QFileDialog::getSaveFileName(this,
                                              tr("CSV File Name"),
                                              settings->value("config/workingdir", QDir::homePath()).toString(),
                                              tr("CSV File (*.csv *.CSV)"));

    if (fn.isEmpty()) return;

    QString out;

    if (ui->tabWidgetMain->currentIndex() == 0) out = lamDisplay->exportCsv(ui->sliderTwoTheta->value());
    if (ui->tabWidgetMain->currentIndex() == 1) out = geqDisplay->exportCsv(ui->sliderTwoTheta->value());
    if (ui->tabWidgetMain->currentIndex() == 2) out = sampDisplay->exportCsv(ui->sliderTwoTheta->value());
    if (ui->tabWidgetMain->currentIndex() == 3) out = convDisplay->exportCsv(ui->sliderTwoTheta->value());

    BgmnFileIO::writeTextFile(fn, out);
}

void BgmnInstrumentProfileDialog::exportPdf()
{
    QString fn = QFileDialog::getSaveFileName(this,
                                              tr("PDF File Name"),
                                              settings->value("config/workingdir", QDir::homePath()).toString(),
                                              tr("PDF File (*.pdf *.PDF)"));

    if (fn.isEmpty()) return;

    if (ui->tabWidgetMain->currentIndex() == 0) lamDisplay->exportPdf(fn);
    if (ui->tabWidgetMain->currentIndex() == 1) geqDisplay->exportPdf(fn);
    if (ui->tabWidgetMain->currentIndex() == 2) sampDisplay->exportPdf(fn);
    if (ui->tabWidgetMain->currentIndex() == 3) convDisplay->exportPdf(fn);
}

void BgmnInstrumentProfileDialog::exportPng()
{
    QString fn = QFileDialog::getSaveFileName(this,
                                              tr("PNG File Name"),
                                              settings->value("config/workingdir", QDir::homePath()).toString(),
                                              tr("PNG File (*.png *.PNG)"));

    if (fn.isEmpty()) return;

    int rasterW = settings->value("graph/rasterResolutionWidth", 1536).toInt();
    int rasterH = settings->value("graph/rasterResolutionHeight", 1024).toInt();

    ImageResolutionDialog *irdlg = new ImageResolutionDialog(this);

    irdlg->setPixelWidth(rasterW);
    irdlg->setPixelHeight(rasterH);

    if (irdlg->exec() == QDialog::Accepted) {
        rasterW = irdlg->pixelWidth();
        rasterH = irdlg->pixelHeight();
        delete irdlg;
    } else {
        delete irdlg;
        return;
    }

    if (ui->tabWidgetMain->currentIndex() == 0) lamDisplay->exportPng(fn, rasterW, rasterH);
    if (ui->tabWidgetMain->currentIndex() == 1) geqDisplay->exportPng(fn, rasterW, rasterH);
    if (ui->tabWidgetMain->currentIndex() == 2) sampDisplay->exportPng(fn, rasterW, rasterH);
    if (ui->tabWidgetMain->currentIndex() == 3) convDisplay->exportPng(fn, rasterW, rasterH);
}

void BgmnInstrumentProfileDialog::tabChanged(int i)
{
    if (i == 0) {
        ui->toolButtonLoadFile->setEnabled(true);
    }

    if (i == 1) {
        ui->toolButtonLoadFile->setEnabled(true);
    }

    if (i == 2) {
        ui->toolButtonLoadFile->setEnabled(false);
    }
}

void BgmnInstrumentProfileDialog::twoThetaChanged(int i)
{
    geqDisplay->displayCurve(i);
    sampDisplay->displayCurve(i);
    convDisplay->displayCurve(i);
}

void BgmnInstrumentProfileDialog::toggleSubCurves(bool b)
{
    lamDisplay->toggleSubCurves(b);
    geqDisplay->toggleSubCurves(b);
    sampDisplay->toggleSubCurves(b);
    convDisplay->toggleSubCurves(b);
}

void BgmnInstrumentProfileDialog::b1Changed(double d)
{
    sampDisplay->setB1(d);
    convDisplay->setB1(d);
}

void BgmnInstrumentProfileDialog::k1Changed(double d)
{
    sampDisplay->setk1(d);
    convDisplay->setk1(d);
}

void BgmnInstrumentProfileDialog::k2Changed(double d)
{
    sampDisplay->setk2(d);
    convDisplay->setk2(d);
}
