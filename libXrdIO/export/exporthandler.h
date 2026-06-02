/***************************************************************************
                          exporthandler.h  -  description
                             -------------------
    begin                : Thu June 18, 2013
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

#ifndef EXPORTHANDLER_H
#define EXPORTHANDLER_H

#include "genericexport.h"
#include "../scan.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT ExportHandler
{
public:
    // set iactive to true if exporters created by this instance are
    // allowed to show interactive dialogs. If set to false, exporters
    // are not allowed to ask for user input.
    explicit ExportHandler();
    ~ExportHandler();

    int save(const QString &id, const QString &f, const Scan &scan, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int save(const QString &id, const QString &f, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());

    GenericExport * exporter(const QString &id, bool * ok = 0);
    GenericExport * isSupported(const QString &uid);

    QMap<QString, QString> uidsByFilter();
    bool hasMultiScanSupport(const QString &);

    QString extensionByFilter(const QString &);
    QString extensionByUid(const QString &);

    QStringList uniqueIds();
    QStringList descriptions();

private:
    QMap<QString, GenericExport *> formats;
    QString file;
};

#endif // EXPORTHANDLER_H
