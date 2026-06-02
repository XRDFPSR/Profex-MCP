/***************************************************************************
                          crystalhklgenerator.cpp  -  description
                             -------------------
    begin                : Wed Oct 19 18:13:00 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#include "crystalhklgenerator.h"
#include "math.h"
#include "functions.h"

CrystalHklGenerator::CrystalHklGenerator(const CrystalUnitCell &u, const QList<CrystalAtom> &l)
    : _uCell(u), _lAtoms(l)
{

}

/*
 * dmin and lambda in Angstrom
 * the key of the returned map contains the d value * 10^8 to sort the map
 * by an int value.
 */
QMap<long, QList<CrystalStructureFactor> > CrystalHklGenerator::getHklList(double dmin, double lambda, double b1)
{
    QStringList err;

    int hmax = 1 + int(sqrt(pow(_uCell.a(), 2.0)/pow(0.1*dmin, 2.0)));
    int kmax = 1 + int(sqrt(pow(_uCell.b(), 2.0)/pow(0.1*dmin, 2.0)));
    int lmax = 1 + int(sqrt(pow(_uCell.c(), 2.0)/pow(0.1*dmin, 2.0)));

    QMap<long, QList<CrystalStructureFactor> > lout;
    double mapExpo = 8.0;

    for (int h = -hmax; h <= hmax; ++h) {
        for (int k = -kmax; k <= kmax; ++k) {
            for (int l = -lmax; l <= lmax; ++l) {
                // cell parameters and returned d spacing are in nm. We need Angstrom
                double d = 10.0 * global::Functions::dSpacing(_uCell.a(), _uCell.b(), _uCell.c(), _uCell.alpha(), _uCell.beta(), _uCell.gamma(), h, k, l);

                if ((d > dmin) && (lambda < 2.0 * d)) {
                    double tt = global::Functions::dToTwoTheta(d, lambda);
                    double fhklSqr = fhklSqrAmplitude(tt, lambda, h, k, l, b1, err);
                    long idx = long(d * pow(10.0, mapExpo));
                    double lp = global::Functions::getLPFactor(tt);

                    if (!qFuzzyIsNull(fhklSqr)) {
                        lout[idx].append(CrystalStructureFactor(h, k, l, fhklSqr * lp, d));
                    }
                }
            }
        }
    }

    if (!err.isEmpty()) qDebug() << QString("CrystalHklGenerator::fhklAmplitude(): No scattering factors found for elements: %1").arg(err.join(", "));

    return lout;
}

/*
 * returns Fhkl^2
 */
double CrystalHklGenerator::fhklSqrAmplitude(double tt, double wl, int h, int k, int l, double b1, QStringList &err)
{
    double sinPart = 0.0;
    double cosPart = 0.0;
    static QRegularExpression rxEl("([A-Z]{1,2})");

    for (int i = 0; i < _lAtoms.size(); ++i) {
        const CrystalAtom atm = _lAtoms.at(i);

        QRegularExpressionMatch rm = rxEl.match(atm.element().toUpper());

        if (rm.hasMatch()) {
            QString el = rm.captured(1);
            double fi = global::Functions::getScatteringFactor(el, tt, wl, b1);

            if (qFuzzyIsNull(fi)) {
                if (!err.contains(_lAtoms.at(i).element())) err.append(el);
            } else {
                double co = fi * atm.occupancy() * std::cos(2.0 * M_PI * (h * atm.x() + k * atm.y() + l * atm.z()));
                double si = fi * atm.occupancy() * std::sin(2.0 * M_PI * (h * atm.x() + k * atm.y() + l * atm.z()));
                cosPart += co;
                sinPart += si;
            }
        } else {
            if (!err.contains(_lAtoms.at(i).element())) err.append(_lAtoms.at(i).element());
        }
    }

    return cosPart * cosPart + sinPart * sinPart;
}

