/***************************************************************************
                          prefpagefullprofconfig.cpp  -  description
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

#include "prefpagefullprofconfig.h"
#include "ui_prefpagefullprofconfig.h"

#include <QFileInfo>
#include <QFileDialog>

PrefPageFullprofConfig::PrefPageFullprofConfig(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageFullprofConfig)
{
    ui->setupUi(this);
}

PrefPageFullprofConfig::~PrefPageFullprofConfig()
{
    delete ui;
}

void PrefPageFullprofConfig::initUi()
{
    connect(ui->pushButtonFpExec,     SIGNAL(clicked(bool)), this, SLOT(selectFpExec()));
    connect(ui->pushButtonFpDevFiles, SIGNAL(clicked(bool)), this, SLOT(selectFpDevFiles()));
    connect(ui->pushButtonFpStrFiles, SIGNAL(clicked(bool)), this, SLOT(selectFpStrFiles()));

    initSettings();
}

void PrefPageFullprofConfig::initSettings()
{
    ui->checkBoxStopOnConv->setChecked(settings->value("config/stopOnConvergence", true).toBool());
    ui->doubleSpinBoxAngle->setValue(settings->value("config/dsAngle", "0.25").toDouble());

    ui->lineEditFpExec->setText(settings->value("fpProject/fpExec", "").toString());
    ui->lineEditFpDevFiles->setText(settings->value("fpProject/deviceDatabase", "").toString());
    ui->lineEditFpStrFiles->setText(settings->value("fpProject/structureDatabase").toString());

    workingDir = ui->lineEditFpExec->text().isEmpty() ? QDir::homePath() : ui->lineEditFpExec->text();
}

void PrefPageFullprofConfig::saveSettings()
{
    settings->setValue("config/dsAngle", ui->doubleSpinBoxAngle->value());
    settings->setValue("config/stopOnConvergence", ui->checkBoxStopOnConv->isChecked());

    settings->setValue("fpProject/fpExec", ui->lineEditFpExec->text());
    settings->setValue("fpProject/deviceDatabase", ui->lineEditFpDevFiles->text());
    settings->setValue("fpProject/structureDatabase", ui->lineEditFpStrFiles->text());
}

void PrefPageFullprofConfig::selectFpExec()
{
    QString str = getExec(tr("Select FP2K executable"));
    if (!str.isEmpty()) {
        ui->lineEditFpExec->setText(str);
    }
}

void PrefPageFullprofConfig::selectFpStrFiles()
{
    QString s = QFileDialog::getExistingDirectory(this, tr("Select structure file directory"), ui->lineEditFpStrFiles->text());
    ui->lineEditFpStrFiles->setText(s);
}

void PrefPageFullprofConfig::selectFpDevFiles()
{
    QString s = QFileDialog::getExistingDirectory(this, tr("Select device file directory"), ui->lineEditFpDevFiles->text());
    ui->lineEditFpDevFiles->setText(s);
}

QString PrefPageFullprofConfig::getExec(const QString &s)
{
    QString str = QFileDialog::getOpenFileName(this, s, workingDir);
    QFileInfo fi(str);
    if (fi.exists()) {
        workingDir = fi.absolutePath();
        return str;
    }

    return QString();
}

