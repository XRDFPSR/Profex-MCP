/***************************************************************************
                          lutgenerator.h  -  description
                             -------------------
    begin                : Fri Sep 26 10:24:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#ifndef LUTGENERATOR_H
#define LUTGENERATOR_H

#include <QList>
#include <QMap>
#include <QStringList>
#include <QRgb>
#include "lutstructs.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT LutGenerator
{
public:
    LutGenerator(int, bool); // int size, bool transparency

    QMap<QString, colorMaps::Lut> getLuts(bool smth = true, bool cntr = true, bool cntl = true, bool ccol = true, bool colc = true);

private:
    QMap<QString, colorMaps::Lut> lutList;
    int size;
    bool transparent;

    colorMaps::Lut lutGrayScale(int, int, int);
    colorMaps::Lut lutBlueRed();
    colorMaps::Lut lutBrightGlow();
    colorMaps::Lut lutDarkGlow();
    colorMaps::Lut lutFalseColor(int, int, int);
};

#endif // LUTGENERATOR_H
