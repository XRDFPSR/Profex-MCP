/***************************************************************************
                          peakfitvariable.cpp  -  description
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

#include "peakfitvariable.h"

PeakFitVariable::PeakFitVariable(const QUuid &u, int type)
    : PeakFitItem(u, type)
{
    setText(1, QString());
    setText(2, QString());
    setText(3, QString());

    setTextAlignment(1, Qt::AlignRight);
    setTextAlignment(2, Qt::AlignRight);
    setTextAlignment(3, Qt::AlignRight);
}

void PeakFitVariable::setValue(double v, const QString &limLo, const QString &limHi)
{
    setValue(QString::number(v, 'f', 6), limLo, limHi);
}

void PeakFitVariable::setValue(const QString &v, const QString &limLo, const QString &limHi)
{
    setText(1, v);
    setText(2, limLo);
    setText(3, limHi);
}

void PeakFitVariable::setValue(double v)
{
    setText(1, QString::number(v, 'f', 6));
}
