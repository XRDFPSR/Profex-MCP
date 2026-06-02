/***************************************************************************
                          emapdatahandler.h  -  description
                             -------------------
    begin                : Tue Jul 12 17:49:22 CEST 2022
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

#ifndef EMAPDATAHANDLER_H
#define EMAPDATAHANDLER_H

#include "emapstructs.h"
#include <QMatrix4x4>
#include <QImage>

class EMapDataHandler
{
public:
    EMapDataHandler();

    static QMatrix4x4 createF2CMatrix(const UnitCell &, projection_t);
    static QString mapToCsvFractional(const EDataMap &, bool *ok = nullptr);
    static QString mapToCsvCartesian(const EDataMap &, const UnitCell &, bool *ok = nullptr);
    static QString layerToCsvFractional(const EDataMap &, projection_t, int, bool *ok = nullptr);
    static QString layerToCsvCartesian(const EDataMap &, const UnitCell &, projection_t, int, bool *ok = nullptr);
    static bool checkEMapSize(const EDataMap &);
    static bool checkEMapSize(const EDataMap *);
    static int numberOfImages(const EDataMap *, projection_t);
};

#endif // EMAPDATAHANDLER_H
