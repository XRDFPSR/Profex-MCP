/***************************************************************************
                          PresetContentSaveDialog.h  -  description
                             -------------------
    begin                : Sat Sep 19 10:30:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#include "presetcontentsavedialog.h"
#include "ui_presetcontentsavedialog.h"

PresetContentSaveDialog::PresetContentSaveDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PresetContentSaveDialog)
{
    ui->setupUi(this);

#ifndef Q_OS_WIN
    ui->checkBoxExcel->hide();
#endif
}

PresetContentSaveDialog::~PresetContentSaveDialog()
{
    delete ui;
}

void PresetContentSaveDialog::setCheckStatus(bool p, bool i, bool c, bool e, bool b, bool f)
{
    ui->checkBoxRefinementProject->setChecked(p);
    ui->checkBoxIntegrals->setChecked(i);
    ui->checkBoxCurveFits->setChecked(c);
    ui->checkBoxExcel->setChecked(e);
    ui->checkBoxBaseLine->setChecked(b);
    ui->checkBoxPeakListfilters->setChecked(f);
}

void PresetContentSaveDialog::getCheckStatus(bool &p, bool &i, bool &c, bool &e, bool &b, bool &f)
{
    p = ui->checkBoxRefinementProject->isChecked();
    i = ui->checkBoxIntegrals->isChecked();
    c = ui->checkBoxCurveFits->isChecked();
    e = ui->checkBoxExcel->isChecked();
    b = ui->checkBoxBaseLine->isChecked();
    f = ui->checkBoxPeakListfilters->isChecked();
}
