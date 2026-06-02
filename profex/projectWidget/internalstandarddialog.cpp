/***************************************************************************
                          internalstandarddialog.h  -  description
                             -------------------
    begin                : Thu Sep 03 21:07:00 CEST 2015
    copyright            : (C) 2015 by Nicola Doebelin
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

#include "internalstandarddialog.h"
#include "ui_internalstandarddialog.h"
#include <QDoubleSpinBox>

InternalStandardDialog::InternalStandardDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::InternalStandardDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Internal Standard"));
}

InternalStandardDialog::~InternalStandardDialog()
{
    delete ui;
}

void InternalStandardDialog::setPhases(const QStringList &s)
{
    ui->comboBoxPhase->clear();
    ui->comboBoxPhase->insertItems(0, s);
}

void InternalStandardDialog::setCurrentPhase(const QString &s)
{
    if (!s.isEmpty()) ui->comboBoxPhase->setCurrentText(s);
}

void InternalStandardDialog::setCurrentQuantity(double d)
{
    if (d >= 0.0) ui->doubleSpinBoxQuantity->setValue(d * 100.0);
}

QString InternalStandardDialog::getPhase()
{
    return ui->comboBoxPhase->currentText();
}

double InternalStandardDialog::getQuantity()
{
    return ui->doubleSpinBoxQuantity->value() / 100.0;
}
