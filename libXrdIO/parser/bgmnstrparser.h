/***************************************************************************
                          strparser.h  -  description
                             -------------------
    begin                : Mon May 20 14:00:00 CEST 2013
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

#ifndef STRPARSER_H
#define STRPARSER_H

#include <QString>
#include <QStringList>
#include <QVariant>
#include "../crystal/crystalstructure.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BgmnStrParser
{
public:
    explicit BgmnStrParser();
    explicit BgmnStrParser(const QString &);
    explicit BgmnStrParser(const QString &, const QString &);

    bool loadFile(const QString &);
    bool writeToFile(const QString &s = QString());
    bool saveFile();
    void setContent(const QString &);
    inline QString getContent() const {return content.join("\n");}
    QString getQuantGoal() const;
    QMultiMap<int, CrystalAtom> parseAtoms() const;
    QString getHermannMauguin() const;
    int getSpacegroupNo() const;
    int getSetting() const;
    void setRefinementState(const QString &, int);
    QString getPhaseName() const;
    QStringList getElements() const;
    CrystalStructure getCrystalStructure() const;

    static QString setSubstitution(const QString &l, bool &ok);
    static QString revertSubstutition(const QString &l, bool &ok);
    static QString allCoordinatesRefined(const QString &s, double e, bool &ok);
    static QString allCoordinatesFixed(const QString &s, bool &ok);
    static QString allTdsRefined(const QString &s, double e, bool &ok);
    static QString allTdsFixed(const QString &s, bool &ok);

private:
    QString fileName;
    QStringList content;
    CrystalUnitCell parseUnitCell() const;
};

#endif // STRPARSER_H
