/***************************************************************************
                          linearcurve.h  -  description
                             -------------------
    begin                : Tue Nov 12 19:20:03 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#ifndef LINEARCURVE_H
#define LINEARCURVE_H

#include "genericcurve.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT LinearCurve : public GenericCurve
{
public:
    LinearCurve();

    ~LinearCurve() {}

    inline CurveType type() const {return LINEAR;}

    static inline QString descriptionStatic() {return "Linear";}
    static inline QString equationTextStatic() {return QString("f(x) = a + bx");}
    static QStringList parameterTextStatic();

    inline QString description() const {return descriptionStatic();}
    QString equationText() const {return equationTextStatic();}
    QStringList parameterText() const {return parameterTextStatic();}

    inline int nParameters() const {return 2;}

    QStringList parameterNames() const;
    inline QList<double> defaultValues() const {return QList<double>() << 0.0 << 0.0;}
    inline QStringList lowerLimits(double, double, double) const {return QStringList() << "-inf" << "-inf";}
    inline QStringList upperLimits(double, double, double) const {return QStringList() << "+inf" << "+inf";}

    double fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x);
    void compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy);    
};

#endif // LINEARCURVE_H
