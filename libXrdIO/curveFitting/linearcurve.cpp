/***************************************************************************
                          linearcurve.cpp  -  description
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

#include "linearcurve.h"

LinearCurve::LinearCurve() : GenericCurve()
{
    _uid = QUuid::createUuid();
}

QStringList LinearCurve::parameterNames() const
{
    return QStringList() << QObject::tr("Constant") << QObject::tr("Linear");
}

QStringList LinearCurve::parameterTextStatic()
{
    QStringList s;
    s << "a;constant coefficient";
    s << "b;linear coefficient";
    return s;
}

double LinearCurve::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    return c[_idx.at(0)] + c[_idx.at(1)] * x[0];
}

void LinearCurve::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    for (int i = 0; i < x.size(); ++i) {
        double d = c[_idx.at(0)] + c[_idx.at(1)] * x[i];
        y[i] = d;
        sy[i] += d;
    }
}
