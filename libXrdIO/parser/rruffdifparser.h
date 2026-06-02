/***************************************************************************
                          rruffdifparser.h  -  description
                             -------------------
    begin                : Tue Aug 25 19:37:00 CEST 2020
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

#ifndef RRUFFDIFPARSER_H
#define RRUFFDIFPARSER_H

#include <QString>
#include "../crystal/crystalstructure.h"
#include "../hkl.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT RruffDifParser
{
public:
    explicit RruffDifParser();
    explicit RruffDifParser(const QString &);

    void parseSourceString(const QString &, const QString &f);
    inline QString difString() const {return difContent.join("\n");}
    inline CrystalStructure getCrystalStructure() const {return crystalStructure;}

private:
    QString fileName;
    QStringList difContent;
    CrystalStructure crystalStructure;
    QString referenceCode;

    void parseContent();
    CrystalUnitCell parseUnitCell();
    QList<CrystalAtom> parseAtoms();
    QList<Hkl> parseHkl();
    QString parseDbRecord(const QString &);
};

#endif // RRUFFDIFPARSER_H
