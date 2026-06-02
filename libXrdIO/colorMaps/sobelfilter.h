/***************************************************************************
                          sobelfilter.h  -  description
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

#ifndef SOBELFILTER_H
#define SOBELFILTER_H

#include <QObject>
#include <QThread>
#include <QImage>
#include <QMutex>
#include "math.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT SobelFilter : public QThread
{
    Q_OBJECT

public:
    explicit SobelFilter(QMutex *);

    void setRange(int, int, const QImage *, QImage *);

private:
    QMutex *mutex;
    int line_start;
    int line_end;
    const QImage *sourceImage;
    QImage *targetImage;

    void generateContourLines();
    static int pixelValue(int, int, const QRgb *);

    void run();
};

#endif // SOBELFILTER_H
