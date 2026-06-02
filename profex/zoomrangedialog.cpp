/***************************************************************************
                          zoomrangedialog.cpp  -  description
                             -------------------
    begin                : Thu Feb 13 15:00:00 CEST 2014
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

#include "zoomrangedialog.h"
#include "ui_zoomrangedialog.h"
#include <QDebug>

ZoomRangeDialog::ZoomRangeDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ZoomRangeDialog)
{
    ui->setupUi(this);

    ui->doubleSpinBoxMinA->setMinimum(-360.0);
    ui->doubleSpinBoxMinA->setMaximum(360.0);
    ui->doubleSpinBoxMinA->setSingleStep(10.0);

    ui->doubleSpinBoxMaxA->setMinimum(-360.0);
    ui->doubleSpinBoxMaxA->setMaximum(360.0);
    ui->doubleSpinBoxMaxA->setSingleStep(10.0);

    ui->doubleSpinBoxMinI->setMinimum(-9999999.0);
    ui->doubleSpinBoxMinI->setMaximum(9999999.0);
    ui->doubleSpinBoxMinI->setSingleStep(10.0);

    ui->doubleSpinBoxMaxI->setMinimum(-9999999.0);
    ui->doubleSpinBoxMaxI->setMaximum(9999999.0);
    ui->doubleSpinBoxMaxI->setSingleStep(10.0);

    i_int_min = 0.0;
    i_int_max = 2000.0;
    i_ang_min = 5.0;
    i_ang_max = 60.0;
}

ZoomRangeDialog::~ZoomRangeDialog()
{
    delete ui;
}

/*
 * sets ideal values for the spin boxes, these are the minimum and maximum intensities and
 * angles of the plot. The same values as used for resetting the zoom range
 */
void ZoomRangeDialog::setIdealValues(double x1, double x2, double y1, double y2)
{
    i_ang_min =qMin(x1, x2);
    i_ang_max =qMax(x1, x2);
    i_int_min =qMin(y1, y2);
    i_int_max =qMax(y1, y2);
}

/*
 * sets the values of the spin boxes
 */
void ZoomRangeDialog::setValues(double x1, double x2, double y1, double y2)
{
    ui->doubleSpinBoxMinA->setValue(x1);
    ui->doubleSpinBoxMaxA->setValue(x2);
    ui->doubleSpinBoxMinI->setValue(y1);
    ui->doubleSpinBoxMaxI->setValue(y2);
}

/*
 * returns the values from the spin boxes
 */
void ZoomRangeDialog::getValues(double &x1, double &x2, double &y1, double &y2)
{
    x1 = qMin(ui->doubleSpinBoxMinA->value(), ui->doubleSpinBoxMaxA->value());
    x2 = qMax(ui->doubleSpinBoxMinA->value(), ui->doubleSpinBoxMaxA->value());
    y1 = qMin(ui->doubleSpinBoxMinI->value(), ui->doubleSpinBoxMaxI->value());
    y2 = qMax(ui->doubleSpinBoxMinI->value(), ui->doubleSpinBoxMaxI->value());
}

/*
 * resets the values to the ideal values of the plot (minimum and maximum measured intensities and angles)
 */
void ZoomRangeDialog::resetValues()
{
    ui->doubleSpinBoxMinA->setValue(i_ang_min);
    ui->doubleSpinBoxMaxA->setValue(i_ang_max);
    ui->doubleSpinBoxMinI->setValue(i_int_min);
    ui->doubleSpinBoxMaxI->setValue(i_int_max);
}
