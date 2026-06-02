/***************************************************************************
                          polynom4curve.cpp  -  description
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

#include "cubiccurve.h"

CubicCurve::CubicCurve() : GenericCurve()
{
    _uid = QUuid::createUuid();
}

QStringList CubicCurve::parameterNames() const
{
    return QStringList() << QObject::tr("Constant") << QObject::tr("Linear") << QObject::tr("Quadratic") << QObject::tr("Cubic");
}

QStringList CubicCurve::parameterTextStatic()
{
    QStringList s;
    s << "a;constant coefficient";
    s << "b;linear coefficient";
    s << "c;quadratic coefficient";
    s << "d;cubic coefficient";
    return s;
}

double CubicCurve::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    return c[_idx.at(0)] + c[_idx.at(1)] * x[0] + c[_idx.at(2)] * pow(x[0], 2.0) + c[_idx.at(3)] * pow(x[0], 3.0);
}

void CubicCurve::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    for (int i = 0; i < x.size(); ++i) {
        double d = c[_idx.at(0)] + c[_idx.at(1)] * x[i] + c[_idx.at(2)] * pow(x[i], 2.0) + c[_idx.at(3)] * pow(x[i], 3.0);
        y[i] = d;
        sy[i] += d;
    }
}
