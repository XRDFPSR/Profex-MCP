/***************************************************************************
                          xrdcustomplot.cpp  -  description
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

#include "xrdcustomplot.h"
#include "../libXrdIO/scan.h"
#include "../libXrdIO/import/importhandler.h"

XrdCustomPlot::XrdCustomPlot(QWidget *parent) : QCustomPlot(parent)
{
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    this->xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
    this->xAxis->axisRect()->setRangeZoom(Qt::Horizontal);
    stepSize = 0.005;

    connect(this->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(zoomX(QCPRange)));
    connect(this, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(zoomReset(QMouseEvent*)));
}

void XrdCustomPlot::zoomX(const QCPRange &newRange)
{
    double wmin = qMin(xmin, xmax);
    double wmax = qMax(xmin, xmax);

    QCPRange fixedRange(newRange);

    if (fixedRange.lower < wmin)
    {
        fixedRange.lower = wmin;
        fixedRange.upper = wmin + newRange.size();

        if (fixedRange.upper > wmax || qFuzzyCompare(newRange.size(), wmax-wmin)) {
            fixedRange.upper = wmax;
        }

        this->xAxis->setRange(fixedRange);
    } else if (fixedRange.upper > wmax)
    {
        fixedRange.upper = wmax;
        fixedRange.lower = wmax - newRange.size();

        if (fixedRange.lower < wmin || qFuzzyCompare(newRange.size(), wmax-wmin)) {
            fixedRange.lower = wmin;
        }

        this->xAxis->setRange(fixedRange);
    }

    this->replot();
}

void XrdCustomPlot::zoomReset(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        this->xAxis->setRange(qMin(xmin, xmax), qMax(xmin, xmax));
        this->replot();
    }
}

void XrdCustomPlot::setXLimits(double mi, double ma)
{
    xmin = mi;
    xmax = ma;
}

void XrdCustomPlot::loadScan(const QString &s, const QString &uid)
{
    this->clearGraphs();

    xmin = std::numeric_limits<double>::max();
    xmax = 0.0;

    QVector<Scan> scanHeap;
    ImportHandler iHandler;

    QString _uid = uid.isEmpty() ? iHandler.uidByFileName(s) : uid;
    iHandler.load(s, _uid, scanHeap, true);

    for (int i = 0; i < scanHeap.size(); ++i) {
        this->addGraph();
        this->graph(this->graphCount() - 1)->addData(scanHeap[i].pDataAngle(), scanHeap[i].pDataIntensity());
        this->graph(this->graphCount() - 1)->setName(scanHeap[i].name());
        this->graph(this->graphCount() - 1)->rescaleAxes(true);
        xmin = qMin(xmin, scanHeap.at(i).minAngle());
        xmax = qMax(xmax, scanHeap.at(i).maxAngle());
    }

    if (scanHeap.size()) stepSize = scanHeap.first().stepSize();

    this->xAxis->setRange(qMin(xmin, xmax), qMax(xmin, xmax));
    this->replot();
}

bool XrdCustomPlot::getValues(int n, QVector<double> &x, QVector<double> &y)
{
    if (n >= this->graphCount()) return false;

    x.clear();
    y.clear();

    for (int i = 0; i < this->graph(n)->data()->size(); ++i) {
        x.append(this->graph(n)->data()->at(i)->key);
        y.append(this->graph(n)->data()->at(i)->value);
    }

    return true;
}

bool XrdCustomPlot::getVisibleValues(int n, QVector<double> &x, QVector<double> &y)
{
    if (n >= this->graphCount()) return false;

    x.clear();
    y.clear();

    for (int i = 0; i < this->graph(n)->data()->size(); ++i) {
        double dx = this->graph(n)->data()->at(i)->key;

        if (dx < this->xAxis->range().lower) continue;
        if (dx > this->xAxis->range().upper) break;

        x.append(dx);
        y.append(graph(n)->data()->at(i)->value);
    }

    return true;
}
