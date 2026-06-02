/***************************************************************************
                          strucimportbgmncheck.cpp  -  description
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


#include "strucimportbgmncheck.h"
#include "strucimportbgmnmessages.h"
#include "../libXrdIO/parser/bgmnstrparser.h"
#include "../libXrdIO/functions.h"

#include <QJSEngine>
#include <math.h>

StrucImportBgmnCheck::StrucImportBgmnCheck(BgmnSgDatParser *sgp, QObject *parent) :
    QObject(parent)
{
    sgParser = sgp;
}

QSet<int> StrucImportBgmnCheck::verifyAtoms(const QString &str)
{
    BgmnStrParser sparser;
    sparser.setContent(str);

    QSet<int> errorLines;
    QMultiMap<int, CrystalAtom> atoms = sparser.parseAtoms();
    int sgno = sparser.getSpacegroupNo();
    int sett = sparser.getSetting();

    getWyckoffList(sgno, sett);

    if ((atoms.size() <= 0) || (sgno < 0) || (sett < 0)) {
        reportString += StrucImportBgmnMessages::showError(QString("%1 atoms found in spacegroup no %2 setting no %3").arg(atoms.size()).arg(sgno).arg(sett));
    }

    QMultiMapIterator<int, CrystalAtom> it(atoms);
    while (it.hasNext()) {
        it.next();

        QStringList msg = checkAtomSymmetry(it.value(), sgno, sett);

        if (!msg.first().isEmpty()) {
            errorLines.insert(it.key());
            reportString += StrucImportBgmnMessages::showError(QString("Line %1 (%2): %3").arg(it.key()+1).arg(it.value().element()).arg(msg.first()));
        } else {
            if (!msg.at(1).isEmpty()) {
                reportString += StrucImportBgmnMessages::showComment(QString("Line %1 (%2): %3").arg(it.key()+1).arg(it.value().element()).arg(msg.at(1)));
            }
        }

        if (qFuzzyIsNull(it.value().biso())) {
            reportString += StrucImportBgmnMessages::showWarning(QString("Line %1 (%2): TDS is 0.0. Fix manually.").arg(it.key()+1).arg(it.value().element()));
        }
    }

    return errorLines;
}

/*
 * the returned string list always contains at least 2 null strings.
 * position 0: Contains an error message if the atoms symmetry is wrong.
 * position 1: Contains additional information, such as obsolete coordinates.
 */
QStringList StrucImportBgmnCheck::checkAtomSymmetry(const CrystalAtom &atom, int inttables, int setting)
{
    QStringList message;
    message << QString() << QString();

    if (atom.wyckoff().trimmed().isEmpty()) {
        qDebug() << QString("StrucImportDialog::checkAtomSymmetry(): Atom %1 has no wyckoff symbol. Skipping.").arg(atom.name());
        message[0] = QString("No Wyckoff symbol found");
        return message;
    }

    if ((inttables < 1) || (inttables > 230) || (setting < 1)) {
        qDebug() << QString("StrucImportDialog::checkAtomSymmetry(): Invalid value: SpacegroupNo=%1 Setting=%2").arg(inttables).arg(setting);
        message[0] = QString("Unknown SpacegroupNo or Setting");
        return message;
    }

    QVector<QStringList> wyckoffSymm = wyckoffList[atom.wyckoff()].operators;

    if (!wyckoffSymm.size()) {
        QString err = QString("No Wyckoff symmetries found for SpacegroupNo %1 Setting %2").arg(inttables).arg(setting);
        qDebug() << QString("StrucImportDialog::checkAtomSymmetry(): ") << err;
        message[0] = err;
        return message;
    }

    bool b = checkWyckoffSymmetry(atom.x(), atom.y(), atom.z(), wyckoffSymm.first());

    if (!b) {
        QString strx = atom.x() < 0.0 ? wyckoffSymm.first()[0] : QString("%1").arg(atom.x(), 0, 'f', 6);
        QString stry = atom.y() < 0.0 ? wyckoffSymm.first()[1] : QString("%1").arg(atom.y(), 0, 'f', 6);
        QString strz = atom.z() < 0.0 ? wyckoffSymm.first()[2] : QString("%1").arg(atom.z(), 0, 'f', 6);

        message[0] = QString("Coordinates %1 %2 %3 do not match first Wyckoff symmetry for %4 (%5).")
            .arg(strx)
            .arg(stry)
            .arg(strz)
            .arg(atom.wyckoff())
            .arg(wyckoffSymm.first().join(" "));
    }

    QString redund = checkRedundancy(atom.x(), atom.y(), atom.z(), wyckoffSymm.first());

    if (!redund.isEmpty()) {
        message[1] = QString("The following coordinates are redundant, consider removing: %1").arg(redund);
    }

    return message;
}

bool StrucImportBgmnCheck::checkWyckoffSymmetry(double x, double y, double z, const QStringList &stdSymOp)
{
    QJSEngine scriptEngine;
    int errors = 0;

    if (x > -1.0) scriptEngine.globalObject().setProperty("x", x);
    if (y > -1.0) scriptEngine.globalObject().setProperty("y", y);
    if (z > -1.0) scriptEngine.globalObject().setProperty("z", z);

    double wx = scriptEngine.evaluate(stdSymOp[0]).toNumber();
    double wy = scriptEngine.evaluate(stdSymOp[1]).toNumber();
    double wz = scriptEngine.evaluate(stdSymOp[2]).toNumber();

    if (x > -1.0) errors += global::Functions::fuzzyCompareFractions(wx, x) ? 0 : 1;
    if (y > -1.0) errors += global::Functions::fuzzyCompareFractions(wy, y) ? 0 : 1;
    if (z > -1.0) errors += global::Functions::fuzzyCompareFractions(wz, z) ? 0 : 1;

    return errors == 0 ? true : false;
}

QString const StrucImportBgmnCheck::checkRedundancy(double ax, double ay, double az, const QStringList &wyck)
{
    if (wyck.size() < 3) return QString();

    QStringList str;

    if (!wyck.at(0).contains("x") && (ax > -1.0)) str << "x";
    if (!wyck.at(1).contains("y") && (ay > -1.0)) str << "y";
    if (!wyck.at(2).contains("z") && (az > -1.0)) str << "z";

    return str.join(" ");
}

void StrucImportBgmnCheck::getWyckoffList(int itnum, int itset)
{
    QList<global::WyckoffPosition> lst = sgParser->getAllWyckoff(itnum, itset);
    wyckoffList.clear();

    for (int i = 0; i < lst.size(); ++i) {
        wyckoffList[lst.at(i).name] = lst.at(i);
    }
}
