/***************************************************************************
                          prefpagegraphprinting.cpp  -  description
                             -------------------
    begin                : Wed May 10 14:00:00 CEST 2017
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

#include "prefpagegraphprinting.h"
#include "ui_prefpagegraphprinting.h"

PrefPageGraphPrinting::PrefPageGraphPrinting(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageGraphPrinting)
{
    ui->setupUi(this);
}

PrefPageGraphPrinting::~PrefPageGraphPrinting()
{
    delete ui;
}

void PrefPageGraphPrinting::initUi()
{
    QRegularExpression rx("[^a-zA-Z0-9]{1,1}");
    QValidator *validator = new QRegularExpressionValidator(rx, this);
    ui->lineEditAsciiFieldSep->setValidator(validator);
    ui->lineEditClipboardFieldSep->setValidator(validator);

    initSettings();
}

void PrefPageGraphPrinting::initSettings()
{
    ui->spinBoxPrintingLineWidth->setValue(settings->value("graph/printingLineWidth", 3).toInt());
    ui->doubleSpinPrintingFontScale->setValue(settings->value("graph/printingFontSize", 1.25).toDouble());
    ui->spinBoxRasterWidth->setValue(settings->value("graph/rasterResolutionWidth", 1536).toInt());
    ui->spinBoxRasterHeight->setValue(settings->value("graph/rasterResolutionHeight", 1024).toInt());
    ui->doubleSpinBoxSvgFontScale->setValue(settings->value("graph/svgExportFontSize", 1.0).toDouble());
    ui->lineEditAsciiFieldSep->setText(settings->value("config/asciiFieldSeparator", " ").toString());
    ui->lineEditClipboardFieldSep->setText(settings->value("config/clipboardFieldSeparator", ";").toString());
    ui->radioButtonBgFilled->setChecked(settings->value("graph/rasterBackgroundFilled", false).toBool());
    ui->radioButtonBgTransparent->setChecked(!ui->radioButtonBgFilled->isChecked());
}

void PrefPageGraphPrinting::saveSettings()
{
    settings->setValue("graph/printingLineWidth", ui->spinBoxPrintingLineWidth->value());
    settings->setValue("graph/printingFontSize", ui->doubleSpinPrintingFontScale->value());
    settings->setValue("graph/rasterResolutionWidth", ui->spinBoxRasterWidth->value());
    settings->setValue("graph/rasterResolutionHeight", ui->spinBoxRasterHeight->value());
    settings->setValue("graph/svgExportFontSize", ui->doubleSpinBoxSvgFontScale->value());
    settings->setValue("config/asciiFieldSeparator", ui->lineEditAsciiFieldSep->text());
    settings->setValue("config/clipboardFieldSeparator", ui->lineEditClipboardFieldSep->text());
    settings->setValue("graph/rasterBackgroundFilled", ui->radioButtonBgFilled->isChecked());
}
