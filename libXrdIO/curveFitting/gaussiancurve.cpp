/***************************************************************************
                          gaussiancurve.cpp  -  description
                             -------------------
    begin                : Tue Nov 12 19:20:03 CEST 2019
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

#include "gaussiancurve.h"

GaussianCurve::GaussianCurve()
{
    _uid = QUuid::createUuid();
}

QStringList GaussianCurve::parameterNames() const
{
    return QStringList() << QObject::tr("Center") << QObject::tr("HWHM") << QObject::tr("Intensity");
}

QStringList GaussianCurve::parameterTextStatic()
{
    QStringList s;
    s << "a;center";
    s << "b;half width at half maximum";
    s << "c;intensity";
    return s;
}

double GaussianCurve::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    return c[_idx.at(2)] * exp(-log(2.0) * pow((x[0] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0));
}

void GaussianCurve::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    for (int i = 0; i < x.size(); ++i) {
        double d = c[_idx.at(2)] * exp(-log(2.0) * pow((x[i] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0));
        y[i] = d;
        sy[i] += d;
    }

    _area = calculateArea(c);
}

/*
 * expected order: [0] center, [1] hwhm, [2] height
 */
double GaussianCurve::calculateArea(const alglib::real_1d_array &c) const
{
    return c[_idx.at(2)] * fabs(c[_idx.at(1)]) * sqrt(M_PI / log(2.0));
}
