/***************************************************************************
                          peakfitcurve.h  -  description
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

#ifndef PEAKFITCURVE_H
#define PEAKFITCURVE_H

#include <QTreeWidgetItem>
#include <QUuid>
#include "peakfititem.h"
#include "peakfitparameter.h"
#include "../libXrdIO/curveFitting/genericcurve.h"

class PeakFitCurve : public PeakFitItem
{
public:
    PeakFitCurve(const QUuid &u, int type = Type);

    inline void setCurveType(int t) {_curveType = t;}

    inline int parameterCount() const {return childCount();}
    inline int curveType()      const {return _curveType;}

    PeakFitParameter * parameterItem(int) const;
    PeakFitParameter * areaItem() const;

    inline void setCurve(std::shared_ptr<GenericCurve> c) {_curve = c;}
    inline std::shared_ptr<GenericCurve> getCurve() const {return _curve;}
    inline std::shared_ptr<GenericCurve> getCurve()       {return _curve;}

    QStringList getParameterNames() const;
    QStringList getVariableNames() const;

private:
    int _curveType;
    std::shared_ptr<GenericCurve> _curve;
};

#endif // PEAKFITCURVE_H
