/***************************************************************************
                          scanops.cpp  -  description
                             -------------------
    begin                : Tue Jan 23 20:16:07 CEST 2018
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

#include "scanops.h"
#include "3rdparty/alglib/src/interpolation.h"
#include "3rdparty/alglib/src/stdafx.h"
#include "3rdparty/alglib/src/dataanalysis.h"
#include "3rdparty/alglib/src/statistics.h"
#include "curveFitting/gaussiancurve.h"
#include "curveFitting/linearcurve.h"
#include "functions.h"
#include <QPointF>
#include <QtMath>
#include <QMap>
#include <QDebug>
#include <algorithm>
#include <limits>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

ScanOps::ScanOps()
{}

Scan ScanOps::convertDivSlitToADS( const Scan &sc, double l, double a, double r )
{
    Scan s = sc.clone();

    QVector<double>::iterator itAng = s.pDataAngle().begin();
    QVector<double>::iterator itInt = s.pDataIntensity().begin();

    for (int i = 0; i < s.pDataIntensity().size(); ++i) {
        if (i >= s.pDataAngle().size()) return s;

        double da = (r * qSin(qDegreesToRadians(a/2.0))) / l;
        double db = (1.0/(qSin(qDegreesToRadians(*itAng + a/2.0))) + 1.0/(qSin(qDegreesToRadians(*itAng - a/2.0))) );

        *itInt /= (da * db);

        ++itAng;
        ++itInt;
    }

    return s;
}

Scan ScanOps::convertDivSlitToFDS( const Scan &sc, double l, double a, double r )
{
    Scan s = sc.clone();

    QVector<double>::iterator itAng = s.pDataAngle().begin();
    QVector<double>::iterator itInt = s.pDataIntensity().begin();

    for (int i = 0; i < s.pDataIntensity().size(); ++i) {
        if (i >= s.pDataAngle().size()) return s;

        double da = ( r * qSin(qDegreesToRadians(a/2))) / l;
        double db = ( 1.0/(qSin(qDegreesToRadians(*itAng + a/2.0))) + 1.0/(qSin(qDegreesToRadians(*itAng - a/2.0))) );

        *itInt *= (da * db);

        ++itAng;
        ++itInt;
    }

    return s;
}

double ScanOps::correlate(const Scan &scanA, const Scan &scanB)
{
    if (scanA.pDataIntensity().size() != scanB.pDataIntensity().size()) {
        return 0.0;
    }

    alglib::real_1d_array mRef;
    alglib::real_1d_array mAct;

    mRef.setcontent(scanA.pDataIntensity().size(), scanA.pDataIntensity().data());
    mAct.setcontent(scanB.pDataIntensity().size(), scanB.pDataIntensity().data());

    return alglib::pearsoncorr2(mAct, mRef);
}

/*
 * Returns the integrated intensity between data points a and b.
 * Set bg = true to subtract a linear background between a and b.
 */
double ScanOps::integrate(const Scan *sc, int a, int b, bool bg)
{
    int a_start = qMin(a, b);
    int a_end = qMax(a, b);
    int dataSize = qMin(sc->pDataIntensity().size(), sc->pDataAngle().size());

    a_start = a_start < 0 ? 0 : a_start;
    a_end = a_end >= dataSize ? dataSize - 1 : a_end;

    if (a_end == a_start) return 0.0;

    double i_integral = 0.0;

    if (bg) {
        double i_start = sc->pDataIntensity().at(a_start);
        double i_end   = sc->pDataIntensity().at(a_end);

        for (int i = a_start; i < a_end - 1; ++i) {
            double frac = double(i - a_start) / double(a_end - a_start);

            // integral +=  stepsize                           *  intensity
            i_integral  += (sc->pDataAngle().at(i+1) - sc->pDataAngle().at(i)) * (sc->pDataIntensity().at(i) - linearInterpolation(i_start, i_end, frac)) * sc->scaleFactor();
        }
    } else {
        for (int i = a_start; i < a_end - 1; ++i) {
            // integral +=  stepsize                           *  intensity
            i_integral  += (sc->pDataAngle().at(i+1) - sc->pDataAngle().at(i)) * (sc->pDataIntensity().at(i)) * sc->scaleFactor();
        }
    }

    return i_integral;
}

/*
 * Returns the integrated intensity between angle a and b.
 * Set bg = true to subtract a linear background between a and b.
 * If a and b are not precise data points, the range will be extended,
 * i.e. start of the integration will be the next data point before a,
 * and end of the integration will be the next data point after b.
 */
double ScanOps::integrate(const Scan *sc, double a, double b, bool bg)
{
    int a_start = sc->indexOfAngle(qMin(a, b), 1);
    int a_end   = sc->indexOfAngle(qMax(a, b), 2);

    return integrate(sc, a_start, a_end, bg);
}

double ScanOps::linearInterpolation(double start, double end, double frac)
{
    return start + frac * (end - start);
}

/*
 * SNIP algorithm taken from:
 * Morhac, M. "An algorithm for determinatino of peak regions and baseline elimination in
 * spectroscopic data", Nuclear instruments and methods in Physics Research A 600 (2009), 478-487.
 *
 * mode = 0: Increasing window (algorithm A)
 * mode = 1: Decreasing window (algorithm C)
 */
Scan ScanOps::baseLineSNIP(const Scan &sc, int m, int mode)
{
    Scan s = sc.clone();
    s.setHklData(QVector<Hkl>());

    if (mode == 0) snipAlgoA(s.pDataIntensity(), m);
    if (mode == 1) snipAlgoC(s.pDataIntensity(), m);

    s.setName(QString("Baseline %1").arg(sc.name()));
    s.setAuxInfo("isBaseLine", QVariant(true));
    s.setAuxInfo("baseLineParameters", QList<QVariant>() << m << mode);
    s.setColor(QString());
    s.setTypes(Scan::XY | Scan::SYNTHETIC | Scan::BACKGROUND);

    return s;
}

void ScanOps::snipAlgoA(QVector<double> &y, int m)
{
    int n = y.size();
    QVector<double> z(n);

    for (int p = 1; p <= m; ++p) {
        for (int i = p; i < n - p; ++i) {
            double a1 = y.at(i);
            double a2 = (y.at(i-p) + y.at(i+p)) / 2.0;
            z[i] = qMin(a1, a2);
        }

        for (int i = p; i < n - p; ++i) {
            y[i] = z.at(i);
        }
    }
}

void ScanOps::snipAlgoC(QVector<double> &y, int m)
{
    int n = y.size();
    QVector<double> z(n);

    for (int p = m; p >= 1; --p) {
        for (int i = p; i < n - p; ++i) {
            double a1 = y.at(i);
            double a2 = (y.at(i-p) + y.at(i+p)) / 2.0;
            z[i] = qMin(a1, a2);
        }

        for (int i = p; i < n - p; ++i) {
            y[i] = z.at(i);
        }
    }
}

/*
 * Algorithm taken from:
 * Golotvin, S., Williams, A. "Improved baseline recognition and modeling of FT NMR Spectra",
 * Journal of Magnetic Resonance 146(1) (2000), 122-125.
 */
QVector<QPointF> ScanOps::baseLineGolot(const Scan &sc, int sm, int w, double n, int steps, int sens, int mode)
{
    if (sc.size() < 2) return QVector<QPointF>();

    double value = 0.0;
    double noise = 0.0;
    double min = 0.0;
    double max = 0.0;
    double win = 0.0;

    QVector<double> dataX = sc.pDataAngle();
    QVector<double> dataY = sm > 0 ? golotAlgoSmooth(sc.pDataIntensity(), sm) : sc.pDataIntensity();
    int sz = qMin(dataX.size(), dataY.size());

    QVector<QPointF> matched;
    QVector<QPointF> consecutiveMatches;

    // the first point should always be present
    matched.append(QPointF(sc.pDataAngle().first(), sc.pDataIntensity().first()));

    int i = 1;
    while (i < sz) {
        value = qMax(dataY.at(i), 0.0);
        noise = n * qSqrt(value);
        min = value;
        max = value;

        for (int j = i-w; j <= i+w; ++j) {
            win = ((j >= 0) && (j < sz)) ? dataY.at(j) : value;
            min = qMin(min, win);
            max = qMax(max, win);
        }

        if (max - min <= noise) {
            consecutiveMatches.append(QPointF(dataX.at(i), value));
        }

        if (consecutiveMatches.size() > sens) {
            int c = int(consecutiveMatches.size() / 2);
            matched.append(QPointF(consecutiveMatches.at(c).x(), consecutiveMatches.at(c).y()));
            consecutiveMatches.clear();
            i += c; // jump forward to avoid scanning some points twice
        }

        i += steps;
    }

    if (!matched.size()) return QVector<QPointF>();

    // the last point of the scan must be fixed. If it is not, we append
    // a value depeding on the GUI selection "mode"
    if (!qFuzzyCompare(dataX.last(), matched.last().x())) {
        double lx = dataX.last();
        double ly = dataY.last();

        switch (mode) {
            case 0:
                ly = dataY.last();
                break;
            case 1:
                ly = matched.last().y();
                break;
            case 2:
                for (int i = 0; i < matched.size(); ++i) {
                    ly = qMin(ly, matched.at(i).y());
                }
                break;
            default:
                ly = matched.last().y();
        }

        matched.append(QPointF(lx, ly));
    }

    return matched;
}

/*
 * smoothing algorithm used by Golotvin et al
 */
QVector<double> ScanOps::golotAlgoSmooth(const QVector<double> &yData, int m)
{
    QVector<double> v(yData.size(), 0.0);
    double y = 0.0;

    for (int i = 0; i < yData.size(); ++i) {
        for (int k = i - m; k <= i + m; ++k) {
            if (k < 0) continue;
            if (k >= yData.size()) break;
            y += yData.at(k) / double((2.0 * m) + 1);
        }

        v[i] = y;
        y = 0.0;
    }

    return v;
}

QVector<double> ScanOps::interpolateSplineLinear(const QVector<double> &matchedX, const QVector<double> &matchedY, const QVector<double> &x)
{
    QVector<double> y(x.size(), 0.0);

    if ((matchedX.size() < 2) || (matchedY.size() < 2)) return y;

    alglib::real_1d_array mX;
    alglib::real_1d_array mY;
    mX.setcontent(matchedX.size(), matchedX.data());
    mY.setcontent(matchedY.size(), matchedY.data());

    alglib::spline1dinterpolant spline;
    alglib::spline1dbuildlinear(mX, mY, spline);

    for (int i = 0; i < y.size(); ++i) {
        y[i] = alglib::spline1dcalc(spline, x.at(i));
    }

    return y;
}

QVector<double> ScanOps::interpolateSplineCatmullRom(const QVector<double> &matchedX, const QVector<double> &matchedY, const QVector<double> &x)
{
    QVector<double> y(x.size(), 0.0);

    if ((matchedX.size() < 2) || (matchedY.size() < 2)) return y;

    alglib::real_1d_array mX;
    alglib::real_1d_array mY;
    mX.setcontent(matchedX.size(), matchedX.data());
    mY.setcontent(matchedY.size(), matchedY.data());

    alglib::spline1dinterpolant spline;
    alglib::spline1dbuildcatmullrom(mX, mY, spline);

    for (int i = 0; i < y.size(); ++i) {
        y[i] = alglib::spline1dcalc(spline, x.at(i));
    }

    return y;
}

QVector<double> ScanOps::interpolateSplineCubic(const QVector<double> &matchedX, const QVector<double> &matchedY, const QVector<double> &x)
{
    QVector<double> y(x.size(), 0.0);

    if ((matchedX.size() < 2) || (matchedY.size() < 2)) return y;

    alglib::real_1d_array mX;
    alglib::real_1d_array mY;
    mX.setcontent(matchedX.size(), matchedX.data());
    mY.setcontent(matchedY.size(), matchedY.data());

    alglib::spline1dinterpolant spline;
    alglib::spline1dbuildcubic(mX, mY, spline);

    for (int i = 0; i < y.size(); ++i) {
        y[i] = alglib::spline1dcalc(spline, x.at(i));
    }

    return y;
}

QVector<double> ScanOps::interpolateSplineAkima(const QVector<double> &matchedX, const QVector<double> &matchedY, const QVector<double> &x)
{
    QVector<double> y(x.size(), 0.0);

    if ((matchedX.size() < 2) || (matchedY.size() < 2)) return y;

    alglib::real_1d_array mX;
    alglib::real_1d_array mY;
    mX.setcontent(matchedX.size(), matchedX.data());
    mY.setcontent(matchedY.size(), matchedY.data());

    alglib::spline1dinterpolant spline;
    alglib::spline1dbuildakima(mX, mY, spline);

    for (int i = 0; i < y.size(); ++i) {
        y[i] = alglib::spline1dcalc(spline, x.at(i));
    }

    return y;
}

QVector<double> ScanOps::interpolateSplineMonotone(const QVector<double> &matchedX, const QVector<double> &matchedY, const QVector<double> &x)
{
    QVector<double> y(x.size(), 0.0);

    if ((matchedX.size() < 2) || (matchedY.size() < 2)) return y;

    alglib::real_1d_array mX;
    alglib::real_1d_array mY;
    mX.setcontent(matchedX.size(), matchedX.data());
    mY.setcontent(matchedY.size(), matchedY.data());

    alglib::spline1dinterpolant spline;
    alglib::spline1dbuildmonotone(mX, mY, spline);

    for (int i = 0; i < y.size(); ++i) {
        y[i] = alglib::spline1dcalc(spline, x.at(i));
    }

    return y;
}

/*
 * appends n elements of value x.last(). This is used for centred smoothing functions.
 */
QVector<double> ScanOps::offsetVector(const QVector<double> &x, int n)
{
    return x + QVector<double>(n, x.last());
}

/*
 * removes n elements from the beginning of x. This is used for centered smoothing functions.
 */
QVector<double> ScanOps::backsetVector(const QVector<double> &x, int n)
{
    return x.mid(n, x.size() - n);
}

QVector<double> ScanOps::smoothMovingAverageSimple(const QVector<double> &x, int s)
{
    if ((x.size() < 2) || (s < 2)) return x;

    // get a centered version (displaced by s/2 elements) for centered SMA
    QVector<double> xd(offsetVector(x, s/2));

    int c = xd.size();
    QVector<double> xsm(c, 0.0);
    alglib::real_1d_array mX;
    mX.setcontent(c, xd.data());

    alglib::filtersma(mX, s);
    std::copy(mX.getcontent(), mX.getcontent() + c, xsm.begin());

    return backsetVector(xsm, s/2);
}

QVector<double> ScanOps::smoothMovingAverageExponential(const QVector<double> &x, double a)
{
    if ((x.size() < 2) || (a < 0.0) || (a > 1.0)) return x;

    double da = 1.0 - a;
    if (qFuzzyIsNull(da)) da = 0.0001; // 0.0 causes a crash in AlgLib

    int c = x.size();
    QVector<double> xsm(c, 0.0);
    alglib::real_1d_array mX;
    mX.setcontent(c, x.data());

    alglib::filterema(mX, da);
    std::copy(mX.getcontent(), mX.getcontent() + c, xsm.begin());

    return xsm;
}

QVector<double> ScanOps::smoothMovingAverageLinearRegression(const QVector<double> &x, int s)
{
    if ((x.size() < 2) || (s < 2)) return x;

    int c = x.size();
    QVector<double> xsm(c, 0.0);
    alglib::real_1d_array mX;
    mX.setcontent(c, x.data());

    alglib::filterlrma(mX, s);
    std::copy(mX.getcontent(), mX.getcontent() + c, xsm.begin());

    return xsm;
}

void ScanOps::scanMetrics(const Scan &sc, double &xmin, double &xmax, double &ymin, double &ymax)
{
    xmin = sc.minAngle();
    xmax = sc.maxAngle();
    ymin = sc.minIntensity();
    ymax = sc.maxIntensity();
}

/*
 * wl in nm
 */
void ScanOps::hklMetricsTt(const Scan &sc, double &xmin, double &xmax, double &ymin, double &ymax, double wl)
{
    double xmi = std::numeric_limits<double>::max();
    double xma = 0.0;
    double ymi = 0.0;
    double yma = 0.0;

    QVector<Hkl> const &vec = sc.pDataHkl();
    for (int i = 0; i < vec.size(); ++i) {
        double tt = global::Functions::dToTwoTheta(vec.at(i).position(), wl);
        xmi = qMin(xmi, tt);
        xma = qMax(xma, tt);
        yma = qMax(yma, vec.at(i).intensity());
    }

    xmin = xmi;
    xmax = xma;
    ymin = ymi;
    ymax = yma;
}

void ScanOps::hklMetricsD(const Scan &sc, double &xmin, double &xmax, double &ymin, double &ymax)
{
    double xmi = std::numeric_limits<double>::max();
    double xma = 0.0;
    double ymi = 0.0;
    double yma = 0.0;

    QVector<Hkl> const &vec = sc.pDataHkl();
    for (int i = 0; i < vec.size(); ++i) {
        double tt = vec.at(i).position();
        xmi = qMin(xmi, tt);
        xma = qMax(xma, tt);
        yma = qMax(yma, vec.at(i).intensity());
    }

    xmin = xmi;
    xmax = xma;
    ymin = ymi;
    ymax = yma;
}

QVector<QVector<double> > ScanOps::detectPeaksZScore(const Scan &sc, int lag, double threshold, double influence)
{
    if (sc.pDataIntensity().size() <= lag + 2) return QVector<QVector<double> >();

    QVector<double> const &inpX = sc.pDataAngle();
    QVector<double> const &inpY = sc.pDataIntensity();
    int sz = qMin(inpX.size(), inpY.size());

    QVector<QVector<double> > outArray(2, QVector<double>());

    QVector<int> sig(sz, 0);
    QVector<double> filtY(sz, 0.0);
    QVector<double> avgFilter(sz, 0.0);
    QVector<double> stdFilter(sz, 0.0);
    QVector<double> subVec(inpY.mid(0, lag));

    alglib::real_1d_array subVecArray;
    subVecArray.setcontent(lag, subVec.data());

    avgFilter[lag] = alglib::samplemean(subVecArray);
    stdFilter[lag] = qSqrt(alglib::samplevariance(subVecArray)); // noise amplitude = sqrt(mean)

    for (int i = lag + 1; i < sz; ++i) {
        if (qAbs(inpY[i] - avgFilter[i - 1]) > threshold * stdFilter[i - 1]) {
            if (inpY[i] > avgFilter[i - 1]) {
                sig[i] = 1;
                outArray[0].append(inpX[i]);
                outArray[1].append(inpY[i]);
            } else {
                sig[i] = -1;
                outArray[0].append(inpX[i]);
                outArray[1].append(inpY[i]);
            }

            filtY[i] = influence * inpY[i] + (1.0 - influence) * filtY[i - 1];
        } else {
            sig[i] = 0;
            filtY[i] = inpY[i];
        }

        subVec = QVector<double>(filtY.mid(i - lag, lag));
        subVecArray.setcontent(lag, subVec.data());

        avgFilter[i] = alglib::samplemean(subVecArray);
        stdFilter[i] = qSqrt(alglib::samplevariance(subVecArray));
    }

    return outArray;
}

/*
 *  appends scanB to scanA in reverse order. This is used to create closed polygons
    between scanA and scanB, for example to fill the area in between
 */
Scan ScanOps::appendScanInReverse(const Scan &scA, const Scan &scB)
{
    Scan scOut = scA;

    for (int i = scB.size() - 1; i <= 0; --i) {
        scOut.pDataAngle().append(scB.pDataAngle().at(i));
        scOut.pDataIntensity().append(scB.pDataIntensity().at(i));
    }

    return scOut;
}
 /*
  * allows to use std::sort on a QVector<Scan> *graphHeap:
  * std::sort(graphHeap->begin(), graphHeap->end(), ScanOps::compareDisplayPosition);
  */
bool ScanOps::compareDisplayPosition(const Scan &a, const Scan &b)
{
    return a.getPosition() < b.getPosition();
}

QVector<double> ScanOps::smoothDCTtypeIV(const QVector<double> &y, int window, int overlap, const QList<bool> &components)
{
    if (y.size() < 2)                return y;
    if (components.size() != window) return y;
    if (overlap >= window)           return y;

    // used to normalize values in case of overlapping windows
    QVector<int> count(y.size(), 0);
    QVector<double> out(y.size(), 0.0);

    int start = 0;
    int m = overlap + 1;

    while (start < y.size()) {
        QVector<double> chunk = y.mid(start, window);

        // Apply DCT Type IV
        QVector<double> dataDCT = dctTypeIV(chunk);

        // Retain only the first 'components' components
        for (int i = 0; i < qMin(dataDCT.size(), components.size()); ++i) {
            if (!components.at(i)) dataDCT[i] = 0.0;
        }

        // Apply inverse DCT Type IV
        QVector<double> dataFiltered = inverseDctTypeIV(dataDCT);
        int s = dataFiltered.size();

        // Add the smoothed chunk to the output vector, considering overlap
        for (int i = 0; i < s; ++i) {
            int n = start + i;

            if (n < out.size()) {
                int z = m;
                if (i < m)     z = i + 1;
                if (i > s - m) z = s - i;

                out[n] += z * dataFiltered[i];
                count[n] += z;
            } else {
                break;
            }
        }

        start += window - overlap;
    }

    // normalize overlapping segments
    for (int i = 0; i < count.size(); ++i) {
        if (count[i] > 0) out[i] /= count[i];
    }

    return out;
}

QVector<double> ScanOps::dctTypeIV(const QVector<double> &data)
{
    QVector<double> result(data.size());
    int n = data.size();

    for (int k = 0; k < n; ++k) {
        double sum = 0.0;

        for (int t = 0; t < n; ++t) {
            sum += data[t] * std::cos(((2.0 * t + 1) * M_PI * k) / (2 * n));
        }

        result[k] = sum;
    }

    return result;
}

QVector<double> ScanOps::inverseDctTypeIV(const QVector<double> &data)
{
    QVector<double> result(data.size());
    int n = data.size();

    for (int t = 0; t < n; ++t) {
        double sum = 0.0;

        for (int k = 0; k < n; ++k) {
            if (qFuzzyIsNull(data[k])) continue;
            double d = data[k] * std::cos(((2.0 * t + 1) * M_PI * k) / (2 * n));
            sum += (k == 0) ? d : 2.0 * d;
        }

        result[t] = sum / n;
    }

    return result;
}

/*
 * Returns a matrix with scan data of format:
 *
 * "Diffraction Angle";"ScanName1";"Diffraction Angle";"ScanName2";"Diffraction Angle";"ScanName3"
 * x1_1;y1_1;x2_1;y2_1;x3_1;y3_1
 * x1_2;y1_2;x2_2;y2_2;x3_2;y3_2
 * x1_3;y1_3;x2_3;y2_3;x3_3;y3_3
 * ...
 *
 * The field separator 'sep' will be used. Values will be stored with 'prec' decimals.
 */
QString ScanOps::scansToAsciiMatrix(const QList<const Scan *> scans, const QString &sep, int prec,
                                    bool applyTransforms, double eps1, double eps2, double eps3)
{
    QString out;
    int rMax = 0;

    for (int i = 0; i < scans.size(); ++i) {
        rMax = qMax(rMax, scans.at(i)->size());
    }

    if (scans.size() > 1) {
        QStringList header;

        for (int c = 0; c < scans.size(); ++c) {
            header << QString("\"Diffraction Angle [%1%2%3]\"").arg(global::degree, "2", global::theta)
                   << QString("\"%1\"").arg(scans.at(c)->name());
        }

        out += header.join(sep) + "\n";
    }

    for (int r = 0; r < rMax; ++r) {
        QStringList line;

        for (int c = 0; c < scans.size(); ++c) {
            const Scan *scan = scans.at(c);

            if (r < scan->size()) {
                double x = scan->pDataAngle().at(r);
                double y = scan->pDataIntensity().at(r);

                if (applyTransforms) {
                    x -= global::Functions::angularCorrection(x, eps1, eps2, eps3);
                    if (!qFuzzyIsNull(scan->xOffset()))           x += scan->xOffset();
                    if (!qFuzzyIsNull(scan->yOffset()))           y *= scan->scaleFactor();
                    if (!qFuzzyCompare(scan->scaleFactor(), 1.0)) y += scan->yOffset();
                }

                line << QString("%1").arg(x, 0, 'f', prec) << QString("%1").arg(y, 0, 'f', prec);
            } else {
                line << QString() << QString();
            }
        }

        out += line.join(sep) + QString("\n");
    }

    return out;
}

/*
 * Returns a matrix with scan data of format:
 *
 * "ScanName1";"ScanName2";"ScanName3"
 * x1_1;x2_1;x3_1
 * x1_2;x2_2;x3_2
 * x1_3;x2_3;x3_3
 * ...
 *
 * The field separator 'sep' will be used. Values will be stored with 'prec' decimals.
 */
QString ScanOps::scansXToAsciiMatrix(const QList<const Scan *> scans, const QString &sep, int prec,
                                    bool applyTransforms, double eps1, double eps2, double eps3)
{
    QString out;
    int rMax = 0;

    for (int i = 0; i < scans.size(); ++i) {
        rMax = qMax(rMax, scans.at(i)->size());
    }

    if (scans.size() > 1) {
        for (int c = 0; c < scans.size(); ++c) {
            out += QString("\"%1\"\n").arg(scans.at(c)->name());
        }
    }

    for (int r = 0; r < rMax; ++r) {
        QStringList line;

        for (int c = 0; c < scans.size(); ++c) {
            const Scan *scan = scans.at(c);

            if (r < scan->size()) {
                double x = scan->pDataAngle().at(r);

                if (applyTransforms) {
                    x -= global::Functions::angularCorrection(x, eps1, eps2, eps3);
                    if (!qFuzzyIsNull(scan->xOffset()))           x += scan->xOffset();
                }

                line << QString("%1").arg(x, 0, 'f', prec);
            } else {
                line << QString();
            }
        }

        out += line.join(sep) + QString("\n");
    }

    return out;
}

/*
 * Returns a matrix with scan data of format:
 *
 * "ScanName1";"ScanName2";"ScanName3"
 * y1_1;y2_1;y3_1
 * y1_2;y2_2;y3_2
 * y1_3;y2_3;y3_3
 * ...
 *
 * The field separator 'sep' will be used. Values will be stored with 'prec' decimals.
 */
QString ScanOps::scansYToAsciiMatrix(const QList<const Scan *> scans, const QString &sep, int prec, bool applyTransforms)
{
    QString out;
    int rMax = 0;

    for (int i = 0; i < scans.size(); ++i) {
        rMax = qMax(rMax, scans.at(i)->size());
    }

    if (scans.size() > 1) {
        for (int c = 0; c < scans.size(); ++c) {
            out += QString("\"%1 Intensity\"\n").arg(scans.at(c)->name());
        }
    }

    for (int r = 0; r < rMax; ++r) {
        QStringList line;

        for (int c = 0; c < scans.size(); ++c) {
            const Scan *scan = scans.at(c);

            if (r < scan->size()) {
                double y = scan->pDataIntensity().at(r);

                if (applyTransforms) {
                    if (!qFuzzyIsNull(scan->yOffset()))           y *= scan->scaleFactor();
                    if (!qFuzzyCompare(scan->scaleFactor(), 1.0)) y += scan->yOffset();
                }

                line << QString("%1").arg(y, 0, 'f', prec);
            } else {
                line << QString();
            }
        }

        out += line.join(sep) + QString("\n");
    }

    return out;
}

/*
 * Finds the maximum intensity in a scan, and determines the hwhm left and right of the maximu.
 * Returns the values for maximum intensity and hwhm left and right, as well as the indices
 * in the vectors.
 *
 * Returns true if a peak was identified, else returns false.
 */
bool ScanOps::maxPeak(const Scan &sc, double &maxPos, double &maxVal, double &hwhmLeftVal, double &hwhmRightVal, int &maxIdx, int &hwhmLeftIdx, int &hwhmRightIdx)
{
    if (sc.size() < 3) return false;

    auto maxIter = std::max_element(sc.pDataIntensity().begin(), sc.pDataIntensity().end());
    auto minIter = std::min_element(sc.pDataIntensity().begin(), sc.pDataIntensity().end());
    maxIdx = std::distance(sc.pDataIntensity().begin(), maxIter);
    maxPos = sc.pDataAngle().at(maxIdx);
    maxVal = *maxIter;
    double minVal = *minIter < 0.0 ? 0.0 : *minIter;
    // int minIdx = std::distance(sc.pDataIntensity().begin(), minIter);
    // double minPos = sc.pDataAngle().at(minIdx);

    if (maxIdx == 0 || maxIdx == sc.pDataIntensity().size() - 1) {
        // No peak data if the highest intensity is at the start or end of the scan
        return false;
    }

    hwhmLeftIdx = maxIdx - 1;
    hwhmRightIdx = maxIdx + 1;

    while ((hwhmLeftIdx >= 0) && (sc.pDataIntensity().at(hwhmLeftIdx) > minVal + 0.5 * (maxVal - minVal))) {
        --hwhmLeftIdx;
    }

    while ((hwhmRightIdx < sc.pDataIntensity().size() - 1) && (sc.pDataIntensity().at(hwhmRightIdx) > minVal + 0.5 * (maxVal - minVal))) {
        ++hwhmRightIdx;
    }

    hwhmLeftVal  = maxPos - sc.pDataAngle().at(hwhmLeftIdx);
    hwhmRightVal = sc.pDataAngle().at(hwhmRightIdx) - maxPos;

    return true;
}
