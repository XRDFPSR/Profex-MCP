/***************************************************************************
                          prefpagereferencelines.cpp  -  description
                             -------------------
    begin                : Wed May 10 10:00:00 CEST 2017
    copyright            : (C) 2017 by Nicola Doebelin
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

#include "../../libXrdIO/functions.h"
#include "prefpagereferencelines.h"
#include "ui_prefpagereferencelines.h"

#include <QFile>
#include <QDir>
#include <QMessageBox>
#include <QColorDialog>
#include <QStandardPaths>
#include <QFileDialog>

PrefPageReferenceLines::PrefPageReferenceLines(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageReferenceLines)
{
    ui->setupUi(this);
}

PrefPageReferenceLines::~PrefPageReferenceLines()
{
    delete ui;
}

void PrefPageReferenceLines::initUi()
{
    refStrManager = BgmnRefStructureManager::getInstance();
    ui->pushButtonClearHklBuffer->setText(tr("Clear Buffer"));
    ui->pushButtonClearHklBuffer->setEnabled(true);
    ui->comboBoxSymbol->addItem(tr("None"));
    ui->comboBoxSymbol->addItem(tr("Line"));
    ui->comboBoxSymbol->addItem(tr("Circle"));
    ui->labelResetInstructions->clear();

    ui->comboBoxWavelength->showKa2(false);
    ui->comboBoxWavelength->showKb(false);
    ui->comboBoxWavelength->initData();

    ui->comboBoxHklBase->addItem(tr("Zero Line"));
    ui->comboBoxHklBase->addItem(tr("Background Curve"));
    ui->comboBoxHklBase->addItem(tr("Difference Curve"));

    ui->comboBoxHklPlotBase->addItem(tr("Zero Line"));
    ui->comboBoxHklPlotBase->addItem(tr("Background Curve"));

    initSettings();
    initConnections();
}

void PrefPageReferenceLines::initSettings()
{
    ui->comboBoxWavelength->setCurrentIndex(settings->value("config/hklIndexWaveLength", 4).toInt());
    ui->checkBoxAutoIndexHkl->setChecked(settings->value("config/indexHkl", 0).toInt() > 0 ? true : false);
    ui->checkBoxAutoSelRefPhase->setChecked(settings->value("config/autoSelRefPhase", false).toBool());

    // the default corresponds to 2theta = 60° for CuKa radiation (yes d happens to be exactly the same value as the WL)
    double ulD = settings->value("config/hklUpperRangeD", 0.1540598).toDouble();
    double wlD = ui->comboBoxWavelength->currentData(Qt::UserRole).toDouble();
    ui->doubleSpinBoxHklUpperRange->setValue(global::Functions::dToTwoTheta(ulD, wlD));

    hklLineCol = QColor(settings->value("graph/hklLineColor", "#009900").toString());
    ui->pushButtonHklLineColor->setStyleSheet(QString("background-color:%1").arg(hklLineCol.name()));
    ui->comboBoxHklBase->setCurrentIndex(settings->value("graph/hklOnBackground", 1).toInt());
    ui->comboBoxSymbol->setCurrentIndex(settings->value("graph/hklSymbol", 0).toInt());

    ui->comboBoxHklPlotBase->setCurrentIndex(settings->value("graph/hklScanOnBackground", 0).toInt());

    ui->lineEditBufferFile->setText(settings->getAppDataLocation());
    ui->lineEditTempFile->setText(settings->getTempLocation());

    ui->spinBoxNStrongestPeaks->setValue(settings->value("bgmnProject/doubleClickNearestPeaks", 1).toInt());
    ui->spinBoxNMatches->setValue(settings->value("bgmnProject/doubleClickNearestMatches", 5).toInt());

    int t = settings->value("config/strucIndexingTimer", 30).toInt();
    ui->spinBoxKillTimer->setValue(t > 0 ? t : 30);
    ui->checkBoxUseKillTimer->setChecked(t > 0);
    ui->spinBoxKillTimer->setEnabled(t > 0);
}

void PrefPageReferenceLines::saveSettings()
{
    settings->setValue("config/hklIndexWaveLength", ui->comboBoxWavelength->currentIndex());
    settings->setValue("config/indexHkl", ui->checkBoxAutoIndexHkl->isChecked() ? 1 : 0);
    settings->setValue("config/autoSelRefPhase", ui->checkBoxAutoSelRefPhase->isChecked());

    double wlD = ui->comboBoxWavelength->currentData(Qt::UserRole).toDouble();
    double ulD = global::Functions::twoThetaToD(ui->doubleSpinBoxHklUpperRange->value(), wlD);
    settings->setValue("config/hklUpperRangeD", ulD);
    settings->setValue("config/hklUpperRange", ui->doubleSpinBoxHklUpperRange->value()); // we still need the value in °2theta

    settings->setValue("graph/hklOnBackground", ui->comboBoxHklBase->currentIndex());
    settings->setValue("graph/hklLineColor", hklLineCol.name());
    settings->setValue("graph/hklSymbol", ui->comboBoxSymbol->currentIndex());

    settings->setValue("graph/hklScanOnBackground", ui->comboBoxHklPlotBase->currentIndex());

    settings->setValue("config/hklBufferFile", ui->lineEditBufferFile->text());
    settings->setValue("config/tempFileLocation", ui->lineEditTempFile->text());

    settings->setValue("bgmnProject/doubleClickNearestPeaks", ui->spinBoxNStrongestPeaks->value());
    settings->setValue("bgmnProject/doubleClickNearestMatches", ui->spinBoxNMatches->value());

    settings->setValue("config/strucIndexingTimer", ui->checkBoxUseKillTimer->isChecked() ? ui->spinBoxKillTimer->value() : 0);
}

void PrefPageReferenceLines::initConnections()
{
    connect(ui->pushButtonClearHklBuffer, SIGNAL(clicked(bool)), this, SLOT(clearHklBuffer()));
    connect(ui->pushButtonHklLineColor, SIGNAL(clicked(bool)), this, SLOT(selectHklLineColor()));
    connect(ui->pushButtonBufferFile, SIGNAL(clicked(bool)), this, SLOT(selectBufferFileLocation()));
    connect(ui->pushButtonTempFile, SIGNAL(clicked(bool)), this, SLOT(selectTempFileLocation()));
    connect(ui->doubleSpinBoxHklUpperRange, SIGNAL(valueChanged(double)), this, SLOT(showHklResetText()));
    connect(ui->comboBoxWavelength, SIGNAL(currentIndexChanged(int)), this, SLOT(showHklResetText()));
}

void PrefPageReferenceLines::clearHklBuffer()
{
    QFile f(settings->getAppDataLocation() + "/" + "hklbufferV5.db3");
    refStrManager->closeDb();

    if (f.remove()) {
        ui->pushButtonClearHklBuffer->setText(tr("Buffer Cleared"));
        ui->pushButtonClearHklBuffer->setEnabled(false);
        refStrManager->sync();
    } else {
        QMessageBox::information(this, QString("Clearing hkl buffer"), QString("Deleting file %1 failed.").arg(f.fileName()));
    }
}

void PrefPageReferenceLines::selectHklLineColor()
{
    QColor c(QColorDialog::getColor(hklLineCol, this, tr("hkl Line Color")));

    if (c.isValid()) {
        hklLineCol = c;
        ui->pushButtonHklLineColor->setStyleSheet(QString("background-color:%1").arg(hklLineCol.name()));
    }
}

void PrefPageReferenceLines::selectBufferFileLocation()
{
    QDir dir(QFileDialog::getExistingDirectory(this, tr("Buffer File Location"), ui->lineEditBufferFile->text()));
    if (!dir.exists()) return;

    ui->lineEditBufferFile->setText(dir.absolutePath());
}

void PrefPageReferenceLines::selectTempFileLocation()
{
    QDir dir(QFileDialog::getExistingDirectory(this, tr("Temporary File Location"), ui->lineEditTempFile->text()));
    if (!dir.exists()) return;

    ui->lineEditTempFile->setText(dir.absolutePath());
}

void PrefPageReferenceLines::showHklResetText()
{
    ui->labelResetInstructions->setText(tr("To apply the changes, click \"Clear Buffer\" now and run \"Tools -> Index Reference Structures\" after closing this dialog."));
}
