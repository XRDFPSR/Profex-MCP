/***************************************************************************
                          bgmngeqdisplay.cpp  -  description
                             -------------------
    begin                : Tue Feb 08 21:10:00 CEST 2018
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

#include "bgmngeqdisplay.h"
#include "../libXrdIO/structs.h"
#include <QDebug>

BgmnGeqDisplay::BgmnGeqDisplay(QWidget *parent) : BgmnDataDisplay(parent)
{
    this->xAxis->setLabel(QString(tr("%1 Diffraction Angle [%2%3%4]"))
                          .arg(global::Delta)
                          .arg(global::degree)
                          .arg("2")
                          .arg(global::theta));
    normY = true;
    stepSizeTwoTheta = 0.0005;
    globalYmax = 0.0;

    this->legend->setVisible(true);
}

void BgmnGeqDisplay::loadFile(const QString &f)
{
    currentFileName = f;
    QFileInfo fi(currentFileName);
    setTitle(fi.fileName());
    reload();
}

void BgmnGeqDisplay::reload()
{
    currentCurve = 0;
    zoomed = false;

    computeDisplayCurves();
    displayCurve(currentCurve);
}

void BgmnGeqDisplay::clearData()
{
    this->clearGraphs();
    geqData.clear();
    this->replot();
}

bool BgmnGeqDisplay::hasData()
{
    if (geqData.count()) return true;
    return false;
}

void BgmnGeqDisplay::computeDisplayCurves()
{
    geqData.clear();
    BgmnGeqData gdata;

    int n = gdata.load(currentFileName);
    if (!n) return;

    double _min = gdata.geqNativeToTwoTheta(gdata.xmin(0));
    double _max = gdata.geqNativeToTwoTheta(gdata.xmax(0));
    xmin = qMin(_min, _max);
    xmax = qMax(_min, _max);

    QVector<double> x(xValues(xmin, xmax, stepSizeTwoTheta));
    gdata.computeCurvesTwoTheta(x);

    for (int i = 0; i < gdata.count(); ++i) {
        double tt = 2.0 * qRadiansToDegrees(qAsin(gdata.sinTheta(i)));

        geqData.append(QVector<global::ProfileCurveData>());

        global::ProfileCurveData sumCurve;
        sumCurve.center = tt;
        sumCurve.xmin = min(gdata.pSumCurve(i).x);
        sumCurve.xmax = max(gdata.pSumCurve(i).x);
        sumCurve.ymin = min(gdata.pSumCurve(i).y);
        sumCurve.ymax = max(gdata.pSumCurve(i).y);
        sumCurve.x = gdata.pSumCurve(i).x;
        sumCurve.y = gdata.pSumCurve(i).y;
        geqData.last().append(sumCurve);

        globalYmax = qMax(globalYmax, sumCurve.ymax);

        for (int j = 0; j < gdata.subCurveCount(i); ++j) {
            global::ProfileCurveData subCurve;
            subCurve.center = tt;
            subCurve.xmin = min(gdata.pSubCurve(i, j).x);
            subCurve.xmax = max(gdata.pSubCurve(i, j).x);
            subCurve.ymin = min(gdata.pSubCurve(i, j).y);
            subCurve.ymax = max(gdata.pSubCurve(i, j).y);
            subCurve.x = gdata.pSubCurve(i, j).x;
            subCurve.y = gdata.pSubCurve(i, j).y;
            geqData.last().append(subCurve);
        }
    }
}

void BgmnGeqDisplay::displayCurve(int n)
{
    currentCurve = n;

    double currentXmin = this->xAxis->range().lower;
    double currentXmax = this->xAxis->range().upper;

    this->clearGraphs();

    if (!checkRanges(currentCurve)) {
        this->replot();
        return;
    }

    appendScan(geqData[currentCurve][0].x,
               geqData[currentCurve][0].y,
               QString("2%1 = %2%3").arg(global::theta).arg(geqData[currentCurve][0].center, 0, 'f', 3).arg(global::degree),
               true);

    if (showSubCurves) {
        for (int i = 1; i < geqData[n].count(); ++i) {
            appendScan(geqData[currentCurve][i].x,
                       geqData[currentCurve][i].y,
                       QString(),
                       false);
        }
    }

    xmin = geqData[currentCurve][0].xmin;
    xmax = geqData[currentCurve][0].xmax;

    if (zoomed) this->xAxis->setRange(currentXmin, currentXmax);
    else        this->xAxis->setRange(xmin, xmax);

    this->yAxis->setRange(0.0, normY ? geqData[currentCurve][0].ymax : globalYmax);

    this->replot();
}

void BgmnGeqDisplay::appendScan(const QVector<double> &x, const QVector<double> &y, const QString &name, bool fill)
{
    int g = this->graphCount();
    QColor colLine = settings->getRandomColor(g);

    this->addGraph();
    this->graph(g)->addData(x, y);
    this->graph(g)->setPen(colLine);

    if (name.isEmpty()) {
        this->graph(g)->removeFromLegend();
    } else {
        this->graph(g)->setName(name);
    }

    if (fill) {
        this->graph(g)->setBrush(QBrush(colLine.lighter(205)));
    }
}

QVector<double> BgmnGeqDisplay::getXdata(int curve, int subcurve) const
{
    int n = curve;
    int m = subcurve;

    if ((n < 0) || (n >= geqData.size()))    n = currentCurve;
    if ((m < 0) || (m >= geqData[n].size())) m = 0;

    return geqData[n][m].x;
}

QVector<double> BgmnGeqDisplay::getYdata(int curve, int subcurve) const
{
    int n = curve;
    int m = subcurve;

    if ((n < 0) || (n >= geqData.size()))    n = currentCurve;
    if ((m < 0) || (m >= geqData[n].size())) m = 0;

    return geqData[n][m].y;
}

int BgmnGeqDisplay::count() const
{
    return geqData.count();
}

QString BgmnGeqDisplay::dataToCsv(int c)
{
    if ((c < 0) || (c >= geqData.count())) return QString();

    qApp->setOverrideCursor(Qt::WaitCursor);
    QVector<QVector<double> > data;

    // x-values
    data.append(getXdata(c, 0));

    // y-values of sub curves
    for (int i = 0; i < geqData[c].count(); ++i) {
        data.append(getYdata(c, i));
    }

    qApp->restoreOverrideCursor();
    return valuesToString(data);
}

QVector<double> BgmnGeqDisplay::peakPositions()
{
    QVector<double> v(geqData.count(), 0.0);

    for (int i = 0; i < geqData.count(); ++i) {
        v[i] = geqData[i][0].center;
    }

    return v;
}

bool BgmnGeqDisplay::checkRanges(int i)
{
    if (!geqData.count())     return false;  // do we have any curves
    if (i >= geqData.count()) return false;  // is the requested curve available
    if (!geqData[i].count())  return false;  // does the requested curve contain at least 1 sum or sub curve

    return true;
}
