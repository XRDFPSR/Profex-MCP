/***************************************************************************
                          chiqimport.h  -  description
                             -------------------
    begin                : Tue Jan 16, 2024
    copyright            : (C) 2024 by Nicola Doebelin
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

#ifndef CHIIMPORT_H
#define CHIIMPORT_H

#include "genericimport.h"

#if defined XRDIO
#define XRDIO_EXPORT Q_DECL_EXPORT
#else
#define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT ChiImport : public GenericImport
{
public:
    explicit ChiImport(QObject *parent = nullptr);

    bool isSupported(const QByteArray &);
    int load(const QString &, QVector<Scan> &, bool minimal = false);
    QString uniqueId() {return "CHI_Q";}
};

#endif // CHIIMPORT_H
