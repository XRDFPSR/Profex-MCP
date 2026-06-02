/***************************************************************************
                          rigakurawimport.h  -  description
                             -------------------
    begin                : Thu July 27, 2013
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

#ifndef RIGAKURAWIMPORT_H
#define RIGAKURAWIMPORT_H

#include "genericimport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT RigakuRawImport : public GenericImport
{
    Q_OBJECT
public:
    explicit RigakuRawImport(QObject *parent = 0);
    
    bool isSupported(const QByteArray &);
    int load(const QString &, QVector<Scan> &, bool minimal = false);
    QString uniqueId() {return "RIGAKU_RAW";}

private:
    bool loadNew(const QByteArray &, QVector<Scan> &, const QFileInfo &);
    bool loadOld(const QByteArray &, QVector<Scan> &, const QFileInfo &);
    
};

#endif // RIGAKURAWIMPORT_H
