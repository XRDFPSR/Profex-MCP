/***************************************************************************
                          imageresolutiondialog.cpp  -  description
                             -------------------
    begin                : Tue Jan 07 16:00:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#include "imageresolutiondialog.h"
#include "ui_imageresolutiondialog.h"

ImageResolutionDialog::ImageResolutionDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ImageResolutionDialog)
{
    ui->setupUi(this);
}

ImageResolutionDialog::~ImageResolutionDialog()
{
    delete ui;
}

void ImageResolutionDialog::setPixelWidth(int _i)
{
    ui->spinBoxWidth->setValue(_i);
}

void ImageResolutionDialog::setPixelHeight(int _i)
{
    ui->spinBoxHeight->setValue(_i);
}

int ImageResolutionDialog::pixelWidth()
{
    return ui->spinBoxWidth->value();
}

int ImageResolutionDialog::pixelHeight()
{
    return ui->spinBoxHeight->value();
}
