/***************************************************************************
                          zoomrangedialog.h  -  description
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

#ifndef ZOOMRANGEDIALOG_H
#define ZOOMRANGEDIALOG_H

#include <QDialog>

namespace Ui {
class ZoomRangeDialog;
}

class ZoomRangeDialog : public QDialog
{
    Q_OBJECT

public:
    explicit ZoomRangeDialog(QWidget *parent = Q_NULLPTR);
    ~ZoomRangeDialog();

    void setIdealValues(double x1, double x2, double y1, double y2);
    void setValues(double x1, double x2, double y1, double y2);
    void getValues(double &x1, double &x2, double &y1, double &y2);

private:
    Ui::ZoomRangeDialog *ui;

    double i_ang_min;
    double i_ang_max;
    double i_int_min;
    double i_int_max;

private slots:
    void resetValues();
};

#endif // ZOOMRANGEDIALOG_H
