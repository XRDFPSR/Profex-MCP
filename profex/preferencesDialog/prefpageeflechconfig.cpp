/***************************************************************************
                          prefpageeflechconfig.cpp  -  description
                             -------------------
    begin                : Tue Jan 22 19:00:00 CET 2019
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

#include "prefpageeflechconfig.h"
#include "ui_prefpageeflechconfig.h"

PrefPageEflechConfig::PrefPageEflechConfig(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageEflechConfig)
{
    ui->setupUi(this);
}

PrefPageEflechConfig::~PrefPageEflechConfig()
{
    delete ui;
}

void PrefPageEflechConfig::initUi()
{
    initSettings();
}

void PrefPageEflechConfig::initSettings()
{
    ui->checkBoxTestN->setChecked(settings->value("eflech/testN", true).toBool());
    ui->checkBoxTestD->setChecked(settings->value("eflech/testD", true).toBool());
    ui->checkBoxTestP->setChecked(settings->value("eflech/testP", false).toBool());
    ui->checkBoxTestM->setChecked(settings->value("eflech/testM", false).toBool());
    ui->checkBoxTest2->setChecked(settings->value("eflech/test2", true).toBool());
    ui->checkBoxTest3->setChecked(settings->value("eflech/test3", true).toBool());
    ui->checkBoxTest4->setChecked(settings->value("eflech/test4", true).toBool());
    ui->checkBoxTestU->setChecked(settings->value("eflech/testU", true).toBool());
    ui->doubleSpinBoxEpsilon->setValue(settings->value("eflech/epsilon", 0.05).toDouble());
    ui->checkBoxOverrideEpsilon->setChecked(settings->value("eflech/overrideEpsilon", false).toBool());
    ui->doubleSpinBoxEpsilon->setEnabled(settings->value("eflech/overrideEpsilon", false).toBool());
}

void PrefPageEflechConfig::saveSettings()
{
    settings->setValue("eflech/testN", ui->checkBoxTestN->isChecked());
    settings->setValue("eflech/testD", ui->checkBoxTestD->isChecked());
    settings->setValue("eflech/testP", ui->checkBoxTestP->isChecked());
    settings->setValue("eflech/testM", ui->checkBoxTestM->isChecked());
    settings->setValue("eflech/test2", ui->checkBoxTest2->isChecked());
    settings->setValue("eflech/test3", ui->checkBoxTest3->isChecked());
    settings->setValue("eflech/test4", ui->checkBoxTest4->isChecked());
    settings->setValue("eflech/testU", ui->checkBoxTestU->isChecked());
    settings->setValue("eflech/epsilon", ui->doubleSpinBoxEpsilon->value());
    settings->setValue("eflech/overrideEpsilon", ui->checkBoxOverrideEpsilon->isChecked());
}

void PrefPageEflechConfig::resetSettings()
{
    ui->checkBoxTestN->setChecked(true);
    ui->checkBoxTestD->setChecked(true);
    ui->checkBoxTestP->setChecked(true);
    ui->checkBoxTestM->setChecked(true);
    ui->checkBoxTest2->setChecked(true);
    ui->checkBoxTest3->setChecked(true);
    ui->checkBoxTest4->setChecked(true);
    ui->checkBoxTestU->setChecked(true);
    ui->doubleSpinBoxEpsilon->setValue(0.05);
    ui->checkBoxOverrideEpsilon->setChecked(false);
    ui->doubleSpinBoxEpsilon->setEnabled(false);
}
