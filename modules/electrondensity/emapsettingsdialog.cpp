/***************************************************************************
                          emapsettingsdialog.h  -  description
                             -------------------
    begin                : Wed Sep 24 20:35:00 CEST 2014
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

#include "emapsettingsdialog.h"
#include "ui_emapsettingsdialog.h"
#include "../libXrdIO/structs.h"
#include <QDoubleSpinBox>

EmapSettingsDialog::EmapSettingsDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EmapSettingsDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();

    initGui();
}

EmapSettingsDialog::~EmapSettingsDialog()
{
    delete ui;
}

void EmapSettingsDialog::accept()
{
    QDialog::accept();
}

void EmapSettingsDialog::initGui()
{
    ui->doubleSpinBoxResolution->setMinimum(0.01);
    ui->doubleSpinBoxResolution->setMaximum(1.0);
    ui->doubleSpinBoxResolution->setSingleStep(0.02);
    ui->doubleSpinBoxResolution->setSuffix(QString(" %1").arg(global::angstrom));

    ui->comboBoxInterpolation->addItem("None");
    ui->comboBoxInterpolation->addItem("Medium");
    ui->comboBoxInterpolation->addItem("High");

    ui->spinBoxImageHeight->setMinimum(256);
    ui->spinBoxImageHeight->setMaximum(10240);

    ui->comboBoxSuperSampling->addItem("off", 1);
    ui->comboBoxSuperSampling->addItem("2x", 2);
    ui->comboBoxSuperSampling->addItem("4x", 4);
}

void EmapSettingsDialog::setResolution(double d)
{
    ui->doubleSpinBoxResolution->setValue(d);
}

void EmapSettingsDialog::setInterpolation(int i)
{
    ui->comboBoxInterpolation->setCurrentIndex(i);
}

void EmapSettingsDialog::setImageHeight(int i)
{
    ui->spinBoxImageHeight->setValue(i);
}

void EmapSettingsDialog::setSuperSampling(int i)
{
    if (i == 1) ui->comboBoxSuperSampling->setCurrentIndex(0);
    if (i == 2) ui->comboBoxSuperSampling->setCurrentIndex(1);
    if (i == 4) ui->comboBoxSuperSampling->setCurrentIndex(2);
}

double EmapSettingsDialog::getResolution()
{
    return ui->doubleSpinBoxResolution->value();
}

int EmapSettingsDialog::getInterpolation()
{
    return ui->comboBoxInterpolation->currentIndex();
}

int EmapSettingsDialog::getImageHeight()
{
    return ui->spinBoxImageHeight->value();
}

int EmapSettingsDialog::getSuperSampling()
{
    return ui->comboBoxSuperSampling->currentData(Qt::UserRole).toInt();
}
