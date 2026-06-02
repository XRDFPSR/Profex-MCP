/***************************************************************************
                          prefpagegraphaxes.cpp  -  description
                             -------------------
    begin                : Wed Apr 27 16:00:00 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#include "prefpagegraphaxes.h"
#include "ui_prefpagegraphaxes.h"

PrefPageGraphAxes::PrefPageGraphAxes(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageGraphAxes)
{
    ui->setupUi(this);

}

PrefPageGraphAxes::~PrefPageGraphAxes()
{
    delete ui;
}

void PrefPageGraphAxes::initUi()
{
    ui->comboBoxMultiScans->addItem(tr("Show individual Scans"));
    ui->comboBoxMultiScans->addItem(tr("Sum Scans"));
    ui->comboBoxMultiScans->addItem(tr("Average Scans"));

    ui->comboBoxYaxisUnit->addItem(tr("Counts"));
    ui->comboBoxYaxisUnit->addItem(tr("Counts per second"));

    ui->comboBoxXaxisUnit->addItem(tr("° 2theta"));
    ui->comboBoxXaxisUnit->addItem(tr("d spacing"));
    ui->comboBoxXaxisUnit->addItem(tr("Q"));

    ui->comboBoxYScaling->addItem(tr("Linear"));
    ui->comboBoxYScaling->addItem(tr("Square Root"));
    ui->comboBoxYScaling->addItem(tr("Log10"));

    initSettings();
}

void PrefPageGraphAxes::initSettings()
{
    ui->spinBoxDisplayLineWidth->setValue(settings->value("graph/lineWidth", 1).toInt());
    ui->spinBoxSymbolSize->setValue(settings->value("graph/crossSize", 1).toInt());

    ui->comboBoxXaxisUnit->setCurrentIndex(settings->value("graph/xAxisUnit", 0).toInt());
    ui->comboBoxYScaling->setCurrentIndex(settings->value("graph/yAxisScale", 0).toInt());
    ui->comboBoxYaxisUnit->setCurrentIndex(settings->value("graph/countsPerSecond", 0).toInt());

    ui->checkBoxMajorGridX->setChecked(settings->value("graph/showMajorGridLinesX", false).toBool());
    ui->checkBoxMinorGridX->setChecked(settings->value("graph/showMinorGridLinesX", false).toBool());
    ui->checkBoxMajorGridY->setChecked(settings->value("graph/showMajorGridLinesY", false).toBool());
    ui->checkBoxMinorGridY->setChecked(settings->value("graph/showMinorGridLinesY", false).toBool());
    ui->checkBoxDifferenceLabels->setChecked(settings->value("graph/differenceTickLabels", false).toBool());

    ui->comboBoxMultiScans->setCurrentIndex(settings->value("scans/multiScanMode", 0).toInt());
    ui->checkBoxWlInXaxis->setChecked(settings->value("graph/showWlInXaxisLabel", false).toBool());
}

void PrefPageGraphAxes::saveSettings()
{
    settings->setValue("graph/lineWidth", ui->spinBoxDisplayLineWidth->value());
    settings->setValue("graph/crossSize", ui->spinBoxSymbolSize->value());

    settings->setValue("graph/xAxisUnit", ui->comboBoxXaxisUnit->currentIndex());
    settings->setValue("graph/yAxisScale", ui->comboBoxYScaling->currentIndex());

    settings->setValue("graph/countsPerSecond", ui->comboBoxYaxisUnit->currentIndex());
    settings->setValue("graph/showMajorGridLinesX", ui->checkBoxMajorGridX->isChecked());
    settings->setValue("graph/showMinorGridLinesX", ui->checkBoxMinorGridX->isChecked());
    settings->setValue("graph/showMajorGridLinesY", ui->checkBoxMajorGridY->isChecked());
    settings->setValue("graph/showMinorGridLinesY", ui->checkBoxMinorGridY->isChecked());
    settings->setValue("graph/differenceTickLabels", ui->checkBoxDifferenceLabels->isChecked());

    settings->setValue("scans/multiScanMode", ui->comboBoxMultiScans->currentIndex());
    settings->setValue("graph/showWlInXaxisLabel", ui->checkBoxWlInXaxis->isChecked());
}

