/***************************************************************************
                          bgmnsampledata.h  -  description
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

#include "bgmnsampledata.h"
#include "convolutiondata.h"
#include <QDebug>

BgmnSampleData::BgmnSampleData()
{

}

void BgmnSampleData::setCurveParameters(double b1, double k1, double k2, double di, double wl)
{
    _curveL1.b1 = b1;
    _curveL1.k1 = k1;
    _curveL1.k2 = k2;
    _curveL1.dInv = di;
    _curveL1.waveLength = wl;

    _curveL2.b1 = b1;
    _curveL2.k1 = k1;
    _curveL2.k2 = k2;
    _curveL2.dInv = di;
    _curveL2.waveLength = wl;

    _curveLc.b1 = b1;
    _curveLc.k1 = k1;
    _curveLc.k2 = k2;
    _curveLc.dInv = di;
    _curveLc.waveLength = wl;
}

void BgmnSampleData::computeCurvesNative(const QVector<double> &x)
{
    if (!x.size()) return;

    QVector<double> xshifted(x);

    for (int i = 0; i < xshifted.size(); ++i) {
        xshifted[i] -= _curveL1.dInv;
    }

    _curveL1.x = xshifted;
    _curveL1.y = QVector<double>(x.size(), 0.0);
    _curveL1.y[x.size() / 2] = 100.0;

    _curveL2.x = xshifted;
    _curveL2.y = QVector<double>(x.size(), 0.0);
    _curveL2.y[x.size() / 2] = 100.0;

    _curveLc.x.clear();
    _curveLc.y.clear();

    double ymaxL1 = l1Value(_curveL1.b1, _curveL1.dInv, _curveL1.dInv);
    double ymaxL2 = l2Value(_curveL2.b1, _curveL2.k1, _curveL2.k2, _curveL2.dInv, _curveL2.dInv);

    if (!qFuzzyIsNull(1000.0 * _curveL1.b1)) {
        for (int i = 0; i < x.size(); ++i) {
            _curveL1.y[i] = 100.0 * l1Value(_curveL1.b1, _curveL1.dInv, x[i]) / ymaxL1;
        }
    }

    if (!qFuzzyIsNull(1000.0 * _curveL2.k2) && (qFuzzyIsNull(1000.0 * _curveL2.b1 * _curveL2.k1))) {
        for (int i = 0; i < x.size(); ++i) {
            _curveL2.y[i] = 100.0 * l2Value(_curveL2.b1, _curveL2.k1, _curveL2.k2, _curveL2.dInv, x[i]) / ymaxL2;
        }
    }

    computeLc();
}

void BgmnSampleData::computeCurvesTwoTheta(const QVector<double> &x, double lambda)
{
    if (!x.size()) return;

    double x0ttheta = nativeToTwoTheta(_curveL1.dInv, lambda);

    QVector<double> xDinv(x.size(), 0.0);
    QVector<double> xttShifted(x);

    for (int i = 0; i < x.size(); ++i) {
        xDinv[i] = twothetaToNative(x[i], lambda);
        xttShifted[i] -= x0ttheta;
    }

    _curveL1.x = xttShifted;
    _curveL1.y = QVector<double>(x.size(), 0.0);
    _curveL1.y[x.size() / 2] = 100.0;

    _curveL2.x = xttShifted;
    _curveL2.y = QVector<double>(x.size(), 0.0);
    _curveL2.y[x.size() / 2] = 100.0;

    _curveLc.x.clear();
    _curveLc.y.clear();

    double ymaxL1 = l1Value(_curveL1.b1, _curveL1.dInv, _curveL1.dInv);
    double ymaxL2 = l2Value(_curveL2.b1, _curveL2.k1, _curveL2.k2, _curveL2.dInv, _curveL2.dInv);

    if (!qFuzzyIsNull(1000.0 * _curveL1.b1)) {
        for (int i = 0; i < x.size(); ++i) {
            _curveL1.y[i] = 100.0 * l1Value(_curveL1.b1, _curveL1.dInv, xDinv[i]) / ymaxL1;
        }
    }

    if (!qFuzzyIsNull(1000.0 * _curveL2.k2) && (qFuzzyIsNull(1000.0 * _curveL2.b1 * _curveL2.k1))) {
        for (int i = 0; i < x.size(); ++i) {
            _curveL2.y[i] = 100.0 * l2Value(_curveL2.b1, _curveL2.k1, _curveL2.k2, _curveL2.dInv, xDinv[i]) / ymaxL2;
        }
    }

    computeLc();

    _curveLc.x = xConvoluted(_curveL1.x, _curveL2.x);
}

void BgmnSampleData::computeLc()
{
    if (!_curveL1.x.size() || !_curveL1.y.size() || !_curveL2.x.size() || !_curveL2.y.size()) return;

    QVector<double> convX = xConvoluted(_curveL1.x, _curveL2.x);
    QVector<double> convY = ConvolutionData::convolute(_curveL1.y, _curveL2.y);

    double ymax = *std::max_element(convY.constBegin(), convY.constEnd());

    _curveLc.x = convX;
    _curveLc.y = QVector<double>(convY.size(), 0.0);

    for (int i = 0; i < convY.size(); ++i) {
        _curveLc.y[i] = 100.0 * convY[i] / ymax;
    }
}

double BgmnSampleData::l1Value(double b1, double x0, double x)
{
    double denom = (qPow(b1, 2.0) + qPow(x - x0, 2.0));
    if (qFuzzyIsNull(1000.0 * denom)) return 0.0;

    return (1.0 / M_PI) * (b1 / denom);
}

double BgmnSampleData::l2Value(double b1, double k1, double k2, double x0, double x)
{
    // note: some values get so small that qfuzzyisnull reports true. It is safer to
    // compare prior to squaring

    double b2sqr = k1*b1*b1 + k2*x0*x0;

    double denom = qPow(b2sqr + qPow(x - x0, 2.0), 2.0);
    if (qFuzzyIsNull(1000.0 * b2sqr) && qFuzzyCompare(x, x0)) return 0.0;

    return (2.0 / M_PI) * b2sqr * qSqrt(b2sqr) / denom;
}

QVector<double> BgmnSampleData::xConvoluted(const QVector<double> &xl1, const QVector<double> &xl2)
{
    QVector<double> x(xl1.size() + xl2.size() - 1, 0.0);
    double stepSize = qAbs((xl1.last() - xl1.first()) / double(xl1.size()));
    double firstX = xl1.first() + xl2.first();

    for (int i = 0; i < x.size(); ++i) {
        x[i] = firstX + (i+1) * stepSize;
    }

    return x;
}

double BgmnSampleData::getCurveWidthL1(double f)
{
    if (qFuzzyIsNull(f)) return 0.0;
    if (qFuzzyIsNull(_curveL1.b1)) return 0.0;

    int n = 1;
    double y0 = l1Value(_curveL1.b1, _curveL1.dInv, _curveL1.dInv);
    double yx = l1Value(_curveL1.b1, _curveL1.dInv, _curveL1.dInv - double(n)*_curveL1.b1);

    while(yx > f * y0) {
        ++n;
        yx = l1Value(_curveL1.b1, _curveL1.dInv, _curveL1.dInv - double(n)*_curveL1.b1);
    }

    return double(n) * _curveL1.b1;
}


double BgmnSampleData::getCurveWidthL2(double f)
{
    if (qFuzzyIsNull(f)) return 0.0;

    double w = _curveL2.b1 + _curveL2.k2;

    if (qFuzzyIsNull(w)) return 0.0;

    int n = 1;
    double y0 = l2Value(_curveL2.b1, _curveL2.k1, _curveL2.k2, _curveL2.dInv, _curveL2.dInv);
    double yx = l2Value(_curveL2.b1, _curveL2.k1, _curveL2.k2, _curveL2.dInv, _curveL2.dInv - double(n) * w);

    if (qFuzzyIsNull(y0)) return 0.0;

    while(yx > f * y0) {
        ++n;
        yx = l2Value(_curveL2.b1, _curveL2.k1, _curveL2.k2, _curveL2.dInv, _curveL2.dInv - double(n) * w);
    }

    return double(n) * w;
}

double BgmnSampleData::twothetaToNative(double ttang, double lam) const
{
    return (2.0 * qSin(qDegreesToRadians(0.5 * ttang))) / lam;
}

double BgmnSampleData::nativeToTwoTheta(double dinv, double lam) const
{
    double d =  2.0 * qRadiansToDegrees(qAsin(qMin(1.0, 0.5 * lam * dinv)));
    if (d < 0.0)   return 0.0;
    if (d > 180.0) return 180.0;
    return d;
}
