/***************************************************************************
                          crystalhklgenerator.h  -  description
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

#ifndef CRYSTALHKLGENERATOR_H
#define CRYSTALHKLGENERATOR_H

#include "crystalunitcell.h"
#include "crystalatom.h"
#include "crystalstructurefactor.h"
#include <QObject>
#include <QList>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT CrystalHklGenerator
{
public:
    CrystalHklGenerator(const CrystalUnitCell &, const QList<CrystalAtom> &);

    QMap<long, QList<CrystalStructureFactor> > getHklList(double dmin, double lambda, double b1);

private:
    CrystalUnitCell _uCell;
    QList<CrystalAtom> _lAtoms;

    double fhklSqrAmplitude(double tt, double wl, int h, int k, int l, double b, QStringList &err);
};

#endif // CRYSTALHKLGENERATOR_H
