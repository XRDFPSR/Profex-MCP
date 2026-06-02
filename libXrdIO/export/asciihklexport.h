/***************************************************************************
                          asciihklexport.h  -  description
                             -------------------
    begin                : Sat Jan 16, 2016
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

#ifndef ASCIIHKLEXPORT_H
#define ASCIIHKLEXPORT_H

#include "genericexport.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT AsciiHklExport : public GenericExport
{
public:
    AsciiHklExport(QObject *parent = 0);

    QString filter() {return "Ascii HKL List (*.hkl *.HKL)";}
    QString extension() {return "hkl";}
    QString description() {return "Text file format for HKL data";}

    int save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());

    QString getData(const Scan &scan, const QString &sep, bool header, int phNumber);
    QString getData(const QVector<Scan> &scanHeap, const QString &sep);

    bool hasMultiScanSupport() {return true;}
};

#endif // ASCIIHKLEXPORT_H
