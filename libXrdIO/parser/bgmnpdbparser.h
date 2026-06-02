/***************************************************************************
                          bgmnpdbparser.cpp  -  description
                             -------------------
    begin                : Oct 01 10:15:07 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#ifndef BGMNPDBPARSER_H
#define BGMNPDBPARSER_H

#include <QString>
#include <QMultiMap>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

struct PdbAtom {
    QString name;
    double xc;
    double yc;
    double zc;
    double sof;
    double tds;
};

class XRDIO_EXPORT BgmnPdbParser
{
public:
    explicit BgmnPdbParser(const QString &);

    QStringList atomsNames() const;
    QMultiMap<ulong, QString> bondLengths();

private:
    double _a, _b, _c, _al, _be, _ga;
    QString _hm;
    QList<PdbAtom> _atomList;

    QList<PdbAtom> parseContent(const QStringList &);
};

#endif // BGMNPDBPARSER_H
