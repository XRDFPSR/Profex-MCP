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

#include "pseudovoigtsplitcurve.h"

PseudoVoigtSplitCurve::PseudoVoigtSplitCurve()
{
    _uid = QUuid::createUuid();
}

QStringList PseudoVoigtSplitCurve::parameterNames() const
{
    return QStringList() << QObject::tr("Center")
                         << QObject::tr("HWHM Left")
                         << QObject::tr("HWHM Right")
                         << QObject::tr("Intensity")
                         << QObject::tr("Shape Left")
                         << QObject::tr("Shape Right");
}

QStringList PseudoVoigtSplitCurve::parameterTextStatic()
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

double PseudoVoigtSplitCurve::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    return (1.0 - part(x[0], c[_idx.at(0)])) * c[_idx.at(3)] * ((1.0 - c[_idx.at(4)]) * exp(-log(2.0) * pow((x[0] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0)) + c[_idx.at(4)] / (1.0 + pow((x[0] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0)))
                + part(x[0], c[_idx.at(0)])  * c[_idx.at(3)] * ((1.0 - c[_idx.at(5)]) * exp(-log(2.0) * pow((x[0] - c[_idx.at(0)]) / c[_idx.at(2)], 2.0)) + c[_idx.at(5)] / (1.0 + pow((x[0] - c[_idx.at(0)]) / c[_idx.at(2)], 2.0)));
}

void PseudoVoigtSplitCurve::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    for (int i = 0; i < x.size(); ++i) {
        double d =  (1.0 - part(x[i], c[_idx.at(0)])) * c[_idx.at(3)] * ((1.0 - c[_idx.at(4)]) * exp(-log(2.0) * pow((x[i] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0)) + c[_idx.at(4)] / (1.0 + pow((x[i] - c[_idx.at(0)]) / c[_idx.at(1)], 2.0)))
                         + part(x[i], c[_idx.at(0)])  * c[_idx.at(3)] * ((1.0 - c[_idx.at(5)]) * exp(-log(2.0) * pow((x[i] - c[_idx.at(0)]) / c[_idx.at(2)], 2.0)) + c[_idx.at(5)] / (1.0 + pow((x[i] - c[_idx.at(0)]) / c[_idx.at(2)], 2.0)));
        y[i] = d;
        sy[i] += d;
    }

    _area = calculateArea(c);
}

double PseudoVoigtSplitCurve::part(const double &x, const double &m)
{
    return x > m ? 1.0 : 0.0;
}

/*
 * expected order: [0] center, [1] hwhm1, [2] hwhm2, [3] height, [4] shape1, [5] shape2
 */
double PseudoVoigtSplitCurve::calculateArea(const alglib::real_1d_array &c) const
{
    double al = c[_idx.at(3)] * fabs(c[_idx.at(1)]) * ((c[_idx.at(4)] * M_PI) + (1.0 - c[_idx.at(4)]) * sqrt(M_PI / log(2.0)));
    double ar = c[_idx.at(3)] * fabs(c[_idx.at(2)]) * ((c[_idx.at(5)] * M_PI) + (1.0 - c[_idx.at(5)]) * sqrt(M_PI / log(2.0)));

    return (al + ar) / 2.0;
}
