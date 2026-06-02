/***************************************************************************
                          crystalstructure.h  -  description
                             -------------------
    begin                : Sat Aug 22 08:53:00 CEST 2015
    copyright            : (C) 2015 by Nicola Doebelin
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

#ifndef CRYSTALSTRUCTURE_H
#define CRYSTALSTRUCTURE_H

#include "crystalatom.h"
#include "crystalunitcell.h"
#include "crystalstructurefactor.h"
#include "hkl.h"

#include <QObject>
#include <QString>
#include <QList>
#include <QMap>
#include <QVariant>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT CrystalStructure
{
public:
    CrystalStructure();
    CrystalStructure(const QString &p);

    void clear();
    void reset();

    inline void setSymops(const QList<QStringList> &l) {_unitCell.setSymmetryOperations(l);}
    inline void setLattice(int i)                      {_lattice = i;}
    inline void setName(const QString &s)              {_name = s;}
    inline void setZ(int i)                            {_z = i;}
    inline void setAuxInfo(const QString &s, const QVariant &v) {_auxinfo.insert(s, v);}
    inline void setUnitCell(const CrystalUnitCell &c)  {_unitCell = c;}
    inline void setAtoms(const QList<CrystalAtom> &l)  {_atoms = l;}
    inline void addAtom(const CrystalAtom &a)          {_atoms.append(a);}
    inline void setDensity(double d)                   {_density = d;}
    inline void setAxisUnit(unitLength u)              {_unitCell.setAxisUnit(u);}
    void setRfactors(double, double, double);
    inline void setStructureFactors(const QList<CrystalStructureFactor> &l) {_fstructs = l;}
    inline void addStructureFactor(const CrystalStructureFactor &s)         {_fstructs.append(s);}

    inline CrystalUnitCell & unitCell()             {return _unitCell;}
    inline CrystalUnitCell unitCell() const         {return _unitCell;}

    inline QList<CrystalAtom> & atoms()             {return _atoms;}
    inline QList<CrystalAtom> atoms() const         {return _atoms;}
    QList<CrystalAtom> generateAtoms(const QList<CrystalAtom> &);

    inline QList<CrystalStructureFactor> & structureFactors()     {return _fstructs;}
    inline QList<CrystalStructureFactor> structureFactors() const {return _fstructs;}

    inline QString name() const                     {return _name;}
    inline int Z() const                            {return _z;}
    inline double density() const                   {return _density;}
    inline QVariant auxInfo(const QString &s) const {return _auxinfo[s];}
    inline unitLength axisUnit() const              {return _unitCell.axisUnit();}

    bool hasWyckoff();

    QString toBgmnStr();
    QString toCif(bool complete, const QString &source = QString(), const QMap<QString, QVariant> &auxData = QMap<QString, QVariant>());
    QString toCastepCell();
    QString toSumFormula(int z = 1);

    QMap<long, QList<CrystalStructureFactor> > getHklList(double, double lambda, double b1 = 1.0);

private:
    CrystalUnitCell _unitCell;
    QList<CrystalAtom> _atoms;
    QList<CrystalStructureFactor> _fstructs;
    QString _name;
    int _lattice;
    int _z;
    double _density;
    QMap<QString, QVariant> _auxinfo;
    double _rWp, _rExp, _chi2;

    QList<QList<CrystalAtom> > mergeAtomsBgmn(const QList<CrystalAtom> &);
    void checkSOF(QList<QList<CrystalAtom> > &);
};



#endif // CRYSTALSTRUCTURE_H
