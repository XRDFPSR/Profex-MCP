/***************************************************************************
                          bgmnpdbparser.h  -  description
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

#include "bgmnpdbparser.h"
#include "bgmnfileio.h"
#include <QStringList>
#include <QVector3D>
#include <QDebug>

BgmnPdbParser::BgmnPdbParser(const QString &f)
{
    _atomList = parseContent(BgmnFileIO::readTextFileLines(f));
}

QList<PdbAtom> BgmnPdbParser::parseContent(const QStringList &l)
{
    QRegularExpression rxCell("^CRYST\\d\\s+(\\d+\\.?\\d*)\\s+(\\d+\\.?\\d*)\\s+(\\d+\\.?\\d*)\\s+(\\d+\\.?\\d*)\\s+(\\d+\\.?\\d*)\\s+(\\d+\\.?\\d*)\\s([^\\s]+)");
    QRegularExpression rxAtom("^ATOM\\s+\\d+\\s+([A-Z0-9]+)\\s+\\d+\\s+\\d+\\s+(-?\\d+\\.?\\d*)\\s+(-?\\d+\\.?\\d*)\\s+(-?\\d+\\.?\\d*)\\s+(\\d+\\.\\d*)\\s+(\\d+\\.\\d*)");

    QList<PdbAtom> al;
    int c = l.indexOf(rxCell);
    int a = l.indexOf(rxAtom);

    if (c >= 0) {
        QRegularExpressionMatch rm = rxCell.match(l.at(c));
        _a = rm.captured(1).toDouble();
        _b = rm.captured(2).toDouble();
        _c = rm.captured(3).toDouble();
        _al = rm.captured(4).toDouble();
        _be = rm.captured(5).toDouble();
        _ga = rm.captured(6).toDouble();
        _hm = rm.captured(7);
    }

    while (a >= 0) {
        PdbAtom atm;

        QRegularExpressionMatch rm = rxAtom.match(l.at(a));
        atm.name = rm.captured(1);
        atm.xc = rm.captured(2).toDouble();
        atm.yc = rm.captured(3).toDouble();
        atm.zc = rm.captured(4).toDouble();
        atm.sof = rm.captured(5).toDouble();
        atm.tds = rm.captured(6).toDouble();

        al.append(atm);
        a = l.indexOf(rxAtom, a+1);
    }

    return al;
}

QStringList BgmnPdbParser::atomsNames() const
{
    QStringList l;

    for (int i = 0; i < _atomList.size(); ++i) {
        l.append(_atomList.at(i).name);
    }

    return l;
}

QMultiMap<ulong, QString> BgmnPdbParser::bondLengths()
{
    QMultiMap<ulong, QString> out;

    for (int a = 0; a < _atomList.size() - 1; ++a) {
        PdbAtom at_a = _atomList.at(a);

        for (int b = a + 1; b < _atomList.size(); ++b) {
            PdbAtom at_b = _atomList.at(b);

            QVector3D va(at_a.xc, at_a.yc, at_a.zc);
            QVector3D vb(at_b.xc, at_b.yc, at_b.zc);
            ulong dst = ulong(10000 * va.distanceToPoint(vb));

            if (dst > 0) {
                out.insert(dst, QString("%1;%2").arg(at_a.name).arg(at_b.name));
            }
        }
    }

    return out;
}
