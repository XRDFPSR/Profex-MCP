/***************************************************************************
                          bgmnl2curve.cpp  -  description
                             -------------------
    begin                : Fri Dec 27 12:35:03 CEST 2024
    copyright            : (C) 2024 by Nicola Doebelin
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

#include "bgmnl2curve.h"

BgmnL2Curve::BgmnL2Curve()
{
    _uid = QUuid::createUuid();
}

QStringList BgmnL2Curve::parameterNames() const
{
    return QStringList() << QObject::tr("Height") << QObject::tr("Center") << QObject::tr("Width");
}

QStringList BgmnL2Curve::parameterTextStatic()
{
    QStringList s;
    s << "g;height";
    s << "e;center";
    s << "q;width";
    return s;
}

double BgmnL2Curve::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    return 2.0 * c[_idx.at(0)] * std::pow(c[_idx.at(2)], 3.0) / std::pow(std::pow(c[_idx.at(2)], 2.0) + std::pow(x[0] - c[_idx.at(1)], 2.0), 2.0);
}

void BgmnL2Curve::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    for (int i = 0; i < x.size(); ++i) {
        double d = 2.0 * c[_idx.at(0)] * std::pow(c[_idx.at(2)], 3.0) / std::pow(std::pow(c[_idx.at(2)], 2.0) + std::pow(x[i] - c[_idx.at(1)], 2.0), 2.0);
        y[i] = d;
        sy[i] += d;
    }

    _area = calculateArea(c);
}

double BgmnL2Curve::calculateArea(const alglib::real_1d_array &c) const
{
    return c[_idx.at(0)] * M_PI;
}
