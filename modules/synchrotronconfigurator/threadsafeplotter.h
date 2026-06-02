/***************************************************************************
                          threadsafeplotter.h  -  description
                             -------------------
    begin                : Tue Jan 07 18:39:00 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#ifndef THREADSAFEPLOTTER_H
#define THREADSAFEPLOTTER_H

#include <QObject>
#include "qcustomplot/qcustomplot.h"
#include "../libXrdIO/scan.h"

class ThreadSafePlotter : public QObject
{
    Q_OBJECT
public:
    ThreadSafePlotter(QObject *parent = nullptr);
    ~ThreadSafePlotter();

    void setPlotter(QCustomPlot *p, const QColor &, const QColor &, const QColor &);

    void reset();
    void setPeakData(const Scan &peak);
    void setLorentzCurves(const QList<Scan> &);
    void rescaleAxes(bool x, bool y);

private:
    QMutex m_mutex;
    QCustomPlot *plot;
    QColor colPeak;
    QColor colCurve;
    QColor colSum;

    bool getBoundaries(double &xmin, double &xmax, double &ymin, double &ymax);

private slots:
    void resetPeakPlotZoom(QMouseEvent*);
    void zoomPeakPlot(QCPRange);

};

#endif // THREADSAFEPLOTTER_H
