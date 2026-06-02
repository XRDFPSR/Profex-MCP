/***************************************************************************
                          rruffdifparser.cpp  -  description
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

#include "rruffdifparser.h"
#include "../bgmnfileio.h"
#include "../structs.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>

RruffDifParser::RruffDifParser()
{
}

RruffDifParser::RruffDifParser(const QString &f)
{
    parseSourceString(BgmnFileIO::readTextFile(f), f);
    referenceCode = parseDbRecord(f);
    crystalStructure.setAuxInfo("Reference", parseDbRecord(f));
}

void RruffDifParser::parseSourceString(const QString &s, const QString &f)
{
    Q_UNUSED(f);
    difContent = s.split(global::rxLineEnding);
    parseContent();
}

void RruffDifParser::parseContent()
{
    CrystalUnitCell ucell = parseUnitCell();
    QList<CrystalAtom> atoms = parseAtoms();
    // QList<Hkl> hkl = parseHkl();

    crystalStructure.setName(difContent.first().simplified());
    crystalStructure.setUnitCell(ucell);
    crystalStructure.setAtoms(atoms);
}

CrystalUnitCell RruffDifParser::parseUnitCell()
{
    CrystalUnitCell ucell;

    static QRegularExpression rxCell("^\\s*CELL PARAMETERS:\\s+"
                                     "(\\d+\\.?\\d*)\\s+"
                                     "(\\d+\\.?\\d*)\\s+"
                                     "(\\d+\\.?\\d*)\\s+"
                                     "(\\d+\\.?\\d*)\\s+"
                                     "(\\d+\\.?\\d*)\\s+"
                                     "(\\d+\\.?\\d*)\\s*$");
    static QRegularExpression rxHm("^\\s*(?:ALTERNATE SETTING FOR\\s+)?SPACE GROUP:\\s+(\\S+)\\s*$");

    int idxCell = difContent.indexOf(rxCell);
    int idxHM   = difContent.indexOf(rxHm);

    if ((idxCell < 0) || (idxHM < 0)) return ucell;

    QRegularExpressionMatch rmCell = rxCell.match(difContent.at(idxCell));
    QRegularExpressionMatch rmHM   = rxHm.match(difContent.at(idxHM));

    if (rmCell.hasMatch()) {
        ucell.setA(0.1*rmCell.captured(1).toDouble());
        ucell.setB(0.1*rmCell.captured(2).toDouble());
        ucell.setC(0.1*rmCell.captured(3).toDouble());
        ucell.setAlpha(rmCell.captured(4).toDouble());
        ucell.setBeta(rmCell.captured(5).toDouble());
        ucell.setGamma(rmCell.captured(6).toDouble());
    }

    if (rmHM.hasMatch()) {
        ucell.setSpaceGroupHMCif(rmHM.captured(1));
    }

    return ucell;
}

QList<CrystalAtom> RruffDifParser::parseAtoms()
{
    QList<CrystalAtom> atLst;

    static QRegularExpression rxAtmTitle("^\\s+ATOM\\s+"
                                         "X\\s+"
                                         "Y\\s+"
                                         "Z\\s+"
                                         "OCCUPANCY\\s+"
                                         "ISO\\(B\\)\\s*$");
    static QRegularExpression rxAtmLine("^\\s+([A-Za-z]+)\\s+"
                                        "(-?\\d+\\.?\\d*)\\s+"
                                        "(-?\\d+\\.?\\d*)\\s+"
                                        "(-?\\d+\\.?\\d*)\\s+"
                                        "(\\d+\\.?\\d*)\\s+"
                                        "(\\d+\\.?\\d*)\\s*$");

    int idxA = difContent.indexOf(rxAtmTitle);
    if (idxA < 0) return atLst;
    if (idxA >= difContent.size() - 1) return atLst;

    for (int i = idxA + 1; i < difContent.size(); ++i) {
        QRegularExpressionMatch rm = rxAtmLine.match(difContent.at(i));

        if (rm.hasMatch()) {
            CrystalAtom atm(rm.captured(1),
                            rm.captured(2).toDouble(),
                            rm.captured(3).toDouble(),
                            rm.captured(4).toDouble(),
                            rm.captured(5).toDouble());

            atm.setBiso(rm.captured(6).toDouble());
            atLst.append(atm);
        } else {
            break;
        }
    }

    return atLst;
}

QList<Hkl> RruffDifParser::parseHkl()
{
    QList<Hkl> hklLst;

    return hklLst;
}

QString RruffDifParser::parseDbRecord(const QString &f)
{
    QRegularExpression rx("__(R\\d+-\\d+)__");
    QRegularExpressionMatch rm = rx.match(f);

    if (rm.hasMatch()) return rm.captured(1);
    return QString();
}

