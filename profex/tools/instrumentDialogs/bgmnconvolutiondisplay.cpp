/***************************************************************************
                          bgmnconvolutiondisplay.cpp  -  description
                             -------------------
    begin                : Fri Feb 09 17:10:00 CEST 2018
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

/*
 * Good resource for explanation and visualization of signal convolutions:
 *
 * http://www.onmyphd.com/?p=convolution
 *
 */

#include "bgmnconvolutiondisplay.h"
#include "../libXrdIO/parser/convolutiondata.h"
#include "../libXrdIO/parser/bgmnlamdata.h"
#include "../libXrdIO/parser/bgmngeqdata.h"
#include "../libXrdIO/parser/bgmnsampledata.h"
#include "../libXrdIO/structs.h"
#include <QDebug>

BgmnConvolutionDisplay::BgmnConvolutionDisplay(QWidget *parent) : BgmnDataDisplay(parent)
{
    this->xAxis->setLabel(QString(tr("%1 Diffraction Angle [%2%3%4]"))
                          .arg(global::Delta)
                          .arg(global::degree)
                          .arg("2")
                          .arg(global::theta));

    stepSizeTwoTheta = 0.0005;
    b1 = 0.001;
    k1 = 0.0;
    k2 = 0.00005;
}

void BgmnConvolutionDisplay::reload()
{
    computeDisplayCurves();
    displayCurve(0);
}

void BgmnConvolutionDisplay::computeDisplayCurves()
{
    lamData.clear();
    geqData.clear();
    sampData.clear();
    convData.clear();

    BgmnLamData ldata;
    BgmnGeqData gdata;
    BgmnSampleData sdata;

    xmin = 180.0;
    xmax = -180.0;

    if (!ldata.load(currentLamFile)) {
        qDebug() << QString("BgmnConvolutionDisplay::compute(): Could not load file %1").arg(currentLamFile);
        return;
    }

    if (!gdata.load(currentGeqFile)) {
        qDebug() << QString("BgmnConvolutionDisplay::compute(): Could not load file %1").arg(currentGeqFile);
        return;
    }

    double wl = 0.1 * ldata.wavelength();

    for (int i = 0; i < gdata.count(); ++i) {
        double tt = 2.0 * qRadiansToDegrees(qAsin(gdata.sinTheta(i)));
        double d = wl / (2.0 * gdata.sinTheta(i));
        sdata.setCurveParameters(b1, k1, k2, 1.0 / d, wl);
        double mx = qMax(sdata.getCurveWidthL1(0.005), sdata.getCurveWidthL2(0.005));

        double geqXmin = gdata.geqNativeToTwoTheta(gdata.xmax(i));
        double geqXmax = gdata.geqNativeToTwoTheta(gdata.xmin(i));
        double lamXmin = ldata.nativeToTwoTheta(ldata.xmax(), d);
        double lamXmax = ldata.nativeToTwoTheta(ldata.xmin(), d);
        double samXmin = sdata.nativeToTwoTheta((1.0 / d) - mx, wl);
        double samXmax = sdata.nativeToTwoTheta((1.0 / d) + mx, wl);

        QVector<double> geqXvalues = xValues(qMin(geqXmin, geqXmax), qMax(geqXmin, geqXmax), stepSizeTwoTheta);
        QVector<double> lamXvalues = xValues(qMin(lamXmin, lamXmax), qMax(lamXmin, lamXmax), stepSizeTwoTheta);
        QVector<double> samXvalues = xValues(qMin(samXmin, samXmax), qMax(samXmin, samXmax), stepSizeTwoTheta);

        gdata.computeCurvesTwoTheta(geqXvalues);
        ldata.computeCurvesTwoTheta(lamXvalues, d);
        sdata.computeCurvesTwoTheta(samXvalues, wl);

        global::ProfileCurveData convDG;
        global::ProfileCurveData convDGP;

        convDG  = convolute(gdata.pSumCurve(i).x, gdata.pSumCurve(i).y, ldata.pSumCurve().x, ldata.pSumCurve().y);

        if (sdata.pCurveLc().x.size()) {
            convDGP = convolute(convDG.x, convDG.y, sdata.pCurveLc().x, sdata.pCurveLc().y);
        } else {
            convDGP = convDG;
        }

        addData(geqData,  gdata.pSumCurve(i).x, gdata.pSumCurve(i).y, max(gdata.pSumCurve(i).y));
        addData(lamData,  ldata.pSumCurve().x,  ldata.pSumCurve().y,  max(ldata.pSumCurve().y));
        addData(sampData, sdata.pCurveLc().x,   sdata.pCurveLc().y,   max(sdata.pCurveLc().y));
        addData(convData, convDGP.x, convDGP.y, max(convDGP.y));

        // don't move geqData to x=0, it already is
        offsetXvalues(lamData.last().x, -tt);
        offsetXvalues(convData.last().x, -tt);

        geqData.last().center = tt;
        lamData.last().center = tt;
        sampData.last().center = tt;
        convData.last().center = tt;

        geqData.last().xmin  = min(geqData.last().x);
        lamData.last().xmin  = min(lamData.last().x);
        sampData.last().xmin = min(sampData.last().x);
        convData.last().xmin = min(convData.last().x);

        geqData.last().xmax  = max(geqData.last().x);
        lamData.last().xmax  = max(lamData.last().x);
        sampData.last().xmax = max(sampData.last().x);
        convData.last().xmax = max(convData.last().x);

        xmin = qMin(xmin, convData.last().xmin);
        xmax = qMax(xmax, convData.last().xmax);

        emit setProgress(i);
        qApp->processEvents();
    }

    this->xAxis->setRange(xmin, xmax);
    this->yAxis->setRange(0.0, 105.0);
}

bool BgmnConvolutionDisplay::hasData()
{
    if (!convData.size()) return false;
    if (convData.first().x.size()) return true;
    return false;
}

void BgmnConvolutionDisplay::addData(QList<global::ProfileCurveData> &m, const QVector<double> &x, const QVector<double> &y, double ymax)
{
    m.append(global::ProfileCurveData());

    int imax = qMin(x.size(), y.size());
    m.last().x = QVector<double>(imax, 0.0);
    m.last().y = QVector<double>(imax, 0.0);

    for (int i = 0; i < imax; ++i) {
        m.last().x[i] = x[i];
        m.last().y[i] = 100.0 * y[i] / ymax;
    }
}

global::ProfileCurveData BgmnConvolutionDisplay::convolute(const QVector<double> &gx, const QVector<double> &gy, const QVector<double> &lx, const QVector<double> &ly)
{
    double stepSize = qAbs((gx.last() - gx.first()) / double(gx.size()));
    double firstX = gx.first() + lx.first();

    QVector<double> convY = ConvolutionData::convolute(ly, gy);

    double ymax = *std::max_element(convY.constBegin(), convY.constEnd()); // needs #include <algorithm>

    global::ProfileCurveData cdata;
    cdata.x = QVector<double>(convY.size(), 0.0);
    cdata.y = QVector<double>(convY.size(), 0.0);

    for (int i = 0; i < convY.size(); ++i) {
        cdata.x[i] = firstX + (i + 1) * stepSize;
        cdata.y[i] = 100.0 * convY[i] / ymax;
    }

    return cdata;
}

void BgmnConvolutionDisplay::clearData()
{
    this->clearGraphs();
    geqData.clear();
    lamData.clear();
    sampData.clear();
    convData.clear();
    this->replot();
}

void BgmnConvolutionDisplay::displayCurve(int c)
{
    currentCurve = c;
    this->clearGraphs();

    if (!checkRanges(c)) {
        this->replot();
        return;
    }

    QColor convColor = settings->getRandomColor(0);
    QColor lamColor  = settings->getRandomColor(1);
    QColor geqColor  = settings->getRandomColor(2);
    QColor samColor  = settings->getRandomColor(3);

    this->addGraph();
    this->graph(0)->addData(convData[c].x, convData[c].y);
    this->graph(0)->setName(QString("Convoluted data, 2%1 = %2%3").arg(global::theta).arg(convData[c].center, 0, 'f', 3).arg(global::degree));
    this->graph(0)->setPen(convColor);
    this->graph(0)->setBrush(QBrush(convColor.lighter(205)));

    if (showSubCurves) {
        this->addGraph();
        this->graph(1)->addData(lamData[c].x, lamData[c].y);
        this->graph(1)->setName("Wavelength distribution");
        this->graph(1)->setPen(lamColor);

        this->addGraph();
        this->graph(2)->addData(geqData[c].x, geqData[c].y);
        this->graph(2)->setName("Instrumental function");
        this->graph(2)->setPen(geqColor);

        this->addGraph();
        this->graph(3)->addData(sampData[c].x, sampData[c].y);
        this->graph(3)->setName("Sample contribution");
        this->graph(3)->setPen(samColor);
    }

    this->legend->setVisible(true);
    this->replot();
}

QVector<double> BgmnConvolutionDisplay::getXdata(int n, int c) const
{
    if (n == 0) return c < convData.count() ? convData.at(c).x : QVector<double>();
    if (n == 1) return c < lamData.count()  ? lamData.at(c).x  : QVector<double>();
    if (n == 2) return c < geqData.count()  ? geqData.at(c).x  : QVector<double>();
    return QVector<double>();
}

QVector<double> BgmnConvolutionDisplay::getYdata(int n, int c) const
{
    if (n == 0) return c < convData.count() ? convData.at(c).y : QVector<double>();
    if (n == 1) return c < lamData.count()  ? lamData.at(c).y  : QVector<double>();
    if (n == 2) return c < geqData.count()  ? geqData.at(c).y  : QVector<double>();
    return QVector<double>();
}

int BgmnConvolutionDisplay::count(int c) const
{
    int n = 0;
    if (convData.size() > c) ++n;
    if (lamData.size() > c) ++n;
    if (geqData.size() > c) ++n;
    return n;
}

QString BgmnConvolutionDisplay::dataToCsv(int c)
{
    if (!count(c)) {
        qDebug() << QString("BgmnConvolutionDisplay::dataToCsv(): "
                            "No data for peak No. %1. Available number of peaks = %2")
                    .arg(c)
                    .arg(convData.size());
        return QString();
    }

    qApp->setOverrideCursor(Qt::WaitCursor);
    QVector< QVector<double> > data;

    // x-values
    data.append(getXdata(0, c));
    qDebug() << QString("BgmnConvolutionDisplay::dataToCsv(): Convoluation data for peak no %1, %2 x-values added")
                .arg(c)
                .arg(data.last().size());

    // y-values of sub curves
    for (int i = 0; i < count(c); ++i) {
        data.append(getYdata(i, c));
        qDebug() << QString("BgmnConvolutionDisplay::dataToCsv(): "
                            "Convoluation data for peak no %1, %2 y-values added to curve %3")
                    .arg(c)
                    .arg(data.last().size())
                    .arg(i);
    }

    qApp->restoreOverrideCursor();
    return valuesToString(data);
}

bool BgmnConvolutionDisplay::checkRanges(int i)
{
    if (!convData.size()) return false;
    int cmax = qMin(convData.count(), qMin(lamData.count(), geqData.count()));
    if (i >= cmax) return false;
    return true;
}

void BgmnConvolutionDisplay::setDvals(QVector<double> ttVals, double lambda)
{
    dvalues = QVector<double>(ttVals.size(), 0.0);

    for (int i = 0; i < ttVals.size(); ++i) {
        dvalues[i] = lambda / (2.0 * qSin(qDegreesToRadians(0.5 * ttVals[i])));
    }
}
