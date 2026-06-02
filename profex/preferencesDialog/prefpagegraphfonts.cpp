/***************************************************************************
                          prefpagegraphfonts.cpp  -  description
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

#include "prefpagegraphfonts.h"
#include "ui_prefpagegraphfonts.h"

#include <QFontDialog>

PrefPageGraphFonts::PrefPageGraphFonts(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageGraphFonts)
{
    ui->setupUi(this);
}

PrefPageGraphFonts::~PrefPageGraphFonts()
{
    delete ui;
}

void PrefPageGraphFonts::initUi()
{
    connect(ui->pushButtonFontAxis,   SIGNAL(clicked(bool)), this, SLOT(selectFontAxis()));
    connect(ui->pushButtonFontLegend, SIGNAL(clicked(bool)), this, SLOT(selectFontLegend()));
    connect(ui->pushButtonFontTick,   SIGNAL(clicked(bool)), this, SLOT(selectFontTick()));
    connect(ui->pushButtonFontTitle,  SIGNAL(clicked(bool)), this, SLOT(selectFontTitle()));
    initSettings();
}

void PrefPageGraphFonts::initSettings()
{
    fontTitle.fromString(settings->value("graph/fontTitle", font().toString()).toString());
    ui->lineEditFontTitle->setFont(fontTitle);
    ui->lineEditFontTitle->setText(QString("%1 %2 pt").arg(fontTitle.family()).arg(fontTitle.pointSize()));

    fontAxis.fromString(settings->value("graph/fontAxis", font().toString()).toString());
    ui->lineEditFontAxis->setFont(fontAxis);
    ui->lineEditFontAxis->setText(QString("%1 %2 pt").arg(fontAxis.family()).arg(fontAxis.pointSize()));

    fontTick.fromString(settings->value("graph/fontTicks", font().toString()).toString());
    ui->lineEditFontTick->setFont(fontTick);
    ui->lineEditFontTick->setText(QString("%1 %2 pt").arg(fontTick.family()).arg(fontTick.pointSize()));

    fontLegend.fromString(settings->value("graph/fontLegend", font().toString()).toString());
    ui->lineEditFontLegend->setFont(fontLegend);
    ui->lineEditFontLegend->setText(QString("%1 %2 pt").arg(fontLegend.family()).arg(fontLegend.pointSize()));
}

void PrefPageGraphFonts::saveSettings()
{
    settings->setValue("graph/fontTitle",  fontTitle.toString());
    settings->setValue("graph/fontAxis",   fontAxis.toString());
    settings->setValue("graph/fontTicks",  fontTick.toString());
    settings->setValue("graph/fontLegend", fontLegend.toString());
}

void PrefPageGraphFonts::selectFontTitle()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok, ui->lineEditFontTitle->font(), this);

    if (ok) {
        fontTitle = f;
        ui->lineEditFontTitle->setFont(fontTitle);
        ui->lineEditFontTitle->setText(QString("%1 %2 pt").arg(fontTitle.family()).arg(fontTitle.pointSize()));
    }
}

void PrefPageGraphFonts::selectFontAxis()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok, ui->lineEditFontAxis->font(), this);

    if (ok) {
        fontAxis = f;
        ui->lineEditFontAxis->setFont(fontAxis);
        ui->lineEditFontAxis->setText(QString("%1 %2 pt").arg(fontAxis.family()).arg(fontAxis.pointSize()));
    }
}

void PrefPageGraphFonts::selectFontTick()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok, ui->lineEditFontTick->font(), this);

    if (ok) {
        fontTick = f;
        ui->lineEditFontTick->setFont(fontTick);
        ui->lineEditFontTick->setText(QString("%1 %2 pt").arg(fontTick.family()).arg(fontTick.pointSize()));
    }
}

void PrefPageGraphFonts::selectFontLegend()
{
    bool ok;
    QFont f = QFontDialog::getFont(&ok, ui->lineEditFontLegend->font(), this);

    if (ok) {
        fontLegend = f;
        ui->lineEditFontLegend->setFont(fontLegend);
        ui->lineEditFontLegend->setText(QString("%1 %2 pt").arg(fontLegend.family()).arg(fontLegend.pointSize()));
    }
}

