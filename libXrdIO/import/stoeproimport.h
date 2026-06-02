/***************************************************************************
                          stoeproimport.h  -  description
                             -------------------
    begin                : Thu Jun 18, 2013
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

#ifndef STOEPROIMPORT_H
#define STOEPROIMPORT_H

#include "genericimport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT StoeProImport : public GenericImport
{
    Q_OBJECT
public:
    explicit StoeProImport(QObject *parent = 0);
    
    bool isSupported(const QByteArray &);
    int load(const QString &, QVector<Scan> &, bool minimal = false);
    QString uniqueId() {return "STOE_PRO";}
};

#endif // STOEPROIMPORT_H
