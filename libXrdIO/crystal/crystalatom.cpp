/***************************************************************************
                          crystalatom.cpp  -  description
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

#include "crystalatom.h"
#include "../structs.h"
#include <math.h>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

CrystalAtom::CrystalAtom()
{

}

CrystalAtom::CrystalAtom(const CrystalAtom &a) :
    _name(a._name),
    _element(a._element),
    _occ(a._occ),
    _occ_esd(a._occ_esd),
    _tds(a._tds),
    _tds_esd(a._tds_esd),
    _wyckoff(a._wyckoff),
    _multiplicity(a._multiplicity),
    _fcoords{a._fcoords[0], a._fcoords[1], a._fcoords[2]},
    _fcoords_esd{a._fcoords_esd[0], a._fcoords_esd[1], a._fcoords_esd[2]},
    _baniso{a._baniso[0], a._baniso[1], a._baniso[2], a._baniso[3], a._baniso[4], a._baniso[5]},
    _baniso_esd{a._baniso_esd[0], a._baniso_esd[1], a._baniso_esd[2], a._baniso_esd[3], a._baniso_esd[4], a._baniso_esd[5]},
    _uaniso{a._uaniso[0], a._uaniso[1], a._uaniso[2], a._uaniso[3], a._uaniso[4], a._uaniso[5]},
    _uaniso_esd{a._uaniso_esd[0], a._uaniso_esd[1], a._uaniso_esd[2], a._uaniso_esd[3], a._uaniso_esd[4], a._uaniso_esd[5]},
    _auxInfo(a._auxInfo)
{
}

CrystalAtom::CrystalAtom(const QString &e, double x, double y, double z, double o, double t) :
      _name(e),
      _element(e),
      _occ(o),
      _occ_esd(-1.0),
      _tds(t),
      _tds_esd(-1.0),
      _wyckoff(QString()),
      _multiplicity(1),
      _fcoords{x, y, z},
      _fcoords_esd{-1.0, -1.0, -1.0},
      _baniso{0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      _baniso_esd{-1.0, -1.0, -1.0, -1.0, -1.0, -1.0},
      _uaniso{0.0, 0.0, 0.0, 0.0, 0.0, 0.0},
      _uaniso_esd{-1.0, -1.0, -1.0, -1.0, -1.0, -1.0}
{
}

void CrystalAtom::operator=(const CrystalAtom &a)
{
    _name = a._name;
    _element = a._element;
    _occ = a._occ;
    _occ_esd = a._occ_esd;
    _tds = a._tds;
    _tds_esd = a._tds_esd;
    _wyckoff = a._wyckoff;
    _multiplicity = a._multiplicity;

    _fcoords[0] = a._fcoords[0];
    _fcoords[1] = a._fcoords[1];
    _fcoords[2] = a._fcoords[2];

    _fcoords_esd[0] = a._fcoords_esd[0];
    _fcoords_esd[1] = a._fcoords_esd[1];
    _fcoords_esd[2] = a._fcoords_esd[2];

    _baniso[0] = a._baniso[0];
    _baniso[1] = a._baniso[1];
    _baniso[2] = a._baniso[2];
    _baniso[3] = a._baniso[3];
    _baniso[4] = a._baniso[4];
    _baniso[5] = a._baniso[5];

    _baniso_esd[0] = a._baniso_esd[0];
    _baniso_esd[1] = a._baniso_esd[1];
    _baniso_esd[2] = a._baniso_esd[2];
    _baniso_esd[3] = a._baniso_esd[3];
    _baniso_esd[4] = a._baniso_esd[4];
    _baniso_esd[5] = a._baniso_esd[5];

    _uaniso[0] = a._uaniso[0];
    _uaniso[1] = a._uaniso[1];
    _uaniso[2] = a._uaniso[2];
    _uaniso[3] = a._uaniso[3];
    _uaniso[4] = a._uaniso[4];
    _uaniso[5] = a._uaniso[5];

    _uaniso_esd[0] = a._uaniso_esd[0];
    _uaniso_esd[1] = a._uaniso_esd[1];
    _uaniso_esd[2] = a._uaniso_esd[2];
    _uaniso_esd[3] = a._uaniso_esd[3];
    _uaniso_esd[4] = a._uaniso_esd[4];
    _uaniso_esd[5] = a._uaniso_esd[5];

    _auxInfo = a._auxInfo;
}

bool CrystalAtom::operator==(const CrystalAtom &a) const
{
    double curX = _fcoords[0];
    double curY = _fcoords[1];
    double curZ = _fcoords[2];
    double aX   = a.x();
    double aY   = a.y();
    double aZ   = a.z();

    while (curX < 0.0) curX += 1.0;
    while (curY < 0.0) curY += 1.0;
    while (curZ < 0.0) curZ += 1.0;
    while (aX   < 0.0) aX   += 1.0;
    while (aY   < 0.0) aY   += 1.0;
    while (aZ   < 0.0) aZ   += 1.0;

    while (curX > 0.0) curX -= 1.0;
    while (curY > 0.0) curY -= 1.0;
    while (curZ > 0.0) curZ -= 1.0;
    while (aX   > 0.0) aX   -= 1.0;
    while (aY   > 0.0) aY   -= 1.0;
    while (aZ   > 0.0) aZ   -= 1.0;

    return  (fabs(float(aX - curX)) < 0.0001) &&
            (fabs(float(aY - curY)) < 0.0001) &&
            (fabs(float(aZ - curZ)) < 0.0001);
}


void CrystalAtom::fcoordinates(double &x, double &y, double &z) const
{
    x = _fcoords[0];
    y = _fcoords[1];
    z = _fcoords[2];
}

void CrystalAtom::fcoords_esd(double &ex, double &ey, double &ez) const
{
    ex = _fcoords_esd[0];
    ey = _fcoords_esd[1];
    ez = _fcoords_esd[2];
}

void CrystalAtom::baniso(double &b11, double &b22, double &b33, double &b12, double &b13, double &b23) const
{
    b11 = _baniso[0];
    b22 = _baniso[1];
    b33 = _baniso[2];
    b12 = _baniso[3];
    b13 = _baniso[4];
    b23 = _baniso[5];
}

void CrystalAtom::baniso_esd(double &eb11, double &eb22, double &eb33, double &eb12, double &eb13, double &eb23) const
{
    eb11 = _baniso_esd[0];
    eb22 = _baniso_esd[1];
    eb33 = _baniso_esd[2];
    eb12 = _baniso_esd[3];
    eb13 = _baniso_esd[4];
    eb23 = _baniso_esd[5];
}

void CrystalAtom::uaniso(double &u11, double &u22, double &u33, double &u12, double &u13, double &u23) const
{
    u11 = _uaniso[0];
    u22 = _uaniso[1];
    u33 = _uaniso[2];
    u12 = _uaniso[3];
    u13 = _uaniso[4];
    u23 = _uaniso[5];
}

void CrystalAtom::uaniso_esd(double &eu11, double &eu22, double &eu33, double &eu12, double &eu13, double &eu23) const
{
    eu11 = _uaniso_esd[0];
    eu22 = _uaniso_esd[1];
    eu33 = _uaniso_esd[2];
    eu12 = _uaniso_esd[3];
    eu13 = _uaniso_esd[4];
    eu23 = _uaniso_esd[5];
}

bool CrystalAtom::has_fcoords_esd()
{
    // at least one must be >= 0.0, then we assume that Fcoord_esds were set
    bool hasFcEsd = false;
    if (_fcoords_esd[0] >= 0.0) hasFcEsd = true;
    if (_fcoords_esd[1] >= 0.0) hasFcEsd = true;
    if (_fcoords_esd[2] >= 0.0) hasFcEsd = true;
    return hasFcEsd;
}

bool CrystalAtom::has_baniso()
{
    // at least one must be != 0.0, then we assume that Baniso were set
    bool hasBaniso = false;
    if (!qFuzzyIsNull(_baniso[0])) hasBaniso = true;
    if (!qFuzzyIsNull(_baniso[1])) hasBaniso = true;
    if (!qFuzzyIsNull(_baniso[2])) hasBaniso = true;
    if (!qFuzzyIsNull(_baniso[3])) hasBaniso = true;
    if (!qFuzzyIsNull(_baniso[4])) hasBaniso = true;
    if (!qFuzzyIsNull(_baniso[5])) hasBaniso = true;
    return hasBaniso;
}

bool CrystalAtom::has_baniso_esd()
{
    // at least one must be >= 0.0, then we assume that Fcoord_esds were set
    bool hasBanisoEsd = false;
    if (_baniso_esd[0] >= 0.0) hasBanisoEsd = true;
    if (_baniso_esd[1] >= 0.0) hasBanisoEsd = true;
    if (_baniso_esd[2] >= 0.0) hasBanisoEsd = true;
    if (_baniso_esd[3] >= 0.0) hasBanisoEsd = true;
    if (_baniso_esd[4] >= 0.0) hasBanisoEsd = true;
    if (_baniso_esd[5] >= 0.0) hasBanisoEsd = true;
    return hasBanisoEsd;
}

bool CrystalAtom::has_uaniso()
{
    // at least one must be != 0.0, then we assume that Baniso were set
    bool hasUaniso = false;
    if (!qFuzzyIsNull(_uaniso[0])) hasUaniso = true;
    if (!qFuzzyIsNull(_uaniso[1])) hasUaniso = true;
    if (!qFuzzyIsNull(_uaniso[2])) hasUaniso = true;
    if (!qFuzzyIsNull(_uaniso[3])) hasUaniso = true;
    if (!qFuzzyIsNull(_uaniso[4])) hasUaniso = true;
    if (!qFuzzyIsNull(_uaniso[5])) hasUaniso = true;
    return hasUaniso;
}

bool CrystalAtom::has_uaniso_esd()
{
    // at least one must be >= 0.0, then we assume that Fcoord_esds were set
    bool hasUanisoEsd = false;
    if (_uaniso_esd[0] >= 0.0) hasUanisoEsd = true;
    if (_uaniso_esd[1] >= 0.0) hasUanisoEsd = true;
    if (_uaniso_esd[2] >= 0.0) hasUanisoEsd = true;
    if (_uaniso_esd[3] >= 0.0) hasUanisoEsd = true;
    if (_uaniso_esd[4] >= 0.0) hasUanisoEsd = true;
    if (_uaniso_esd[5] >= 0.0) hasUanisoEsd = true;
    return hasUanisoEsd;
}

void CrystalAtom::setFcoordinates(double x, double y, double z)
{
    _fcoords[0] = x;
    _fcoords[1] = y;
    _fcoords[2] = z;
}

void CrystalAtom::setFcoords_esd(double ex, double ey, double ez)
{
    _fcoords[0] = ex;
    _fcoords[1] = ey;
    _fcoords[2] = ez;
}

void CrystalAtom::setBaniso(double b11, double b22, double b33, double b12, double b13, double b23)
{
    _baniso[0] = b11;
    _baniso[1] = b22;
    _baniso[2] = b33;
    _baniso[3] = b12;
    _baniso[4] = b13;
    _baniso[5] = b23;
}

void CrystalAtom::setBaniso_esd(double eb11, double eb22, double eb33, double eb12, double eb13, double eb23)
{
    _baniso_esd[0] = eb11;
    _baniso_esd[1] = eb22;
    _baniso_esd[2] = eb33;
    _baniso_esd[3] = eb12;
    _baniso_esd[4] = eb13;
    _baniso_esd[5] = eb23;
}

void CrystalAtom::setUaniso(double u11, double u22, double u33, double u12, double u13, double u23)
{
    _uaniso[0] = u11;
    _uaniso[1] = u22;
    _uaniso[2] = u33;
    _uaniso[3] = u12;
    _uaniso[4] = u13;
    _uaniso[5] = u23;
}

void CrystalAtom::setUaniso_esd(double eu11, double eu22, double eu33, double eu12, double eu13, double eu23)
{
    _uaniso_esd[0] = eu11;
    _uaniso_esd[1] = eu22;
    _uaniso_esd[2] = eu33;
    _uaniso_esd[3] = eu12;
    _uaniso_esd[4] = eu13;
    _uaniso_esd[5] = eu23;
}

void CrystalAtom::setAuxInfo(const QString &s, const QVariant &v)
{
    _auxInfo[s] = v;
}

QVariant CrystalAtom::auxInfo(const QString &s, const QVariant &d) const
{
    return _auxInfo.value(s, d);
}
