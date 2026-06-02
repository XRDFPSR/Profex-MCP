/***************************************************************************
                          symmetrymultiplier.cpp  -  description
                             -------------------
    begin                : Wed Oct 07 10:34:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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


#include "symmetrymultiplier.h"
#include <math.h>
#include <QJSEngine>
#include <QDebug>

SymmetryMultiplier::SymmetryMultiplier(const CrystalAtom &atom, const QVector<QStringList> &symops, const QString &transla, const QString &lattice)
    : _atom(atom), _symops(symops), _transla(transla), _lattice(lattice)
{
    _symEqs.append(normalizeAtomCoordinates(atom));
    applyTranslation(_symEqs, _transla, _lattice);
    _symEqs = applySymops(_symEqs, _symops);
    _symEqs = elimitateDuplicates(_symEqs);
}

CrystalAtom SymmetryMultiplier::normalizeAtomCoordinates(const CrystalAtom &cAtm)
{
    CrystalAtom oAtm = cAtm;

    while (oAtm.x() < 0.0) oAtm.x() += 1.0;
    while (oAtm.y() < 0.0) oAtm.y() += 1.0;
    while (oAtm.z() < 0.0) oAtm.z() += 1.0;

    oAtm.x() = fmod(oAtm.x(), 1.0);
    oAtm.y() = fmod(oAtm.y(), 1.0);
    oAtm.z() = fmod(oAtm.z(), 1.0);

    return oAtm;
}


void SymmetryMultiplier::applyTranslation(QList<CrystalAtom> &atoms, const QString &transla, const QString &lattice)
{
    if (transla == "P") {
        return;
    }

    if (transla == "A") {
        CrystalAtom tAtm = atoms.first();
        tAtm.y() += 0.5;
        tAtm.z() += 0.5;
        atoms.append(normalizeAtomCoordinates(tAtm));
    }

    if (transla == "B") {
        CrystalAtom tAtm = atoms.first();
        tAtm.x() += 0.5;
        tAtm.z() += 0.5;
        atoms.append(normalizeAtomCoordinates(tAtm));
    }

    if (transla == "C") {
        CrystalAtom tAtm = atoms.first();
        tAtm.x() += 0.5;
        tAtm.y() += 0.5;
        atoms.append(normalizeAtomCoordinates(tAtm));
    }

    if (transla == "I") {
        CrystalAtom tAtm = atoms.first();
        tAtm.x() += 0.5;
        tAtm.y() += 0.5;
        tAtm.z() += 0.5;
        atoms.append(normalizeAtomCoordinates(tAtm));
    }

    if (transla == "F") {
        CrystalAtom tAtmA = atoms.first();
        CrystalAtom tAtmB = atoms.first();
        CrystalAtom tAtmC = atoms.first();

        tAtmA.y() += 0.5;
        tAtmA.z() += 0.5;

        tAtmB.x() += 0.5;
        tAtmB.z() += 0.5;

        tAtmC.x() += 0.5;
        tAtmC.y() += 0.5;

        atoms.append(normalizeAtomCoordinates(tAtmA));
        atoms.append(normalizeAtomCoordinates(tAtmB));
        atoms.append(normalizeAtomCoordinates(tAtmC));
    }

    if (transla == "R") { // trigonal setting
        if (lattice.toUpper() == "TRIGONAL") {
            CrystalAtom tAtmA = atoms.first();
            CrystalAtom tAtmB = atoms.first();

            tAtmA.x() += 2.0/3.0;
            tAtmA.y() += 1.0/3.0;
            tAtmA.z() += 1.0/3.0;

            tAtmB.x() += 1.0/3.0;
            tAtmB.y() += 2.0/3.0;
            tAtmB.z() += 2.0/3.0;

            atoms.append(normalizeAtomCoordinates(tAtmA));
            atoms.append(normalizeAtomCoordinates(tAtmB));
        } else { // rhombohedral setting
            return;
        }
    }
}

QList<CrystalAtom> SymmetryMultiplier::applySymops(const QList<CrystalAtom> &atoms, const QVector<QStringList> &symops)
{
    QList<CrystalAtom> out;
    QJSEngine scriptEngine;

    for (int a = 0; a < atoms.size(); ++a) {
        for (int s = 0; s < symops.size(); ++s) {
            CrystalAtom oAtm = atoms.at(a);

            scriptEngine.globalObject().setProperty("x", atoms.at(a).x());
            scriptEngine.globalObject().setProperty("y", atoms.at(a).y());
            scriptEngine.globalObject().setProperty("z", atoms.at(a).z());

            double wx = scriptEngine.evaluate(symops.at(s)[0]).toNumber();
            double wy = scriptEngine.evaluate(symops.at(s)[1]).toNumber();
            double wz = scriptEngine.evaluate(symops.at(s)[2]).toNumber();

            oAtm.setFcoordinates(wx, wy, wz);
            out.append(normalizeAtomCoordinates(oAtm));
        }
    }

    return out;
}

QList<CrystalAtom> SymmetryMultiplier::elimitateDuplicates(const QList<CrystalAtom> &atoms)
{
    QSet<QString> keyList;
    QList<CrystalAtom> outList;

    double t = 10000.0;

    qDebug() << QString("    Symmetric equivalent positions");

    for (int i = 0; i < atoms.size(); ++i) {
        QString key = QString("%1;%2;%3")
                .arg(int(t * atoms.at(i).x() + 0.5))
                .arg(int(t * atoms.at(i).y() + 0.5))
                .arg(int(t * atoms.at(i).z() + 0.5));

        if (!keyList.contains(key.simplified())) {
            outList.append(atoms.at(i));
            keyList << key.simplified();
            qDebug() << QString("        %1(%2) %3 %4 %5")
                .arg(atoms.at(i).name())
                .arg(outList.size())
                .arg(atoms.at(i).x())
                .arg(atoms.at(i).y())
                .arg(atoms.at(i).z());
        }
    }

    return outList;
}
