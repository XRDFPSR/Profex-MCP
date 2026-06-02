/***************************************************************************
                          genericcurve.cpp  -  description
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

#include "genericcurve.h"
#include <QDebug>

GenericCurve::GenericCurve()
{
    _uid = QUuid::createUuid();
    _area = -1.0;
    _displayName = QString();
}

GenericCurve::~GenericCurve()
{}

double GenericCurve::fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x)
{
    Q_UNUSED(c)
    Q_UNUSED(x)
    return 0.0;
}

void GenericCurve::compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy)
{
    Q_UNUSED(c)
    Q_UNUSED(x)
    Q_UNUSED(y)
    Q_UNUSED(sy)
}
