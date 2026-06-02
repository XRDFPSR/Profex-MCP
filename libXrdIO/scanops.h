/***************************************************************************
                          scanops.h  -  description
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

#ifndef SCANOPS_H
#define SCANOPS_H

#include "scan.h"
#include "3rdparty/alglib/src/ap.h"
#include "curveFitting/curvefittingmanager.h"
#include <QVector>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

enum SplineType {
    SPLINE_LINEAR,
    SPLINE_CATMULL_ROM,
    SPLINE_CUBIC,
    SPLINE_AKIMA,
    SPLINE_MONOTONE
};

class XRDIO_EXPORT ScanOps
{
public:
    ScanOps();

    static Scan convertDivSlitToADS(const Scan &sc, double l, double a, double r);
    static Scan convertDivSlitToFDS(const Scan &sc, double l, double a, double r);
    static double correlate(const Scan &, const Scan &);

    static double integrate(const Scan *sc, int, int, bool bg = true);
    static double integrate(const Scan *sc, double, double, bool bg = true);

    static double linearInterpolation(double start, double end, double frac);

    static Scan baseLineSNIP(const Scan &sc, int m, int mode);
    static QVector<QPointF> baseLineGolot(const Scan &sc, int sm, int w, double n, int steps, int sens, int mode);

    static QVector<double> interpolateSplineLinear(const QVector<double> &matchedX, const QVector<double> &matchedY, const QVector<double> &x);
    static QVector<double> interpolateSplineCatmullRom(const QVector<double> &matchedX, const QVector<double> &matchedY, const QVector<double> &);
    static QVector<double> interpolateSplineCubic(const QVector<double> &matchedX, const QVector<double> &matchedY, const QVector<double> &);
    static QVector<double> interpolateSplineAkima(const QVector<double> &matchedX, const QVector<double> &matchedY, const QVector<double> &);
    static QVector<double> interpolateSplineMonotone(const QVector<double> &matchedX, const QVector<double> &matchedY, const QVector<double> &);

    static QVector<double> smoothMovingAverageSimple(const QVector<double> &x, int s);
    static QVector<double> smoothMovingAverageExponential(const QVector<double> &x, double a);
    static QVector<double> smoothMovingAverageLinearRegression(const QVector<double> &x, int s);

    static QVector<double> smoothDCTtypeIV(const QVector<double> &y, int window, int overlap, const QList<bool> &components);

    static void scanMetrics(const Scan &sc, double &xmin, double &xmax, double &ymin, double &ymax);
    static void hklMetricsTt(const Scan &sc, double &xmin, double &xmax, double &ymin, double &ymax, double wl);
    static void hklMetricsD(const Scan &sc, double &xmin, double &xmax, double &ymin, double &ymax);
    static QVector<QVector<double> > detectPeaksZScore(const Scan &sc, int lag = 5, double threshold = 3.5, double influence = 0.5);

    static Scan appendScanInReverse(const Scan &, const Scan &);

    static bool compareDisplayPosition(const Scan &, const Scan &);

    static QString scansToAsciiMatrix(const QList<const Scan *> scans, const QString &sep, int prec,
                                      bool applyTransforms, double eps1 = 0.0, double eps2 = 0.0, double eps3 = 0.0);

    static QString scansXToAsciiMatrix(const QList<const Scan *> scans, const QString &sep, int prec,
                                      bool applyTransforms, double eps1 = 0.0, double eps2 = 0.0, double eps3 = 0.0);

    static QString scansYToAsciiMatrix(const QList<const Scan *> scans, const QString &sep, int prec, bool applyTransforms);

    static bool maxPeak(const Scan &sc, double &maxPos, double &maxVal, double &hwhmLeftVal, double &hwhmRightVal, int &maxIdx, int &hwhmLeftIdx, int &hwhmRightIdx);

private:
    static void snipAlgoA(QVector<double> &y, int m);
    static void snipAlgoC(QVector<double> &y, int m);
    static QVector<double> golotAlgoSmooth(const QVector<double> &x, int m);
    static QVector<double> offsetVector(const QVector<double> &x, int n);
    static QVector<double> backsetVector(const QVector<double> &x, int n);
    static QVector<double> dctTypeIV(const QVector<double> &);
    static QVector<double> inverseDctTypeIV(const QVector<double> &);
};

#endif // SCANOPS_H
