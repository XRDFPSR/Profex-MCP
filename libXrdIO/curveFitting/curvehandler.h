/***************************************************************************
                          curvehandler.h  -  description
                             -------------------
    begin                : Thu Jan 16 19:20:03 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#ifndef CURVEHANDLER_H
#define CURVEHANDLER_H

#include "../libXrdIO/curveFitting/genericcurve.h"
#include <memory>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT CurveHandler
{
public:
    CurveHandler();

    static std::shared_ptr<GenericCurve> createCurve(int);
    static QString description(int);
    static QString equationText(int);
    static QStringList parameterText(int);
};

#endif // CURVEHANDLER_H
