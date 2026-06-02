/***************************************************************************
                          philipsudfimport.h  -  description
                             -------------------
    begin                : Thu May 21, 2013
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

#ifndef PHILIPSUDFIMPORT_H
#define PHILIPSUDFIMPORT_H

#include "genericimport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT PhilipsUdfImport : public GenericImport
{
    Q_OBJECT
public:
    explicit PhilipsUdfImport(QObject *parent = 0);

    bool isSupported(const QByteArray &);
    int load(const QString &, QVector<Scan> &, bool minimal = false);
    QString uniqueId() {return "PHILIPS_UDF";}
};

#endif // PHILIPSUDFIMPORT_H
