/***************************************************************************
                          threadsafeplotter.cpp  -  description
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

#include "threadsafeplotter.h"
#include <QMutex>
#include <QMutexLocker>

ThreadSafePlotter::ThreadSafePlotter(QObject *parent)
    : QObject(parent)
{
    plot = nullptr;
    colPeak = QColor(Qt::blue);
    colCurve = QColor(Qt::red);
    colSum = QColor(Qt::green);
}

ThreadSafePlotter::~ThreadSafePlotter()
{}

void ThreadSafePlotter::reset()
{
    if (!plot) return;
    QMutexLocker locker(&m_mutex);

    // fixed plot positions:
    // 1: Peak function

    while (plot->graphCount() > 1) { // remove plots 1 .. n
        plot->removeGraph(plot->graphCount() - 1);
    }

    plot->graph(0)->data()->clear();
    plot->replot();
}

void ThreadSafePlotter::setPlotter(QCustomPlot *p, const QColor &cPeak, const QColor &cCurves, const QColor &cSum)
{
    plot = p;
    colPeak = cPeak;
    colCurve = cCurves;
    colSum = cSum;
}

void ThreadSafePlotter::setPeakData(const Scan &peak)
{
    if (!plot) return;
    QMutexLocker locker(&m_mutex);

    plot->graph(0)->setPen(colPeak);
    plot->graph(0)->setData(peak.pDataAngle(), peak.pDataIntensity());
}

/*
 * provide Lorentz curves in -theta [rad].
 * Will be converted to 2theta [deg] by the plotter.
 */
void ThreadSafePlotter::setLorentzCurves(const QList<Scan> &scans)
{
    if (!plot) return;
    QMutexLocker locker(&m_mutex);

    for (int i = 0; i < scans.size(); ++i) {
        int m = scans.at(i).size();
        QList<double> x(m, 0.0);
        QList<double> y(m, 0.0);

        for (int j = 0; j < m; ++j) {
            x[j] = -scans.at(i).pDataAngle().at(m - j - 1) * 360.0 / M_PI;
            y[j] = scans.at(i).pDataIntensity().at(m - j - 1);
        }

        int n = plot->graphCount();
        plot->addGraph();
        plot->graph(n)->setPen(i == scans.size() - 1 ? colSum : colCurve);
        plot->graph(n)->setData(x, y);
    }
}

void ThreadSafePlotter::rescaleAxes(bool x, bool y)
{
    if (!plot) return;

    double minX;
    double maxX;
    double minY;
    double maxY;

    if (!getBoundaries(minX, maxX, minY, maxY)) return;

    double rx = 0.025 * (maxX - minX);
    if (x) plot->xAxis->setRange(minX - rx, maxX + rx);
    if (y) plot->yAxis->setRange(minY, maxY * 1.025);
    plot->replot();
}

bool ThreadSafePlotter::getBoundaries(double &xmin, double &xmax, double &ymin, double &ymax)
{
    xmin = std::numeric_limits<double>::max();
    xmax = 0.0;
    ymin = std::numeric_limits<double>::max();
    ymax = 0.0;
    int validRanges = 0;

    for (int i = 0; i < plot->graphCount(); ++i) {
        if (!plot->graph(i)->data()->size()) continue;

        bool xok, yok;
        QCPRange xrange = plot->graph(i)->getKeyRange(xok);
        QCPRange yrange = plot->graph(i)->getValueRange(yok);

        if (xok && yok) ++validRanges;
        else            continue;

        xmin = qMin(xmin, xrange.lower);
        xmax = qMax(xmax, xrange.upper);
        ymin = qMin(ymin, yrange.lower);
        ymax = qMax(ymax, yrange.upper);
    }

    return validRanges > 0;
}

void ThreadSafePlotter::resetPeakPlotZoom(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) rescaleAxes(true, true);
}

void ThreadSafePlotter::zoomPeakPlot(QCPRange newRange)
{
    double xmin;
    double xmax;
    double ymin;
    double ymax;

    if (!getBoundaries(xmin, xmax, ymin, ymax)) return;

    QCPRange fixedRange(newRange);

    if (fixedRange.lower < xmin)
    {
        fixedRange.lower = xmin;
        fixedRange.upper = xmin + newRange.size();

        if (fixedRange.upper > xmax || qFuzzyCompare(newRange.size(), xmax - xmin)) {
            fixedRange.upper = xmax;
        }

        plot->xAxis->setRange(fixedRange);
    } else if (fixedRange.upper > xmax)
    {
        fixedRange.upper = xmax;
        fixedRange.lower = xmax - newRange.size();

        if (fixedRange.lower < xmin || qFuzzyCompare(newRange.size(), xmax - xmin)) {
            fixedRange.lower = xmin;
        }

        plot->xAxis->setRange(fixedRange);
    }

    plot->replot();
}
