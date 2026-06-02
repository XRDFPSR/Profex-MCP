/***************************************************************************
                          symmetrymultiplier.h  -  description
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


#ifndef SYMMETRYMULTIPLIER_H
#define SYMMETRYMULTIPLIER_H

#include "../libXrdIO/crystal/crystalatom.h"
#include <QStringList>
#include <QString>
#include <QVector>
#include <QList>

class SymmetryMultiplier
{
public:
    typedef QList<CrystalAtom> result_type;
    SymmetryMultiplier(const CrystalAtom &atom, const QVector<QStringList> &symops, const QString &transla, const QString &lattice);

    inline QList<CrystalAtom> getEquivalents() const {return _symEqs;}

private:
    CrystalAtom _atom;
    QVector<QStringList> _symops;
    QString _transla;
    QString _lattice;
    QList<CrystalAtom> _symEqs;

    CrystalAtom normalizeAtomCoordinates(const CrystalAtom &);
    void applyTranslation(QList<CrystalAtom> &atoms, const QString &transla, const QString &lattice);
    QList<CrystalAtom> applySymops(const QList<CrystalAtom> &, const QVector<QStringList> &);
    QList<CrystalAtom> elimitateDuplicates(const QList<CrystalAtom> &);
};

#endif // SYMMETRYMULTIPLIER_H
