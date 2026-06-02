/***************************************************************************
                          prefpagegraphs.h  -  description
                             -------------------
    begin                : Tue May 09 16:00:00 CEST 2017
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

#include "prefpagegraphs.h"
#include "ui_prefpagegraphs.h"

#include <QColorDialog>
#include <QButtonGroup>

PrefPageGraphs::PrefPageGraphs(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageGraphs)
{
    ui->setupUi(this);
}

PrefPageGraphs::~PrefPageGraphs()
{
    delete ui;
}

void PrefPageGraphs::initUi()
{
    connect(ui->groupBoxMarginColors, SIGNAL(toggled(bool)), ui->pushButtonBgColorIdle,     SLOT(setEnabled(bool)));
    connect(ui->groupBoxMarginColors, SIGNAL(toggled(bool)), ui->pushButtonBgColorActive,   SLOT(setEnabled(bool)));
    connect(ui->groupBoxMarginColors, SIGNAL(toggled(bool)), ui->pushButtonBgColorComplete, SLOT(setEnabled(bool)));

    connect(ui->pushButtonBgColorIdle,     SIGNAL(clicked(bool)), this, SLOT(graphIdleColorChanged()));
    connect(ui->pushButtonBgColorActive,   SIGNAL(clicked(bool)), this, SLOT(graphActiveColorChanged()));
    connect(ui->pushButtonBgColorError,    SIGNAL(clicked(bool)), this, SLOT(graphErrorColorChanged()));
    connect(ui->pushButtonBgColorComplete, SIGNAL(clicked(bool)), this, SLOT(graphCompleteColorChanged()));

    QButtonGroup *paletteButtons = new QButtonGroup(this);
    QButtonGroup *legendOrderButtons = new QButtonGroup(this);

    paletteButtons->addButton(ui->radioButtonSystemPalette);
    paletteButtons->addButton(ui->radioButtonCustomColor);
    legendOrderButtons->addButton(ui->radioButtonStackedScansTop);
    legendOrderButtons->addButton(ui->radioButtonStackedScansBottom);

    initSettings();
}

void PrefPageGraphs::initSettings()
{
    ui->checkBoxAntiAliasing->setChecked(settings->value("graph/antiAliasing", false).toBool());
    ui->checkBoxCompleteFileName->setChecked(settings->value("graph/drawCompleteFileName", false).toBool());
    ui->checkBoxFillActive->setChecked(settings->value("graph/fillActiveScan", true).toBool());
    ui->checkBoxFillActiveHkl->setChecked(settings->value("graph/fillActiveHkl", false).toBool());

    bool useMarginColors = settings->value("graph/useBackgroundColors", true).toBool();

    ui->groupBoxMarginColors->setChecked(useMarginColors);
    ui->pushButtonBgColorIdle->setEnabled(useMarginColors);
    ui->pushButtonBgColorActive->setEnabled(useMarginColors);
    ui->pushButtonBgColorComplete->setEnabled(useMarginColors);

    colBgIdle     = QColor(settings->value("graph/idleColor",     "#ffffff").toString());
    colBgActive   = QColor(settings->value("graph/activeColor",   "#ff9999").toString());
    colBgError    = QColor(settings->value("graph/errorColor",    "#ffff99").toString());
    colBgComplete = QColor(settings->value("graph/completeColor", "#99ff99").toString());

    ui->pushButtonBgColorIdle->setStyleSheet(QString("background-color:%1").arg(colBgIdle.name()));
    ui->pushButtonBgColorActive->setStyleSheet(QString("background-color:%1").arg(colBgActive.name()));
    ui->pushButtonBgColorError->setStyleSheet(QString("background-color:%1").arg(colBgError.name()));
    ui->pushButtonBgColorComplete->setStyleSheet(QString("background-color:%1").arg(colBgComplete.name()));

    ui->checkBoxActiveScanLineWidth->setChecked(settings->value("graph/useActiveLineWidth", true).toBool());
    ui->spinBoxActiveScanLineWidth->setEnabled(settings->value("graph/useActiveLineWidth", true).toBool());
    ui->spinBoxActiveScanLineWidth->setValue(settings->value("graph/activeLineWidth", 2).toInt());

    ui->checkBoxPhaseDataPattern->setChecked(settings->value("graph/phaseVisibilityPattern", true).toBool());
    ui->checkBoxPhaseDataHkl->setChecked(settings->value("graph/phaseVisibilityHkl", true).toBool());

    colBgBackground = QColor(settings->value("graph/backgroundColor", "").toString());

    if (colBgBackground.isValid()) {
        ui->radioButtonSystemPalette->setChecked(false);
        ui->radioButtonCustomColor->setChecked(true);
        ui->toolButtonBackgroundColor->setStyleSheet(QString("background-color:%1").arg(colBgBackground.name()));
    } else {
        ui->radioButtonSystemPalette->setChecked(true);
        ui->radioButtonCustomColor->setChecked(false);
    }

    bool stackedScansFirstTop = settings->value("graph/stackedScansFirstOnTop", false).toBool();
    ui->radioButtonStackedScansTop->setChecked(stackedScansFirstTop);
    ui->radioButtonStackedScansBottom->setChecked(!stackedScansFirstTop);
}

void PrefPageGraphs::saveSettings()
{
    settings->setValue("graph/antiAliasing", ui->checkBoxAntiAliasing->isChecked());
    settings->setValue("graph/fillActiveScan", ui->checkBoxFillActive->isChecked());
    settings->setValue("graph/fillActiveHkl", ui->checkBoxFillActiveHkl->isChecked());

    settings->setValue("graph/drawCompleteFileName", ui->checkBoxCompleteFileName->isChecked());

    settings->setValue("graph/useBackgroundColors", ui->groupBoxMarginColors->isChecked());

    settings->setValue("graph/idleColor", colBgIdle.name());
    settings->setValue("graph/activeColor", colBgActive.name());
    settings->setValue("graph/errorColor", colBgError.name());
    settings->setValue("graph/completeColor", colBgComplete.name());

    settings->setValue("graph/useActiveLineWidth", ui->checkBoxActiveScanLineWidth->isChecked());
    settings->setValue("graph/activeLineWidth", ui->spinBoxActiveScanLineWidth->value());

    settings->setValue("graph/phaseVisibilityPattern", ui->checkBoxPhaseDataPattern->isChecked());
    settings->setValue("graph/phaseVisibilityHkl", ui->checkBoxPhaseDataHkl->isChecked());

    settings->setValue("graph/backgroundColor", ui->radioButtonSystemPalette->isChecked() ? "" : colBgBackground.name());

    settings->setValue("graph/stackedScansFirstOnTop", ui->radioButtonStackedScansTop->isChecked());
}

void PrefPageGraphs::graphIdleColorChanged()
{
    QColor tcolBgIdle = QColorDialog::getColor(colBgIdle, this, tr("Background Color for Idle Status"));

    if (tcolBgIdle.isValid()) {
        colBgIdle = tcolBgIdle;
        ui->pushButtonBgColorIdle->setStyleSheet(QString("background-color:%1").arg(colBgIdle.name()));
    }

}

void PrefPageGraphs::graphActiveColorChanged()
{
    QColor tcolBgActive = QColorDialog::getColor(colBgActive, this, tr("Background Color for Active Status"));

    if (tcolBgActive.isValid()) {
        colBgActive = tcolBgActive;
        ui->pushButtonBgColorActive->setStyleSheet(QString("background-color:%1").arg(colBgActive.name()));
    }
}

void PrefPageGraphs::graphErrorColorChanged()
{
    QColor tcolBgError = QColorDialog::getColor(colBgError, this, tr("Background Color for Error or Aborted Status"));

    if (tcolBgError.isValid()) {
        colBgActive = tcolBgError;
        ui->pushButtonBgColorActive->setStyleSheet(QString("background-color:%1").arg(colBgActive.name()));
    }
}

void PrefPageGraphs::graphCompleteColorChanged()
{
    QColor tcolBgComplete = QColorDialog::getColor(colBgComplete, this, tr("Background Color for Complete Status"));

    if (tcolBgComplete.isValid()) {
        colBgComplete = tcolBgComplete;
        ui->pushButtonBgColorComplete->setStyleSheet(QString("background-color:%1").arg(colBgComplete.name()));
    }
}

void PrefPageGraphs::customBackgroundColor()
{
    QColor col = QColorDialog::getColor(colBgBackground, this, tr("Select color"));

    if (col.isValid()) {
        colBgBackground = col;
        ui->toolButtonBackgroundColor->setStyleSheet(QString("background-color:%1").arg(colBgBackground.name()));
    }
}
