/***************************************************************************
                          crystalsymop.cpp  -  description
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

#include "crystalsymop.h"
#include "crystalatom.h"
#include "functions.h"
#include <QRegularExpression>
#include <QDebug>

CrystalSymOp::CrystalSymOp(double _x11, double _x12, double _x13, double _x14,
                           double _x21, double _x22, double _x23, double _x24,
                           double _x31, double _x32, double _x33, double _x34)
    : _m11(_x11), _m12(_x12), _m13(_x13), _m14(_x14),
      _m21(_x21), _m22(_x22), _m23(_x23), _m24(_x24),
      _m31(_x31), _m32(_x32), _m33(_x33), _m34(_x34)
{

}

CrystalSymOp::CrystalSymOp(const CrystalSymOp &a)
    : _m11(a._m11), _m12(a._m12), _m13(a._m13), _m14(a._m14),
      _m21(a._m21), _m22(a._m22), _m23(a._m23), _m24(a._m24),
      _m31(a._m31), _m32(a._m32), _m33(a._m33), _m34(a._m34)
{

}

CrystalSymOp::CrystalSymOp(const QStringList &l)
{
    parseString(l);
}

CrystalSymOp::CrystalSymOp(const QString &s)
{
    static QRegularExpression rx("[,\\s]");
    parseString(s.split(rx));
}

void CrystalSymOp::operator=(const CrystalSymOp &a)
{
    _m11 = a._m11;
    _m12 = a._m12;
    _m13 = a._m13;
    _m14 = a._m14;
    _m21 = a._m21;
    _m22 = a._m22;
    _m23 = a._m23;
    _m24 = a._m24;
    _m31 = a._m31;
    _m32 = a._m32;
    _m33 = a._m33;
    _m34 = a._m34;
}

bool CrystalSymOp::operator==(const CrystalSymOp &a) const
{
    if (!qFuzzyCompare(_m11, a._m11)) return false;
    if (!qFuzzyCompare(_m12, a._m12)) return false;
    if (!qFuzzyCompare(_m13, a._m13)) return false;
    if (!qFuzzyCompare(_m14, a._m14)) return false;
    if (!qFuzzyCompare(_m21, a._m21)) return false;
    if (!qFuzzyCompare(_m22, a._m22)) return false;
    if (!qFuzzyCompare(_m23, a._m23)) return false;
    if (!qFuzzyCompare(_m24, a._m24)) return false;
    if (!qFuzzyCompare(_m31, a._m31)) return false;
    if (!qFuzzyCompare(_m32, a._m32)) return false;
    if (!qFuzzyCompare(_m33, a._m33)) return false;
    if (!qFuzzyCompare(_m34, a._m34)) return false;
    return true;
}

bool CrystalSymOp::isEmpty() const
{
    if (!qFuzzyIsNull(_m11)) return false;
    if (!qFuzzyIsNull(_m12)) return false;
    if (!qFuzzyIsNull(_m13)) return false;
    if (!qFuzzyIsNull(_m14)) return false;
    if (!qFuzzyIsNull(_m21)) return false;
    if (!qFuzzyIsNull(_m22)) return false;
    if (!qFuzzyIsNull(_m23)) return false;
    if (!qFuzzyIsNull(_m24)) return false;
    if (!qFuzzyIsNull(_m31)) return false;
    if (!qFuzzyIsNull(_m32)) return false;
    if (!qFuzzyIsNull(_m33)) return false;
    if (!qFuzzyIsNull(_m34)) return false;
    return true;
}

void CrystalSymOp::setMatrix(double _x11, double _x12, double _x13, double _x14,
                             double _x21, double _x22, double _x23, double _x24,
                             double _x31, double _x32, double _x33, double _x34)
{
    _m11 = _x11;
    _m12 = _x12;
    _m13 = _x13;
    _m14 = _x14;
    _m21 = _x21;
    _m22 = _x22;
    _m23 = _x23;
    _m24 = _x24;
    _m31 = _x31;
    _m32 = _x32;
    _m33 = _x33;
    _m34 = _x34;
}

void CrystalSymOp::parseString(const QStringList &l)
{
    _m11 = 0.0;
    _m12 = 0.0;
    _m13 = 0.0;
    _m14 = 0.0;
    _m21 = 0.0;
    _m22 = 0.0;
    _m23 = 0.0;
    _m24 = 0.0;
    _m31 = 0.0;
    _m32 = 0.0;
    _m33 = 0.0;
    _m34 = 0.0;

    if (l.size() != 3) {
        qDebug() << QString("CrystalSymOp::parseString(): Wrong size of string list: %1").arg(l.size());
        return;
    }

    parseOperator(l.at(0), _m11, _m12, _m13, _m14);
    parseOperator(l.at(1), _m21, _m22, _m23, _m24);
    parseOperator(l.at(2), _m31, _m32, _m33, _m34);
}

void CrystalSymOp::parseOperator(const QString &o, double &_mx1, double &_mx2, double &_mx3, double &_mx4)
{
    // operators can have format "2/3-x+y" or "-x+y+2/3"
    // we extract all general operators and one fraction

    static QRegularExpression rxOp("(-?[x-zX-Z])");
    static QRegularExpression rxTr("(-?\\d+)\\/(\\d+)");

    QStringList op;
    int num = 0;
    int den = 1;

    QRegularExpressionMatchIterator riOp = rxOp.globalMatch(o);

    while (riOp.hasNext()) {
        op.append(riOp.next().captured(1));
    }

    for (int i = 0; i < op.size(); ++i) {
        if      (op.at(i).toLower() == "x")  _mx1 += 1.0;
        else if (op.at(i).toLower() == "-x") _mx1 -= 1.0;
        else if (op.at(i).toLower() == "y")  _mx2 += 1.0;
        else if (op.at(i).toLower() == "-y") _mx2 -= 1.0;
        else if (op.at(i).toLower() == "z")  _mx3 += 1.0;
        else if (op.at(i).toLower() == "-z") _mx3 -= 1.0;
    }

    QRegularExpressionMatch rmTr = rxTr.match(o);

    if (rmTr.hasMatch()) {
        bool numOk;
        bool denOk;
        num = rmTr.captured(1).toInt(&numOk);
        den = rmTr.captured(2).toInt(&denOk);

        if (numOk && denOk) {
            while (num > den) num -= den;
            double frac = double(num)/double(den);
            while (frac < 0.0) frac += 1.0;
            _mx4 += frac;
        }
    }
}

QList<CrystalSymOp> CrystalSymOp::translate(const QString &t)
{
    if (t.toUpper() == "I") {
        CrystalSymOp ci = *this;

        ci._m14 += 0.5;
        ci._m24 += 0.5;
        ci._m34 += 0.5;

        if (ci._m14 > 0.999) ci._m14 -= 1.0;
        if (ci._m24 > 0.999) ci._m24 -= 1.0;
        if (ci._m34 > 0.999) ci._m34 -= 1.0;

        return QList<CrystalSymOp>() << ci;
    }

    if (t.toUpper() == "F") {
        CrystalSymOp ca = *this;
        CrystalSymOp cb = *this;
        CrystalSymOp cc = *this;

        ca._m14 += 0.5;
        ca._m24 += 0.5;

        cb._m14 += 0.5;
        cb._m34 += 0.5;

        cc._m24 += 0.5;
        cc._m34 += 0.5;

        if (ca._m14 > 0.999) ca._m14 -= 1.0;
        if (ca._m24 > 0.999) ca._m24 -= 1.0;

        if (cb._m14 > 0.999) cb._m14 -= 1.0;
        if (cb._m34 > 0.999) cb._m34 -= 1.0;

        if (cc._m24 > 0.999) cc._m24 -= 1.0;
        if (cc._m34 > 0.999) cc._m34 -= 1.0;

        return QList<CrystalSymOp>() << ca << cb << cc;
    }

    if (t.toUpper() == "A") {
        CrystalSymOp ca = *this;

        ca._m24 += 0.5;
        ca._m34 += 0.5;

        if (ca._m24 > 0.999) ca._m24 -= 1.0;
        if (ca._m34 > 0.999) ca._m34 -= 1.0;

        return QList<CrystalSymOp>() << ca;
    }

    if (t.toUpper() == "B") {
        CrystalSymOp cb = *this;

        cb._m14 += 0.5;
        cb._m34 += 0.5;

        if (cb._m14 > 0.999) cb._m14 -= 1.0;
        if (cb._m34 > 0.999) cb._m34 -= 1.0;

        return QList<CrystalSymOp>() << cb;
    }

    if (t.toUpper() == "C") {
        CrystalSymOp cc = *this;

        cc._m14 += 0.5;
        cc._m24 += 0.5;

        if (cc._m14 > 0.999) cc._m14 -= 1.0;
        if (cc._m24 > 0.999) cc._m24 -= 1.0;

        return QList<CrystalSymOp>() << cc;
    }

    if (t.toUpper() == "RHX") {
        CrystalSymOp ca = *this;
        CrystalSymOp cb = *this;

        ca._m14 += 2.0/3.0;
        ca._m24 += 1.0/3.0;
        ca._m34 += 1.0/3.0;

        cb._m14 += 1.0/3.0;
        cb._m24 += 2.0/3.0;
        cb._m34 += 2.0/3.0;

        if (ca._m14 > 0.999) ca._m14 -= 1.0;
        if (ca._m24 > 0.999) ca._m24 -= 1.0;
        if (ca._m34 > 0.999) ca._m34 -= 1.0;

        if (cb._m14 > 0.999) cb._m14 -= 1.0;
        if (cb._m24 > 0.999) cb._m24 -= 1.0;
        if (cb._m34 > 0.999) cb._m34 -= 1.0;

        return QList<CrystalSymOp>() << ca << cb;
    }

    if (t.toUpper() == "D") {
        CrystalSymOp ca = *this;
        CrystalSymOp cb = *this;

        ca._m14 += 1.0/3.0;
        ca._m24 += 1.0/3.0;
        ca._m34 += 1.0/3.0;

        cb._m14 += 2.0/3.0;
        cb._m24 += 2.0/3.0;
        cb._m34 += 2.0/3.0;

        if (ca._m14 > 0.999) ca._m14 -= 1.0;
        if (ca._m24 > 0.999) ca._m24 -= 1.0;
        if (ca._m34 > 0.999) ca._m34 -= 1.0;

        if (cb._m14 > 0.999) cb._m14 -= 1.0;
        if (cb._m24 > 0.999) cb._m24 -= 1.0;
        if (cb._m34 > 0.999) cb._m34 -= 1.0;

        return QList<CrystalSymOp>() << ca << cb;
    }

    return QList<CrystalSymOp>();
}

QStringList CrystalSymOp::toStringList() const
{
    QString sx, sy, sz;
    QString fx = global::Functions::floatToFractionStringNormalized(_m14);
    QString fy = global::Functions::floatToFractionStringNormalized(_m24);
    QString fz = global::Functions::floatToFractionStringNormalized(_m34);

    if (!qFuzzyIsNull(_m11)) sx += _m11 < 0.0 ? "-x" : "x";
    if (!qFuzzyIsNull(_m12)) sx += _m12 < 0.0 ? "-y" : (sx.isEmpty() ? "y" : "+y");
    if (!qFuzzyIsNull(_m13)) sx += _m13 < 0.0 ? "-z" : (sx.isEmpty() ? "z" : "+z");
    if (!qFuzzyIsNull(_m14)) sx += _m14 < 0.0 ? fx   : (sx.isEmpty() ? fx  : ("+" + fx));

    if (!qFuzzyIsNull(_m21)) sy += _m21 < 0.0 ? "-x" : "x";
    if (!qFuzzyIsNull(_m22)) sy += _m22 < 0.0 ? "-y" : (sy.isEmpty() ? "y" : "+y");
    if (!qFuzzyIsNull(_m23)) sy += _m23 < 0.0 ? "-z" : (sy.isEmpty() ? "z" : "+z");
    if (!qFuzzyIsNull(_m24)) sy += _m24 < 0.0 ? fy   : (sy.isEmpty() ? fy  : ("+" + fy));

    if (!qFuzzyIsNull(_m31)) sz += _m31 < 0.0 ? "-x" : "x";
    if (!qFuzzyIsNull(_m32)) sz += _m32 < 0.0 ? "-y" : (sz.isEmpty() ? "y" : "+y");
    if (!qFuzzyIsNull(_m33)) sz += _m33 < 0.0 ? "-z" : (sz.isEmpty() ? "z" : "+z");
    if (!qFuzzyIsNull(_m34)) sz += _m34 < 0.0 ? fz   : (sz.isEmpty() ? fz  : ("+" + fz));

    return QStringList() << sx << sy << sz;
}

CrystalAtom CrystalSymOp::transform(const CrystalAtom &atIn)
{
    CrystalAtom atOut = atIn;

    double _ax = _m11 * atIn.x() + _m12 * atIn.y() + _m13 * atIn.z() + _m14;
    double _ay = _m21 * atIn.x() + _m22 * atIn.y() + _m23 * atIn.z() + _m24;
    double _az = _m31 * atIn.x() + _m32 * atIn.y() + _m33 * atIn.z() + _m34;

    atOut.setFcoord_x(_ax);
    atOut.setFcoord_y(_ay);
    atOut.setFcoord_z(_az);

    return atOut;
}
