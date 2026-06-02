/***************************************************************************
                          imageeffects.h  -  description
                             -------------------
    begin                : Sat Jan 06 13:00:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#ifndef IMAGEEFFECTS_H
#define IMAGEEFFECTS_H

#include <QImage>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT ImageEffects
{
public:
    ImageEffects();

    static void contourLinesBlackOnColor(QImage *, int);
    static void contourLinesBlackOnWhite(QImage *, int);
    static void contourLinesColor(QImage *, int);

private:
    static QImage renderContourLines(const QImage *, int);
};

#endif // IMAGEEFFECTS_H
