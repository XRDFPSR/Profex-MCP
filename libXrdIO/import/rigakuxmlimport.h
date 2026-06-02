/***************************************************************************
                          rigakuxmlimport.h  -  description
                             -------------------
    begin                : Sun Aug 25 19:00:00 CEST 2013
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



#ifndef RIGAKUXMLIMPORT_H
#define RIGAKUXMLIMPORT_H

#include <QDomNode>
#include <QVariantHash>
#include "genericimport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT RigakuXmlImport : public GenericImport
{
public:
    RigakuXmlImport(QObject * = 0);

    bool isSupported(const QByteArray &);
    int load(const QString &, QVector<Scan> &, bool minimal = false);
    QString uniqueId() {return "RIGAKU_XML";}

private:
    QList<QStringList> getHeaderBlocks(const QStringList &);
    QList<QStringList> getDataBlocks(const QStringList &);
    QString getComment(const QStringList &);
    double getWaveLength(const QStringList &, int);
    void parseDataBlocks(Scan &, const QStringList &);
};

#endif // RIGAKUXMLIMPORT_H
