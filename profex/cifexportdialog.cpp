/***************************************************************************
                          cifexportdialog.cpp  -  description
                             -------------------
    begin                : Sun Jul 24 10:30:00 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#include "cifexportdialog.h"
#include "ui_cifexportdialog.h"
#include <QRadioButton>
#include <QStringList>
#include <QInputDialog>

CifExportDialog::CifExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::CifExportDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();

    QStringList instruments(settings->value("cifExport/instruments", QStringList()).toStringList());
    instruments.sort(Qt::CaseInsensitive);
    ui->comboBoxInstrument->addItems(instruments);

    QString currentInstrument(settings->value("cifExport/currentInstrument", QString()).toString());

    if (!currentInstrument.isNull()) {
        ui->comboBoxInstrument->setCurrentText(currentInstrument);
    }

    ui->comboBoxRadiationSource->addItem("x-ray");
    ui->comboBoxRadiationSource->addItem("neutron");
    ui->comboBoxRadiationSource->addItem("electron");
    ui->comboBoxRadiationSource->addItem("gamma");

    ui->comboBoxRadiationSource->setCurrentText(settings->value("cifExport/currentRadiationSource", "x-ray").toString());

    CifExportMode mode = static_cast<CifExportMode>(settings->value("cifExport/currentMode", 0).toInt());

    if      (mode == SINGLE)  ui->radioButtonSingleFiles->setChecked(true);
    else if (mode == PROJECT) ui->radioButtonMergeProject->setChecked(true);
    else                      ui->radioButtonMergeAll->setChecked(true);
}

CifExportDialog::~CifExportDialog()
{
    delete ui;
}

CifExportMode CifExportDialog::getOutputMode()
{
    if (ui->radioButtonSingleFiles->isChecked()) return SINGLE;
    if (ui->radioButtonMergeProject->isChecked()) return PROJECT;
    return GLOBAL;
}

QMap<QString, QVariant> CifExportDialog::getAuxData()
{
    QMap<QString, QVariant> auxData;

    auxData["_diffrn_ambient_temperature"] = QVariant(ui->spinBoxTemperature->value());

    if (!ui->comboBoxInstrument->currentText().isEmpty()) {
        auxData["_diffrn_measurement_device_type"] = QVariant("'" + ui->comboBoxInstrument->currentText() + "'");
    }

    if (!ui->comboBoxRadiationSource->currentText().isEmpty()) {
        auxData["_diffrn_radiation_probe"] = QVariant("'" + ui->comboBoxRadiationSource->currentText() + "'");
    }

    return auxData;
}

void CifExportDialog::accept()
{
    int mode = static_cast<int>(SINGLE);
    if      (ui->radioButtonMergeProject->isChecked()) mode = static_cast<int>(PROJECT);
    else if (ui->radioButtonMergeAll->isChecked())     mode = static_cast<int>(GLOBAL);

    settings->setValue("cifExport/instruments", getAllInstruments());
    settings->setValue("cifExport/currentIinstrument", ui->comboBoxInstrument->currentText());
    settings->setValue("cifExport/currentRadiationSource", ui->comboBoxRadiationSource->currentText());
    settings->setValue("cifExport/currentMode", mode);

    QDialog::accept();
}

void CifExportDialog::addInstrument()
{
    QStringList instruments(getAllInstruments());

    QString newInstr = QInputDialog::getText(this, tr("New Instrument Name"), tr("New instrument Name")).trimmed();

    if (newInstr.isNull()) return;
    if (instruments.contains(newInstr)) return;

    instruments.append(newInstr);
    instruments.sort(Qt::CaseInsensitive);

    ui->comboBoxInstrument->clear();
    ui->comboBoxInstrument->addItems(instruments);
    ui->comboBoxInstrument->setCurrentText(newInstr);
}

void CifExportDialog::removeInstrument()
{
    int i = ui->comboBoxInstrument->currentIndex();
    ui->comboBoxInstrument->removeItem(i);
}

void CifExportDialog::resetTemperature()
{
    ui->spinBoxTemperature->setValue(295);
}

QStringList CifExportDialog::getAllInstruments()
{
    QStringList instruments;

    for (int i = 0; i < ui->comboBoxInstrument->count(); ++i) {
        instruments.append(ui->comboBoxInstrument->itemText(i));
    }

    instruments.sort(Qt::CaseInsensitive);
    return instruments;
}
