/***************************************************************************
                          philipsudfexport.h  -  description
                             -------------------
    begin                : May 21, 2013
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

#ifndef PHILIPSUDFEXPORT_H
#define PHILIPSUDFEXPORT_H

#include "genericexport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT PhilipsUdfExport : public GenericExport
{
    Q_OBJECT
public:
    explicit PhilipsUdfExport(QObject *parent = 0);

    QString filter() {return "Philips UDF format (*.udf *.UDF)";}
    QString extension() {return "udf";}
    QString description() {return "Philips UDF file format";}

    int save(const QString &, const Scan &, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int save(const QString &, const QVector<Scan> &, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());

    bool hasMultiScanSupport() {return false;}

};

#endif // PHILIPSUDFEXPORT_H
