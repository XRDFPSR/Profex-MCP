/***************************************************************************
                          eflechparparser.h  -  description
                             -------------------
    begin                : Jan 19 12:15:00 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#ifndef EFLECHPARPARSER_H
#define EFLECHPARPARSER_H

#include <QtCore/QtGlobal>
#include <QVector>
#include <QStringList>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT EflechParParser
{
public:
    explicit EflechParParser();
    explicit EflechParParser(const QString &);

    bool load(const QString &);
    QVector<QVector<double> > getPeakData();

private:
    QStringList content;
};

#endif // EFLECHPARPARSER_H
