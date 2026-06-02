/***************************************************************************
                          importhandler.h  -  description
                             -------------------
    begin                : Mon Nov 12 15:54:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#ifndef EMAPIMPORTHANDLER_H
#define EMAPIMPORTHANDLER_H

#include "../libXrdIO/colorMaps/lutstructs.h"
#include "emapstructs.h"
#include <QString>
#include <QMap>

class EmapImportHandler
{
public:
    EmapImportHandler();

    bool readFcf(const QString &, UnitCell &);
    bool readRes(const QString &, UnitCell &, QList<Atom> &);
    bool readPdb(const QString &, UnitCell &);

    bool readFou(const QString &, UnitCell &);
    bool readHkl(const QString &, UnitCell &);

private:
    QMap<QString, QList<float> > atomicRadii;
    QList<Atom> generateAtoms(const QList<Atom> &primaryAtoms, const QList<QStringList> &symOps, int latt);
    Atom normalizeAtom(const Atom &);
    QList<Atom> extendAtoms(const QList<Atom> &ucAtoms, const UnitCell &uc, float nx, float px, float ny, float py, float nz, float pz);
    QList<Atom> extendAtoms(const QList<Atom> &ucAtoms, const UnitCell &uc, float dst);
};

#endif // EMAPIMPORTHANDLER_H
