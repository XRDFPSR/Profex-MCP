/***************************************************************************
                          brukerrawimport.h  -  description
                             -------------------
    begin                : Thu June 05, 2013
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

#ifndef BRUKERRAWIMPORT_H
#define BRUKERRAWIMPORT_H

#include "genericimport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BrukerRawImport : public GenericImport
{
    Q_OBJECT
public:
    explicit BrukerRawImport(QObject *parent = 0);
    
    bool isSupported(const QByteArray &);
    int load(const QString &, QVector<Scan> &, bool minimal = false);
    QString uniqueId() {return "BRUKER_RAW";}

private:
    int readV1(const QByteArray &, QVector<Scan> &, const QString &);
    int readV2(const QByteArray &, QVector<Scan> &, const QString &);
    int readV3(const QByteArray &, QVector<Scan> &, const QString &);
    int readV4(const QByteArray &, QVector<Scan> &, const QString &, bool minimal = false);
    QVariantHash readV4ExtraRecord10(const QByteArray &, bool minimal = false);
    QVariantHash readV4ExtraRecord30(const QByteArray &, bool minimal = false);
    QVariantHash readV4RangeHeader(const QByteArray &, bool minimal = false);

};

#endif // BRUKERRAWIMPORT_H
