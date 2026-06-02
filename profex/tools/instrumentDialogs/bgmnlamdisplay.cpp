/***************************************************************************
                          bgmnlamdisplay.cpp  -  description
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

#include "bgmnlamdisplay.h"

BgmnLamDisplay::BgmnLamDisplay(QWidget *parent) : BgmnDataDisplay(parent)
{
    this->legend->setVisible(true);
    this->xAxis->setLabel(QString("Wavelength [Angstrom]"));
    stepSizeLambda = 0.00002;
}

void BgmnLamDisplay::loadFile(const QString &f)
{
    currentFileName = f;

    QFileInfo fi(currentFileName);
    setTitle(fi.fileName());

    reload();
}

void BgmnLamDisplay::reload()
{
    computeDisplayCurves();
    displayCurve(0);
}

void BgmnLamDisplay::clearData()
{
    this->clearGraphs();
    lamData.clear();
    this->replot();
}

bool BgmnLamDisplay::hasData()
{
    if (lamData.count()) return true;
    return false;
}

void BgmnLamDisplay::computeDisplayCurves()
{
    clearData();

    BgmnLamData ldata;
    int n = ldata.load(currentFileName);
    if (!n) return;

    double _mi = ldata.nativeToAngstrom(ldata.xmin());
    double _ma = ldata.nativeToAngstrom(ldata.xmax());

    xmin = qMin(_mi, _ma);
    xmax = qMax(_mi, _ma);

    QVector<double> x(xValues(xmin, xmax, stepSizeLambda));
    ldata.computeCurvesLambda(x);

    global::ProfileCurveData sumCurve;
    sumCurve.center = ldata.wavelength();
    sumCurve.xmin = min(ldata.pSumCurve().x);
    sumCurve.xmax = max(ldata.pSumCurve().x);
    sumCurve.ymin = min(ldata.pSumCurve().y);
    sumCurve.ymax = max(ldata.pSumCurve().y);
    sumCurve.x = ldata.pSumCurve().x;
    sumCurve.y = ldata.pSumCurve().y;
    lamData.append(sumCurve);

    for (int i = 0; i < ldata.count(); ++i) {
        global::ProfileCurveData subCurve;
        subCurve.center = ldata.wavelength();
        subCurve.xmin = min(ldata.pSubCurve(i).x);
        subCurve.xmax = max(ldata.pSubCurve(i).x);
        subCurve.ymin = min(ldata.pSubCurve(i).y);
        subCurve.ymax = max(ldata.pSubCurve(i).y);
        subCurve.x = ldata.pSubCurve(i).x;
        subCurve.y = ldata.pSubCurve(i).y;
        lamData.append(subCurve);
    }
}

void BgmnLamDisplay::displayCurve(int n)
{
    this->clearGraphs();

    if (!checkRanges(n)) {
        this->replot();
        return;
    }

    appendScan(lamData[0].x,
               lamData[0].y,
               QString("Emission Spectrum"),
               true);

    if (showSubCurves) {
        for (int i = 1; i < lamData.count(); ++i) {
            appendScan(lamData[i].x,
                       lamData[i].y,
                       QString(),
                       false);
        }
    }

    this->xAxis->setRange(xmin, xmax);
    this->yAxis->setRange(0.0, 1.05 * lamData[0].ymax);
    this->replot();
}

void BgmnLamDisplay::appendScan(const QVector<double> &x, const QVector<double> &y, const QString &name, bool fill)
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

QVector<double> BgmnLamDisplay::getXdata(int m) const
{
    if ((m < 1) || (m >= lamData.count())) return lamData[0].x;
    return lamData[m].x;
}

QVector<double> BgmnLamDisplay::getYdata(int m) const
{
    if ((m < 1) || (m >= lamData.count())) return lamData[0].y;
    return lamData[m].y;
}

QString BgmnLamDisplay::dataToCsv(int n)
{
    Q_UNUSED(n);

    if (!count()) return QString();

    qApp->setOverrideCursor(Qt::WaitCursor);
    QVector< QVector<double> > data;

    // x-values
    data.append(getXdata(0));

    // y-values of sub curves
    for (int i = 0; i < count(); ++i) {
        data.append(getYdata(i));
    }

    qApp->restoreOverrideCursor();
    return valuesToString(data);
}

double BgmnLamDisplay::getWavelength() const
{
    if (!lamData.count()) return 1.54056;
    return lamData[0].center;
}

bool BgmnLamDisplay::checkRanges(int i)
{
    Q_UNUSED(i);
    if (!lamData.count()) return false;
    return true;
}
