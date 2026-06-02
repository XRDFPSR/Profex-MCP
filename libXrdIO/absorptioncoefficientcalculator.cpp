/***************************************************************************
                          absorptioncoefficientcalculator.cpp  -  description
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

#include "absorptioncoefficientcalculator.h"
#include "structs.h"
#include "functions.h"
#include "elementscatteringdatamanager.h"

#include <QRegularExpression>

AbsorptionCoefficientCalculator::AbsorptionCoefficientCalculator() {}

QString AbsorptionCoefficientCalculator::getToolTip()
{
    return QString("Sum formula, for example\n\nSi\nSi O2\nAl2 O3\nCa3 P2 O8\n\nParentheses are not allowed.");
}

double AbsorptionCoefficientCalculator::getMacFromFormula(const QString &f, double wl)
{
    QMap<QString, double> formula = parseFormula(f);
    QMap<QString, double> fracFormula = fractionalFormula(formula);
    return calcMac(fracFormula, wl);
}

double AbsorptionCoefficientCalculator::getLacFromFormula(const QString &f, double wl, double rho)
{
    double mac = getMacFromFormula(f, wl);
    if (mac < 0.0) return -1.0;
    return mac * rho;
}

/*
 * returns the sum formula as a map [element string][number of atoms double]
 */
QMap<QString, double> AbsorptionCoefficientCalculator::parseFormula(const QString &form)
{
    QMap<QString, double> formula;
    if (form.isEmpty()) return formula;

    static QRegularExpression rxEl("([A-Za-z]{1,2})(\\d+\\.?\\d*)?");
    static QRegularExpression rxSep("[\\s;_]+");
    QRegularExpressionMatch rm;
    QStringList elements(form.split(rxSep));

    for (int i = 0; i < elements.size(); ++i) {
        rm = rxEl.match(elements.at(i));
        if (!rm.hasMatch()) continue;

        QString el = rm.captured(1).toUpper();
        double idx = rm.captured(2).isEmpty() ? 1.0 : rm.captured(2).toDouble();

        if (formula.contains(el)) formula[el] += idx;
        else                      formula[el] = idx;
    }

    return formula;
}

/*
 * returns a map with the sum formula elements by weight [element string][weight in g pfu double]
 */
QMap<QString, double> AbsorptionCoefficientCalculator::fractionalFormula(const QMap<QString, double> &formula)
{
    QStringList atms = global::atoms.split(";");
    QMap<QString, double> mapMolWeight;

    for (int i = 0; i < atms.size() - 5; i += 5) {
        mapMolWeight[atms.at(i+1).toUpper()] = atms.at(i+2).toDouble();
    }

    double molWt = 0.0;
    QMap<QString, double> fformula;
    QMapIterator<QString, double> it(formula);

    while (it.hasNext()) {
        it.next();
        molWt += mapMolWeight.value(it.key()) * it.value();
    }

    it.toFront();

    while (it.hasNext()) {
        it.next();
        fformula[it.key()] = (mapMolWeight.value(it.key()) * it.value()) / molWt;
    }

    return fformula;
}

double AbsorptionCoefficientCalculator::calcMac(const QMap<QString, double> &fracFormula, double wl)
{
    ElementScatteringDataManager scatData;
    double mac = 0.0;
    double en = global::Functions::wavelengthToEnergy(wl);

    QMapIterator<QString, double> it(fracFormula);

    while (it.hasNext()) {
        it.next();

        double elMac = scatData.getMacForKeV(it.key(), en);
        if (elMac < 0.0) return -1.0;

        mac += scatData.getMacForKeV(it.key(), en) * it.value();
    }

    return mac;
}

double AbsorptionCoefficientCalculator::calcLac(double mac, double rho)
{
    return mac * rho;
}
