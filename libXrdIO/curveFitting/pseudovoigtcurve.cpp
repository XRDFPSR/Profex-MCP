/***************************************************************************
                          pseudovoigtcurve.cpp  -  description
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

#include "pseudovoigtcurve.h"

PseudoVoigtCurve::PseudoVoigtCurve()
{
    _uid = QUuid::createUuid();
}

QStringList PseudoVoigtCurve::parameterNames() const
{
    return QStringList() << QObject::tr("Center") << QObject::tr("HWHM") << QObject::tr("Intensity") << QObject::tr("Shape");
}

QStringList PseudoVoigtCurve::parameterTextStatic()
{
    QStringList s;
    s << "a;center";
    s << "b;half width at half maximum";
    s << "c;intensity";
    s << "d;shape";
    return s;
}

double PseudoVoigtCurve::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    return c[_idx.at(2)] * ((1.0 - c[_idx.at(3)]) * exp(-log(2.0) * pow((x[0] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0)) + c[_idx.at(3)] / (1.0 + pow((x[0] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0)));
}

void PseudoVoigtCurve::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    for (int i = 0; i < x.size(); ++i) {
        double d = c[_idx.at(2)] * ((1.0 - c[_idx.at(3)]) * exp(-log(2.0) * pow((x[i] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0)) + c[_idx.at(3)] / (1.0 + pow((x[i] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0)));
        y[i] = d;
        sy[i] += d;
    }

    _area = calculateArea(c);
}

/*
 * expected order: [0] center, [1] hwhm, [2] height, [3] shape
 */
double PseudoVoigtCurve::calculateArea(const alglib::real_1d_array &c) const
{
    return c[_idx.at(2)] * fabs(c[_idx.at(1)]) * ((c[_idx.at(3)] * M_PI) + (1.0 - c[_idx.at(3)]) * sqrt(M_PI / log(2.0)));
}
