/***************************************************************************
                          pdcifimport.h  -  description
                             -------------------
    begin                : Tue Jul 19, 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#ifndef PDCIFIMPORT_H
#define PDCIFIMPORT_H

#include "genericimport.h"
#include <QObject>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT PdCifImport : public GenericImport
{
public:
    PdCifImport(QObject * = 0);

    bool isSupported(const QByteArray &);
    int load(const QString &, QVector<Scan> &, bool minimal = false);
    QString uniqueId() {return "CIF_PD";}

private:
    int parseLoop(const QStringList &, QStringList &);
    QVector<Scan> parseData(const QString &, const QStringList &, int, int, const QList<int> &, const QStringList &);
};

#endif // PDCIFIMPORT_H
