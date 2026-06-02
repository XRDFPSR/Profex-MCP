/***************************************************************************
                          bgmngeqdata.cpp  -  description
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

#include "bgmngeqdata.h"
#include <QtMath>
#include <QDebug>
#include <limits>

BgmnGeqData::BgmnGeqData()
{
    _ymax = 0.0;
}

BgmnGeqData::BgmnGeqData(const BgmnGeqData &gd) : _curves(gd._curves), _ymax(gd._ymax), _xlabel(gd._xlabel)
{
}

int BgmnGeqData::load(const QString &s)
{
    _ymax = 0.0;
    BgmnGeqParser gparser;
    gparser.loadFile(s);
    _curves = gparser.curves();
    return _curves.size();
}

int BgmnGeqData::subCurveCount(int i) const
{
    int n = -1;

    if ((i >= 0) && (i < _curves.size())) {
        n = _curves[i].subCurveCount();
    }

    return n;
}

/*
 * receives a vector of equidistant x-values on the curve's
 * native scale (-theta [rad]) and computes y-values
 *
 * the resulting curve holds x = -theta [rad] and y values
 */
void BgmnGeqData::computeCurvesNative(const QVector<double> &x)
{
    _ymax = 0.0;

    for (int i = 0; i < _curves.size(); ++i) {
        for (int s = 0; s < _curves[i].subCurveCount(); ++s) {
            QVector<double> y(_curves[i].computeSubCurve(s, x));
            _curves[i].pSubCurve(s).x = x;
            _curves[i].pSubCurve(s).y = y;
        }

        _curves[i].integrateSubCurves();
        _ymax = qMax(_ymax, _curves[i].ymax());
    }
}

/*
 * receives a vector of equidistant x-values in 2theta [deg]
 * and computes y-values
 *
 * the resulting curve holds x = 2theta [deg] and y values
 */
void BgmnGeqData::computeCurvesTwoTheta(const QVector<double> &x)
{
    int z = x.size();
    QVector<double> xrad(z, 0.0);
    _ymax = 0.0;

    for (int i = 0; i < z; ++i) {
        xrad[i] = twoThetaToGeqNative(x[i]);
    }

    for (int i = 0; i < _curves.size(); ++i) {
        for (int s = 0; s < _curves[i].subCurveCount(); ++s) {
            QVector<double> y(_curves[i].computeSubCurve(s, xrad));
            _curves[i].pSubCurve(s).x = x; // use 2theta values
            _curves[i].pSubCurve(s).y = y;
        }

        _curves[i].integrateSubCurves();
        _ymax = qMax(_ymax, _curves[i].ymax());
    }
}

double BgmnGeqData::sinTheta(int n) const
{
    if ((n < 0) || (n >= _curves.size())) {
        return 0.0;
    }

    return _curves[n].sinTheta();
}

double BgmnGeqData::ymax(int i) const
{
    if (i < 0) return _ymax * 1.05;
    return _curves[i].ymax() * 1.05;
}
