/***************************************************************************
                          strucimportbgmncheck.h  -  description
                             -------------------
    begin                : Mon Dec 16 10:34:00 CEST 2019
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


#ifndef STRUCIMPORTBGMNCHECK_H
#define STRUCIMPORTBGMNCHECK_H

#include "../libXrdIO/parser/bgmnsgdatparser.h"
#include "../libXrdIO/crystal/crystalstructure.h"
#include "../libXrdIO/structs.h"

#include <QObject>

class StrucImportBgmnCheck : public QObject
{
    Q_OBJECT
public:
    explicit StrucImportBgmnCheck(BgmnSgDatParser *, QObject *parent = nullptr);

    QSet<int> verifyAtoms(const QString &);
    inline QString const report() {return reportString;}

private:
    BgmnSgDatParser *sgParser;
    QString reportString;
    QMap<QString, global::WyckoffPosition> wyckoffList;

    void getWyckoffList(int, int);
    bool checkWyckoffSymmetry(double, double, double, const QStringList &);
    QString const checkRedundancy(double, double, double, const QStringList &);
    QStringList checkAtomSymmetry(const CrystalAtom &, int, int);
};

#endif // STRUCIMPORTBGMNCHECK_H
