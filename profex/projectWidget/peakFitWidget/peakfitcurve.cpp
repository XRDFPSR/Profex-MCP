/***************************************************************************
                          peakfitcurve.cpp  -  description
                             -------------------
    begin                : Fri Jul 29 21:34:00 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#include "peakfitcurve.h"

PeakFitCurve::PeakFitCurve(const QUuid &u, int type)
    : PeakFitItem(u, type)
{
    _curve = nullptr;
}

PeakFitParameter * PeakFitCurve::parameterItem(int i) const
{
    if (i >= childCount()) return nullptr;

    PeakFitParameter *it = dynamic_cast<PeakFitParameter*>(child(i));
    if (!it) return nullptr;

    if (!it->isAreaItem()) return it;
    return nullptr;
}

PeakFitParameter * PeakFitCurve::areaItem() const
{
    for (int i = 0; i < childCount(); ++i) {
        PeakFitParameter *it = dynamic_cast<PeakFitParameter*>(child(i));
        if (!it) continue;

        if (it->isAreaItem()) return it;
    }

    return nullptr;
}

QStringList PeakFitCurve::getParameterNames() const
{
    QStringList l;

    for (int i = 0; i < childCount(); ++i) {
        PeakFitParameter *it = dynamic_cast<PeakFitParameter*>(child(i));
        if (it) {
            if (!it->isAreaItem()) l.append(it->parameterName());
        }
    }

    return l;
}

QStringList PeakFitCurve::getVariableNames() const
{
    QStringList l;

    for (int i = 0; i < childCount(); ++i) {
        PeakFitParameter *it = dynamic_cast<PeakFitParameter*>(child(i));
        if (it) l.append(it->variableName());
    }

    return l;
}
