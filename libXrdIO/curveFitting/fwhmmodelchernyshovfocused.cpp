/***************************************************************************
                          fwhmmodelchernyshovfocused.cpp  -  description
                             -------------------
    begin                : Sun Feb 23 12:35:03 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#include "fwhmmodelchernyshovfocused.h"

FwhmModelChernyshovFocused::FwhmModelChernyshovFocused()
{
    _uid = QUuid::createUuid();
}

QStringList FwhmModelChernyshovFocused::parameterNames() const
{
    return QStringList() << QObject::tr("Height") << QObject::tr("Center") << QObject::tr("Width");
}

QStringList FwhmModelChernyshovFocused::parameterTextStatic()
{
    QStringList s;
    s << "D;Distance sample-to-detector (mm)";
    s << "p;Detector pixel size (mm)";
    s << "t;Detector layer thickness (mm)";
    s << "c;Sample size (mm)";
    return s;
}

double FwhmModelChernyshovFocused::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    double ln2 = 2.0 * std::log(2.0);
    double cosTT = std::cos(x[0] * M_PI / 180.0);
    double sinTT = std::sin(x[0] * M_PI / 180.0);
    double cfocsqr = std::pow(c[_idx.at(3)] * sinTT, 2.0);
    double phi = 2.0 * std::atan(0.5 * c[_idx.at(3)] / c[_idx.at(0)]);

    double A = ln2 * (c[_idx.at(1)]*c[_idx.at(1)] - 2.0 * c[_idx.at(2)]*c[_idx.at(2)] - cfocsqr) / (c[_idx.at(0)]*c[_idx.at(0)]);
    double B = ln2 * 2.0 * (c[_idx.at(2)] * c[_idx.at(2)] + cfocsqr) / (c[_idx.at(0)] * c[_idx.at(0)]);
    double C = ln2 * std::pow(phi * (1.0 - cosTT), 2.0);

    return std::sqrt(A * std::pow(cosTT, 4.0) + B * std::pow(cosTT, 2.0) + C);
}

void FwhmModelChernyshovFocused::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    double ln2 = 2.0 * std::log(2.0);
    double phi = 2.0 * std::atan(0.5 * c[_idx.at(3)] / c[_idx.at(0)]);

    for (int i = 0; i < x.size(); ++i) {
        double cosTT = std::cos(x[i] * M_PI / 180.0);
        double sinTT = std::sin(x[i] * M_PI / 180.0);
        double cfocsqr = std::pow(c[_idx.at(3)] * sinTT, 2.0);

        double A = ln2 * (c[_idx.at(1)]*c[_idx.at(1)] - 2.0 * c[_idx.at(2)]*c[_idx.at(2)] - cfocsqr) / (c[_idx.at(0)]*c[_idx.at(0)]);
        double B = ln2 * 2.0 * (c[_idx.at(2)] * c[_idx.at(2)] + cfocsqr) / (c[_idx.at(0)] * c[_idx.at(0)]);
        double C = ln2 * std::pow(phi * (1.0 - cosTT), 2.0);

        double d = std::sqrt(A * std::pow(cosTT, 4.0) + B * std::pow(cosTT, 2.0) + C);

        y[i] = d;
        sy[i] += d;
    }

    _area = calculateArea(c);
}

double FwhmModelChernyshovFocused::calculateArea(const alglib::real_1d_array &) const
{
    return 0.0;
}
