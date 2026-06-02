/***************************************************************************
                          spgrdatparser.h  -  description
                             -------------------
    begin                : Tue Jul 19 10:54:00 CEST 2016
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

#ifndef SPGRDATPARSER_H
#define SPGRDATPARSER_H

#include <QDomDocument>
#include <QStringList>
#include <QList>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT SpgrDatParser
{
public:
    SpgrDatParser();

    QList<QStringList> getSymOps(const QString &, int);

private:
    QDomDocument spaceGroupDat;

    QDomNode settingNodes(const QString &, int);
    QStringList positions(const QDomNode &);
    QStringList symOps(const QDomNode &);
    QList<QStringList> splitSymOps(const QStringList &);
};

#endif // SPGRDATPARSER_H
