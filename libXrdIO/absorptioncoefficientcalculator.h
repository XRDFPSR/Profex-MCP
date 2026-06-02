/***************************************************************************
                          absorptioncoefficientcalculator.h  -  description
                             -------------------
    begin                : Mon Feb 03 18:00:00 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#ifndef ABSORPTIONCOEFFICIENTCALCULATOR_H
#define ABSORPTIONCOEFFICIENTCALCULATOR_H

#include <QMap>
#include <QString>

#if defined XRDIO
#define XRDIO_EXPORT Q_DECL_EXPORT
#else
#define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT AbsorptionCoefficientCalculator
{
public:
    AbsorptionCoefficientCalculator();

    /*
     * Always provide formula string in the form "Al2 O3"
     */
    static QString getToolTip();
    static double getMacFromFormula(const QString &f, double wl);
    static double getLacFromFormula(const QString &f, double wl, double rho);
    static QMap<QString, double> parseFormula(const QString &);
    static QMap<QString, double> fractionalFormula(const QMap<QString, double> &);
    static double calcMac(const QMap<QString, double> &, double wl);
    static double calcLac(double, double);
};

#endif // ABSORPTIONCOEFFICIENTCALCULATOR_H
