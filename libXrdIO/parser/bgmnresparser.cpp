/***************************************************************************
                          bgmnresparser.cpp  -  description
                             -------------------
    begin                : Sat Aug 22 21:04:07 CEST 2015
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

#include "bgmnresparser.h"
#include "bgmnfileio.h"
#include <QFile>
#include <QTextStream>
#include <QList>
#include <QRegularExpression>
#include <QDebug>
#include <math.h>

BgmnResParser::BgmnResParser(const QString &f)
{
    PI = atan(1.0) * 4.0;
    readRes(f);
}

/*
 * reads a RES-File exported by BGMN (using RESOUT[n]=<phase>.res)
 *
 * Format description:
 * http://shelx.uni-ac.gwdg.de/SHELX/shelxl_html.php
 */
void BgmnResParser::readRes(const QString &f)
{
    qDebug() << QString("BgmnResParser::readRes(): Reading file %1").arg(f);

    static QRegularExpression rxSplitSpace("\\s+");
    static QRegularExpression rxSplitAll("[,\\s]+");

    QStringList rawContent;
    rawContent = BgmnFileIO::readTextFileLines(f);

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

    QList<CrystalAtom> atoms;
    CrystalUnitCell uc;

    QString hmsymb = QString();
    QString name = QString();
    int itnum = 0;
    int lattice = 0;
    int strucZ = 1;

    double ucaxes[3]   = {-1.0, -1.0, -1.0};
    double ucangles[3] = {-1.0, -1.0, -1.0};
    double ucesd[6]    = {-1.0, -1.0, -1.0, -1.0, -1.0, -1.0};

    QList<QStringList> symops;
    symops.append(QStringList() << "X" << "Y" << "Z");

    atoms.clear();

    // loop over the file content to consolidate split lines (ending on '=') and strip comments
    static QRegularExpression rxline("^([^!=]+)([!=]?)");
    QRegularExpressionMatch rmline;

    for (int i = 0; i < rawContent.size(); ++i) {
        if (rawContent.at(i).isEmpty()) continue;

        rmline = rxline.match(rawContent.at(i));
        if (rmline.hasMatch()) {
            QString newline = rmline.captured(1);

            // if captured(2) is a '=' sign, append the next line
            while ((rmline.captured(2) == "=") && (i < rawContent.size())) {
                ++i;
                rmline = rxline.match(rawContent.at(i));
                newline += " " + rmline.captured(1).trimmed();
            }

            _content.append(newline);
        }
    }

    for (int i = 0; i < _content.size(); ++i) {
        QString line = _content.at(i);

        // skip empty lines and those starting with comment signs (space, +)
        if (line.isEmpty()) continue;
        if (line.left(1) == " ") continue;
        if (line.left(1) == "+") continue;

        // parse title line
        if (line.left(4) == "TITL") {
            static QRegularExpression rxtitl("^TITL\\s+(\\S+)\\s+No\\.(\\d+)\\s+(\\S+)");
            QRegularExpressionMatch rmtitl = rxtitl.match(line);

            hmsymb = rmtitl.captured(1);
            itnum = rmtitl.captured(2).toInt();
            name = rmtitl.captured(3);
        }

        // parse unit cell dimensions
        if (line.left(4) == "CELL") {
            static QRegularExpression rxCell("^CELL\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)");
            QRegularExpressionMatch rmCell = rxCell.match(line);

            if (rmCell.hasMatch()) {
                // captured(1) is the wavelength
                ucaxes[0] = rmCell.captured(2).toDouble();
                ucaxes[1] = rmCell.captured(3).toDouble();
                ucaxes[2] = rmCell.captured(4).toDouble();
                ucangles[0] = rmCell.captured(5).toDouble();
                ucangles[1] = rmCell.captured(6).toDouble();
                ucangles[2] = rmCell.captured(7).toDouble();
            }
        }

        if (line.left(4) == "SYMM") {
            // store the symmetry operations in a list:
            // QList<QStringList> "X;Y;Z" "-X;-Y;-Z" ...
            QStringList symm = line.split(rxSplitAll);
            if (symm.size() >= 4) symops.append(symm.mid(1, 3));
        }

        if (line.left(4) == "LATT") {
            // LATT N[1]
            // Lattice type: 1=P, 2=I, 3=rhombohedral obverse on hexagonal axes,
            // 4=F, 5=A, 6=B, 7=C. N must be made negative if the structure is non-centrosymmetric.

            QStringList lattline = line.split(rxSplitSpace);
            if (lattline.size() >= 2) lattice = lattline.at(1).toInt();
        }

        if (line.left(4) == "ZERR") {
            // ZERR Z esd_a esd_b esd_c esd_al esd_be esd_ga
            static QRegularExpression rxZerr("^ZERR\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)"
                                             "\\s+(\\d*\\.\\d*)");
            QRegularExpressionMatch rmZerr = rxZerr.match(line);
            if (rmZerr.hasMatch()) {
                strucZ = rmZerr.captured(1).toInt();
                if (!qFuzzyIsNull(rmZerr.captured(2).toDouble())) ucesd[0] = rmZerr.captured(2).toDouble();
                if (!qFuzzyIsNull(rmZerr.captured(3).toDouble())) ucesd[1] = rmZerr.captured(3).toDouble();
                if (!qFuzzyIsNull(rmZerr.captured(4).toDouble())) ucesd[2] = rmZerr.captured(4).toDouble();
                if (!qFuzzyIsNull(rmZerr.captured(5).toDouble())) ucesd[3] = rmZerr.captured(5).toDouble();
                if (!qFuzzyIsNull(rmZerr.captured(6).toDouble())) ucesd[4] = rmZerr.captured(6).toDouble();
                if (!qFuzzyIsNull(rmZerr.captured(7).toDouble())) ucesd[5] = rmZerr.captured(7).toDouble();
            }
        }

        if (!shelxtags.contains(line.left(4))) {
            // must be an atom position, according to the shelx .ins format description.
            // store the atome in a temporary list
            static QRegularExpression rxatl("^(([A-Za-z]+)\\d+)\\s+"                                 // name (element)           / 1 (2)
                                     "(\\d+)\\s+"                                                    // scattering factor index  / 3
                                     "(-?\\d*\\.\\d+)\\s+(-?\\d*\\.\\d+)\\s+(-?\\d*\\.\\d+)\\s+"     // x y z                    / 4 5 6
                                     "(\\d*\\.\\d+)\\s+"                                             // sof                      / 7
                                     "(-?\\d*\\.\\d+)(?:\\s+(-?\\d*\\.\\d+)\\s+(-?\\d*\\.\\d+)\\s+"  // Uiso or U11 (U22 U33     / 8 (9 10
                                     "(-?\\d*\\.\\d+)\\s+(-?\\d*\\.\\d+)\\s+(-?\\d*\\.\\d+))?");     // U12 U13 U23)?            / 11 12 13)
            QRegularExpressionMatch rmatl = rxatl.match(line);

            if (rmatl.hasMatch()) {
                double sof = rmatl.captured(7).toDouble();
                if (sof >= 10.0) sof -= 10.0;
                CrystalAtom atom(rmatl.captured(2),
                                 rmatl.captured(4).toDouble(),
                                 rmatl.captured(5).toDouble(),
                                 rmatl.captured(6).toDouble(),
                                 sof);
                atom.setName(rmatl.captured(1));

                if (rmatl.capturedTexts().size() > 9) {
                    // has Uaniso
                    double uaniso[6] = {rmatl.captured(8).toDouble(),
                                        rmatl.captured(9).toDouble(),
                                        rmatl.captured(10).toDouble(),
                                        rmatl.captured(11).toDouble(),
                                        rmatl.captured(12).toDouble(),
                                        rmatl.captured(13).toDouble()};

                    atom.setBiso(bisoFromUaniso(ucangles[0], ucangles[1], ucangles[2],
                                                uaniso[0],   uaniso[1],   uaniso[2],
                                                uaniso[3],   uaniso[4],   uaniso[5]));
                } else {
                    atom.setBiso(bisoFromUiso(rmatl.captured(8).toDouble()));
                }

                atoms.append(atom);
            }
        }
    }

    uc.setSpaceGroupHMBgmn(hmsymb);
    uc.setItNumber(itnum);
    uc.setCell(ucaxes[0], ucaxes[1], ucaxes[2], ucangles[0], ucangles[1], ucangles[2],
               ucesd[0],  ucesd[1],  ucesd[2],  ucesd[3],  ucesd[4],  ucesd[5]);
    _structure.setLattice(lattice);
    _structure.setZ(strucZ);
    _structure.setName(name);
    _structure.setSymops(symops);
    _structure.setAtoms(atoms);
    _structure.setUnitCell(uc);

    qDebug() << QString("BgmnResParser::readRes(): Found phase %1 with space group %2(%3)").arg(name, hmsymb).arg(itnum);
}


/*
 * conversion from Uiso to Biso
 */
double BgmnResParser::bisoFromUiso(double uiso)
{
    return uiso * PI * PI * 8.0;
}

/*
 * conversion from U_ij to Biso, based on Fischer & Tillmanns, 1988
 */
double BgmnResParser::bisoFromUaniso(double alpha, double beta, double gamma,
                                 double u11, double u22, double u33, double u12, double u13, double u23)
{
    double biso = 0.0;

    // if angles are 0.0, assume they are 90.0 but not mentioned in the cif
    double ca = alpha == 0.0 ? 0.0 : cos(alpha * PI / 180.0);
    double cb = beta  == 0.0 ? 0.0 : cos(beta  * PI / 180.0);
    double cc = gamma == 0.0 ? 0.0 : cos(gamma * PI / 180.0);

    double sa = alpha == 0.0 ? 1.0 : sin(alpha * PI / 180.0);
    double sb = beta  == 0.0 ? 1.0 : sin(beta  * PI / 180.0);
    double sc = gamma == 0.0 ? 1.0 : sin(gamma * PI / 180.0);

    biso = (u11*sa*sa + u22*sb*sb + u33*sc*sc + 2.0*u12*sa*sb*cc + 2.0*u13*sa*cb*sc + 2.0*u23*ca*sb*sc);
    biso /= (1.0 + 2.0*ca*cb*cc - ca*ca - cb*cb - cc*cc);
    biso *= 8.0 * PI * PI / 3.0;

    return biso;
}
