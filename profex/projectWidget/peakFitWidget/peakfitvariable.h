/***************************************************************************
                          peakfitvariable.h  -  description
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

#ifndef PEAKFITVARIABLE_H
#define PEAKFITVARIABLE_H

#include <QTreeWidgetItem>
#include <QUuid>
#include "peakfititem.h"

class PeakFitVariable : public PeakFitItem
{
public:
    PeakFitVariable(const QUuid &, int type = Type);

    inline void setChecked(bool b) {setCheckState(0, b ? Qt::Checked : Qt::Unchecked);}

    void setValue(double v, const QString &limLo, const QString &limHi);
    void setValue(const QString &v, const QString &limLo, const QString &limHi);
    void setValue(double v);

    inline QString variableName() const {return text(0);}
    inline QString valueT()       const {return text(1);}
    inline QString lowerLimitT()  const {return text(2);}
    inline QString upperLimitT()  const {return text(3);}
    inline QString isCheckedT()   const {return checkState(0) == Qt::Checked ? "1" : "0";}

    inline double value()         const {return text(1).toDouble();}
    inline double isChecked()     const {return checkState(0) == Qt::Checked;}

private:
};

#endif // PEAKFITVARIABLE_H
