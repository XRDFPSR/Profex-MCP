/***************************************************************************
                          bgmnl2curve.h  -  description
                             -------------------
    begin                : Fri Dec 27 12:35:03 CEST 2024
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

#ifndef BGMNL2CURVE_H
#define BGMNL2CURVE_H

#include "genericcurve.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BgmnL2Curve : public GenericCurve
{
public:
    BgmnL2Curve();

    ~BgmnL2Curve() {}

    inline CurveType type() const {return BGMNL2;}

    static inline QString descriptionStatic() {return "Bgmn L2";}
    static inline QString equationTextStatic() {return QString("f(x) = 2 * g * q^3 / (q^2 + (e-x)^2)^2");}
    static QStringList parameterTextStatic();

    inline QString description() const {return descriptionStatic();}
    QString equationText() const {return equationTextStatic();}
    QStringList parameterText() const {return parameterTextStatic();}

    inline int nParameters() const {return 3;}
    inline bool hasArea() const {return true;}

    QStringList parameterNames() const;
    inline QList<double> defaultValues() const {return QList<double>() << -1.0 << 0.001 << 1.0;}
    inline QStringList lowerLimits(double mi, double, double ss) const {return QStringList() << QString::number(mi, 'f', 5) << QString::number(ss, 'f', 5) << "0.00000";}
    inline QStringList upperLimits(double, double ma, double)    const {return QStringList() << QString::number(ma, 'f', 5) << "+inf" << "+inf";}

    double fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x);
    void compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy);

private:
    double calculateArea(const alglib::real_1d_array &) const;
};

#endif // LBGMNL2CURVE_H
