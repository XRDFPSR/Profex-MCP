/***************************************************************************
                          bgmnsampledisplay.cpp  -  description
                             -------------------
    begin                : Wed May 23 21:35:00 CEST 2018
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

#include "bgmnsampledisplay.h"
#include "../libXrdIO/structs.h"

BgmnSampleDisplay::BgmnSampleDisplay(QWidget *parent) : BgmnDataDisplay(parent)
{
    this->legend->setVisible(true);
    this->xAxis->setLabel(QString(tr("1/d [1/nm]")));

    waveLength = 0.154056;
    b1 = 0.001;
    k1 = 0.0;
    k2 = 0.00005;
}

void BgmnSampleDisplay::reload()
{
    computeDisplayCurves();
    displayCurve(0);
}

void BgmnSampleDisplay::clearData()
{
    this->clearGraphs();
    sampData.clear();
    this->replot();
}

bool BgmnSampleDisplay::hasData()
{
    // if (!sampData.count()) return false;
    // if (!sampData[0].count()) return false;
    return true;
}

void BgmnSampleDisplay::computeDisplayCurves()
{
    qDebug() << QString("b1=%1, k1=%2, k2=%3").arg(b1).arg(k1).arg(k2);
    sampData.clear();

    double _mi = std::numeric_limits<double>::max();
    double _ma = std::numeric_limits<double>::min();

    for (int i = 0; i < dInvValues.size(); ++i) {
        BgmnSampleData sdata;
        sdata.setCurveParameters(b1, k1, k2, dInvValues[i], waveLength);
        double mx = qMax(sdata.getCurveWidthL1(0.005), sdata.getCurveWidthL2(0.005));
        sdata.computeCurvesNative(xValues(dInvValues[i] - mx, dInvValues[i] + mx, 0.002 * mx));

        global::ProfileCurveData curveL1;
        global::ProfileCurveData curveL2;
        global::ProfileCurveData curveLc;

        curveLc.center = 2.0 * qRadiansToDegrees(qAsin(0.5 * waveLength * dInvValues[i]));
        curveLc.xmin = sdata.pCurveLc().x.first();
        curveLc.xmax = sdata.pCurveLc().x.last();
        curveLc.ymin = 0.0;
        curveLc.ymax = max(sdata.pCurveLc().y);

        curveL1.center = curveLc.center;
        curveL1.xmin = curveLc.xmin;
        curveL1.xmax = curveLc.xmax;
        curveL1.ymin = curveLc.ymin;
        curveL1.ymax = curveLc.ymax;

        curveL2.center = curveLc.center;
        curveL2.xmin = curveLc.xmin;
        curveL2.xmax = curveLc.xmax;
        curveL2.ymin = curveLc.ymin;
        curveL2.ymax = curveLc.ymax;

        curveL1.x = sdata.pCurveL1().x;
        curveL1.y = sdata.pCurveL1().y;

        curveL2.x = sdata.pCurveL2().x;
        curveL2.y = sdata.pCurveL2().y;

        curveLc.x = sdata.pCurveLc().x;
        curveLc.y = sdata.pCurveLc().y;

        sampData.append(QVector<global::ProfileCurveData>());
        sampData.last().append(curveLc);
        sampData.last().append(curveL1);
        sampData.last().append(curveL2);

        _mi = qMin(_mi, curveLc.xmin);
        _ma = qMax(_ma, curveLc.xmax);
    }

    xmin = _mi;
    xmax = _ma;
    this->yAxis->setRange(0.0, 105.0);
}

void BgmnSampleDisplay::displayCurve(int n)
{
    currentCurve = n;

    double currentXmin = this->xAxis->range().lower;
    double currentXmax = this->xAxis->range().upper;

    this->clearGraphs();

    if (!checkRanges(currentCurve)) {
        this->replot();
        return;
    }

    QColor colLine = settings->getRandomColor(0);

    this->addGraph();
    this->graph(0)->addData(sampData[currentCurve][0].x, sampData[currentCurve][0].y);
    this->graph(0)->setName(tr("Sample Function L1*L2, 2%1 = %2%3")
                            .arg(global::theta)
                            .arg(sampData[currentCurve][0].center, 0, 'f', 3)
                            .arg(global::degree));
    this->graph(0)->setPen(colLine);
    this->graph(0)->setBrush(QBrush(colLine.lighter(205)));

    if (showSubCurves) {
        if (sampData[currentCurve].size() > 1) {
            this->addGraph();
            this->graph(1)->addData(sampData[currentCurve][1].x, sampData[currentCurve][1].y);
            this->graph(1)->setName(tr("Crystallite Size Broadening B1"));
            this->graph(1)->setPen(settings->getRandomColor(1));
        }

        if (sampData[currentCurve].size() > 2) {
            this->addGraph();
            this->graph(2)->addData(sampData[currentCurve][2].x, sampData[currentCurve][2].y);
            this->graph(2)->setName(tr("Microstrain Broadening k2"));
            this->graph(2)->setPen(settings->getRandomColor(2));
        }
    }

    if (zoomed) this->xAxis->setRange(currentXmin, currentXmax);
    else        this->xAxis->setRange(xmin, xmax);

    this->replot();
}

QString BgmnSampleDisplay::dataToCsv(int c)
{
    if ((c < 0) || (c >= sampData.count())) return QString();

    qApp->setOverrideCursor(Qt::WaitCursor);
    QVector<QVector<double> > data;

    // x-values
    data.append(getXdata(c, 0));

    // y-values of sub curves
    for (int i = 0; i < sampData[c].count(); ++i) {
        data.append(getYdata(c, i));
    }

    qApp->restoreOverrideCursor();
    return valuesToString(data);
}

/*
 * lambda and d in nm
 */
void BgmnSampleDisplay::setPeakPositionsTwoTheta(QVector<double> ttVals, double lambda)
{
    waveLength = 0.1 * lambda;
    dInvValues = QVector<double>(ttVals.size(), 0.0);

    for (int i = 0; i < ttVals.size(); ++i) {
        dInvValues[i] = (2.0 * qSin(qDegreesToRadians(0.5 * ttVals[i]))) / waveLength;
    }
}

bool BgmnSampleDisplay::checkRanges(int i)
{
    if (!sampData.count())     return false;  // do we have any curves
    if (i >= sampData.count()) return false;  // is the requested curve available
    if (!sampData[i].count())  return false;  // does the requested curve contain at least 1 sum or sub curve

    return true;
}

QVector<double> BgmnSampleDisplay::getXdata(int curve, int contribution) const
{
    int n = curve;
    int m = contribution;

    if ((n < 0) || (n >= sampData.size()))    n = currentCurve;
    if ((m < 0) || (m >= sampData[n].size())) m = 0;

    return sampData[n][m].x;
}

QVector<double> BgmnSampleDisplay::getYdata(int curve, int contribution) const
{
    int n = curve;
    int m = contribution;

    if ((n < 0) || (n >= sampData.size()))    n = currentCurve;
    if ((m < 0) || (m >= sampData[n].size())) m = 0;

    return sampData[n][m].y;
}
