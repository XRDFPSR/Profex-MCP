/***************************************************************************
                          bgmnlamdata.h  -  description
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

#include "bgmnlamdata.h"
#include <limits>
#include <QDebug>

BgmnLamData::BgmnLamData()
{
}

BgmnLamData::BgmnLamData(const BgmnLamData &ld) : _curve(ld._curve)
{
}

int BgmnLamData::load(const QString &s)
{
    BgmnLamParser lparser;
    lparser.load(s);
    _curve = lparser.getCurve();
    return _curve.subCurveCount();
}

int BgmnLamData::parse(const QString &s)
{
    BgmnLamParser lparser;
    lparser.parse(s);
    _curve = lparser.getCurve();
    return _curve.subCurveCount();
}

/*
 * receives a vector of equidistant x-values in Lambda [A]
 * and computes y-values
 *
 * the resulting curve holds x = Lambda [A] and y values
 */
void BgmnLamData::computeCurvesLambda(const QVector<double> &x)
{
    int z = x.size();
    QVector<double> xlinv(z, 0.0);

    for (int i = 0; i < z; ++i) {
        xlinv[i] = angstromToNative(x[i]);
    }

    for (int s = 0; s < _curve.subCurveCount(); ++s) {
        QVector<double> y(_curve.computeSubCurve(s, xlinv));
        _curve.pSubCurve(s).x = x;
        _curve.pSubCurve(s).y = y;
    }

    _curve.integrateSubCurves();
}

/*
 * receives a vector of equidistant x-values in 2theta [degrees]
 * and computes y-values
 *
 * the resulting curve holds x = 2theta [deg] and y values
 */
void BgmnLamData::computeCurvesTwoTheta(const QVector<double> &x, double d)
{
    int z = x.size();
    QVector<double> xlinv(z, 0.0);

    for (int i = 0; i < z; ++i) {
        xlinv[i] = twothetaToNative(x[i], d);
    }

    for (int s = 0; s < _curve.subCurveCount(); ++s) {
        QVector<double> y(_curve.computeSubCurve(s, xlinv));
        _curve.pSubCurve(s).x = x;
        _curve.pSubCurve(s).y = y;
    }

    _curve.integrateSubCurves();
}

/*
 * returns l0 in A, based on calculation in bgmn's lam.c
 */
double BgmnLamData::wavelength() const
{
    double l0 = 0.0;
    double sum = 0.0;

    for (int i = 0; i < _curve.subCurveCount(); ++i) {
        double ij = _curve.pSubCurve(i).g;
        sum += ij;
        double lj = _curve.pSubCurve(i).e;
        l0 += ij * lj;
    }

    if (qFuzzyIsNull(sum) || qFuzzyIsNull(l0)) return 0.0;
    l0 /= sum;

    return 10.0 / l0;
}
