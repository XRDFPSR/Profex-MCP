/***************************************************************************
                          wavelengthselectdialog.cpp  -  description
                             -------------------
    begin                : Tue Mar 16 21:30:00 CEST 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#include "wavelengthselectdialog.h"
#include "ui_wavelengthselectdialog.h"

WavelengthSelectDialog::WavelengthSelectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::WavelengthSelectDialog)
{
    ui->setupUi(this);

    settings = SettingsManager::getInstance();

    ui->radioButtonCharacteristic->setChecked(true);
    ui->radioButtonSynchrotron->setChecked(false);
    ui->comboBoxCharacteristic->setEnabled(true);
    ui->doubleSpinBoxSynchrotron->setEnabled(false);

    ui->comboBoxCharacteristic->showKa2(true);
    ui->comboBoxCharacteristic->showKb(true);
    ui->comboBoxCharacteristic->initData(true);

    ui->comboBoxCharacteristic->setCurrentIndex(settings->value("wavelengthDialog/charIndex", 0).toInt());
    ui->doubleSpinBoxSynchrotron->setValue(settings->value("wavelengthDialog/synchrotron", 0.1).toDouble());
}

WavelengthSelectDialog::~WavelengthSelectDialog()
{
    delete ui;
}

void WavelengthSelectDialog::accept()
{
    settings->setValue("wavelengthDialog/charIndex", ui->comboBoxCharacteristic->currentIndex());
    settings->setValue("wavelengthDialog/synchrotron", ui->doubleSpinBoxSynchrotron->value());
    QDialog::accept();
}

Scan::WavelengthMode WavelengthSelectDialog::wavelengthMode()
{
    return ui->radioButtonCharacteristic->isChecked() ? Scan::WavelengthMode::CHARACTERISTIC : Scan::WavelengthMode::SYNCHROTRON;
}

double WavelengthSelectDialog::wavelength()
{
    if (ui->radioButtonCharacteristic->isChecked()) {
        return ui->comboBoxCharacteristic->currentData(Qt::UserRole).toDouble();
    }

    return ui->doubleSpinBoxSynchrotron->value();
}
