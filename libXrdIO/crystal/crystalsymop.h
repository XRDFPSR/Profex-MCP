/***************************************************************************
                          crystalsymop.h  -  description
                             -------------------
    begin                : Mon Sept 28 07:53:00 CEST 2020
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

#ifndef CRYSTALSYMOP_H
#define CRYSTALSYMOP_H

#include <QStringList>
#include <QString>

class CrystalAtom;

/*
 * /m11 m12 m13\ /m14\
 * |m21 m22 m23| |m24|
 * \m31 m32 m33/ \m34/
 */

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT CrystalSymOp
{
public:
    CrystalSymOp(double _x11, double _x12, double _x13, double _x14,
                 double _x21, double _x22, double _x23, double _x24,
                 double _x31, double _x32, double _x33, double _x34);

    CrystalSymOp(const CrystalSymOp &);
    CrystalSymOp(const QStringList &);
    CrystalSymOp(const QString &);

    void operator=(const CrystalSymOp &);
    bool operator==(const CrystalSymOp &) const;

    void setMatrix(double _x11, double _x12, double _x13, double _x14,
                   double _x21, double _x22, double _x23, double _x24,
                   double _x31, double _x32, double _x33, double _x34);
    void setSymOp(const QStringList &);
    void setSymOp(const QString &);

    bool isEmpty() const;

    CrystalAtom transform(const CrystalAtom &);
    QList<CrystalSymOp> translate(const QString &);
    QStringList toStringList() const;

    inline double m11() const {return _m11;}
    inline double m12() const {return _m12;}
    inline double m13() const {return _m13;}
    inline double m14() const {return _m14;}
    inline double m21() const {return _m21;}
    inline double m22() const {return _m22;}
    inline double m23() const {return _m23;}
    inline double m24() const {return _m24;}
    inline double m31() const {return _m31;}
    inline double m32() const {return _m32;}
    inline double m33() const {return _m33;}
    inline double m34() const {return _m34;}

private:
    double _m11;
    double _m12;
    double _m13;
    double _m14;
    double _m21;
    double _m22;
    double _m23;
    double _m24;
    double _m31;
    double _m32;
    double _m33;
    double _m34;

    void parseString(const QStringList &);
    void parseOperator(const QString &, double &, double &, double &, double &);
};

#endif // CRYSTALSYMOP_H
