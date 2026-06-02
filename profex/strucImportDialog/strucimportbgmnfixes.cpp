/***************************************************************************
                          strucimportbgmnfixes.cpp  -  description
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


#include "strucimportbgmnfixes.h"
#include "inputspacegroupdialog.h"
#include "strucimportbgmnmessages.h"
#include "symmetrymultiplier.h"
#include "../libXrdIO/functions.h"

#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QInputDialog>
#include <QString>
#include <QJSEngine>
#include <QCoreApplication>
#include <QColor>
#include <math.h>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

StrucImportBgmnFixes::StrucImportBgmnFixes(BgmnSgDatParser *sgp, QObject *parent) :
    QObject(parent)
{
    sgParser = sgp;
    abort = false;
    debugLevel = 2;
}

CrystalStructure StrucImportBgmnFixes::fixStructure(const CrystalStructure &cs, bool interactive)
{
    reportString.clear();
    wyckoffList.clear();
    generalOperators.clear();
    firstWycks.clear();
    translation.clear();
    lattice.clear();
    atomCollection.clear();

    CrystalStructure cStruc = cs;

    QString cifHM = cStruc.unitCell().spaceGroupHMCif();
    int     itNum = cStruc.unitCell().itNumber();

    reportString += QString("Read from file:<br>SpacegroupNo=%1<br>HermannMauguin=%2<br><br>").arg(itNum).arg(cifHM);

    QList<BgmnSpaceGroup> sgList = sgParser->getSpaceGroup(cifHM, itNum);

    abort = false;

    if (!abort) fixScatteringFactors(cStruc);
    if (!abort) identifyBgmnSetting(sgList, cStruc, interactive);

    return cStruc;
}

/*
 * shows a dialog prompting for user input
 */
BgmnSpaceGroup StrucImportBgmnFixes::queryIntlTableNo()
{
    QMap<int, QMap<int, BgmnSpaceGroup> > sgMap = sgParser->getAllSpaceGroups();
    BgmnSpaceGroup sg;

    InputSpacegroupDialog *isgdlg = new InputSpacegroupDialog;
    isgdlg->setLabelText(tr("No valid Space Group found. Select Intl. Tables number and setting."));
    isgdlg->setSpaceGroups(sgMap);

    if (isgdlg->exec() == QDialog::Accepted) {
        sg = sgMap[isgdlg->intTableNo()][isgdlg->settingNo()];
    } else {
        abort = true;
    }

    delete isgdlg;
    return sg;
}

void StrucImportBgmnFixes::identifyBgmnSetting(const QList<BgmnSpaceGroup> &sgMatches, CrystalStructure &cStruc, bool interactive)
{
    bool hasWyck = cStruc.hasWyckoff();

    QList<BgmnSpaceGroup> sgl = preFilterMatches(sgMatches, cStruc);
    sgl = preFilterSymops(sgl, cStruc);

    QMultiMap<ulong, CrystalStructure> multSettings;

    if (sgl.size() == 0) {
        if (interactive) {
            sgl.append(queryIntlTableNo());
            if (abort) {
                qDebug() << QString("StrucImportBgmnFixes::identifyBgmnSetting(): User aborted SpaceGroup query dialog.");
                return;
            }
        } else {
            qDebug() << QString("StrucImportBgmnFixes::identifyBgmnSetting(): No space group found and interactive mode is disabled. Skipping.");
            return;
        }
    }

    qDebug() << QString("StrucImportBgmnFixes::identifyBgmnSetting(): Automatic "
                        "identification of space group found %1 matching settings:").arg(sgl.size());

    for (int i = 0; i < sgl.size(); ++i) {
        qDebug() << QString("    %1").arg(sgl.at(i).settingLine);
    }

    for (int i = 0; i < sgl.size(); ++i) {
        CrystalStructure testStruc = cStruc;
        testStruc.unitCell().setSpaceGroupHMBgmn(sgl.at(i).HermannMauguin);
        testStruc.setAuxInfo("BgmnSettingsLine", sgl.at(i).settingLine);
        parseBgmnSettingsLine(testStruc);

        qDebug() << QString("    Checking Wyckoff symmetries of %1").arg(sgl.at(i).settingLine);
        reportString += StrucImportBgmnMessages::showComment(QString("    Checking Wyckoff symmetries of:"));
        reportString += StrucImportBgmnMessages::showComment(QString("    SpacegroupNo %1 Setting %2 (%3)")
                                                             .arg(sgl.at(i).itNumber).arg(sgl.at(i).setting).arg(sgl.at(i).HermannMauguin));

        if (hasWyck) reportString += StrucImportBgmnMessages::showSuccess("Imported structure has Wyckoff symbols. Checking for symmetry match:");
        else         reportString += StrucImportBgmnMessages::showWarning("Imported structure has no Wyckoff symbols. Attempting to auto-detect:");

        if (fixWyckoff(testStruc)) {
            reportString += StrucImportBgmnMessages::showSuccess(QString("    --> Found valid Wyckoff symmetries for all atoms.<br>"));

            double minDistRatio = checkBondLengths(testStruc, atomCollection);
            testStruc.setAuxInfo("MinDistRatio", minDistRatio);
            // we want to identify the structure with minDistRatio closest to 1.0. With this key, it will be the first in the map
            multSettings.insert(ulong(10000.0 * fabs(minDistRatio - 1.0)), testStruc);
        } else {
            reportString += StrucImportBgmnMessages::showWarning(QString("    --> Non-matching Wyckoff symbols detected. Discarded.<br>"));
        }
    }

    if (multSettings.size() == 0) {
        qDebug() << QString("StrucImportBgmnFixes::identifyBgmnSetting(): Error detecting space group setting. Falling back to the first matching setting:");
        qDebug() << QString(cStruc.auxInfo("BgmnSettingsLine").toString());

        reportString += StrucImportBgmnMessages::showError("Error detecting space group setting. Falling back to the first matching setting, please verify:");
        reportString += StrucImportBgmnMessages::showError(cStruc.auxInfo("BgmnSettingsLine").toString() + "<br>");

        cStruc.unitCell().setSpaceGroupHMBgmn(sgl.first().HermannMauguin);
        cStruc.setAuxInfo("BgmnSettingsLine", sgl.first().settingLine);
        parseBgmnSettingsLine(cStruc);
        fixWyckoff(cStruc);
        checkBondLengths(cStruc, atomCollection);
    } else if (multSettings.size() == 1) {
        cStruc = multSettings.first();

        qDebug() << QString("StrucImportBgmnFixes::identifyBgmnSetting(): Automatic identification of space group setting successful:");
        qDebug() << QString(cStruc.auxInfo("BgmnSettingsLine").toString());

        reportString += StrucImportBgmnMessages::showSuccess("Automatic identification of space group setting successful:");
        reportString += StrucImportBgmnMessages::showSuccess(cStruc.auxInfo("BgmnSettingsLine").toString() + "<br>");
    } else {
        QList<CrystalStructure> bestMatches = multSettings.values(multSettings.firstKey());

        if (bestMatches.size() == 1) {
            cStruc = bestMatches.first();

            qDebug() << QString("StrucImportBgmnFixes::identifyBgmnSetting(): Automatic identification of space group setting successful:");
            qDebug() << QString(cStruc.auxInfo("BgmnSettingsLine").toString());

            reportString += StrucImportBgmnMessages::showSuccess("Automatic identification of space group setting successful:");
            reportString += StrucImportBgmnMessages::showSuccess(cStruc.auxInfo("BgmnSettingsLine").toString() + "<br>");
        } else {
            int firstSettingIdx = 0;
            int prevSetting = std::numeric_limits<int>::max();
            int curSetting = std::numeric_limits<int>::max();

            // find the first setting among the best matches
            for (int i = 0; i < bestMatches.size(); ++i) {
                curSetting = bestMatches.at(i).auxInfo("BgmnSetting").toInt();

                if (curSetting < prevSetting) {
                    firstSettingIdx = i;
                    prevSetting = curSetting;
                }
            }

            qDebug() << QString("Selected: i=%1").arg(firstSettingIdx);
            cStruc = bestMatches.at(firstSettingIdx);

            qDebug() << QString("StrucImportBgmnFixes::identifyBgmnSetting(): Automatic identification of space group setting found multiple matches:");
            reportString += StrucImportBgmnMessages::showWarning("Automatic identification of space group setting found multiple matches:");

            for (int i = bestMatches.size() - 1; i >= 0; --i) {
                QString sline = bestMatches.at(i).auxInfo("BgmnSettingsLine").toString();
                double mdratio = bestMatches.at(i).auxInfo("MinDistRatio").toDouble();

                qDebug() << QString(sline);
                reportString += StrucImportBgmnMessages::showWarning(QString("%2 (%1)").arg(mdratio, 0, 'f', 6).arg(sline));
            }

            qDebug() << QString("    Selecting the following setting:");
            qDebug() << cStruc.auxInfo("BgmnSettingsLine").toString();

            reportString += StrucImportBgmnMessages::showComment("<br>Interatomic distances are identical. Selecting the first matching setting:");
            reportString += StrucImportBgmnMessages::showComment(cStruc.auxInfo("BgmnSettingsLine").toString() + "<br>");
        }
    }
}

QList<BgmnSpaceGroup> StrucImportBgmnFixes::preFilterMatches(const QList<BgmnSpaceGroup> &sgl, const CrystalStructure &str)
{
    qDebug() << QString("StrucImportBgmnFixes::preFilterMatches(): Filtering by HM symbols and unit cell");

    QList<BgmnSpaceGroup> matchingSgl;
    QString bravaisStr = determineBravaisType(str.unitCell());

    reportString += StrucImportBgmnMessages::showComment("Filtering by Bravais type:");
    reportString += StrucImportBgmnMessages::showComment(QString("    Imported structure has Bravais type: %1").arg(bravaisStr));

    for (int i = 0; i < sgl.size(); ++i) {
        QString bravaisSgl = sgl.at(i).HermannMauguin.left(1).toUpper() + ";";

        if      (sgl.at(i).lattice.toLower() == "trigonal")  bravaisSgl += "trighexagonal;";
        else if (sgl.at(i).lattice.toLower() == "hexagonal") bravaisSgl += "trighexagonal;";
        else                                                 bravaisSgl += sgl.at(i).lattice.toLower() + ";";

        bravaisSgl += sgl.at(i).uniqueAxis.toLower();

        qDebug() << QString("    Comparing strings %1 = %2").arg(bravaisStr, bravaisSgl);

        if (bravaisSgl == bravaisStr) {
            matchingSgl.append(sgl.at(i));
            QString dbgStr = QString("    SpacegroupNo %1 Setting %2 (%3) matched: %4")
                    .arg(sgl.at(i).itNumber).arg(sgl.at(i).setting)
                    .arg(sgl.at(i).HermannMauguin, bravaisSgl);
            reportString += StrucImportBgmnMessages::showSuccess(dbgStr);
        } else {
            QString dbgStr = QString("    SpacegroupNo %1 Setting %2 (%3) discarded: %4")
                     .arg(sgl.at(i).itNumber).arg(sgl.at(i).setting)
                     .arg(sgl.at(i).HermannMauguin, bravaisSgl);
            reportString += StrucImportBgmnMessages::showWarning(dbgStr);
        }
    }

    if (matchingSgl.size()) {
        reportString += StrucImportBgmnMessages::showSuccess(QString("--> Found %1 matching settings").arg(matchingSgl.size()));
    } else {
        reportString += StrucImportBgmnMessages::showWarning("--> No matching settings found. Skipping this test.");
    }

    reportString += "<br>";
    return matchingSgl.size() ? matchingSgl : sgl;
}

QList<BgmnSpaceGroup> StrucImportBgmnFixes::preFilterSymops(const QList<BgmnSpaceGroup> &sgl, const CrystalStructure &str)
{
    qDebug() << QString("StrucImportBgmnFixes::preFilterSymops(): Filtering by symmetry operands");
    reportString += StrucImportBgmnMessages::showComment("Filtering by symmetry operators:");

    QList<CrystalSymOp> cifSops = str.unitCell().symmetryOperationMatrices();

    if (!cifSops.size()) {
        qDebug() << QString("    No symmetry operands found in crystal structure. Skipping filtering.");
        reportString += StrucImportBgmnMessages::showWarning("    Imported structure has no symmetry operators. Filtering skipped.<br>");
        return sgl;
    }

    QList<BgmnSpaceGroup> matchingSgl;

    for (int i = 0; i < sgl.size(); ++i) {
        // get all symop strings from sgdat, including translated ones
        QList<CrystalSymOp> bgmnSops = sgParser->getSymmetryOperations(sgl.at(i).itNumber, sgl.at(i).setting);

        qDebug() << QString("    BGMN SpacegroupNo %1 Setting %2 contains %3 operands")
                    .arg(sgl.at(i).itNumber).arg(sgl.at(i).setting).arg(bgmnSops.size());

        // if the number of symOps is not the same, we don't even need to compare the operators
        if (bgmnSops.size() != cifSops.size()) {
            QString dbgStr = QString("SpacegroupNo %1 Setting %2 (%3) has %4 symmetry operators, imported structure has %5. Discarded.")
                                     .arg(sgl.at(i).itNumber)
                                     .arg(sgl.at(i).setting)
                                     .arg(sgl.at(i).HermannMauguin)
                                     .arg(bgmnSops.size())
                                     .arg(cifSops.size());
            reportString += StrucImportBgmnMessages::showWarning(dbgStr);
            continue;
        }

        int match = 0;

        for (int j = 0; j < bgmnSops.size(); ++j) {
            for (int k = 0; k < cifSops.size(); ++k) {
                if (cifSops.at(k) == bgmnSops.at(j)) {
                    ++match;
                    break;
                }
            }
        }

        qDebug() << QString("    It num %1 setting %2 has %3 out of %4 matching symmetry operators.")
                    .arg(sgl.at(i).itNumber).arg(sgl.at(i).setting).arg(match).arg(cifSops.size());

        if (match == cifSops.size()) {
            matchingSgl.append(sgl.at(i));
            QString dbgStr = QString("SpacegroupNo %1 Setting %2 (%3) matches all %4 symmetry operators.")
                                     .arg(sgl.at(i).itNumber)
                                     .arg(sgl.at(i).setting)
                                     .arg(sgl.at(i).HermannMauguin)
                                     .arg(match);
            reportString += StrucImportBgmnMessages::showSuccess(dbgStr);
        } else {
            QString dbgStr = QString("SpacegroupNo %1 Setting %2 (%3) matches %4 out of %5 symmetry operators. Discarded.")
                                     .arg(sgl.at(i).itNumber)
                                     .arg(sgl.at(i).setting)
                                     .arg(sgl.at(i).HermannMauguin)
                                     .arg(match)
                                     .arg(cifSops.size());
            reportString += StrucImportBgmnMessages::showWarning(dbgStr);
        }
    }

    if (matchingSgl.size()) {
        reportString += StrucImportBgmnMessages::showSuccess(QString("--> Found %1 matching settings").arg(matchingSgl.size()));
    } else {
        reportString += StrucImportBgmnMessages::showWarning("--> No matching settings found. Skipping this test.");
    }

    reportString += "<br>";
    return matchingSgl.size() ? matchingSgl : sgl;
}

/*
 * returns a string describing the bravais type as follows:
 * T;system;u
 * where: T = translation symbol (upper case)
 *        system = crystal system "triclinic", "monoclinic", "orthorhombic", "tetragonal", "trighexagonal", "rhombohedral", "cubic"
 *        u = unique axis (lower case), empty in case of no unique axis
 */
QString StrucImportBgmnFixes::determineBravaisType(const CrystalUnitCell &uc)
{
    QString s = uc.spaceGroupHMCif().left(1).toUpper() + ";";

    int identAxes = countIdentAxes(uc.a(), uc.b(), uc.c());
    int angles90  = count90Angles(uc.alpha(), uc.beta(), uc.gamma());
    int angles120 = count120Angles(uc.alpha(), uc.beta(), uc.gamma());

    if ((identAxes == 0) && (angles90 == 0) && (angles120 == 0)) s += "triclinic;";
    if ((identAxes == 0) && (angles90 == 2) && (angles120 == 0)) s += "monoclinic;" + uniqueAxisNonOrthogonal(uc.alpha(), uc.beta(), uc.gamma());
    if ((identAxes == 0) && (angles90 == 3) && (angles120 == 0)) s += "orthorhombic;";
    if ((identAxes == 2) && (angles90 == 3) && (angles120 == 0)) s += "tetragonal;" + uniqueAxisOrthogonal(uc.a(), uc.b(), uc.c());
    if ((identAxes == 2) && (angles90 == 2) && (angles120 == 1)) s += "trighexagonal;" + uniqueAxisNonOrthogonal(uc.alpha(), uc.beta(), uc.gamma());
    if ((identAxes == 3) && (angles90 == 0) && (angles120 == 0)) s += "rhombohedral;";
    if ((identAxes == 3) && (angles90 == 3) && (angles120 == 0)) s += "cubic;";

    return s;
}

void StrucImportBgmnFixes::fixScatteringFactors(CrystalStructure &cStruc)
{
    QList<CrystalAtom> lAtoms = cStruc.atoms();
    checkIonicScatFactors(lAtoms);
    cStruc.setAtoms(lAtoms);
}

void StrucImportBgmnFixes::checkIonicScatFactors(QList<CrystalAtom> &l)
{
    QVector<QStringList> sfact(3, QStringList());
    bool allIonic = true;
    QString dbgStr;

    QStringList sfac = global::BgmnScatteringFactorSymbols;
    QRegularExpression rxIonic(QString("^(%1)(?:([1-5])?(\\+|-)([1-5])?)$").arg(global::rxElements));
    QRegularExpression rxAtomic(QString("^(%1)").arg(global::rxElements));
    static QRegularExpression rxFixOH("^O\\-?H");
    static QRegularExpression rxFixD("^D(?![BSY])");

    for (int i = 0; i < l.size(); ++i) {
        QString elStr = l.at(i).element().toUpper();
        QRegularExpressionMatch rmIonic = rxIonic.match(elStr);
        QRegularExpressionMatch rmAtomic = rxAtomic.match(elStr);

        if (elStr.left(2) == "WA") {
            dbgStr += QString("    %1: No scattering factor found for %2. "
                              "Assuming water molecule. Using O-2 instead. Please verify.<br>").arg(l.at(i).name(), 5).arg(elStr);
            appendScatFactors(sfact, "O-2", "O", l.at(i).element());
        } else if (elStr.contains(rxFixOH)) {
            dbgStr += QString("    %1: No scattering factor found for %2. "
                              "Assuming water molecule. Using O-2 instead. Please verify.<br>").arg(l.at(i).name(), 5).arg(elStr);
            appendScatFactors(sfact, "O-2", "O", l.at(i).element());
        } else if (elStr.contains(rxFixD)) {
            dbgStr += QString("    %1: No scattering factor found for %2. "
                              "Assuming deuterium. Using H instead. Please verify.<br>").arg(l.at(i).name(), 5).arg(elStr);
            allIonic = false; // no H+ available in BGMN
            appendScatFactors(sfact, "O-2", "O", l.at(i).element());
        } else if (rmIonic.hasMatch()) {
            // ionic ones can be of the following structure in CIF: NA+, NA+1, NA1+
            QString fAtom = rmIonic.captured(1);
            QString fIon = fAtom;
            QString fSgn = rmIonic.captured(3);
            QString fVal("1");

            if (!rmIonic.captured(2).isEmpty())      fVal = rmIonic.captured(2);
            else if (!rmIonic.captured(4).isEmpty()) fVal = rmIonic.captured(4);

            fIon += fSgn + fVal;

            if (sfac.contains(fIon)) {
                appendScatFactors(sfact, fIon, fAtom, QString());
            } else {
                dbgStr += QString("    %1: No ionic scattering factor found for %2. "
                          "Using neutral scattering factors for all atoms.<br>").arg(l.at(i).name(), 5).arg(fIon);
                allIonic = false;
                appendScatFactors(sfact, fAtom, fAtom, QString());
            }
        } else if (rmAtomic.hasMatch()) {
            QString fAtom = rmAtomic.captured(1);
            allIonic = false;
            appendScatFactors(sfact, fAtom, fAtom, QString());
        } else {
            dbgStr += QString("    %1: No scattering factor found for %2. "
                              "Falling back to element string. Please fix manually.<br>").arg(l.at(i).name(), 5).arg(elStr);
            appendScatFactors(sfact, l.at(i).element(), l.at(i).element(), "check element");
        }
    }

    for (int i = 0; i < l.size(); ++i) {
        l[i].setElement(allIonic ? sfact.at(0).at(i) : sfact.at(1).at(i));
        QString elCom = sfact.at(2).at(i);
        if (!elCom.isEmpty()) {
            QStringList com = l.at(i).auxInfo("bgmnEComment", QStringList()).toStringList();
            com.append(elCom);
            l[i].setAuxInfo("bgmnEComment", com);
        }
    }

    reportString += StrucImportBgmnMessages::showComment("Checking scattering factors:");

    if (!dbgStr.isEmpty()) {    
        reportString += StrucImportBgmnMessages::showWarning(dbgStr);
    } else {
        reportString += StrucImportBgmnMessages::showSuccess("--> all ok<br>");
    }
}

void StrucImportBgmnFixes::appendScatFactors(QVector<QStringList> &vec, const QString &ion, const QString &atm, const QString &com)
{
    if (vec.size() < 3) return;
    vec[0].append(ion);
    vec[1].append(atm);
    vec[2].append(com);
}

void StrucImportBgmnFixes::parseBgmnSettingsLine(CrystalStructure &cStruc)
{
    QString setLine = cStruc.auxInfo("BgmnSettingsLine").toString();

    static QRegularExpression rxHM("HermannMauguin=([a-zA-Z0-9/_\\-]+)");
    static QRegularExpression rxIT("SpacegroupNo=(\\d+)");
    static QRegularExpression rxST("Setting=(\\d+)");
    static QRegularExpression rxCC("CellChoice=(\\d+)");
    static QRegularExpression rxLT("Lattice=([a-zA-Z]+)");
    static QRegularExpression rxOC("OriginChoice=(\\d+)");
    static QRegularExpression rxUA("UniqueAxis=([a-c])");

    QRegularExpressionMatch rmHM = rxHM.match(setLine);
    QRegularExpressionMatch rmIT = rxIT.match(setLine);
    QRegularExpressionMatch rmST = rxST.match(setLine);
    QRegularExpressionMatch rmCC = rxCC.match(setLine);
    QRegularExpressionMatch rmLT = rxLT.match(setLine);
    QRegularExpressionMatch rmOC = rxOC.match(setLine);
    QRegularExpressionMatch rmUA = rxUA.match(setLine);

    cStruc.setAuxInfo("BgmnHermannMauguin", rmHM.hasMatch() ? rmHM.captured(1) : QVariant());
    cStruc.setAuxInfo("BgmnSpacegroupNo",   rmIT.hasMatch() ? rmIT.captured(1) : QVariant());
    cStruc.setAuxInfo("BgmnSetting",        rmST.hasMatch() ? rmST.captured(1) : QVariant());
    cStruc.setAuxInfo("BgmnCellChoice",     rmCC.hasMatch() ? rmCC.captured(1) : QVariant());
    cStruc.setAuxInfo("BgmnLattice",        rmLT.hasMatch() ? rmLT.captured(1) : QVariant());
    cStruc.setAuxInfo("BgmnOriginChoice",   rmOC.hasMatch() ? rmOC.captured(1) : QVariant());
    cStruc.setAuxInfo("BgmnUniqueAxis",     rmUA.hasMatch() ? rmUA.captured(1) : QVariant());

    qDebug() << QString("StrucImportBgmnFixes::parseBgmnSettingsLine(): HermannMauguin = %1").arg(cStruc.auxInfo("BgmnHermannMauguin").toString());
    qDebug() << QString("                                               SpacegroupNo   = %1").arg(cStruc.auxInfo("BgmnSpacegroupNo").toInt());
    qDebug() << QString("                                               Setting        = %1").arg(cStruc.auxInfo("BgmnSetting").toInt());
    qDebug() << QString("                                               CellChoice     = %1").arg(cStruc.auxInfo("BgmnCellChoice").toString());
    qDebug() << QString("                                               Lattice        = %1").arg(cStruc.auxInfo("BgmnLattice").toString());
    qDebug() << QString("                                               OriginChoice   = %1").arg(cStruc.auxInfo("BgmnOriginChoice").toString());
    qDebug() << QString("                                               UniqueAxis     = %1").arg(cStruc.auxInfo("BgmnUniqueAxis").toString());
}

bool StrucImportBgmnFixes::fixWyckoff(CrystalStructure &cStruc)
{
    int bgNum = cStruc.auxInfo("BgmnSpacegroupNo").toInt();
    int bgSet  = cStruc.auxInfo("BgmnSetting").toInt();
    QString bgHm = cStruc.auxInfo("BgmnHermannMauguin").toString();
    QString sLine = cStruc.auxInfo("BgmnSettingsLine").toString();

    wyckoffList = sgParser->getAllWyckoff(bgNum, bgSet);
    generalOperators = wyckoffList.last().operators;
    firstWycks = getFirstWyckoffs(wyckoffList);
    translation = bgHm.left(1).toUpper();
    errCount = 0;

    static QRegularExpression rx("Lattice=([A-Za-z]+)");
    QRegularExpressionMatch rm = rx.match(sLine);
    lattice = rm.captured(1);

    if (debugLevel > 1) {
        qDebug() << QString("StrucImportBgmnFixes::fixWyckoff(): Using symmetry operators (possibly without translation):");
        for (int i = 0; i < generalOperators.size(); ++i) {
            qDebug() << "    " << generalOperators.at(i).join(",");
        }
    }

    // loop over all atomic sites and determine the wyckoff symbol
    for (int i = 0; i < cStruc.atoms().size(); ++i) {
        cStruc.atoms().replace(i, detectAndCheckAtomWyckoff(cStruc.atoms().at(i)));
    }

    return errCount == 0;
}

CrystalAtom StrucImportBgmnFixes::detectAndCheckAtomWyckoff(const CrystalAtom &atm)
{
    if (firstWycks.isEmpty()) return atm;

    QString wyckSymb = atm.wyckoff();

    qDebug() << QString("StrucImportBgmnFixes::fixWyckoff(): Atom %1: Using general position %3 to create equivalent sites.")
                    .arg(atm.name(), firstWycks.lastKey());

    qDebug() << QString("    Current Coordinates: %1 %2 %3")
                .arg(atm.x())
                .arg(atm.y())
                .arg(atm.z());

    SymmetryMultiplier symeq(atm, generalOperators, translation, lattice);
    QList<CrystalAtom> symEqList = symeq.getEquivalents();

    CrystalAtom match;
    bool ok = false;
    QString dbgStr;

    if (!wyckSymb.isEmpty()) {
        // Wyckoff symbol was provided in CIF. Find symmetric equivalent of asymmetric unit.
        match = findFirstWyckoff(symEqList, firstWycks.value(wyckSymb), ok);
        dbgStr = ok ? "matching" : "not matching";

        if (!ok) qDebug() << QString("    Has illegal Wyckoff symbol %1. Attempting auto-detection.").arg(wyckSymb);
    }

    if (wyckSymb.isEmpty() || !ok) {
        // Either the Wyckoff symbol was not provided, or the identification of asymmetric unit position failed
        // (e.g. in case of an illegal / nonexistent Wyckoff symbol). Try to determine it based on the coordinates.
        match = detectWyckoff(symEqList, firstWycks, firstWycks.lastKey(), ok);
        dbgStr = ok ? "identified as" : "not matching";
    }

    QString dbgNme = QString("%1: (%2, %3, %4) ").arg(match.name()).arg(match.x(), 0, 'f', 6).arg(match.y(), 0, 'f', 6).arg(match.z(), 0, 'f', 6);
    QString dbgSym = QString(" Wyckoff %1 (%2)").arg(match.wyckoff(), firstWycks.value(match.wyckoff()).join(", "));

    if (ok) {
        atomCollection.append(symEqList);

        qDebug() << dbgNme + dbgStr + dbgSym;
        reportString += StrucImportBgmnMessages::showComment(dbgNme + dbgStr + dbgSym);
        return match;
    }

    errCount++;
    qDebug() << dbgNme + dbgStr + dbgSym;
    reportString += StrucImportBgmnMessages::showWarning(dbgNme + dbgStr + dbgSym);
    return atm;
}

QMap<QString, QStringList> StrucImportBgmnFixes::getFirstWyckoffs(const QList<global::WyckoffPosition> &wyckList)
{
    QMap<QString, QStringList> firstWycks;

    for (int i = 0; i < wyckList.size(); ++i) {
        firstWycks[wyckList.at(i).name] = wyckList.at(i).operators.first();
    }

    return firstWycks;
}

CrystalAtom StrucImportBgmnFixes::findFirstWyckoff(const QList<CrystalAtom> &atoms, const QStringList &wyckSymmetry, bool &ok)
{
    if (wyckSymmetry.size() < 3) {
        ok = false;
        return atoms.constFirst();
    }

    QJSEngine scriptEngine;

    for (int i = 0; i < atoms.size(); ++i) {
        const CrystalAtom *atm = &(atoms.at(i));
        scriptEngine.globalObject().setProperty("x", atm->x());
        scriptEngine.globalObject().setProperty("y", atm->y());
        scriptEngine.globalObject().setProperty("z", atm->z());

        double wx = scriptEngine.evaluate(wyckSymmetry[0]).toNumber();
        double wy = scriptEngine.evaluate(wyckSymmetry[1]).toNumber();
        double wz = scriptEngine.evaluate(wyckSymmetry[2]).toNumber();

        int errors = 0;
        errors += global::Functions::fuzzyCompareFractions(wx, atm->x()) ? 0 : 1;
        errors += global::Functions::fuzzyCompareFractions(wy, atm->y()) ? 0 : 1;
        errors += global::Functions::fuzzyCompareFractions(wz, atm->z()) ? 0 : 1;

        if (!errors) {
            ok = true;
            return atoms.at(i);
        }
    }

    ok = false;
    return atoms.constFirst();
}

CrystalAtom StrucImportBgmnFixes::detectWyckoff(const QList<CrystalAtom> &atoms, const QMap<QString, QStringList> &wyckoffList, const QString &general, bool &ok)
{
    QMapIterator<QString, QStringList> it(wyckoffList);

    while (it.hasNext()) {
        it.next();

        bool wok;
        CrystalAtom natm = findFirstWyckoff(atoms, it.value(), wok);

        if (wok) {
            ok = true;
            natm.setWyckoff(it.key());
            int err = checkGeneralWyckoffForSpecial(natm, general);

            if (err > 0) {
                QStringList beComm = natm.auxInfo("bgmnEComment", QStringList()).toStringList();
                beComm.append("check Wyckoff");
                natm.setAuxInfo("bgmnEComment", beComm);
            }

            return natm;
        }
    }

    ok = false;
    return atoms.first();
}

int StrucImportBgmnFixes::checkGeneralWyckoffForSpecial(const CrystalAtom &atm, const QString &genSymb)
{
    if (atm.wyckoff().toLower() != genSymb.toLower()) {
        qDebug() << QString("    Position %1 was identified as special").arg(atm.name());
        return 0;
    } else {
        qDebug() << QString("    Position %1 was identified as general. Has symbol %2, general symbol is %3").arg(atm.name(), atm.wyckoff(), genSymb);
    }

    QList<double> coords;
    coords << atm.x() << atm.y() << atm.z();

    int special = 0;

    for (int i = 0; i < coords.size(); ++i) {
        if (global::Functions::isSpecialCoordinate(coords.at(i))) {
            special++;
        }
    }

    if (qFuzzyCompare(atm.x(), atm.y())) special++;
    if (qFuzzyCompare(atm.x(), atm.z())) special++;
    if (qFuzzyCompare(atm.y(), atm.z())) special++;

    if (special > 0) {
        qDebug() << QString("    Warning: Atom %1 was identified as a general position, "
                            "but seems to have special coordinates. Please check.").arg(atm.name());

        reportString += StrucImportBgmnMessages::showWarning(QString("Warning: Atom was %1 identified as a general position, "
                                                                     "but seems to have special coordinates. Please check.").arg(atm.name()));
    }

    return special;
}

/*
 * Attention: Returns the number of identical axes. This can be 0, 2, or 3.
 * It will never return 1.
 */
int StrucImportBgmnFixes::countIdentAxes(const CrystalStructure &str)
{
    return countIdentAxes(str.unitCell().a(), str.unitCell().b(), str.unitCell().c());
}

int StrucImportBgmnFixes::countIdentAxes(double a, double b, double c)
{
    int n = 0;

    if (qFuzzyCompare(a, b)) n++;
    if (qFuzzyCompare(a, c)) n++;
    if (qFuzzyCompare(b, c)) n++;

    if (n == 1) return 2;   // 1 matching pair = 2 identical axes
    if (n >= 2) return 3;   // if 2 pairs are matching, the 3rd must be matching, too = 3 identical axes
    return 0;               // 0 matching pair = 0 identical axes
}

int StrucImportBgmnFixes::count90Angles(const CrystalStructure &str)
{
    return count90Angles(str.unitCell().alpha(), str.unitCell().beta(), str.unitCell().gamma());
}

int StrucImportBgmnFixes::count90Angles(double al, double be, double ga)
{
    int n = 0;

    if (qFuzzyCompare(al, 90.0)) n++;
    if (qFuzzyCompare(be, 90.0)) n++;
    if (qFuzzyCompare(ga, 90.0)) n++;

    return n;
}

int StrucImportBgmnFixes::count120Angles(const CrystalStructure &str)
{
    return count120Angles(str.unitCell().alpha(), str.unitCell().beta(), str.unitCell().gamma());
}

int StrucImportBgmnFixes::count120Angles(double al, double be, double ga)
{
    int n = 0;

    if (qFuzzyCompare(al, 120.0)) n++;
    if (qFuzzyCompare(be, 120.0)) n++;
    if (qFuzzyCompare(ga, 120.0)) n++;

    return n;
}

QString StrucImportBgmnFixes::uniqueAxisNonOrthogonal(const CrystalStructure &str)
{
    return uniqueAxisNonOrthogonal(str.unitCell().alpha(), str.unitCell().beta(), str.unitCell().gamma());
}

QString StrucImportBgmnFixes::uniqueAxisNonOrthogonal(double al, double be, double ga)
{
    if (qFuzzyCompare(al, 90.0) && qFuzzyCompare(be, 90.0) && !qFuzzyCompare(ga, 90.0)) return QString("c");
    if (qFuzzyCompare(al, 90.0) && qFuzzyCompare(ga, 90.0) && !qFuzzyCompare(be, 90.0)) return QString("b");
    if (qFuzzyCompare(be, 90.0) && qFuzzyCompare(ga, 90.0) && !qFuzzyCompare(al, 90.0)) return QString("a");

    return QString();
}

QString StrucImportBgmnFixes::uniqueAxisOrthogonal(const CrystalStructure &str)
{
    return uniqueAxisOrthogonal(str.unitCell().a(), str.unitCell().b(), str.unitCell().c());
}

QString StrucImportBgmnFixes::uniqueAxisOrthogonal(double a, double b, double c)
{
    if (qFuzzyCompare(a, b) && !qFuzzyCompare(a, c) && !qFuzzyCompare(b, c)) return QString("c");
    if (!qFuzzyCompare(a, b) && qFuzzyCompare(a, c) && !qFuzzyCompare(b, c)) return QString("b");
    if (!qFuzzyCompare(a, b) && !qFuzzyCompare(a, c) && qFuzzyCompare(b, c)) return QString("a");

    return QString();
}

double StrucImportBgmnFixes::checkBondLengths(const CrystalStructure &cs, const QList<CrystalAtom> &al)
{
    QMatrix4x4 m = getF2Cmatrix(cs.unitCell());
    QList<QList<QVector4D> > cartAtoms;

    for (int i = 0; i < al.size(); ++i) {
        cartAtoms.append(fracToCartesian(al.at(i), m, true));
    }

    QList<BondPair> dstList;

    for (int i = 0; i < al.size(); ++i) {
        double minD = std::numeric_limits<double>().max();
        int minJ = 0;

        for (int j = 0; j < al.size(); ++j) {
            if (al.at(i).occupancy() + al.at(j).occupancy() < 1.01) { // don't compare with 1.0 because of rounding errors
                // both are partially occupied and may thus be closer than the bond distance, so we skip them
                continue;
            }

            for (int k = 0; k < cartAtoms.at(j).size(); ++k) {
                double d = cartAtoms.at(i).first().toVector3D().distanceToPoint(cartAtoms.at(j).at(k).toVector3D());

                if ((d < minD) && (!qFuzzyIsNull(d))) {
                    minD = d;
                    minJ = j;
                }
            }
        }

        if (!qFuzzyCompare(minD, std::numeric_limits<double>().max())) {
            BondPair bp(al.at(i), al.at(minJ), minD);
            if (!dstList.contains(bp)) dstList.append(bp);
        }
    }

    return compareBondLengths(dstList);
}

/*
 *  http://www.ruppweb.org/Xray/tutorial/Coordinate%20system%20transformation.htm
 */
QMatrix4x4 StrucImportBgmnFixes::getF2Cmatrix(const CrystalUnitCell &uc)
{
    float data[16] = {0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 1.0};

    // nm to Angstrom
    double a = uc.a() * 10.0;
    double b = uc.b() * 10.0;
    double c = uc.c() * 10.0;

    // cosines and sines
    double ca = qFuzzyCompare(uc.alpha(), 90.0) ? 0.0 : cos(uc.alpha() * M_PI / 180.0);
    double cb = qFuzzyCompare(uc.beta(),  90.0) ? 0.0 : cos(uc.beta()  * M_PI / 180.0);
    double cc = qFuzzyCompare(uc.gamma(), 90.0) ? 0.0 : cos(uc.gamma() * M_PI / 180.0);
    double sc = qFuzzyCompare(uc.gamma(), 90.0) ? 1.0 : sin(uc.gamma() * M_PI / 180.0);

    double v = a * b * c * sqrt(1.0 - pow(ca, 2.0) - pow(cb, 2.0) - pow(cc, 2.0)
                   + 2.0 * ca * cb * cc);

    // write the matrix elements
    data[0] = float(a);
    data[1] = float(b * cc);
    data[2] = float(c * cb);
    data[5] = float(b * sc);
    data[6] = float(c * (ca - cb * cc) / sc);
    data[10] = float(v / (a * b * sc));

    QMatrix4x4 m(data);
    return m;
}

QList<QVector4D> StrucImportBgmnFixes::fracToCartesian(const CrystalAtom &at, const QMatrix4x4 &m, bool expand)
{
    QList<QVector4D> cartAtoms;
    cartAtoms.append(QVector4D(at.x(), at.y(), at.z(), 1.0) * m);
    qDebug() << QString("%1(%2): f = %3 %4 %5  c = %6 %7 %8")
                .arg(at.name(), at.element())
                .arg(at.x(), 0, 'f', 4)
                .arg(at.y(), 0, 'f', 4)
                .arg(at.z(), 0, 'f', 4)
                .arg(cartAtoms.constLast().x(), 0, 'f', 6)
                .arg(cartAtoms.constLast().y(), 0, 'f', 6)
                .arg(cartAtoms.constLast().z(), 0, 'f', 6);

    if (expand) {
        // generate atoms outside the unit cell in all 26 neighboring unit cells

        cartAtoms.append(QVector4D(at.x()+1.0, at.y()     , at.z()    , 1.0) * m);
        cartAtoms.append(QVector4D(at.x()    , at.y()+1.0 , at.z()    , 1.0) * m);
        cartAtoms.append(QVector4D(at.x()    , at.y()     , at.z()+1.0, 1.0) * m);

        cartAtoms.append(QVector4D(at.x()-1.0, at.y()     , at.z()    , 1.0) * m);
        cartAtoms.append(QVector4D(at.x()    , at.y()-1.0 , at.z()    , 1.0) * m);
        cartAtoms.append(QVector4D(at.x()    , at.y()     , at.z()-1.0, 1.0) * m);

        cartAtoms.append(QVector4D(at.x()+1.0, at.y()+1.0 , at.z()    , 1.0) * m);
        cartAtoms.append(QVector4D(at.x()    , at.y()+1.0 , at.z()+1.0, 1.0) * m);
        cartAtoms.append(QVector4D(at.x()+1.0, at.y()     , at.z()+1.0, 1.0) * m);

        cartAtoms.append(QVector4D(at.x()-1.0, at.y()-1.0 , at.z()    , 1.0) * m);
        cartAtoms.append(QVector4D(at.x()    , at.y()-1.0 , at.z()-1.0, 1.0) * m);
        cartAtoms.append(QVector4D(at.x()-1.0, at.y()     , at.z()-1.0, 1.0) * m);

        cartAtoms.append(QVector4D(at.x()+1.0, at.y()-1.0 , at.z()    , 1.0) * m);
        cartAtoms.append(QVector4D(at.x()    , at.y()+1.0 , at.z()-1.0, 1.0) * m);
        cartAtoms.append(QVector4D(at.x()+1.0, at.y()     , at.z()-1.0, 1.0) * m);

        cartAtoms.append(QVector4D(at.x()-1.0, at.y()+1.0 , at.z()    , 1.0) * m);
        cartAtoms.append(QVector4D(at.x()    , at.y()-1.0 , at.z()+1.0, 1.0) * m);
        cartAtoms.append(QVector4D(at.x()-1.0, at.y()     , at.z()+1.0, 1.0) * m);

        cartAtoms.append(QVector4D(at.x()+1.0, at.y()+1.0 , at.z()+1.0, 1.0) * m);
        cartAtoms.append(QVector4D(at.x()-1.0, at.y()+1.0 , at.z()+1.0, 1.0) * m);
        cartAtoms.append(QVector4D(at.x()+1.0, at.y()-1.0 , at.z()+1.0, 1.0) * m);
        cartAtoms.append(QVector4D(at.x()+1.0, at.y()+1.0 , at.z()-1.0, 1.0) * m);
        cartAtoms.append(QVector4D(at.x()-1.0, at.y()-1.0 , at.z()+1.0, 1.0) * m);
        cartAtoms.append(QVector4D(at.x()+1.0, at.y()-1.0 , at.z()-1.0, 1.0) * m);
        cartAtoms.append(QVector4D(at.x()-1.0, at.y()+1.0 , at.z()-1.0, 1.0) * m);
        cartAtoms.append(QVector4D(at.x()-1.0, at.y()-1.0 , at.z()-1.0, 1.0) * m);
    }

    return cartAtoms;
}

double StrucImportBgmnFixes::compareBondLengths(const QList<BondPair> &lst)
{
    reportString += StrucImportBgmnMessages::showComment("<br>Analyzing interatomic distances between nearest neighbors:");
    reportString += StrucImportBgmnMessages::showComment("(Ignoring pairs of partially occupied sites)");

    double minDist = 0.0;
    double minRef  = 0.0;

    for (int i = 0; i < lst.size(); ++i) {
        double dref = global::ionicRadii.value(lst.at(i).atomA.element()) + global::ionicRadii.value(lst.at(i).atomB.element());
        double d = lst.at(i).dist;
        double ratio = qFuzzyIsNull(dref) ? 0.0 : d / dref;

        if (ratio < 1.5) {
            minDist += d;
            minRef += dref;
        }

        QString msg;
        msg = QString("%1 - %2: %3 (ref: %4)").arg(lst.at(i).atomA.name(), lst.at(i).atomB.name()).arg(d, 0, 'f', 4).arg(dref);
        qDebug() << msg;

        if (ratio < 1.5) {
            int colR = int(180.0 * (ratio > 1.00 ? 0.0 : 1.0 - 4.0 * (ratio - 0.75)));
            int colG = int(180.0 * (ratio < 0.75 ? 0.0 : (ratio < 1.0 ? (4.0 * ratio) - 3.0 : 1.0 - 2.0 * (ratio - 1.0))));
            int colB = int(180.0 * (ratio < 1.00 ? 0.0 : 2.0 * (ratio - 0.5) - 1.0));

            QColor col(colR > 255 ? 255 : colR, colG > 255 ? 255 : colG, colB > 255 ? 255 : colB);
            reportString += QString("<font color=\"%1\">%2</font><br>").arg(col.name(), msg);
        }
    }

    if (!qFuzzyIsNull(minRef)) {
        reportString += StrucImportBgmnMessages::showComment(QString("--> Mean distance length ratio: %1<br>").arg(minDist / minRef, 0, 'f', 5));
        return minDist / minRef;
    }

    reportString += StrucImportBgmnMessages::showComment(QString("--> Mean distance length ratio: Error, no reference values found<br>"));
    return 0.0;
}
