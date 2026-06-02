/***************************************************************************
                          xrdcustomplot.h  -  description
                             -------------------
    begin                : Thu May 31 09:00:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#ifndef XRDCUSTOMPLOT_H
#define XRDCUSTOMPLOT_H

#include "3rdparty/qcustomplot/qcustomplot.h"

class XrdCustomPlot : public QCustomPlot
{
    Q_OBJECT

public:
    explicit XrdCustomPlot(QWidget *parent = Q_NULLPTR);

    void setXLimits(double, double);
    void loadScan(const QString &s, const QString &uid = QString());

    bool getValues(int n, QVector<double> &, QVector<double> &);
    bool getVisibleValues(int n, QVector<double> &, QVector<double> &);
    inline double getStepSize() const {return stepSize;}

private:
    double xmin, xmax, stepSize;

private slots:
    void zoomX(const QCPRange &);
    void zoomReset(QMouseEvent *);
};

#endif // XRDCUSTOMPLOT_H
