/***************************************************************************
                          icddxmlparser.h  -  description
                             -------------------
    begin                : Fri Mar 13 10:37:00 CEST 2015
    copyright            : (C) 2015 by Nicola Doebelin
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

#ifndef ICDDXMLPARSER_H
#define ICDDXMLPARSER_H

#include <QString>
#include <QDomDocument>
#include "../crystal/crystalstructure.h"
#include "../crystal/crystalstructurefactor.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT IcddXmlParser
{
public:
    explicit IcddXmlParser();
    explicit IcddXmlParser(const QString &);

    void parseSourceString(const QString &, const QString &f);
    inline QString icddString() const {return icddXML.toString();}
    inline CrystalStructure getCrystalStructure() const {return crystalStructure;}

private:
    QDomDocument icddXML;
    QString fileName;
    CrystalStructure crystalStructure;

    void parseXml(const QDomDocument &);
    bool hasElement(const QString &, const QDomDocument &);
    QString getStringByTagName(const QString &, const QDomDocument &);
    QString getPhaseName(const QDomDocument &);
    QList<CrystalStructureFactor> getStructureFactors(const QDomDocument &) const;
    QList<QStringList> getSymOps(const QDomDocument &, const CrystalUnitCell &) const;
};

#endif // ICDDXMLPARSER_H
