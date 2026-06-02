/***************************************************************************
                          lorentziansplitcurve.cpp  -  description
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

#include "lorentziansplitcurve.h"

LorentzianSplitCurve::LorentzianSplitCurve()
{
    _uid = QUuid::createUuid();
}

QStringList LorentzianSplitCurve::parameterNames() const
{
    return QStringList() << QObject::tr("Center") << QObject::tr("HWHM Left") << QObject::tr("HWHM Right") << QObject::tr("Intensity");
}

QStringList LorentzianSplitCurve::parameterTextStatic()
{
    QStringList s;
    s << "a;center";
    s << "b1;half width at half maximum left side";
    s << "b2;half width at half maximum right side";
    s << "c;intensity";
    return s;
}

double LorentzianSplitCurve::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    return (1.0 - part(x[0], c[_idx.at(0)])) * c[_idx.at(3)] / (1.0 + pow((x[0] - c[_idx.at(0)])/c[_idx.at(1)], 2.0))
                + part(x[0], c[_idx.at(0)])  * c[_idx.at(3)] / (1.0 + pow((x[0] - c[_idx.at(0)])/c[_idx.at(2)], 2.0));
}

void LorentzianSplitCurve::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    for (int i = 0; i < x.size(); ++i) {
        double d =  (1.0 - part(x[i], c[_idx.at(0)])) * c[_idx.at(3)] / (1.0 + pow((x[i] - c[_idx.at(0)])/c[_idx.at(1)], 2.0))
                         + part(x[i], c[_idx.at(0)])  * c[_idx.at(3)] / (1.0 + pow((x[i] - c[_idx.at(0)])/c[_idx.at(2)], 2.0));
        y[i] = d;
        sy[i] += d;
    }

    _area = calculateArea(c);
}

double LorentzianSplitCurve::part(const double &x, const double &m)
{
    return x > m ? 1.0 : 0.0;
}

/*
 * expected order: [0] center, [1] hwhm1, [2] hwhm2, [3] height
 */
double LorentzianSplitCurve::calculateArea(const alglib::real_1d_array &c) const
{
    return c[_idx.at(2)] * fabs(c[_idx.at(1)]) * fabs(c[_idx.at(2)]) * 0.5 * M_PI;
}
