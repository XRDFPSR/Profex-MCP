/***************************************************************************
                          peakfitparameter.h  -  description
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

#ifndef PEAKFITPARAMETER_H
#define PEAKFITPARAMETER_H

#include "peakfititem.h"
#include <QUuid>

class PeakFitParameter : public PeakFitItem
{
public:
    PeakFitParameter(const QUuid &u, int type = Type);

    inline QString parameterName() const {return text(0);}
    inline QString variableName()  const {return text(1);}
    inline QString valueT()        const {return text(2);}
    inline double value()          const {return text(2).toDouble();}

    inline void setVariableName(const QString &s) {setText(1, s);}
    inline void setValue(double v)                {setText(2, QString::number(v, 'f', 6));}
    inline void setValue(const QString &s)        {setText(2, s);}

    inline bool isAreaItem() const  {return _isAreaItem;}
    inline void setAreaItem(bool b) {_isAreaItem = b;}

private:
    bool _isAreaItem;
};

#endif // PEAKFITPARAMETER_H
