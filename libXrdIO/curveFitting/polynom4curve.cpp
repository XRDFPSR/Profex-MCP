/***************************************************************************
                          polynom5curve.cpp  -  description
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

#include "polynom4curve.h"

Polynom4Curve::Polynom4Curve() : GenericCurve()
{
    _uid = QUuid::createUuid();
}

QStringList Polynom4Curve::parameterNames() const
{
    return QStringList() << QObject::tr("a0") << QObject::tr("a1") << QObject::tr("a2") << QObject::tr("a3") << QObject::tr("a4");
}

QStringList Polynom4Curve::parameterTextStatic()
{
    QStringList s;
    s << "a;constant coefficient";
    s << "b;linear coefficient";
    s << "c;quadratic coefficient";
    s << "d;cubic coefficient";
    s << "e;quartic coefficient";
    return s;
}

double Polynom4Curve::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    return c[_idx.at(0)] + c[_idx.at(1)] * x[0] + c[_idx.at(2)] * pow(x[0], 2.0) + c[_idx.at(3)] * pow(x[0], 3.0) + c[_idx.at(4)] * pow(x[0], 4.0);
}

void Polynom4Curve::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    for (int i = 0; i < x.size(); ++i) {
        double d = c[_idx.at(0)] + c[_idx.at(1)] * x[i] + c[_idx.at(2)] * pow(x[i], 2.0) + c[_idx.at(3)] * pow(x[i], 3.0) + c[_idx.at(4)] * pow(x[i], 4.0);
        y[i] = d;
        sy[i] += d;
    }
}
