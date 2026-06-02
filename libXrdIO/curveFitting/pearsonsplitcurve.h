/***************************************************************************
                          pearsonsplitcurve.h  -  description
                             -------------------
    begin                : Tue Dec 17 19:20:03 CEST 2019
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

#ifndef PEARSONSPLITCURVE_H
#define PEARSONSPLITCURVE_H

#include "genericcurve.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT PearsonSplitCurve : public GenericCurve
{
public:
    PearsonSplitCurve();

    ~PearsonSplitCurve() {}

    inline CurveType type() const {return PEARSONSPLIT;}

    static inline QString descriptionStatic() {return "Split Pearson-VII";}
    static inline QString equationTextStatic() {return QString("x <= a: f(x) = c / (1.0 + ((x - a) / b1)^2 * ((2.0, 1.0 / d1) - 1.0))^d1\nx >  a: f(x) = c / (1.0 + ((x - a) / b2)^2 * ((2.0, 1.0 / d2) - 1.0))^d2");}
    static QStringList parameterTextStatic();

    inline QString description() const {return descriptionStatic();}
    QString equationText() const {return equationTextStatic();}
    QStringList parameterText() const {return parameterTextStatic();}

    inline int nParameters() const {return 6;}
    inline bool hasArea() const {return true;}

    QStringList parameterNames() const;
    inline QList<double> defaultValues() const {return QList<double>() << -1.0 << 0.001 << 0.001 << 1.0 << 2.0 << 2.0;}
    inline QStringList lowerLimits(double mi, double, double ss) const {return QStringList() << QString::number(mi, 'f', 5) << QString::number(ss, 'f', 5) << QString::number(ss, 'f', 5) << "0.00000" << "1.00000" << "1.00000";}
    inline QStringList upperLimits(double, double ma, double)    const {return QStringList() << QString::number(ma, 'f', 5) << "+inf" << "+inf" << "+inf" << "+inf" << "+inf";}

    double fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x);
    void compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy);

private:
    double part(const double &, const double &);
    double calculateArea(const alglib::real_1d_array &) const;
};

#endif // PEARSONSPLITCURVE_H
