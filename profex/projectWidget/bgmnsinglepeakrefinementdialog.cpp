/***************************************************************************
                          bgmnsinglepeakrefinementdialog.h  -  description
                             -------------------
    begin                : Wed Jan 24 23:00:00 CEST 2024
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

#include <QComboBox>
#include <QLineEdit>
#include "bgmnsinglepeakrefinementdialog.h"
#include "bgmnbackendconfig.h"
#include "ui_bgmnsinglepeakrefinementdialog.h"

BgmnSinglePeakRefinementDialog::BgmnSinglePeakRefinementDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::BgmnSinglePeakRefinementDialog)
{
    setWindowTitle(tr("Single Peak Refinement Project"));
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    initSettings();
}

BgmnSinglePeakRefinementDialog::~BgmnSinglePeakRefinementDialog()
{
    delete ui;
}

void BgmnSinglePeakRefinementDialog::initSettings()
{
    ui->comboBoxDevices->lineEdit()->setPlaceholderText(QString("<select instrument>"));

    setupDevFiles();

    int n = ui->comboBoxDevices->findText(settings->value("bgmnProject/defaultInstrument", QString()).toString());
    ui->comboBoxDevices->setCurrentIndex(n < 0 ? 0 : n);
}

void BgmnSinglePeakRefinementDialog::saveSettings()
{
    settings->setValue("bgmnProject/defaultInstrument", ui->comboBoxDevices->currentText());
}

void BgmnSinglePeakRefinementDialog::accept()
{
    saveSettings();
    QDialog::accept();
}

void BgmnSinglePeakRefinementDialog::setupDevFiles()
{
    ui->comboBoxDevices->clear();
    ui->comboBoxDevices->addItem(QString(), QString());

    BgmnBackendConfig bkgCfg;
    QMultiMap<QString,QString> fileMap = bkgCfg.getAllDevFiles();

    // iterate over the dev file map
    QMultiMap<QString,QString>::const_iterator i = fileMap.constBegin();

    while (i != fileMap.constEnd()) {
        ui->comboBoxDevices->addItem(i.key(), i.value());
        ++i;
    }
}

QString BgmnSinglePeakRefinementDialog::deviceFile() const
{
    return ui->comboBoxDevices->currentData(Qt::UserRole).toString();
}

bool BgmnSinglePeakRefinementDialog::overwrite() const
{
    return ui->checkBoxOverwrite->isChecked();
}

bool BgmnSinglePeakRefinementDialog::usePhaseName() const
{
    return ui->radioButtonFileNamePhase->isChecked();
}

QString BgmnSinglePeakRefinementDialog::useOtherName() const
{
    return ui->lineEditFileNameOther->text();
}
