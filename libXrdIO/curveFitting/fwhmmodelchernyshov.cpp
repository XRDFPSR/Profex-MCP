/***************************************************************************
                          fwhmmodelchernyshov.cpp  -  description
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

#include "fwhmmodelchernyshov.h"

FwhmModelChernyshov::FwhmModelChernyshov()
{
    _uid = QUuid::createUuid();
}

QStringList FwhmModelChernyshov::parameterNames() const
{
    return QStringList() << QObject::tr("Height") << QObject::tr("Center") << QObject::tr("Width");
}

QStringList FwhmModelChernyshov::parameterTextStatic()
{
    QStringList s;
    s << "D;Distance sample-to-detector (mm)";
    s << "p;Detector pixel size (mm)";
    s << "t;Detector layer thickness (mm)";
    s << "c;Sample size (mm)";
    s << "phi;Beam divergence (degrees)";
    return s;
}

double FwhmModelChernyshov::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    double ln2 = 2.0 * std::log(2.0);
    double cosTT = std::cos(x[0] * M_PI / 180.0);

    double A = ln2 * (c[_idx.at(1)]*c[_idx.at(1)] - 2.0 * c[_idx.at(2)]*c[_idx.at(2)] - c[_idx.at(3)]*c[_idx.at(3)]) / (c[_idx.at(0)]*c[_idx.at(0)]);
    double B = ln2 * 2.0 * (c[_idx.at(2)]*c[_idx.at(2)] + c[_idx.at(3)]*c[_idx.at(3)]) / (c[_idx.at(0)]*c[_idx.at(0)]);
    double C = ln2 * c[_idx.at(4)]*c[_idx.at(4)];

    return std::sqrt(A * std::pow(cosTT, 4.0) + B * std::pow(cosTT, 2.0) + C);
}

void FwhmModelChernyshov::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    double ln2 = 2.0 * std::log(2.0);

    for (int i = 0; i < x.size(); ++i) {
        double cosTT = std::cos(x[i] * M_PI / 180.0);

        double A = ln2 * (c[_idx.at(1)]*c[_idx.at(1)] - 2.0 * c[_idx.at(2)]*c[_idx.at(2)] - c[_idx.at(3)]*c[_idx.at(3)]) / (c[_idx.at(0)]*c[_idx.at(0)]);
        double B = ln2 * 2.0 * (c[_idx.at(2)]*c[_idx.at(2)] + c[_idx.at(3)]*c[_idx.at(3)]) / (c[_idx.at(0)]*c[_idx.at(0)]);
        double C = ln2 * c[_idx.at(4)]*c[_idx.at(4)];

        double d = std::sqrt(A * std::pow(cosTT, 4.0) + B * std::pow(cosTT, 2.0) + C);

        y[i] = d;
        sy[i] += d;
    }

    _area = calculateArea(c);
}

double FwhmModelChernyshov::calculateArea(const alglib::real_1d_array &) const
{
    return 0.0;
}
