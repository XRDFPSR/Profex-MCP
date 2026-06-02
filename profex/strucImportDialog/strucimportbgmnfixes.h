/***************************************************************************
                          strucimportbgmnfixes.h  -  description
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


#ifndef STRUCIMPORTBGMNFIXES_H
#define STRUCIMPORTBGMNFIXES_H

#include "../libXrdIO/crystal/crystalstructure.h"
#include "../libXrdIO/crystal/crystalsymop.h"
#include "../libXrdIO/parser/bgmnsgdatparser.h"
#include "../libXrdIO/structs.h"

#include <QSet>
#include <QObject>

struct BondPair {
    CrystalAtom atomA;
    CrystalAtom atomB;
    double dist;

    BondPair(CrystalAtom a, CrystalAtom b, double d) : atomA(a), atomB(b), dist(d) {}

    inline bool operator==(BondPair a) const {
        if (a.atomA.name() == atomA.name() && a.atomB.name() == atomB.name())
            return true;
        else
            return false;
    }
};

class StrucImportBgmnFixes : public QObject
{
    Q_OBJECT

public:
    explicit StrucImportBgmnFixes(BgmnSgDatParser *, QObject *parent = nullptr);

    CrystalStructure fixStructure(const CrystalStructure &cStruc, bool interactive = true);
    inline QString const report() {return reportString;}

private:
    BgmnSgDatParser *sgParser;
    QString reportString;
    int debugLevel;
    bool abort;
    QList<CrystalAtom> atomCollection;
    QList<global::WyckoffPosition> wyckoffList;
    QVector<QStringList> generalOperators;
    QMap<QString, QStringList> firstWycks;
    QString translation;
    QString lattice;
    int errCount;

    /* tries to determine the BGMN HermannMauguin symbol form the CIF space group.
     * if successful, writes the HMsymbol, ITnumber, and settings line to CrystalStructure.
     * asks for user input if not successful */
    void identifyBgmnSetting(const QList<BgmnSpaceGroup> &, CrystalStructure &, bool interactive = true);

    /* applies some manual filtering to the list of matching space groups */
    QList<BgmnSpaceGroup> preFilterMatches(const QList<BgmnSpaceGroup> &sgl, const CrystalStructure &);

    /* returns BgmnSpaceGroups with identical symmetry operators to CrystalStructure.
     * if no matches are found, returns the unfiltered list.
     */
    QList<BgmnSpaceGroup> preFilterSymops(const QList<BgmnSpaceGroup> &sgl, const CrystalStructure &);

    /* converts the element symbol to a BGMN structure factor symbol */
    void fixScatteringFactors(CrystalStructure &);

    /* show dialog to select IT space group number */
    BgmnSpaceGroup queryIntlTableNo();

    /* parses the settings line for BGMN str files and stores the info in the crystal structure */
    void parseBgmnSettingsLine(CrystalStructure &);

    /* auto-detect wyckoff position of all atoms */
    bool fixWyckoff(CrystalStructure &);

    QMap<QString, QStringList> getFirstWyckoffs(const QList<global::WyckoffPosition> &);

    /* checks which symmetric equivalent matches the first wyckoff position */
    CrystalAtom findFirstWyckoff(const QList<CrystalAtom> &, const QStringList &, bool &ok);

    /* auto-detects the wyckoff position from all symmetric equivalents */
    CrystalAtom detectWyckoff(const QList<CrystalAtom> &, const QMap<QString, QStringList> &, const QString &, bool &ok);

    /* returns a list with scattering factors. If all ionic ones are available, the list contains ionic
     * factors. if one or more ionic ones are missing, it returns neutral ones.
     */
    void checkIonicScatFactors(QList<CrystalAtom> &);
    void appendScatFactors(QVector<QStringList> &, const QString &, const QString &, const QString &);

    /*
     * Checks if the atom was identified as a general position. If yes, it checks
     * if the coordinates contain any special values (0, 0.5, 0.333 or so).
     * if they do, it outputs a warning that this might be a special position.
     */
    int checkGeneralWyckoffForSpecial(const CrystalAtom &, const QString &);

    /*
     * creates a string containing the translation, lattice, and unique axis
     * for comparison of two bravais types
     */
    QString determineBravaisType(const CrystalUnitCell &);

    /*
     * Converts fractional coordinates to cartesian and calculates all
     * bond lengths. The list must contain all symmetric equivalents.
     */
    double checkBondLengths(const CrystalStructure &, const QList<CrystalAtom> &);
    QMatrix4x4 getF2Cmatrix(const CrystalUnitCell &);
    QList<QVector4D> fracToCartesian(const CrystalAtom &, const QMatrix4x4 &, bool expand = true);
    double compareBondLengths(const QList<BondPair> &lst);

    int countIdentAxes(const CrystalStructure &);
    int count90Angles(const CrystalStructure &);
    int count120Angles(const CrystalStructure &);
    QString uniqueAxisNonOrthogonal(const CrystalStructure &);
    QString uniqueAxisOrthogonal(const CrystalStructure &);

    int countIdentAxes(double, double, double);
    int count90Angles(double, double, double);
    int count120Angles(double, double, double);
    QString uniqueAxisNonOrthogonal(double, double, double);
    QString uniqueAxisOrthogonal(double, double, double);

    CrystalAtom detectAndCheckAtomWyckoff(const CrystalAtom &);

signals:
    void setProgress(int);
};

#endif // STRUCIMPORTBGMNFIXES_H
