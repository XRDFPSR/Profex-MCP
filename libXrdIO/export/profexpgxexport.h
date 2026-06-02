/***************************************************************************
                          profexpgxexport.h  -  description
                             -------------------
    begin                : Wed Apr 17 22:16:07 CEST 2024
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

#ifndef PROFEXPGXEXPORT_H
#define PROFEXPGXEXPORT_H

#include "genericexport.h"

#if defined XRDIO
#define XRDIO_EXPORT Q_DECL_EXPORT
#else
#define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT ProfexPgxExport : public GenericExport
{
public:
    explicit ProfexPgxExport(QObject *parent = nullptr);

    inline QString filter()           {return "Profex Graph Exchange (*.pgx *.PGX)";}
    inline QString extension()        {return "pgx";}
    inline QString description()      {return "Space-separated text file for graph exchange";}

    int save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());
    int save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags = QMap<QString, QVariant>());

    inline bool hasMultiScanSupport() {return true;}

private:
    static const int precision = 6;

    QString getData(const QVector<Scan> &scanHeap);
    QString getHkl(const QVector<Scan> &scanHeap);

    QString getDataSep(const QVector<Scan> &scanHeap);
    QString getHklSep(const QVector<Scan> &scanHeap);
};

#endif // PROFEXPGXEXPORT_H
