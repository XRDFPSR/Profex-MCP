/***************************************************************************
                          codhklparser.cpp  -  description
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

#include "codhklparser.h"
#include "../bgmnfileio.h"
#include "../functions.h"
#include "../structs.h"
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QString>
#include <QDebug>

CodHklParser::CodHklParser(const QString &f)
{
    hklList.clear();
    hklContent = BgmnFileIO::readTextFileLines(f);

    QFileInfo fi(f);
    codRecord = fi.completeBaseName();

    if (parseUnitCell()) {
        parseFhkl();
    }
}

CodHklParser::CodHklParser(const QByteArray &ct, const QString &rcd)
{
    hklList.clear();
    hklContent = QString(ct).split(global::rxLineEnding);
    codRecord = rcd;

    if (parseUnitCell()) {
        parseFhkl();
    }
}

bool CodHklParser::parseUnitCell()
{
    QRegularExpression rxa("_cell_length_a\\s+(\\d+\\.?\\d*)(?:\\(\\d+\\))?");
    QRegularExpression rxb("_cell_length_b\\s+(\\d+\\.?\\d*)(?:\\(\\d+\\))?");
    QRegularExpression rxc("_cell_length_c\\s+(\\d+\\.?\\d*)(?:\\(\\d+\\))?");
    QRegularExpression rxal("_cell_angle_alpha\\s+(\\d+\\.?\\d*)(?:\\(\\d+\\))?");
    QRegularExpression rxbe("_cell_angle_beta\\s+(\\d+\\.?\\d*)(?:\\(\\d+\\))?");
    QRegularExpression rxga("_cell_angle_gamma\\s+(\\d+\\.?\\d*)(?:\\(\\d+\\))?");

    QRegularExpressionMatch rm;

    int idxA = hklContent.indexOf(rxa);
    int idxB = hklContent.indexOf(rxb);
    int idxC = hklContent.indexOf(rxc);
    int idxAl = hklContent.indexOf(rxal);
    int idxBe = hklContent.indexOf(rxbe);
    int idxGa = hklContent.indexOf(rxga);

    int succ = 0;

    if (idxA >= 0) {
        rm = rxa.match(hklContent.at(idxA));
        a = rm.captured(1).toDouble();
        succ++;
    } else {
        qDebug() << QString("CodHklParser::parseUnitCell(): Value of a axis not found. Exiting.");
    }

    if (idxB >= 0) {
        rm = rxb.match(hklContent.at(idxB));
        b = rm.captured(1).toDouble();
        succ++;
    } else {
        qDebug() << QString("CodHklParser::parseUnitCell(): Value of b axis not found. Exiting.");
    }

    if (idxC >= 0) {
        rm = rxc.match(hklContent.at(idxC));
        c = rm.captured(1).toDouble();
        succ++;
    } else {
        qDebug() << QString("CodHklParser::parseUnitCell(): Value of c axis not found. Exiting.");
    }

    if (idxAl >= 0) {
        rm = rxal.match(hklContent.at(idxAl));
        al = rm.captured(1).toDouble();
        succ++;
    } else {
        qDebug() << QString("CodHklParser::parseUnitCell(): Value of alpha angle not found. Exiting.");
    }

    if (idxBe >= 0) {
        rm = rxbe.match(hklContent.at(idxBe));
        be = rm.captured(1).toDouble();
        succ++;
    } else {
        qDebug() << QString("CodHklParser::parseUnitCell(): Value of beta angle not found. Exiting.");
    }

    if (idxGa >= 0) {
        rm = rxga.match(hklContent.at(idxGa));
        ga = rm.captured(1).toDouble();
        succ++;
    } else {
        qDebug() << QString("CodHklParser::parseUnitCell(): Value of gamma angle not found. Exiting.");
    }

    return succ == 6;
}

void CodHklParser::parseFhkl()
{
    int lineH = hklContent.indexOf("_refln_index_h");
    int lineK = hklContent.indexOf("_refln_index_k");
    int lineL = hklContent.indexOf("_refln_index_l");
    int lineF2c = hklContent.indexOf("_refln_F_squared_calc");
    int lineF2m = hklContent.indexOf("_refln_F_squared_meas");

    if ((lineH < 0) || (lineK < 0) || (lineL < 0)) {
        qDebug() << QString("CodHklParser::parseFhkl(): Could not find block containing hkl indices. Exiting.");
    }

    int zmin = qMin(lineH, qMin(lineK, lineL));
    int zmax = qMax(lineH, qMax(lineK, lineL));

    int lineF2;

    if (lineF2c >= 0) {
        lineF2 = lineF2c;
        zmin = qMin(zmin, lineF2);
        zmax = qMax(zmax, lineF2);
    } else if (lineF2m >= 0) {
        lineF2 = lineF2m;
        zmin = qMin(zmin, lineF2);
        zmax = qMax(zmax, lineF2);
    } else  {
        lineF2 = -1;
    }

    int idxH = lineH - zmin;
    int idxK = lineK - zmin;
    int idxL = lineL - zmin;
    int idxF2 = lineF2 - zmin;

    for (int i = zmin; i < hklContent.size(); ++i) {
        QString ln = hklContent.at(i).simplified();
        if (ln.left(1) == "_") continue;
        if (ln.isEmpty()) break;

        QStringList lst = ln.split(QRegularExpression("\\s+"));
        int h = lst.size() >= idxH ? lst.at(idxH).toInt() : 0;
        int k = lst.size() >= idxK ? lst.at(idxK).toInt() : 0;
        int l = lst.size() >= idxL ? lst.at(idxL).toInt() : 0;

        double f = 1.0;
        if ((idxF2 >= 0) && (lst.size() >= idxF2)) f = lst.at(idxF2).toDouble();

        double d = 0.1 * global::Functions::dSpacing(a, b, c, al, be, ga, h, k, l);
        hklList.append(Hkl(d, QString("%1%2%3").arg(h).arg(k).arg(l), 0, codRecord, QColor(), f));
    }
}
