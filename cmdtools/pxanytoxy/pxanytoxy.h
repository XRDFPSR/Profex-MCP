/***************************************************************************
                          pxanytoxy.h  -  description
                             -------------------
    begin                : Tue Nov 24 19:42:15 CEST 2022
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

#include <QStringList>

class PxAnyToXy {

public:
    PxAnyToXy();

    static QStringList getUniqueIds();
    static bool convert(const QString &file, const QString &iformat, const QString &oformat, int nscan = 0);
    static void listFormats();
};
