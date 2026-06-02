/***************************************************************************
                          pearsonsplitcurve.cpp  -  description
                             -------------------
    begin                : Tue Dec 17 19:20:03 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#include "pearsonsplitcurve.h"
#include <QDebug>

PearsonSplitCurve::PearsonSplitCurve()
{
    _uid = QUuid::createUuid();
}

QStringList PearsonSplitCurve::parameterNames() const
{
    return QStringList() << QObject::tr("Center")
                         << QObject::tr("HWHM Left")
                         << QObject::tr("HWHM Right")
                         << QObject::tr("Intensity")
                         << QObject::tr("Shape Left")
                         << QObject::tr("Shape Right");
}

QStringList PearsonSplitCurve::parameterTextStatic()
{
    QStringList s;
    s << "a;center";
    s << "b1;half width at half maximum left side";
    s << "b2;half width at half maximum right side";
    s << "c;intensity";
    s << "d1;shape left side";
    s << "d2;shape right side";
    return s;
}

double PearsonSplitCurve::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    return (1.0 - part(x[0], c[_idx.at(0)])) * c[_idx.at(3)] / pow(1.0 + pow((x[0] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0) * (pow(2.0, 1.0 / c[_idx.at(4)]) - 1.0), c[_idx.at(4)])
                + part(x[0], c[_idx.at(0)])  * c[_idx.at(3)] / pow(1.0 + pow((x[0] - c[_idx.at(0)]) / c[_idx.at(2)], 2.0) * (pow(2.0, 1.0 / c[_idx.at(5)]) - 1.0), c[_idx.at(5)]);
}

void PearsonSplitCurve::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    for (int i = 0; i < x.size(); ++i) {
        double d =  (1.0 - part(x[i], c[_idx.at(0)])) * c[_idx.at(3)] / pow(1.0 + pow((x[i] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0) * (pow(2.0, 1.0 / c[_idx.at(4)]) - 1.0), c[_idx.at(4)])
                         + part(x[i], c[_idx.at(0)])  * c[_idx.at(3)] / pow(1.0 + pow((x[i] - c[_idx.at(0)]) / c[_idx.at(2)], 2.0) * (pow(2.0, 1.0 / c[_idx.at(5)]) - 1.0), c[_idx.at(5)]);
        y[i] = d;
        sy[i] += d;
    }

    _area = calculateArea(c);
}

double PearsonSplitCurve::part(const double &x, const double &m)
{
    return x > m ? 1.0 : 0.0;
}

/*
 * expected order: [0] center, [1] hwhm1, [2] hwhm2, [3] height, [4] shape1, [5] shape2
 */
double PearsonSplitCurve::calculateArea(const alglib::real_1d_array &c) const
{
    if (c[_idx.at(4)] <= 0.5) return -1.0;
    if (c[_idx.at(5)] <= 0.5) return -1.0;

    double gl = exp(lgamma(c[_idx.at(4)] - 0.5) - lgamma(c[_idx.at(4)]));
    double gr = exp(lgamma(c[_idx.at(5)] - 0.5) - lgamma(c[_idx.at(5)]));

    double fl = pow(2.0, 1.0 / c[_idx.at(4)]) - 1.0;
    double fr = pow(2.0, 1.0 / c[_idx.at(5)]) - 1.0;

    double al = c[_idx.at(3)] * fabs(c[_idx.at(1)]) * sqrt(M_PI) * gl / (2.0 * sqrt(fl));
    double ar = c[_idx.at(3)] * fabs(c[_idx.at(2)]) * sqrt(M_PI) * gr / (2.0 * sqrt(fr));

    return al + ar;
}
