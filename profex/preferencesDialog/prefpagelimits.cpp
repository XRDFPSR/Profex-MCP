/***************************************************************************
                          prefpagelimits.cpp  -  description
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

#include "prefpagelimits.h"
#include "../libXrdIO/structs.h"
#include "ui_prefpagelimits.h"

PrefPageLimits::PrefPageLimits(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageLimits)
{
    ui->setupUi(this);
}

PrefPageLimits::~PrefPageLimits()
{
    delete ui;
}

void PrefPageLimits::initUi()
{
    ui->labelChi2acceptance->setText(QString("%1%2 acceptance level").arg(global::chi).arg(global::superTwo));
    initSettings();
}

void PrefPageLimits::initSettings()
{
    ui->doubleSpinBoxFcoordLimit->setValue(settings->value("bgmnProject/coordinatelimits", 0.05).toDouble());
    ui->doubleSpinBoxChi2Acceptance->setValue(settings->value("convergenceDisplay/chi2acceptance", 1.5).toDouble());
    ui->doubleSpinBoxUCLimit->setValue(settings->value("cifimport/limits", 0.01).toDouble() * 100.0);
    ui->doubleSpinBoxB1Limit->setValue(settings->value("bgmnProject/b1limits", 0.01).toDouble());
    ui->doubleSpinBoxK2Limit->setValue(settings->value("bgmnProject/k2limits", 0.0001).toDouble());
    ui->doubleSpinBoxTDSLimit->setValue(settings->value("bgmnProject/tdslimits", 0.02).toDouble());
}

void PrefPageLimits::saveSettings()
{
    settings->setValue("bgmnProject/coordinatelimits", ui->doubleSpinBoxFcoordLimit->value());
    settings->setValue("convergenceDisplay/chi2acceptance", ui->doubleSpinBoxChi2Acceptance->value());
    settings->setValue("cifimport/limits", ui->doubleSpinBoxUCLimit->value() / 100.0);
    settings->setValue("bgmnProject/b1limits", ui->doubleSpinBoxB1Limit->value());
    settings->setValue("bgmnProject/k2limits", ui->doubleSpinBoxK2Limit->value());
    settings->setValue("bgmnProject/tdslimits", ui->doubleSpinBoxTDSLimit->value());
}

void PrefPageLimits::resetValues()
{
    ui->doubleSpinBoxFcoordLimit->setValue(0.05);
    ui->doubleSpinBoxChi2Acceptance->setValue(1.5);
    ui->doubleSpinBoxUCLimit->setValue(1.0);
    ui->doubleSpinBoxB1Limit->setValue(0.01);
    ui->doubleSpinBoxK2Limit->setValue(0.0001);
    ui->doubleSpinBoxTDSLimit->setValue(0.02);
}
