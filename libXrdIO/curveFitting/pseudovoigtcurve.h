/***************************************************************************
                          pseudovoigtcurve.h  -  description
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

#ifndef PSEUDOVOIGTCURVE_H
#define PSEUDOVOIGTCURVE_H


#include "genericcurve.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT PseudoVoigtCurve : public GenericCurve
{
public:
    PseudoVoigtCurve();

    ~PseudoVoigtCurve() {}

    inline CurveType type() const {return PSEUDOVOIGT;}

    static inline QString descriptionStatic() {return "Pseudo-Voigt";}
    static inline QString equationTextStatic() {return QString("f(x) = c * ((1 - d) * exp(-ln(2) * ((x - a) / b)^2) + d / (1 + ((x - a) / b)^2))");}
    static QStringList parameterTextStatic();

    inline QString description() const {return descriptionStatic();}
    QString equationText() const {return equationTextStatic();}
    QStringList parameterText() const {return parameterTextStatic();}

    inline int nParameters() const {return 4;}
    inline bool hasArea() const {return true;}

    QStringList parameterNames() const;
    inline QList<double> defaultValues() const {return QList<double>() << -1.0 << 0.001 << 1.0 << 0.5;}
    inline QStringList lowerLimits(double mi, double, double ss) const {return QStringList() << QString::number(mi, 'f', 5) << QString::number(ss, 'f', 5) << "0.00000" << "0.00000";}
    inline QStringList upperLimits(double, double ma, double)       const {return QStringList() << QString::number(ma, 'f', 5) << "+inf" << "+inf" << "1.00000";}

    double fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x);
    void compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy);

private:
    double calculateArea(const alglib::real_1d_array &) const;
};

#endif // PSEUDOVOIGTCURVE_H
