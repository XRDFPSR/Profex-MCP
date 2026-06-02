/***************************************************************************
                          rigakurasimport.h  -  description
                             -------------------
    begin                : Mon Dec 03, 2018
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

#ifndef RIGAKURASIMPORT_H
#define RIGAKURASIMPORT_H

#include "genericimport.h"
#include "scan.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class RigakuRasImport : public GenericImport
{
public:
    RigakuRasImport();

    bool isSupported(const QByteArray &);
    int load(const QString &, QVector<Scan> &, bool minimal = false);
    QString uniqueId() {return "RIGAKU_RAS";}

private:
    QList<QStringList> getHeaderBlocks(const QStringList &);
    QList<QStringList> getDataBlocks(const QStringList &);
    QString getComment(const QStringList &);
    double getWaveLength(const QStringList &, int);
    void parseDataBlocks(Scan &, const QStringList &);
};

#endif // RIGAKURASIMPORT_H
