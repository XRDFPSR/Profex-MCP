/***************************************************************************
                          bgmnsgdatparser.cpp  -  description
                             -------------------
    begin                : Thu May 17 19:10:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#include "bgmnsgdatparser.h"
#include "bgmnfileio.h"
#include "crystal/crystalsymop.h"
#include <QDebug>

BgmnSgDatParser::BgmnSgDatParser()
{
}

void BgmnSgDatParser::init()
{
    spaceGroupDat.setContent(BgmnFileIO::readTextFile(":/resources/spacegrp.xml"));
    parseXml(spaceGroupDat);
}

QList<BgmnSpaceGroup> BgmnSgDatParser::getSpaceGroup(const QString &hm, int it)
{
    qDebug() << QString("BgmnSgDatParser::getSpaceGroup(): Identifying BGMN space group for HM symbol %1 and IT number %2").arg(hm).arg(it);

    // we strip spaces and _ from the hm symbol. CIF files don't use _ for screw axes (21 instead of 2_1), but
    // RRUFF files do. BGMN uses _, but no spaces. The lowest common denominator to compare symbols
    // is after stripping spaces and _ from the input string (CIF or RRUFF) and BGMN string.
    QString hmStripped = hm;
    static QRegularExpression rxSpc("[ _]+");
    hmStripped.replace(rxSpc, "");
    // make sure the translation symbol is upper case but the rest lower case
    hmStripped = hmStripped.left(1).toUpper() + hmStripped.mid(1, -1).toLower();

    QList<ItNumSetting>   matchedSettings;
    QList<BgmnSpaceGroup> matchedSpaceGroups;

    if (hmFull.contains(hmStripped)) {
        matchedSettings = hmFull.value(hmStripped);
        qDebug() << QString("    Found %1 settings matching the full HM symbol %2").arg(matchedSettings.size()).arg(hmStripped);
    } else if (hmShort.contains(hmStripped)) {
        matchedSettings = hmShort.value(hmStripped);
        qDebug() << QString("    Found %1 settings matching the short HM symbol %2").arg(matchedSettings.size()).arg(hmStripped);
    } else {
        qDebug() << QString("    HM symbol %1 was not found in SPACEGRP.DAT").arg(hmStripped);
    }

    for (int i = 0; i < matchedSettings.size(); ++i) {
        BgmnSpaceGroup bsg = sgMap[matchedSettings.at(i).itNum][matchedSettings.at(i).setting];

        if (it > 0) {
            if (bsg.itNumber == it) {
                matchedSpaceGroups.append(bsg);
                qDebug() << QString("    Found candidate: SpacegroupNo = %1 Setting = %2 HermannMauguin = %3").arg(bsg.itNumber).arg(bsg.setting).arg(bsg.HermannMauguin);
            }
        } else {
            matchedSpaceGroups.append(bsg);
            qDebug() << QString("    Found candidate: SpacegroupNo = %1 Setting = %2 HermannMauguin = %3").arg(bsg.itNumber).arg(bsg.setting).arg(bsg.HermannMauguin);
        }
    }

    qDebug() << QString("BgmnSgDatParser::getSpaceGroup(): Returning %1 matching BGMN space groups").arg(matchedSpaceGroups.size());
    return matchedSpaceGroups;
}

void BgmnSgDatParser::parseXml(const QDomDocument &d)
{
    sgMap.clear();
    sgSymOps.clear();

    QDomNodeList refSpGrList = d.elementsByTagName("spacegroup");
    QDomNodeList refSettingsList;

    for (int i = 0; i < refSpGrList.size(); ++i) {
        int itnum = refSpGrList.at(i).toElement().attribute("Number").toInt();

        refSettingsList = refSpGrList.at(i).toElement().elementsByTagName("setting");

        for (int j = 0; j < refSettingsList.size(); ++j) {
            BgmnSpaceGroup spgr;
            spgr.itNumber = itnum;
            spgr.setting = 1;
            spgr.uniqueAxis = QString();
            spgr.lattice = QString();
            spgr.cellChoice = 0;
            spgr.originChoice = 0;
            QString l;

            QDomElement n = refSettingsList.at(j).toElement();

            if (n.hasAttribute("HermannMauguin")) {
                l.append(QString("HermannMauguin=%1 ").arg(n.attribute("HermannMauguin")));
                spgr.HermannMauguin = n.attribute("HermannMauguin");
            }

            if (n.hasAttribute("Number")) {
                l.append(QString("Setting=%1 ").arg(n.attribute("Number")));
                spgr.setting = n.attribute("Number").toInt();
            }

            if (n.hasAttribute("CellChoice")) {
                l.append(QString("CellChoice=%1 ").arg(n.attribute("CellChoice")));
                spgr.cellChoice = n.attribute("CellChoice").toInt();
            }

            if (n.hasAttribute("OriginChoice")) {
                l.append(QString("OriginChoice=%1 ").arg(n.attribute("OriginChoice")));
                spgr.originChoice = n.attribute("OriginChoice").toInt();
            }

            if (n.hasAttribute("UniqueAxis")) {
                l.append(QString("UniqueAxis=%1 ").arg(n.attribute("UniqueAxis")));
                spgr.uniqueAxis = n.attribute("UniqueAxis");
            }

            if (n.hasAttribute("Lattice")) {
                l.append(QString("Lattice=%1 ").arg(n.attribute("Lattice")));
                spgr.lattice = n.attribute("Lattice");
            }

            // add a string of format:
            // "SpacegroupNo=n HermannMauguin=xxxx Setting=n CellChoice=n OriginChoice=n UniqueAxis=n Lattice=n"
            spgr.settingLine = QString("SpacegroupNo=%1 %2").arg(itnum).arg(l.trimmed());

            sgMap[spgr.itNumber][spgr.setting] = spgr;

            QDomElement elWyck = n.firstChildElement("wyckoff");
            QDomNodeList nWyckPos = elWyck.elementsByTagName("position");
            for (int k = 0; k < nWyckPos.size(); ++k) {
                QDomElement eWyckPos = nWyckPos.at(k).toElement();
                QStringList plst = eWyckPos.text().split(" ");
                sgSymOps[spgr.itNumber][spgr.setting].append(plst);
            }
        }
    }
}

QString BgmnSgDatParser::sgBgmnToCifFull(const QString &sgBgmn, bool withUnitSymbol, bool withSpaces)
{
    QRegularExpression rx("([ABCFIPR])"
                          "(-?[ABCDMNabcdmn12346](?:_[123])?(?:/[ABCDMNabcdmn])?)"
                          "(-?[ABCDMNabcdmn12346](?:_[123])?(?:/[ABCDMNabcdmn])?)?"
                          "(-?[ABCDMNabcdmn12346](?:_[123])?(?:/[ABCDMNabcdmn])?)?");

    QStringList sgCif;
    QRegularExpressionMatch rm = rx.match(sgBgmn);

    for (int j = 1; j < 5; ++j) {
        QString p = j <= rm.lastCapturedIndex() ? rm.captured(j) : QString();

        // convert 2_1/m to 21/m
        p.remove(QString("_"));

        // ignore positions "1" if requested
        if ((p.trimmed() == "1") && (!withUnitSymbol)) p = QString();

        sgCif << p;
    }

    return sgCif.join(withSpaces ? QString(" ") : QString()).simplified();
}

QString BgmnSgDatParser::sgBgmnToCifShort(const QString &sgBgmn, bool withUnitSymbol, bool withSpaces)
{
    QRegularExpression rx("([ABCFIPR])"
                          "(-?[ABCDMNabcdmn12346](?:_[123])?(?:/[ABCDMNabcdmn])?)"
                          "(-?[ABCDMNabcdmn12346](?:_[123])?(?:/[ABCDMNabcdmn])?)?"
                          "(-?[ABCDMNabcdmn12346](?:_[123])?(?:/[ABCDMNabcdmn])?)?");

    QRegularExpression rxshort("^\\d+/([ABCDMNabcdmn])");

    QStringList sgCif;
    QRegularExpressionMatch rm = rx.match(sgBgmn);

    for (int j = 1; j < 5; ++j) {
        QString p = j <= rm.lastCapturedIndex() ? rm.captured(j) : QString();

        // convert 2_1/m to 21/m
        p.remove(QString("_"));

        // convert 2/n to n
        QRegularExpressionMatch rmshort = rxshort.match(p);
        if (rmshort.hasMatch()) {
            p = rmshort.captured(1);
        }

        // ignore positions "1" if requested
        if ((p.trimmed() == "1") && (!withUnitSymbol)) p = QString();

        sgCif << p;
    }

    return sgCif.join(withSpaces ? QString(" ") : QString()).simplified();
}

QList<BgmnSpaceGroup> BgmnSgDatParser::checkForPerfectSGMatch(const QMap<int, BgmnSpaceGroup> &bgmnSgSymLst, const QString &cifSgSym)
{
    // warning: CIF space groups contain upper and lower case characters. Converting
    // all to upper case for comparison
    QString sgCif  = cifSgSym.simplified().toUpper();
    QString sgBgmn;
    bool dbg = false;

    QList<BgmnSpaceGroup> matches;

    // sg symbols from ICDD XML files don't contain spaces,
    // those from CIF files can (do?) contain spaces.
    // by checking for the presence of spaces, the number of
    // attempted conversions from BGMN format to other formats
    // for comparison can be reduced.

    if (sgCif.contains(QRegularExpression("\\s"))) {
        QMapIterator<int, BgmnSpaceGroup> it(bgmnSgSymLst);

        while (it.hasNext()) {
            it.next();

            if (it.value().HermannMauguin.left(1).toUpper() != sgCif.left(1).toUpper()) {
                continue;
            }

            sgBgmn = sgBgmnToCifFull(it.value().HermannMauguin, true, true).toUpper();
            if (dbg) qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Comparing %1 with %2").arg(sgCif).arg(sgBgmn);
            if (sgBgmn == sgCif) {
                qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Found space group match %1").arg(sgBgmn);
                matches.append(it.value());
                continue;
            }

            sgBgmn = sgBgmnToCifShort(it.value().HermannMauguin, true, true).toUpper();
            if (dbg) qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Comparing %1 with %2").arg(sgCif).arg(sgBgmn);
            if (sgBgmn == sgCif) {
                qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Found space group match %1").arg(sgBgmn);
                matches.append(it.value());
                continue;
            }

            sgBgmn = sgBgmnToCifFull(it.value().HermannMauguin, false, true).toUpper();
            if (dbg) qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Comparing %1 with %2").arg(sgCif).arg(sgBgmn);
            if (sgBgmn == sgCif) {
                qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Found space group match %1").arg(sgBgmn);
                matches.append(it.value());
                continue;
            }

            sgBgmn = sgBgmnToCifShort(it.value().HermannMauguin, false, true).toUpper();
            if (dbg) qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Comparing %1 with %2").arg(sgCif).arg(sgBgmn);
            if (sgBgmn == sgCif) {
                qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Found space group match %1").arg(sgBgmn);
                matches.append(it.value());
                continue;
            }
        }
    } else {
        QMapIterator<int, BgmnSpaceGroup> it(bgmnSgSymLst);

        while (it.hasNext()) {
            it.next();

            sgBgmn = sgBgmnToCifFull(it.value().HermannMauguin, true, false).toUpper();
            if (dbg) qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Comparing %1 with %2").arg(sgCif).arg(sgBgmn);
            if (sgBgmn == sgCif) {
                qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Found space group match %1").arg(sgBgmn);
                matches.append(it.value());
                continue;
            }

            sgBgmn = sgBgmnToCifShort(it.value().HermannMauguin, true, false).toUpper();
            if (dbg) qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Comparing %1 with %2").arg(sgCif).arg(sgBgmn);
            if (sgBgmn == sgCif) {
                qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Found space group match %1").arg(sgBgmn);
                matches.append(it.value());
                continue;
            }

            sgBgmn = sgBgmnToCifFull(it.value().HermannMauguin, false, false).toUpper();
            if (dbg) qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Comparing %1 with %2").arg(sgCif).arg(sgBgmn);
            if (sgBgmn == sgCif) {
                qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Found space group match %1").arg(sgBgmn);
                matches.append(it.value());
                continue;
            }

            sgBgmn = sgBgmnToCifShort(it.value().HermannMauguin, false, false).toUpper();
            if (dbg) qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Comparing %1 with %2").arg(sgCif).arg(sgBgmn);
            if (sgBgmn == sgCif) {
                qDebug() << QString("BgmnSgDatParser::checkForPerfectSGMatch(): Found space group match %1").arg(sgBgmn);
                matches.append(it.value());
                continue;
            }
        }
    }

    // some debug output
    if (matches.size()) {
        qDebug() << QString("    Matched space groups:");

        for (int i = 0; i < matches.size(); ++i) {
            qDebug() << QString("        Nr %1 Setting %2 = %3")
                        .arg(matches.at(i).itNumber)
                        .arg(matches.at(i).setting)
                        .arg(matches.at(i).HermannMauguin);
        }
    }

    return matches;
}

/*
 * returns a map containing all Hermann Mauguin symbols and setting numbers
 * for space group no "itnum":
 *
 * <hm1>, <settingNo1>
 * <hm1>, <settingNo2>
 * <hm1>, <settingNo3>
 * <hm2>, <settingNo1>
 * <hm2>, <settingNo2>
 * <hm3>, <settingNo1>
 * ...
 */
QMultiMap<QString, int> BgmnSgDatParser::getAllBgmnHMSymbols(int itnum)
{
    QMultiMap<QString, int> m;

    QMap<int, BgmnSpaceGroup> map = sgMap[itnum];
    QMapIterator<int, BgmnSpaceGroup> it(map);

    while (it.hasNext()) {
        it.next();
        m.insert(it.value().HermannMauguin, it.value().setting);
    }

    return m;
}

QMap<int, QList<global::WyckoffPosition> > BgmnSgDatParser::getAllWyckoff(int it)
{
    QMap<int, QList<global::WyckoffPosition> > wyckoffSettingsMap;
    QList<int> settingsNumbers = getSpaceGroupSettings(it);

    for (int i = 0; i < settingsNumbers.size(); ++i) {
        wyckoffSettingsMap[settingsNumbers.at(i)] = getAllWyckoff(it, settingsNumbers.at(i));
    }

    return wyckoffSettingsMap;
}

QList<global::WyckoffPosition> BgmnSgDatParser::getAllWyckoff(int it, int setting)
{
    // we must make absolutely sure that the wyckoffs are returned in ascending order,
    // which is tricky because space group 47 contains "alpha" after "z". Relying on an
    // alphatetic map doesn't work.
    QMap<QString, int> sortOrder;
    sortOrder["a"] = 0;
    sortOrder["b"] = 1;
    sortOrder["c"] = 2;
    sortOrder["d"] = 3;
    sortOrder["e"] = 4;
    sortOrder["f"] = 5;
    sortOrder["g"] = 6;
    sortOrder["h"] = 7;
    sortOrder["i"] = 8;
    sortOrder["j"] = 9;
    sortOrder["k"] = 10;
    sortOrder["l"] = 11;
    sortOrder["m"] = 12;
    sortOrder["n"] = 13;
    sortOrder["o"] = 14;
    sortOrder["p"] = 15;
    sortOrder["q"] = 16;
    sortOrder["r"] = 17;
    sortOrder["s"] = 18;
    sortOrder["t"] = 19;
    sortOrder["u"] = 20;
    sortOrder["v"] = 21;
    sortOrder["w"] = 22;
    sortOrder["x"] = 23;
    sortOrder["y"] = 24;
    sortOrder["z"] = 25;
    sortOrder["alpha"] = 26;

    QMap<int, global::WyckoffPosition> sortedWyckoff;

    QDomNode settingNode;
    QRegularExpression rxSplit("\\s+");

    if (!getSpaceGroupSettingNode(it, setting, settingNode)) {
        return sortedWyckoff.values();
    }

    QDomNodeList refWyckoffList = settingNode.toElement().elementsByTagName("wyckoff");

    for (int i = 0; i < refWyckoffList.size(); ++i) {
        QDomNodeList refSymmList = refWyckoffList.at(i).toElement().elementsByTagName("position");

        global::WyckoffPosition wyckoff;
        wyckoff.name = refWyckoffList.at(i).toElement().attribute("Symbol");
        wyckoff.itNum = it;
        wyckoff.setting = setting;

        for (int j = 0; j < refSymmList.size(); ++j) {
            // make sure we read exactly 3 elements
            QStringList sm = refSymmList.at(j).toElement().text().split(rxSplit);
            if (sm.size() < 3) continue;

            QStringList smx = sm.mid(0, 3);
            if (refSymmList.at(j).toElement().attribute("Standard").toInt() == 1) {
                wyckoff.operators.prepend(smx);
            } else {
                wyckoff.operators.append(smx);
            }
        }

        sortedWyckoff[sortOrder[wyckoff.name]] = wyckoff;
    }

    return sortedWyckoff.values();
}

QMap<QString, int> BgmnSgDatParser::getAllMultiplicities(int it, int setting)
{
    QDomNode settingNode;
    QRegularExpression rxSplit("\\s+");

    if (!getSpaceGroupSettingNode(it, setting, settingNode)) {
        return QMap<QString, int>();
    }

    QDomNodeList refWyckoffList = settingNode.toElement().elementsByTagName("wyckoff");

    QMap<QString, int> wyckoffMap;

    for (int i = 0; i < refWyckoffList.size(); ++i) {
        QDomNodeList refSymmList = refWyckoffList.at(i).toElement().elementsByTagName("position");

        QString wyckoff = refWyckoffList.at(i).toElement().attribute("Symbol");
        int multi   = refWyckoffList.at(i).toElement().attribute("N").toInt();

        wyckoffMap[wyckoff] = multi;
    }

    return wyckoffMap;
}

bool BgmnSgDatParser::getSpaceGroupSettingNode(int it, int setting, QDomNode &node)
{
    QDomNodeList refSpGrList = spaceGroupDat.elementsByTagName("spacegroup");
    QDomNodeList refSettingsList;

    for (int i = 0; i < refSpGrList.size(); ++i) {
        if (refSpGrList.at(i).toElement().attribute("Number").toInt() == it) {
            refSettingsList = refSpGrList.at(i).toElement().elementsByTagName("setting");

            for (int j = 0; j < refSettingsList.size(); ++j) {
                if (refSettingsList.at(j).toElement().attribute("Number").toInt() == setting) {
                    node = refSettingsList.at(j);
                    return true;
                }
            }
        }
    }

    return false;
}

QList<int> BgmnSgDatParser::getSpaceGroupSettings(int it)
{
    return sgMap.value(it).keys();
}

QList<int> BgmnSgDatParser::getSpaceGroupSettings(int it, const QString &transl)
{
    QList<int> lst;

    QMapIterator<int, BgmnSpaceGroup> iter(sgMap.value(it));

    while (iter.hasNext()) {
        iter.next();

        if (iter.value().HermannMauguin.left(1).toUpper() == transl.toUpper()) {
            lst.append(iter.key());
        }
    }

    return lst;
}

QList<CrystalSymOp> BgmnSgDatParser::getSymmetryOperations(int it, int setting)
{
    QList<QStringList> soOrig;
    QList<CrystalSymOp> symOpsTrans;

    if (!sgMap.contains(it))                return QList<CrystalSymOp>();
    if (!sgMap.value(it).contains(setting)) return QList<CrystalSymOp>();

    BgmnSpaceGroup sg = sgMap.value(it).value(setting);
    QString tr = sg.HermannMauguin.left(1).toUpper();
    soOrig = sgSymOps.value(it).value(setting);

    if (tr == "R") {
        if (sg.lattice.toLower() == "trigonal") tr = "RHX";
        else                                    tr = "RRH";
    }

    for (int i = 0; i < soOrig.size(); ++i) {
        CrystalSymOp so(soOrig.at(i));
        QList<CrystalSymOp> strans = so.translate(tr);

        symOpsTrans.append(so);
        if (strans.size()) symOpsTrans.append(strans);
    }

    return symOpsTrans;
}

