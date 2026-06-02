/***************************************************************************
                          textureplusexport.h  -  description
                             -------------------
    begin                : Thu June 17, 2013
    copyright            : (C) 2013 by Nicola Doebelin
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

#ifndef TEXTUREPLUSEXPORT_H
#define TEXTUREPLUSEXPORT_H

#include "genericexport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT TexturePlusExport : public GenericExport
{
    Q_OBJECT
public:
    explicit TexturePlusExport(QObject *parent = 0);
    
    QString filter() {return "TexturePlus scan (*.xyp *.XYP)";}
    QString extension() {return "xyp";}
    QString description() {return "Theta-2theta and omega (theta) scan data format for TexturePlus by Mark D. Vaudin";}

    int save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int save(const QString &, const QVector<Scan> &, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());

    bool hasMultiScanSupport() {return false;}
    
};

#endif // TEXTUREPLUSEXPORT_H
