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

#include "prefpagesearchmatch.h"
#include "ui_prefpagesearchmatch.h"

PrefPageSearchMatch::PrefPageSearchMatch(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageSearchMatch)
{
    ui->setupUi(this);
}

PrefPageSearchMatch::~PrefPageSearchMatch()
{
    delete ui;
}

void PrefPageSearchMatch::initUi()
{
    connect(ui->toolButtonReset, SIGNAL(clicked(bool)), this, SLOT(resetValues()));
    initSettings();
}

void PrefPageSearchMatch::initSettings()
{
    ui->doubleSpinBoxA->setValue(settings->value("seachMatch/weightA", 1.0).toDouble());
    ui->doubleSpinBoxB->setValue(settings->value("seachMatch/weightB", 1.0).toDouble());
    ui->doubleSpinBoxC->setValue(settings->value("seachMatch/weightC", 1.0).toDouble());
    ui->doubleSpinBoxD->setValue(settings->value("seachMatch/weightD", 1.0).toDouble());
    ui->doubleSpinBoxB1->setValue(settings->value("seachMatch/weightB1", 0.02).toDouble());
    ui->doubleSpinBoxk2->setValue(settings->value("seachMatch/weightK2", 0.00001).toDouble());
    ui->doubleSpinBoxDupAxes->setValue(settings->value("searchMatch/compareAxis", 0.02).toDouble());
    ui->doubleSpinBoxDupAngles->setValue(settings->value("searchMatch/compareAngle", 0.1).toDouble());
}

void PrefPageSearchMatch::saveSettings()
{
    settings->setValue("searchMatch/weightA", ui->doubleSpinBoxA->value());
    settings->setValue("searchMatch/weightB", ui->doubleSpinBoxB->value());
    settings->setValue("searchMatch/weightC", ui->doubleSpinBoxC->value());
    settings->setValue("searchMatch/weightD", ui->doubleSpinBoxD->value());
    settings->setValue("searchMatch/weightB1", ui->doubleSpinBoxB1->value());
    settings->setValue("searchMatch/weightK2", ui->doubleSpinBoxk2->value());
    settings->setValue("searchMatch/compareAxis", ui->doubleSpinBoxDupAxes->value());
    settings->setValue("searchMatch/compareAngle", ui->doubleSpinBoxDupAngles->value());
}

void PrefPageSearchMatch::resetValues()
{
    ui->doubleSpinBoxA->setValue(1.0);
    ui->doubleSpinBoxB->setValue(1.0);
    ui->doubleSpinBoxC->setValue(1.0);
    ui->doubleSpinBoxD->setValue(1.0);
    ui->doubleSpinBoxB1->setValue(0.02);
    ui->doubleSpinBoxk2->setValue(0.00001);
    ui->doubleSpinBoxDupAxes->setValue(0.02);
    ui->doubleSpinBoxDupAngles->setValue(0.1);
}
