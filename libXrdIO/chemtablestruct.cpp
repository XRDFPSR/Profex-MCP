/***************************************************************************
                          chemtablestruct.cpp  -  description
                             -------------------
    begin                : Wed Oct 08 11:30:00 CEST 2025
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

#include "chemtablestruct.h"

ChemTableStruct::ChemTableStruct(const ChemistryMode &m)
    : _mode(m)
{

}

void ChemTableStruct::clear()
{
    _mode = UNKNOWN;
    _phaseLabels.clear();
    _elementLabels.clear();
    _values.clear();
    _esds.clear();

}

void ChemTableStruct::addValue(const QString &ph, const QString &el, double v)
{
    _values[ph][el] = v;
}

void ChemTableStruct::addEsd(const QString &ph, const QString &el, double v)
{
    _esds[ph][el] = v;
}

double ChemTableStruct::getValue(int r, int c) const
{
    if (r < 0 || c < 0)             return 0.0;
    if (r >= _phaseLabels.size())   return 0.0;
    if (c >= _elementLabels.size()) return 0.0;

    QString ph = _phaseLabels.at(r);
    QString el = _elementLabels.at(c);

    return getValue(ph, el);
}

double ChemTableStruct::getEsd(int r, int c) const
{
    if (r < 0 || c < 0)             return 0.0;
    if (r >= _phaseLabels.size())   return 0.0;
    if (c >= _elementLabels.size()) return 0.0;

    QString ph = _phaseLabels.at(r);
    QString el = _elementLabels.at(c);

    return getEsd(ph, el);
}

double ChemTableStruct::getTotal(int c) const
{
    if (c < 0)                      return -1.0;
    if (c >= _elementLabels.size()) return -1.0;

    QString el = _elementLabels.at(c);
    return getTotal(el);
}

double ChemTableStruct::getValue(const QString &p, const QString &e) const
{
    if (!_values.contains(p)) return 0.0;
    return (_values.value(p).value(e, 0.0));
}

double ChemTableStruct::getEsd(const QString &p, const QString &e) const
{
    if (!_esds.contains(p)) return 0.0;
    return (_esds.value(p).value(e, 0.0));
}

double ChemTableStruct::getTotal(const QString &e) const
{
    double val = 0.0;

    for (int i = 0; i < _phaseLabels.size(); ++i) {
        double q = _phaseQuantities.value(_phaseLabels.at(i), 0.0);
        double v = _values.value(_phaseLabels.at(i)).value(e, 0.0);
        val += q * v;
    }

    return val;
}

double ChemTableStruct::getSumQuantities() const
{
    double q = 0.0;
    QHashIterator<QString, double> it(_phaseQuantities);

    while (it.hasNext()) {
        it.next();
        q += it.value();
    }

    return q;
}

bool ChemTableStruct::hasData() const
{
    if (_phaseLabels.isEmpty()) return false;
    if (_elementLabels.isEmpty()) return false;
    if (_values.isEmpty()) return false;
    return true;
}
