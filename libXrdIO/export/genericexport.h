/***************************************************************************
                          genericexport.h  -  description
                             -------------------
    begin                : Thu June 17, 2013
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

#ifndef GENERICEXPORT_H
#define GENERICEXPORT_H

#include <QtCore>
#include <QDebug>
#include "../scan.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT GenericExport : public QObject
{
    Q_OBJECT
public:
    explicit GenericExport(QObject *parent = 0);

    virtual QString filter() = 0;
    virtual QString extension() = 0;
    virtual QString description() = 0;
    virtual QString uniqueId() {return uId;}
    
    /*
     * flags can be used to pass various information (format-dependent) to the derived export filters.
     * Supported flags:
     *
     * flag                 Format          Used by filter
     * ----------------------------------------------------------------------------------------
     * fieldSeparator       QString         AsciiXyExport, AsciiHklExport
     * fixBgmnZero          bool            AsciiXyExport, AsciiHklExport
     * ----------------------------------------------------------------------------------------
     * */
    virtual int save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>()) = 0;
    virtual int save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>()) = 0;

    virtual bool hasMultiScanSupport() = 0;

protected:
    int writeFile(const QString &file, const QString &str);

    QString uId;

signals:
    
public slots:
    
};

#endif // GENERICEXPORT_H
