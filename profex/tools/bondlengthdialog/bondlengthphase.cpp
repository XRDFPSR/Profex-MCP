/***************************************************************************
                          bondlengthphase.cpp  -  description
                             -------------------
    begin                : Fri Apr 26 15:24:00 CEST 2024
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

#include "bondlengthphase.h"
#include "../libXrdIO/functions.h"
#include "../libXrdIO/structs.h"

BondLengthPhase::BondLengthPhase()
{
}

BondLengthPhase::BondLengthPhase(const CrystalStructure &c, double filter)
{
    setStructure(c, filter);
}

void BondLengthPhase::setStructure(const CrystalStructure &c, double filter)
{
    _structure = c;
    _headerLabels = parseHeader(_structure.atoms());
    _symEqAtoms = generateSymEqAtoms(_structure);
    _bondLengthsTable = calculateBondLengths(_structure, _symEqAtoms, filter);
}

QStringList BondLengthPhase::parseHeader(const QList<CrystalAtom> &l)
{
    QStringList header;

    for (int i = 0; i < l.size(); ++i) {
        header.append(l.at(i).name());
    }

    return header;
}

QMap<int, QList<CrystalAtom> > BondLengthPhase::generateSymEqAtoms(CrystalStructure &struc)
{
    QMap<int, QList<CrystalAtom> > map;

    for (int i = 0; i < struc.atoms().size(); ++i) {
        QList<CrystalAtom> lst = QList<CrystalAtom>() << struc.atoms().at(i);
        map[i] = lst;
        map[i].append(struc.generateAtoms(lst));
    }

    return map;
}

QList<QList<double> > BondLengthPhase::calculateBondLengths(const CrystalStructure &struc, const QMap<int, QList<CrystalAtom> > &symEq, double filter)
{
    if (!struc.atoms().size() || !symEq.size()) {
        return QList<QList<double> >();
    }

    int n = struc.atoms().size();

    double ucA  = struc.unitCell().a();
    double ucB  = struc.unitCell().b();
    double ucC  = struc.unitCell().c();
    double ucAl = struc.unitCell().alpha();
    double ucBe = struc.unitCell().beta();
    double ucGa = struc.unitCell().gamma();

    QList<QList<double> > out(n, QList<double>(n, -1.0));

    for (int r = 0; r < n; ++r) {
        for (int c = 0; c < n; ++c) {
            // double bl = bondLength(ucA, ucB, ucC, ucAl, ucBe, ucGa, struc.atoms().at(r), symEq.value(c));
            double bl = bondLength(ucA, ucB, ucC, ucAl, ucBe, ucGa, symEq.value(r), symEq.value(c));

            if (filter > 0.0) {
                double radA = global::ionicRadii.value(struc.atoms().at(r).element().toUpper(), -1.0);
                double radB = global::ionicRadii.value(symEq.value(c).constFirst().element().toUpper(), -1.0);

                if ((radA > 0.0) && (radB > 0.0)) {
                    double th = filter + radA + radB;
                    if (bl > th) bl = -1.0;
                }
            }

            out[r][c] = bl;
        }
    }

    return out;
}

double BondLengthPhase::bondLength(double a, double b, double c, double al, double be, double ga, const QList<CrystalAtom> &atAlist, const QList<CrystalAtom> &atBlist)
{
    QList<double> l;

    for (int j = 0; j < atAlist.size(); ++j) {
        const CrystalAtom atA = atAlist.at(j);

        double cAx, cAy, cAz;
        global::Functions::fractionalToCartesian(a, b, c, al, be, ga, atA.x(), atA.y(), atA.z(), cAx, cAy, cAz);

        for (int i = 0; i < atBlist.size(); ++i) {
            const CrystalAtom atB = atBlist.at(i);

            double cBx, cBy, cBz;
            global::Functions::fractionalToCartesian(a, b, c, al, be, ga, atB.x(), atB.y(), atB.z(), cBx, cBy, cBz);

            l << std::sqrt(std::pow(cBx - cAx, 2.0)
                           + std::pow(cBy - cAy, 2.0)
                           + std::pow(cBz - cAz, 2.0));
        }
    }

    return lowestValue(l);
}

double BondLengthPhase::lowestValue(const QList<double> &l)
{
    QList<double> lst(l);
    std::sort(lst.begin(), lst.end());
    return lst.constFirst();
}

int BondLengthPhase::rowCount() const
{
    return _bondLengthsTable.size();
}

int BondLengthPhase::columnCount() const
{
    if (!_bondLengthsTable.size()) return 0;
    return _bondLengthsTable.constFirst().size();
}
