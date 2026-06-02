/***************************************************************************
                          prefpagesummarytables.cpp  -  description
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

#include "prefpagesummarytables.h"
#include "../libXrdIO/structs.h"
#include "ui_prefpagesummarytables.h"

#include <QColorDialog>
#include <QMessageBox>

PrefPageSummaryTables::PrefPageSummaryTables(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageSummaryTables)
{
    ui->setupUi(this);
}

PrefPageSummaryTables::~PrefPageSummaryTables()
{
    delete ui;
}

void PrefPageSummaryTables::initUi()
{
    connect(ui->pushButtonLODColor, SIGNAL(clicked(bool)), this, SLOT(selectLODColor()));
    connect(ui->pushButtonLOQColor, SIGNAL(clicked(bool)), this, SLOT(selectLOQColor()));
    connect(ui->toolButtonResetGlobalGoals, SIGNAL(clicked(bool)), this, SLOT(defaultBgmnGlobalGoals()));
    connect(ui->toolButtonResetLocalGoals, SIGNAL(clicked(bool)), this, SLOT(defaultBgmnLocalGoals()));

    connect(ui->checkBoxShowLOWarn, SIGNAL(toggled(bool)), this, SLOT(toggleWarningsEnabled(bool)));
    initSettings();
}

void PrefPageSummaryTables::initSettings()
{
    QString goalsGlobal = settings->value("bgmnProject/reportedGlobalGoals",     global::defaultBgmnGlobalGoals).toString();
    QString goalsLocal  = settings->value("bgmnProject/reportedLocalParameters", global::defaultBgmnLocalGoals).toString();

    // check again due to an incompatibility in Profex 5.4.0 with previous settings format (was a QStringList before)
    if (goalsGlobal.isEmpty()) goalsGlobal = global::defaultBgmnGlobalGoals;
    if (goalsLocal.isEmpty())  goalsLocal  = global::defaultBgmnLocalGoals;

    ui->plainTextEditBgmnGlobalGoals->setPlainText(goalsGlobal);
    ui->plainTextEditBgmnOutputParameters->setPlainText(goalsLocal);

    ui->doubleSpinBoxLOD->setValue(settings->value("bgmnProject/warnLODvalue", 0.50).toDouble());
    ui->doubleSpinBoxLOQ->setValue(settings->value("bgmnProject/warnLOQvalue", 0.25).toDouble());
    ui->doubleSpinBoxMinEsd->setValue(settings->value("bgmnProject/minESDvalue", 0.05).toDouble());
    ui->checkBoxShowLOWarn->setChecked(settings->value("bgmnProject/warnShow", false).toBool());

    lodWarningCol = QColor(settings->value("bgmnProject/warnLODColor", "#ff0000").toString());
    loqWarningCol = QColor(settings->value("bgmnProject/warnLOQColor", "#ff5500").toString());

    ui->pushButtonLODColor->setStyleSheet(QString("background-color:%1").arg(lodWarningCol.name()));
    ui->pushButtonLOQColor->setStyleSheet(QString("background-color:%1").arg(loqWarningCol.name()));

    ui->checkBoxRwp->setChecked(settings->value("bgmnProject/resultsTree/showRwp", true).toBool());
    ui->checkBoxRexp->setChecked(settings->value("bgmnProject/resultsTree/showRexp", true).toBool());
    ui->checkBoxChi2->setChecked(settings->value("bgmnProject/resultsTree/showChi2", true).toBool());
    ui->checkBoxGoF->setChecked(settings->value("bgmnProject/resultsTree/showGof", true).toBool());
    ui->checkBoxBgCoeff->setChecked(settings->value("bgmnProject/resultsTree/showBkgrCoeff", true).toBool());

    toggleWarningsEnabled(ui->checkBoxShowLOWarn->isChecked());
}

void PrefPageSummaryTables::saveSettings()
{
    settings->setValue("bgmnProject/warnShow",     ui->checkBoxShowLOWarn->isChecked());
    settings->setValue("bgmnProject/warnLODColor", lodWarningCol);
    settings->setValue("bgmnProject/warnLOQColor", loqWarningCol);

    settings->setValue("bgmnProject/warnLODvalue", ui->doubleSpinBoxLOD->value());
    settings->setValue("bgmnProject/warnLOQvalue", ui->doubleSpinBoxLOQ->value());
    settings->setValue("bgmnProject/minESDvalue",  ui->doubleSpinBoxMinEsd->value());

    settings->setValue("bgmnProject/resultsTree/showRwp", ui->checkBoxRwp->isChecked());
    settings->setValue("bgmnProject/resultsTree/showRexp", ui->checkBoxRexp->isChecked());
    settings->setValue("bgmnProject/resultsTree/showChi2", ui->checkBoxChi2->isChecked());
    settings->setValue("bgmnProject/resultsTree/showGof", ui->checkBoxGoF->isChecked());
    settings->setValue("bgmnProject/resultsTree/showBkgrCoeff", ui->checkBoxBgCoeff->isChecked());

    QString goalsGlobal = ui->plainTextEditBgmnGlobalGoals->toPlainText();
    QString goalsLocal  = ui->plainTextEditBgmnOutputParameters->toPlainText();

    settings->setValue("bgmnProject/reportedGlobalGoals", goalsGlobal);
    settings->setValue("bgmnProject/reportedLocalParameters", goalsLocal);
}

void PrefPageSummaryTables::selectLODColor()
{
    QColor lod;
    lod = QColorDialog::getColor(lodWarningCol, this, tr("Color of Limit of Detection warnings"));

    if (!lod.isValid()) return;

    lodWarningCol = lod;
    ui->pushButtonLODColor->setStyleSheet(QString("background-color:%1").arg(lodWarningCol.name()));
}

void PrefPageSummaryTables::selectLOQColor()
{
    QColor loq;
    loq = QColorDialog::getColor(loqWarningCol, this, tr("Color of Limit of Quantification warnings"));

    if (!loq.isValid()) return;

    loqWarningCol = loq;
    ui->pushButtonLOQColor->setStyleSheet(QString("background-color:%1").arg(loqWarningCol.name()));
}


void PrefPageSummaryTables::defaultBgmnGlobalGoals()
{
    if (QMessageBox::question(this,
                              tr("Reset Parameters"),
                              tr("Do you really want to reset the global parameters list?"))
            == QMessageBox::No) {
        return;
    }

    ui->plainTextEditBgmnGlobalGoals->setPlainText(global::defaultBgmnGlobalGoals);
}

void PrefPageSummaryTables::defaultBgmnLocalGoals()
{
   if (QMessageBox::question(this,
                             tr("Reset Parameters"),
                             tr("Do you really want to reset the local parameters list?"))
           == QMessageBox::No) {
       return;
   }

   ui->plainTextEditBgmnOutputParameters->setPlainText(global::defaultBgmnLocalGoals);
}

void PrefPageSummaryTables::toggleWarningsEnabled(bool b)
{
    ui->labelLOD->setEnabled(b);
    ui->labelLOQ->setEnabled(b);
    ui->doubleSpinBoxLOD->setEnabled(b);
    ui->doubleSpinBoxLOQ->setEnabled(b);
    ui->pushButtonLODColor->setEnabled(b);
    ui->pushButtonLOQColor->setEnabled(b);
    ui->labelMinEsd->setEnabled(b);
    ui->doubleSpinBoxMinEsd->setEnabled(b);
}
