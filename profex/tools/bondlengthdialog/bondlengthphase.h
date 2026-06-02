/***************************************************************************
                          bondlengthphase.h  -  description
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

#ifndef BONDLENGTHPHASE_H
#define BONDLENGTHPHASE_H

#include "../libXrdIO/crystal/crystalstructure.h"

class BondLengthPhase
{
public:
    BondLengthPhase();
    BondLengthPhase(const CrystalStructure &, double filter);

    void setStructure(const CrystalStructure &, double filter);

    inline QList<QList<double> > getBondLengths() const {return _bondLengthsTable;}
    inline QStringList getHeaderLabels() const {return _headerLabels;}

    int rowCount() const;
    int columnCount() const;

private:
    CrystalStructure _structure;
    QStringList _headerLabels;
    QMap<int, QList<CrystalAtom> > _symEqAtoms;
    QList<QList<double> > _bondLengthsTable;

    QStringList parseHeader(const QList<CrystalAtom> &);
    QMap<int, QList<CrystalAtom> > generateSymEqAtoms(CrystalStructure &);
    QList<QList<double> > calculateBondLengths(const CrystalStructure &, const QMap<int, QList<CrystalAtom> > &, double);
    double bondLength(double a, double b, double c, double al, double be, double ga, const QList<CrystalAtom> &, const QList<CrystalAtom> &);
    double lowestValue(const QList<double> &);
};

#endif // BONDLENGTHPHASE_H
