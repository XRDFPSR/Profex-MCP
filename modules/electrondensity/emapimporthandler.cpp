/***************************************************************************
                          importhandler.cpp  -  description
                             -------------------
    begin                : Mon Nov 12 15:54:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#include <QStringList>
#include <QRegularExpression>
#include <QFile>
#include <QTextStream>
#include <QVector>
#include <QJSEngine>
#include <QDebug>
#include "emapimporthandler.h"
#include "math.h"

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

EmapImportHandler::EmapImportHandler()
{
    QList<float> values;
    // all radii in [Å], taken from http://www.crystalmaker.com/support/tutorials/atomic-radii/
    // Atomic Radius << Ionic Radius << Covalent Radius << Van-der-Waals Radius << "Crystal" Radius
    values << 0.53 << 0.25 << 0.37 << 1.20 << 0.10; atomicRadii["h"]  = values; values.clear();
    values << 0.31 << 0.31 << 0.32 << 1.40 << 0.00; atomicRadii["he"] = values; values.clear();
    values << 1.67 << 1.45 << 1.34 << 1.82 << 0.90; atomicRadii["li"] = values; values.clear();
    values << 1.12 << 1.05 << 0.90 << 0.00 << 0.41; atomicRadii["be"] = values; values.clear();
    values << 0.87 << 0.85 << 0.82 << 0.00 << 0.25; atomicRadii["b"]  = values; values.clear();
    values << 0.67 << 0.70 << 0.77 << 1.70 << 0.29; atomicRadii["c"]  = values; values.clear();
    values << 0.56 << 0.65 << 0.75 << 1.55 << 0.30; atomicRadii["n"]  = values; values.clear();
    values << 0.48 << 0.60 << 0.73 << 1.52 << 1.21; atomicRadii["o"]  = values; values.clear();
    values << 0.42 << 0.50 << 0.71 << 1.47 << 1.19; atomicRadii["f"]  = values; values.clear();
    values << 0.38 << 0.38 << 0.69 << 1.54 << 0.00; atomicRadii["ne"] = values; values.clear();
    values << 1.90 << 1.80 << 1.54 << 2.27 << 1.16; atomicRadii["na"] = values; values.clear();
    values << 1.45 << 1.50 << 1.30 << 1.73 << 0.86; atomicRadii["mg"] = values; values.clear();
    values << 1.18 << 1.25 << 1.18 << 0.00 << 0.53; atomicRadii["al"] = values; values.clear();
    values << 1.11 << 1.10 << 1.11 << 2.10 << 0.40; atomicRadii["si"] = values; values.clear();
    values << 0.98 << 1.00 << 1.06 << 1.80 << 0.31; atomicRadii["p"]  = values; values.clear();
    values << 0.88 << 1.00 << 1.02 << 1.80 << 0.43; atomicRadii["s"]  = values; values.clear();
    values << 0.79 << 1.00 << 0.99 << 1.75 << 1.67; atomicRadii["cl"] = values; values.clear();
    values << 0.71 << 0.71 << 0.97 << 1.88 << 0.00; atomicRadii["ar"] = values; values.clear();
    values << 2.43 << 2.20 << 1.96 << 2.75 << 1.52; atomicRadii["k"]  = values; values.clear();
    values << 1.94 << 1.80 << 1.74 << 0.00 << 1.14; atomicRadii["ca"] = values; values.clear();
    values << 1.84 << 1.60 << 1.44 << 0.00 << 0.89; atomicRadii["sc"] = values; values.clear();
    values << 1.76 << 1.40 << 1.36 << 0.00 << 0.75; atomicRadii["ti"] = values; values.clear();
    values << 1.71 << 1.35 << 1.25 << 0.00 << 0.68; atomicRadii["v"]  = values; values.clear();
    values << 1.66 << 1.40 << 1.27 << 0.00 << 0.76; atomicRadii["cr"] = values; values.clear();
    values << 1.61 << 1.40 << 1.39 << 0.00 << 0.81; atomicRadii["mn"] = values; values.clear();
    values << 1.56 << 1.40 << 1.25 << 0.00 << 0.69; atomicRadii["fe"] = values; values.clear();
    values << 1.52 << 1.35 << 1.26 << 0.00 << 0.54; atomicRadii["co"] = values; values.clear();
    values << 1.49 << 1.35 << 1.21 << 1.63 << 0.70; atomicRadii["ni"] = values; values.clear();
    values << 1.45 << 1.35 << 1.38 << 1.40 << 0.71; atomicRadii["cu"] = values; values.clear();
    values << 1.42 << 1.35 << 1.31 << 1.39 << 0.74; atomicRadii["zn"] = values; values.clear();
    values << 1.36 << 1.30 << 1.26 << 1.87 << 0.76; atomicRadii["ga"] = values; values.clear();
    values << 1.25 << 1.25 << 1.22 << 0.00 << 0.53; atomicRadii["ge"] = values; values.clear();
    values << 1.14 << 1.15 << 1.19 << 1.85 << 0.72; atomicRadii["as"] = values; values.clear();
    values << 1.03 << 1.15 << 1.16 << 1.90 << 0.56; atomicRadii["se"] = values; values.clear();
    values << 0.94 << 1.15 << 1.14 << 1.85 << 1.82; atomicRadii["br"] = values; values.clear();
    values << 0.88 << 0.88 << 1.10 << 2.02 << 0.00; atomicRadii["kr"] = values; values.clear();
    values << 2.65 << 2.35 << 2.11 << 0.00 << 1.66; atomicRadii["rb"] = values; values.clear();
    values << 2.19 << 2.00 << 1.92 << 0.00 << 1.32; atomicRadii["sr"] = values; values.clear();
    values << 2.12 << 1.85 << 1.62 << 0.00 << 1.04; atomicRadii["y"]  = values; values.clear();
    values << 2.06 << 1.55 << 1.48 << 0.00 << 0.86; atomicRadii["zr"] = values; values.clear();
    values << 1.98 << 1.45 << 1.37 << 0.00 << 0.78; atomicRadii["nb"] = values; values.clear();
    values << 1.90 << 1.45 << 1.45 << 0.00 << 0.79; atomicRadii["mo"] = values; values.clear();
    values << 1.83 << 1.35 << 1.56 << 0.00 << 0.79; atomicRadii["tc"] = values; values.clear();
    values << 1.78 << 1.30 << 1.26 << 0.00 << 0.82; atomicRadii["ru"] = values; values.clear();
    values << 1.73 << 1.35 << 1.35 << 0.00 << 0.81; atomicRadii["rh"] = values; values.clear();
    values << 1.69 << 1.40 << 1.31 << 1.63 << 0.78; atomicRadii["pd"] = values; values.clear();
    values << 1.65 << 1.60 << 1.53 << 1.72 << 1.29; atomicRadii["ag"] = values; values.clear();
    values << 1.61 << 1.55 << 1.48 << 1.58 << 0.92; atomicRadii["cd"] = values; values.clear();
    values << 1.56 << 1.55 << 1.44 << 1.93 << 0.94; atomicRadii["in"] = values; values.clear();
    values << 1.45 << 1.45 << 1.41 << 2.17 << 0.69; atomicRadii["sn"] = values; values.clear();
    values << 1.33 << 1.45 << 1.38 << 0.00 << 0.90; atomicRadii["sb"] = values; values.clear();
    values << 1.23 << 1.40 << 1.35 << 2.06 << 1.11; atomicRadii["te"] = values; values.clear();
    values << 1.15 << 1.40 << 1.33 << 1.98 << 2.06; atomicRadii["i"]  = values; values.clear();
    values << 1.08 << 1.08 << 1.30 << 2.16 << 0.62; atomicRadii["xe"] = values; values.clear();
    values << 2.98 << 2.60 << 2.25 << 0.00 << 1.81; atomicRadii["cs"] = values; values.clear();
    values << 2.53 << 2.15 << 1.98 << 0.00 << 1.49; atomicRadii["ba"] = values; values.clear();
    values << 1.95 << 1.95 << 1.69 << 0.00 << 1.36; atomicRadii["la"] = values; values.clear();
    values << 1.85 << 1.85 << 0.00 << 0.00 << 1.15; atomicRadii["ce"] = values; values.clear();
    values << 2.47 << 1.85 << 0.00 << 0.00 << 1.32; atomicRadii["pr"] = values; values.clear();
    values << 2.06 << 1.85 << 0.00 << 0.00 << 1.30; atomicRadii["nd"] = values; values.clear();
    values << 2.05 << 1.85 << 0.00 << 0.00 << 1.28; atomicRadii["pm"] = values; values.clear();
    values << 2.38 << 1.85 << 0.00 << 0.00 << 1.10; atomicRadii["sm"] = values; values.clear();
    values << 2.31 << 1.85 << 0.00 << 0.00 << 1.31; atomicRadii["eu"] = values; values.clear();
    values << 2.33 << 1.80 << 0.00 << 0.00 << 1.08; atomicRadii["gd"] = values; values.clear();
    values << 2.25 << 1.75 << 0.00 << 0.00 << 1.18; atomicRadii["tb"] = values; values.clear();
    values << 2.28 << 1.75 << 0.00 << 0.00 << 1.05; atomicRadii["dy"] = values; values.clear();
    values << 2.26 << 1.75 << 0.00 << 0.00 << 1.04; atomicRadii["ho"] = values; values.clear();
    values << 2.26 << 1.75 << 0.00 << 0.00 << 1.03; atomicRadii["er"] = values; values.clear();
    values << 2.22 << 1.75 << 0.00 << 0.00 << 1.02; atomicRadii["tm"] = values; values.clear();
    values << 2.22 << 1.75 << 0.00 << 0.00 << 1.13; atomicRadii["yb"] = values; values.clear();
    values << 2.17 << 1.75 << 1.60 << 0.00 << 1.00; atomicRadii["lu"] = values; values.clear();
    values << 2.08 << 1.55 << 1.50 << 0.00 << 0.85; atomicRadii["hf"] = values; values.clear();
    values << 2.00 << 1.45 << 1.38 << 0.00 << 0.78; atomicRadii["ta"] = values; values.clear();
    values << 1.93 << 1.35 << 1.46 << 0.00 << 0.74; atomicRadii["w"]  = values; values.clear();
    values << 1.88 << 1.35 << 1.59 << 0.00 << 0.77; atomicRadii["re"] = values; values.clear();
    values << 1.85 << 1.30 << 1.28 << 0.00 << 0.77; atomicRadii["os"] = values; values.clear();
    values << 1.80 << 1.35 << 1.37 << 0.00 << 0.77; atomicRadii["ir"] = values; values.clear();
    values << 1.77 << 1.35 << 1.28 << 1.75 << 0.74; atomicRadii["pt"] = values; values.clear();
    values << 1.74 << 1.35 << 1.44 << 1.66 << 1.51; atomicRadii["au"] = values; values.clear();
    values << 1.71 << 1.50 << 1.49 << 1.55 << 0.83; atomicRadii["hg"] = values; values.clear();
    values << 1.56 << 1.90 << 1.48 << 1.96 << 1.03; atomicRadii["tl"] = values; values.clear();
    values << 1.54 << 1.80 << 1.47 << 2.02 << 1.49; atomicRadii["pb"] = values; values.clear();
    values << 1.43 << 1.60 << 1.46 << 0.00 << 1.17; atomicRadii["bi"] = values; values.clear();
    values << 1.35 << 1.90 << 0.00 << 0.00 << 1.08; atomicRadii["po"] = values; values.clear();
    values << 1.27 << 1.27 << 0.00 << 0.00 << 0.76; atomicRadii["at"] = values; values.clear();
    values << 1.20 << 1.20 << 1.45 << 0.00 << 0.00; atomicRadii["rn"] = values; values.clear();
    values << 0.00 << 0.00 << 0.00 << 0.00 << 1.94; atomicRadii["fr"] = values; values.clear();
    values << 0.00 << 2.15 << 0.00 << 0.00 << 1.62; atomicRadii["ra"] = values; values.clear();
    values << 1.95 << 1.95 << 0.00 << 0.00 << 1.26; atomicRadii["ac"] = values; values.clear();
    values << 1.80 << 1.80 << 0.00 << 0.00 << 1.19; atomicRadii["th"] = values; values.clear();
    values << 1.80 << 1.80 << 0.00 << 0.00 << 1.09; atomicRadii["pa"] = values; values.clear();
    values << 1.75 << 1.75 << 0.00 << 1.86 << 0.87; atomicRadii["u"]  = values; values.clear();
    values << 1.75 << 1.75 << 0.00 << 0.00 << 0.00; atomicRadii["np"] = values; values.clear();
    values << 1.75 << 1.75 << 0.00 << 0.00 << 1.00; atomicRadii["pu"] = values; values.clear();
    values << 1.75 << 1.75 << 0.00 << 0.00 << 1.12; atomicRadii["am"] = values; values.clear();
    values << 0.00 << 0.00 << 0.00 << 0.00 << 1.11; atomicRadii["cm"] = values; values.clear();
}

/*
 * reads a FCF-File exported by BGMN (using FCFOUT[n]=<phase>.fcf)
 */
bool EmapImportHandler::readFcf(const QString &s, UnitCell &uc)
{
    uc.hkl.clear();

    QFile file(s);

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine(64);
        QStringList list = line.split(QRegularExpression("\\s"), Qt::SkipEmptyParts);
        QVector<float> index;

        bool ok;
        for (int j = 0; j < list.size(); ++j) {
            float val = list.at(j).toFloat(&ok);
            index.append(ok ? val : 0.0);
        }

        uc.hkl.append(index);
    }

    file.close();
    return true;
}

/*
 * reads a RES-File exported by BGMN (using RESOUT[n]=<phase>.res)
 */
bool EmapImportHandler::readRes(const QString &f, UnitCell &uc, QList<Atom> &atoms)
{
    QFile file(f);

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QStringList shelxtags;
    shelxtags << "TITL" << "CELL" << "ZERR" << "LATT" << "SYMM" << "SFAC" << "HKLF";
    shelxtags << "DISP" << "UNIT" << "LAUE" << "REM"  << "MORE" << "TIME" << "END";
    shelxtags << "OMIT" << "SHEL" << "BASF" << "TWIN" << "EXTI" << "SWAT" << "HOPE";
    shelxtags << "MERG" << "SPEC" << "RESI" << "MOVE" << "ANIS" << "AFIX" << "HFIX";
    shelxtags << "FRAG" << "FEND" << "EXYZ" << "EADP" << "EQIV" << "CONN" << "PART";
    shelxtags << "BIND" << "DFIX" << "DANG" << "BUMP" << "SAME" << "SADI" << "CHIV";
    shelxtags << "FLAT" << "DELU" << "SIMU" << "DEFS" << "ISOR" << "NCSY" << "SUMP";
    shelxtags << "L.S." << "CGLS" << "BLOC" << "DAMP" << "STIR" << "WGHT" << "FVAR";
    shelxtags << "BOND" << "CONF" << "MPLA" << "RTAB" << "HTAB" << "LIST" << "ACTA";
    shelxtags << "SIZE" << "TEMP" << "WPDB" << "FMAP" << "GRID" << "PLAN" << "MOLE";

    QList<QStringList> symops;
    symops.append(QString("X;Y;Z").split(";"));

    atoms.clear();
    QList<Atom> tAtoms;
    int latt = 1;

    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine(80);

        if (line.isEmpty()) continue;
        if (line.left(1).contains(QRegularExpression("\\s"))) continue;
        if (line.left(1) == "+") continue;

        // parse unit cell dimensions
        if (line.left(4) == "CELL") {
            QStringList list = line.split(QRegularExpression("\\s"), Qt::SkipEmptyParts);

            uc.a = list.at(2).toFloat();
            uc.b = list.at(3).toFloat();
            uc.c = list.at(4).toFloat();
            uc.alpha = list.at(5).toFloat();
            uc.beta = list.at(6).toFloat();
            uc.gamma = list.at(7).toFloat();

            // rounding errors due to limite PI resolution may cause problems. therefore
            // we hard-code the cosine of 90° to 0.0
            double calpha = uc.alpha == 90.0 ? 0.0 : cos(uc.alpha * M_PI / 180.0);
            double cbeta  = uc.beta  == 90.0 ? 0.0 : cos(uc.beta  * M_PI / 180.0);
            double cgamma = uc.gamma == 90.0 ? 0.0 : cos(uc.gamma * M_PI / 180.0);

            uc.volume = uc.a * uc.b * uc.c * sqrt(1.0
                        - pow(calpha, 2.0) - pow(cbeta, 2.0) - pow(cgamma, 2.0)
                        + 2.0 * calpha * cbeta * cgamma);
        }

        if (line.left(4) == "SYMM") {
            // store the symmetry operations in a list:
            // QList<QStringList> "X;Y;Z" "-X;-Y;-Z" ...
            QStringList symm = line.split(QRegularExpression("[,\\s]+"), Qt::SkipEmptyParts);
            if (symm.size() >= 4) symops.append(symm.mid(1, 3));
        }

        if (line.left(4) == "LATT") {
            // LATT N[1]
            // Lattice type: 1=P, 2=I, 3=rhombohedral obverse on hexagonal axes,
            // 4=F, 5=A, 6=B, 7=C. N must be made negative if the structure is non-centrosymmetric.

            QStringList lattline = line.split(QRegularExpression("[\\s]+"), Qt::SkipEmptyParts);
            if (lattline.size() >= 2) latt = lattline.at(1).toInt();
        }

        if (!shelxtags.contains(line.left(4))) {
            // must be an atom position, according to the shelx .ins format description
            // store the atome in a temporary list
            QStringList atmline = line.split(QRegularExpression("\\s+"));

            Atom atom(atmline.at(0), atmline.at(2).toFloat(), atmline.at(3).toFloat(), atmline.at(4).toFloat(), 1.0);
            tAtoms.append(atom);
        }
    }

    file.close();

    qDebug() << QString("Unit cell a      = %1 A").arg(uc.a, 11, 'f', 6);
    qDebug() << QString("Unit cell b      = %1 A").arg(uc.b, 11, 'f', 6);
    qDebug() << QString("Unit cell c      = %1 A").arg(uc.c, 11, 'f', 6);
    qDebug() << QString("Unit cell alpha  = %1°").arg(uc.alpha, 8, 'f', 3);
    qDebug() << QString("Unit cell beta   = %1°").arg(uc.beta, 8, 'f', 3);
    qDebug() << QString("Unit cell gamma  = %1°").arg(uc.gamma, 8, 'f', 3);
    qDebug() << QString("Unit cell volume = %1 A^3").arg(uc.volume, 8, 'f', 3);

    // generate symmetric equivalents
    if (!tAtoms.isEmpty() && !symops.isEmpty()) {
        atoms = generateAtoms(tAtoms, symops, latt);
        atoms = extendAtoms(atoms, uc, -1.0);
    }

    return true;
}

/*
 * reads a PDB-File exported by BGMN (using PDBOUT[n]=<phase>.pdb)
 */
bool EmapImportHandler::readPdb(const QString &f, UnitCell &uc)
{
    QFile file(f);

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine(80);
        if (line.left(6) != "CRYST1") continue;

        QStringList list = line.split(QRegularExpression("\\s"), Qt::SkipEmptyParts);

        uc.a = list.at(1).toFloat();
        uc.b = list.at(2).toFloat();
        uc.c = list.at(3).toFloat();
        uc.alpha = list.at(4).toFloat();
        uc.beta = list.at(5).toFloat();
        uc.gamma = list.at(6).toFloat();

        // rounding errors due to limite PI resolution may cause problems. therefore
        // we hard-code the cosine of 90° to 0.0
        double calpha = uc.alpha == 90.0 ? 0.0 : cos(uc.alpha * M_PI / 180.0);
        double cbeta  = uc.beta  == 90.0 ? 0.0 : cos(uc.beta  * M_PI / 180.0);
        double cgamma = uc.gamma == 90.0 ? 0.0 : cos(uc.gamma * M_PI / 180.0);

        uc.volume = uc.a * uc.b * uc.c * sqrt(1.0
                    - pow(calpha, 2.0) - pow(cbeta, 2.0) - pow(cgamma, 2.0)
                    + 2.0 * calpha * cbeta * cgamma);

        break;
    }

    file.close();

    qDebug() << QString("Unit cell a      = %1 A").arg(uc.a, 11, 'f', 6);
    qDebug() << QString("Unit cell b      = %1 A").arg(uc.b, 11, 'f', 6);
    qDebug() << QString("Unit cell c      = %1 A").arg(uc.c, 11, 'f', 6);
    qDebug() << QString("Unit cell alpha  = %1°").arg(uc.alpha, 8, 'f', 3);
    qDebug() << QString("Unit cell beta   = %1°").arg(uc.beta, 8, 'f', 3);
    qDebug() << QString("Unit cell gamma  = %1°").arg(uc.gamma, 8, 'f', 3);
    qDebug() << QString("Unit cell volume = %1 A^3").arg(uc.volume, 8, 'f', 3);

    return true;
}

/*
 * reads a FOU-File exported by Fullprof (using JFOU=4)
 */
bool EmapImportHandler::readFou(const QString &s, UnitCell &uc)
{
    uc.hkl.clear();

    QFile file(s);

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QTextStream in(&file);
    in.readLine(80); // read first comment line

    while (!in.atEnd()) {
        QString line = in.readLine(80);
        QStringList list = line.split(QRegularExpression("\\s"), Qt::SkipEmptyParts);
        QVector<float> index;

        bool ok;
        for (int j = 0; j < list.size(); ++j) {
            float val = list.at(j).toFloat(&ok);
            index.append(ok ? val : 0.0);
        }

        uc.hkl.append(index);
    }

    file.close();
    return true;
}

/*
 * reads a HKL-File exported by Fullprof (using JLKH=3)
 */
bool EmapImportHandler::readHkl(const QString &f, UnitCell &uc)
{
    QFile file(f);

    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QTextStream in(&file);

    while (!in.atEnd()) {
        QString line = in.readLine(256);
        QStringList list = line.split(QRegularExpression("\\s"), Qt::SkipEmptyParts);

        int idx = list.indexOf("CELL:");

        if (idx < 0) continue;

        uc.a = list.at(idx+1).toFloat();
        uc.b = list.at(idx+2).toFloat();
        uc.c = list.at(idx+3).toFloat();
        uc.alpha = list.at(idx+4).toFloat();
        uc.beta = list.at(idx+5).toFloat();
        uc.gamma = list.at(idx+6).toFloat();

        // rounding errors due to limite PI resolution may cause problems. therefore
        // we hard-code the cosine of 90° to 0.0
        double calpha = uc.alpha == 90.0 ? 0.0 : cos(uc.alpha * M_PI / 180.0);
        double cbeta  = uc.beta  == 90.0 ? 0.0 : cos(uc.beta  * M_PI / 180.0);
        double cgamma = uc.gamma == 90.0 ? 0.0 : cos(uc.gamma * M_PI / 180.0);

        uc.volume = uc.a * uc.b * uc.c * sqrt(1.0
                    - pow(calpha, 2.0) - pow(cbeta, 2.0) - pow(cgamma, 2.0)
                    + 2.0 * calpha * cbeta * cgamma);

        break;
    }

    file.close();

    qDebug() << QString("Unit cell a      = %1 A").arg(uc.a, 11, 'f', 6);
    qDebug() << QString("Unit cell b      = %1 A").arg(uc.b, 11, 'f', 6);
    qDebug() << QString("Unit cell c      = %1 A").arg(uc.c, 11, 'f', 6);
    qDebug() << QString("Unit cell alpha  = %1°").arg(uc.alpha, 8, 'f', 3);
    qDebug() << QString("Unit cell beta   = %1°").arg(uc.beta, 8, 'f', 3);
    qDebug() << QString("Unit cell gamma  = %1°").arg(uc.gamma, 8, 'f', 3);
    qDebug() << QString("Unit cell volume = %1 A^3").arg(uc.volume, 8, 'f', 3);

    return true;
}

/*
 * generates all symmetric equivalent atoms of the primary atoms stored in primaryAtoms
 */
QList<Atom> EmapImportHandler::generateAtoms(const QList<Atom> &primaryAtoms, const QList<QStringList> &symOps, int latt)
{
    // primary atoms = asymmetric unit
    // secondary atoms = + all lattice translations
    // tertiary atoms = + inversion center
    // all atoms = + all symmetry operations - duplicates
    QList<Atom> secondaryAtoms;
    QList<Atom> tertiaryAtoms;
    QList<Atom> allAtoms;
    QJSEngine scriptEngine;

    // generate translated atoms
    for (int i = 0; i < primaryAtoms.size(); ++i) {
        Atom atom = primaryAtoms.at(i);
        secondaryAtoms.append(atom);

        if (abs(latt) == 2) { // body centered
            secondaryAtoms.append(Atom(atom.name, atom.x + 0.5, atom.y + 0.5, atom.z + 0.5, 1.0));
        }

        if (abs(latt) == 3) { // rhombohedral hexagonal axes
            secondaryAtoms.append(Atom(atom.name, atom.x + 2.0/3.0, atom.y + 1.0/3.0, atom.z + 1.0/3.0, 1.0));
            secondaryAtoms.append(Atom(atom.name, atom.x + 1.0/3.0, atom.y + 2.0/3.0, atom.z + 2.0/3.0, 1.0));
        }

        if (abs(latt) == 4) { // face centered
            secondaryAtoms.append(Atom(atom.name, atom.x, atom.y + 0.5, atom.z + 0.5, 1.0));
            secondaryAtoms.append(Atom(atom.name, atom.x + 0.5, atom.y, atom.z + 0.5, 1.0));
            secondaryAtoms.append(Atom(atom.name, atom.x + 0.5, atom.y + 0.5, atom.z, 1.0));
        }

        if (abs(latt) == 5) { // A centered
            secondaryAtoms.append(Atom(atom.name, atom.x, atom.y + 0.5, atom.z + 0.5, 1.0));
        }

        if (abs(latt) == 6) { // B centered
            secondaryAtoms.append(Atom(atom.name, atom.x + 0.5, atom.y, atom.z + 0.5, 1.0));
        }

        if (abs(latt) == 7) { // C centered
            secondaryAtoms.append(Atom(atom.name, atom.x + 0.5, atom.y + 0.5, atom.z, 1.0));
        }
    }

    // generate inversion atoms
    for (int i = 0; i < secondaryAtoms.size(); ++i) {
        Atom atom = secondaryAtoms.at(i);
        tertiaryAtoms.append(atom);
        if (latt > 0) {
            tertiaryAtoms.append(Atom(atom.name, -atom.x, -atom.y, -atom.z, 1.0));
        }
    }

    int n = 1;
    for (int i = 0; i < tertiaryAtoms.size(); ++i) {
        for (int j = 0; j < symOps.size(); ++j) {
            scriptEngine.globalObject().setProperty("X", tertiaryAtoms.at(i).x);
            scriptEngine.globalObject().setProperty("Y", tertiaryAtoms.at(i).y);
            scriptEngine.globalObject().setProperty("Z", tertiaryAtoms.at(i).z);

            float x = scriptEngine.evaluate(symOps.at(j).at(0)).toNumber();
            float y = scriptEngine.evaluate(symOps.at(j).at(1)).toNumber();
            float z = scriptEngine.evaluate(symOps.at(j).at(2)).toNumber();

            float radius = 0.5;
            QRegularExpression rx("[A-Za-z]{1,2}");
            QRegularExpressionMatch match = rx.match(tertiaryAtoms.at(i).name);

            if (match.hasMatch()) {
                QString element = match.captured(0).toLower();
                if (atomicRadii.contains(element)) {
                    radius = atomicRadii.value(element).at(4);
                }
            }

            QString name = QString("%1_%2").arg(tertiaryAtoms.at(i).name).arg(n);
            Atom atom = normalizeAtom(Atom(name, x, y, z, radius));

            if (!allAtoms.contains(atom)) {
                allAtoms.append(atom);
                ++n;
                qDebug() << QString("Inserting atom %1 at %2, %3, %4")
                            .arg(atom.name)
                            .arg(atom.x, 0, 'f', 8)
                            .arg(atom.y, 0, 'f', 8)
                            .arg(atom.z, 0, 'f', 8);
            }
        }
    }

    qDebug() << QString("%1 atoms generated").arg(allAtoms.size());
    return allAtoms;
}

/*
 * shifts the fractional coorinates of atom a into the unit cell (0 < 1)
 */
Atom EmapImportHandler::normalizeAtom(const Atom &a)
{
    Atom atom = a;
    while (atom.x < 0.0) atom.x += 1.0;
    while (atom.y < 0.0) atom.y += 1.0;
    while (atom.z < 0.0) atom.z += 1.0;

    while (atom.x > 1.0) atom.x -= 1.0;
    while (atom.y > 1.0) atom.y -= 1.0;
    while (atom.z > 1.0) atom.z -= 1.0;

    // replace numbers between 0.3332 and 0.3334 with 1/3
    if (fabs(atom.x - 0.3333) < 0.0001) atom.x = 1.0/3.0;
    if (fabs(atom.y - 0.3333) < 0.0001) atom.y = 1.0/3.0;
    if (fabs(atom.z - 0.3333) < 0.0001) atom.z = 1.0/3.0;

    // replace numbers between 0.6666 and 0.6668 with 2/3
    if (fabs(atom.x - 0.6667) < 0.0001) atom.x = 2.0/3.0;
    if (fabs(atom.y - 0.6667) < 0.0001) atom.y = 2.0/3.0;
    if (fabs(atom.z - 0.6667) < 0.0001) atom.z = 2.0/3.0;

    // replace numbers between 0.1666 and 0.1668 with 1/6
    if (fabs(atom.x - 0.1667) < 0.0001) atom.x = 1.0/6.0;
    if (fabs(atom.y - 0.1667) < 0.0001) atom.y = 1.0/6.0;
    if (fabs(atom.z - 0.1667) < 0.0001) atom.z = 1.0/6.0;

    // replace numbers between 0.8332 and 0.8334 with 5/6
    if (fabs(atom.x - 0.8333) < 0.0001) atom.x = 5.0/6.0;
    if (fabs(atom.y - 0.8333) < 0.0001) atom.y = 5.0/6.0;
    if (fabs(atom.z - 0.8333) < 0.0001) atom.z = 5.0/6.0;

    return atom;
}

/*
 * Adds atoms outside of the unit cell if nx, px, ... are != 0.0.
 * nx, px, ... are to be given in fractional coordinates, positive values for all directions.
 * Give any negative value (e.g. -1.0) to add only one layer of atoms touching the central unit cell.
 * This is used to make sure that atoms are drawn if they extend into the central unit cell even if
 * their core coordinate is outside of the unit cell.
 *
 * nx = negative x-direction
 * px = poxitive x-direction
 * ...
 *
 * Examples: nx = 0.0, px = 0.0, ny = 0.0, py = 0.0, nz = 0.0, pz = 0.0
 *           draws only atoms inside the central unit cell from 0,0,0 to 1,0,0 and 0,1,0 and 0,0,1
 *
 *           nx = px = ny = py = nz = pz = -1.0
 *           draws all atoms outside the central unit cell if their distance from the unit cell is
 *           less than the ionic radius
 *
 *           nx = px = ny = py = nz = pz = 1.0
 *           draws 9 unit cells, covering the space from -1,0,0 to 2,0,0 and 0,-1,0 to 0,2,0 and 0,0,-1 to 0,0,2
 */
QList<Atom> EmapImportHandler::extendAtoms(const QList<Atom> &ucAtoms, const UnitCell &uc, float nx, float px, float ny, float py, float nz, float pz)
{
    if ((nx == 0.0) && (px == 0.0) &&
        (ny == 0.0) && (py == 0.0) &&
        (nz == 0.0) && (pz == 0.0)) return ucAtoms;

    QList<Atom> atoms = ucAtoms;
    QList<Atom> newAtoms;

    // loop over all atoms and add new ones in x-direction (if required)
    for (int i = 0; i < atoms.size(); ++i) {
        Atom a = atoms.at(i);

        if (nx != 0.0) {
            if (nx < 0.0) {
                if (uc.a * (1.0 - a.x) <= a.r) newAtoms.append(Atom(a.name, a.x - 1.0, a.y, a.z, a.r));
            } else {
                if (uc.a * (1.0 - a.x) <= nx) newAtoms.append(Atom(a.name, a.x - 1.0, a.y, a.z, a.r));
            }
        }

        if (px != 0.0) {
            if (px < 0.0) {
                if (uc.a * a.x <= a.r) newAtoms.append(Atom(a.name, a.x + 1.0, a.y, a.z, a.r));
            } else {
                if (uc.a * a.x <= nx) newAtoms.append(Atom(a.name, a.x + 1.0, a.y, a.z, a.r));
            }
        }
    }

    // store the newly generated atoms in the list
    atoms.append(newAtoms);
    newAtoms.clear();

    // loop over all atoms and add new ones in y-direction (if required)
    for (int i = 0; i < atoms.size(); ++i) {
        Atom a = atoms.at(i);

        if (ny != 0.0) {
            if (ny < 0.0) {
                if (uc.b * (1.0 - a.y) <= a.r) newAtoms.append(Atom(a.name, a.x, a.y - 1.0, a.z, a.r));
            } else {
                if (uc.b * (1.0 - a.y) <= ny) newAtoms.append(Atom(a.name, a.x, a.y - 1.0, a.z, a.r));
            }
        }

        if (py != 0.0) {
            if (py < 0.0) {
                if (uc.b * a.y <= a.r) newAtoms.append(Atom(a.name, a.x, a.y + 1.0, a.z, a.r));
            } else {
                if (uc.b * a.y <= ny) newAtoms.append(Atom(a.name, a.x, a.y + 1.0, a.z, a.r));
            }
        }
    }

    // store the newly generated atoms in the list
    atoms.append(newAtoms);
    newAtoms.clear();

    // loop over all atoms and add new ones in y-direction (if required)
    for (int i = 0; i < atoms.size(); ++i) {
        Atom a = atoms.at(i);

        if (nz != 0.0) {
            if (nz < 0.0) {
                if (uc.c * (1.0 - a.z) <= a.r) newAtoms.append(Atom(a.name, a.x, a.y, a.z - 1.0, a.r));
            } else {
                if (uc.c * (1.0 - a.z) <= nz) newAtoms.append(Atom(a.name, a.x, a.y, a.z - 1.0, a.r));
            }
        }

        if (pz != 0.0) {
            if (pz < 0.0) {
                if (uc.c * a.z <= a.r) newAtoms.append(Atom(a.name, a.x, a.y, a.z + 1.0, a.r));
            } else {
                if (uc.c * a.z <= nz) newAtoms.append(Atom(a.name, a.x, a.y, a.z + 1.0, a.r));
            }
        }
    }

    // store the newly generated atoms in the list
    atoms.append(newAtoms);
    return atoms;
}

/*
 * convenience function, uses the same margin in all directions
 */
QList<Atom> EmapImportHandler::extendAtoms(const QList<Atom> &ucAtoms, const UnitCell &uc, float dst)
{
    return extendAtoms(ucAtoms, uc, dst, dst, dst, dst, dst, dst);
}
