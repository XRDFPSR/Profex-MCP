/***************************************************************************
                          fwhmmodelchernyshov.h  -  description
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

#ifndef FWHMMODELCHERNYSHOV_H
#define FWHMMODELCHERNYSHOV_H

#include "genericcurve.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT FwhmModelChernyshov : public GenericCurve
{
public:
    FwhmModelChernyshov();

    ~FwhmModelChernyshov() {}

    inline CurveType type() const {return FWHMCHERNYSHOV;}

    static inline QString descriptionStatic() {return "FWHM Model Chernyshov";}
    static inline QString equationTextStatic() {return QString("f(x) = (2*ln(2)/D^2) * (p^2-2t^2-c^2) * cos^4(x) + (2*ln(2)/D^2) * (2t^2+2c^2) * cos^2(x) + 2*phi^2*ln(2)");}
    static QStringList parameterTextStatic();

    inline QString description() const {return descriptionStatic();}
    QString equationText() const {return equationTextStatic();}
    QStringList parameterText() const {return parameterTextStatic();}

    inline int nParameters() const {return 5;}
    inline bool hasArea() const {return false;}

    QStringList parameterNames() const;
    inline QList<double> defaultValues() const {return QList<double>() << 0.0 << 0.0 << 0.0 << 0.0 << 0.0 << 0.0;}
    inline QStringList lowerLimits(double, double, double) const {return QStringList() << "0.00000" << "0.0000" << "0.0000" << "0.0000" << "0.0000";}
    inline QStringList upperLimits(double, double, double) const {return QStringList() << "+inf" << "+inf" << "+inf" << "+inf" << "+inf";}

    double fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x);
    void compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy);

private:
    double calculateArea(const alglib::real_1d_array &) const;
};

#endif // FWHMMODELCHERNYSHOV_H
