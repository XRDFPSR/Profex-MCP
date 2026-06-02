/***************************************************************************
                          structs.h  -  description
                             -------------------
    begin                : Mon Oct 07 12:46:00 CEST 2014
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

#ifndef EMAPSTRUCTS_H
#define EMAPSTRUCTS_H

#include <QVector>
#include <QString>
#include "math.h"

enum synthesis_t {UNDEFINED, FOBS, FCALC, FDIFF};
enum projection_t {AB_PLANE, AC_PLANE, BC_PLANE, A_AXIS, B_AXIS, C_AXIS};

struct UnitCell {
    float a;
    float b;
    float c;
    float alpha;
    float beta;
    float gamma;
    float volume;
    QVector<QVector<float> > hkl;
};

struct Atom {
    // atom position in fractional coordinates
    QString name;
    float x;
    float y;
    float z;
    float r;

    Atom() : name(), x(), y(), z(), r() {}
    Atom(QString n, float fx, float fy, float fz, float fr) : name(n), x(fx), y(fy), z(fz), r(fr) {}

    bool operator==(const Atom &a) const {
        return  (fabs(a.x - x) < 0.0001) &&
                (fabs(a.y - y) < 0.0001) &&
                (fabs(a.z - z) < 0.0001);
    }
};

typedef QVector<QVector<QVector<float> > > EDataMap;

#endif // EMAPSTRUCTS_H
