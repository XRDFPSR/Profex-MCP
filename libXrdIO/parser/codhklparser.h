/***************************************************************************
                          codhklparser.h  -  description
                             -------------------
    begin                : Thu Jan 07 19:37:00 CEST 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#ifndef CODHKLPARSER_H
#define CODHKLPARSER_H

#include "../hkl.h"
#include <QVector>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT CodHklParser
{
public:
    CodHklParser(const QString &);
    CodHklParser(const QByteArray &, const QString &);

    inline QVector<Hkl> getHklList() const {return hklList;}

private:
    QString codRecord;
    QStringList hklContent;
    double a, b, c, al, be, ga;
    QVector<Hkl> hklList;

    bool parseUnitCell();
    void parseFhkl();
};

#endif // CODHKLPARSER_H
