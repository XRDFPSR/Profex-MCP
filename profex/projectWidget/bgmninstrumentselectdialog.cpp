/***************************************************************************
                          bgmninstrumentselectdialog.cpp  -  description
                             -------------------
    begin                : Mon Jan 21 16:00:00 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#include "bgmninstrumentselectdialog.h"
#include "bgmnbackendconfig.h"
#include "ui_bgmninstrumentselectdialog.h"

BgmnInstrumentSelectDialog::BgmnInstrumentSelectDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BgmnInstrumentSelectDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    init();
    initSettings();
}

BgmnInstrumentSelectDialog::~BgmnInstrumentSelectDialog()
{
    saveSettings();
    delete ui;
}

void BgmnInstrumentSelectDialog::initSettings()
{
    restoreGeometry(settings->value("instrumentSelectDialog/geometry").toByteArray());
    ui->comboBoxGeq->setCurrentIndex(settings->value("instrumentSelectDialog/geqFile", 0).toInt());
    ui->comboBoxLam->setCurrentIndex(settings->value("instrumentSelectDialog/lamFile", 0).toInt());
    ui->doubleSpinBoxSynchrotron->setValue(settings->value("instrumentSelectDialog/synchrotron", 0.07).toDouble());

    bool lam = settings->value("instrumentSelectDialog/wavelength", true).toBool();
    ui->radioButtonLam->setChecked(lam);
    ui->radioButtonSynchrotron->setChecked(!lam);
    ui->comboBoxLam->setEnabled(lam);
    ui->doubleSpinBoxSynchrotron->setEnabled(!lam);
}

void BgmnInstrumentSelectDialog::saveSettings()
{
    settings->setValue("instrumentSelectDialog/geometry", saveGeometry());
    settings->setValue("instrumentSelectDialog/geqFile", ui->comboBoxGeq->currentIndex());
    settings->setValue("instrumentSelectDialog/lamFile", ui->comboBoxLam->currentIndex());
    settings->setValue("instrumentSelectDialog/synchrotron", ui->doubleSpinBoxSynchrotron->value());
    settings->setValue("instrumentSelectDialog/wavelength", ui->radioButtonLam->isChecked());
}

void BgmnInstrumentSelectDialog::init()
{
    BgmnBackendConfig bkgCfg;
    QMultiMap<QString, QString> devFiles(bkgCfg.getAllDevFiles());
    QMultiMap<QString, QString> lamFiles(bkgCfg.getAllLamFiles());

    QMultiMapIterator<QString, QString> dIt(devFiles);

    while (dIt.hasNext()) {
        dIt.next();
        ui->comboBoxGeq->addItem(dIt.key(), dIt.value());
    }

    QMultiMapIterator<QString, QString> lIt(lamFiles);

    while (lIt.hasNext()) {
        lIt.next();
        ui->comboBoxLam->addItem(lIt.key(), lIt.value());
    }
}

QString BgmnInstrumentSelectDialog::getDevFile()
{
    return ui->comboBoxGeq->currentData(Qt::UserRole).toString();
}

QString BgmnInstrumentSelectDialog::getLamFile()
{
    return ui->comboBoxLam->currentData(Qt::UserRole).toString();
}

double BgmnInstrumentSelectDialog::getSynchrotronValue()
{
    return ui->doubleSpinBoxSynchrotron->value();
}

bool BgmnInstrumentSelectDialog::isLamSelected()
{
    return ui->radioButtonLam->isChecked();
}
