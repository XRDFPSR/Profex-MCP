/***************************************************************************
                          xyexport.h  -  description
                             -------------------
    begin                : Sun Oct 04, 2009
    copyright            : (C) 2009 by Nicola Doebelin
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

#ifndef XYEXPORT_H
#define XYEXPORT_H

#include <math.h>
#include "genericexport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT AsciiXyExport : public GenericExport
{
public:
    AsciiXyExport(QObject *parent = 0);

    inline QString filter()           {return "Ascii Free format (*.xy *.XY)";}
    inline QString extension()        {return "xy";}
    inline QString description()      {return "Generic ASCII text format";}

    int save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());

    // returns the content as a string
    QString getData(const Scan &scan, const QString &sep, bool fix = false);
    QString getData(const QVector<Scan> &scanHeap, const QString &sep, bool fix = false);

    inline bool hasMultiScanSupport() {return true;}

private:
    static const int precision = 6;
    const double zeroVal = std::pow(10.0, double(-precision));

    inline double fixZero(double);
    bool doFixBgmnZero;
};
#endif
