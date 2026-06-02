/***************************************************************************
                          emapexportcoordinatedialog.cpp  -  description
                             -------------------
    begin                : Fri Jul 15 18:00:00 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#include "emapexportcoordinatedialog.h"
#include "ui_emapexportcoordinatedialog.h"

EMapExportCoordinateDialog::EMapExportCoordinateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EMapExportCoordinateDialog)
{
    ui->setupUi(this);
    ui->radioButtonCartesian->setChecked(true);
    ui->radioButtonMap->setChecked(true);
}

EMapExportCoordinateDialog::~EMapExportCoordinateDialog()
{
    delete ui;
}

int EMapExportCoordinateDialog::dataMode()
{
    return ui->radioButtonMap->isChecked() ? 0 : 1;
}

int EMapExportCoordinateDialog::coordinateMode()
{
    return ui->radioButtonFractional->isChecked() ? 0 : 1;
}
