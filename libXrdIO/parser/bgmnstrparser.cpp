/***************************************************************************
                          strparser.cpp  -  description
                             -------------------
    begin                : Mon May 20 14:00:00 CEST 2013
    copyright            : (C) 2013 by Nicola Doebelin
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

#include <QTextStream>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QJSEngine>
#include <QDebug>
#include "bgmnfileio.h"
#include "bgmnstrparser.h"
#include "structs.h"

BgmnStrParser::BgmnStrParser()
{
}

BgmnStrParser::BgmnStrParser(const QString &s)
{
    loadFile(s);
}

BgmnStrParser::BgmnStrParser(const QString &s, const QString &f)
{
    fileName = f;
    setContent(s);
}

bool BgmnStrParser::loadFile(const QString &s)
{
    QString str = BgmnFileIO::readTextFile(s);

    if (str.isEmpty()) {
        fileName.clear();
        content.clear();
        return false;
    }

    fileName = s;
    content = str.split(global::rxLineEnding);
    return true;
}

void BgmnStrParser::setContent(const QString &s)
{
    content = s.split(global::rxLineEnding);
}

bool BgmnStrParser::writeToFile(const QString &s)
{
    return BgmnFileIO::writeTextFile(s, content.join("\n"));
}

bool BgmnStrParser::saveFile()
{
    if (fileName.isEmpty()) {
        qDebug() << "StrParser::saveFile: no file name given. Cannot write file.";
        return false;
    }

    return writeToFile(fileName);
}

QString BgmnStrParser::getQuantGoal() const
{
    static QRegularExpression rxp("GOAL:([^=]+)=GEWICHT(?:\\s|\\*ifthenelse|\\*exp\\(my\\*d\\*3\\/4\\)|$)");
    QRegularExpressionMatch match;

    for (int i = 0; i < content.size(); ++i) {
        // check if it is a comment line. if yes, skip it
        if (content.at(i).trimmed().left(1) == "%") {
            continue;
        }

        match = rxp.match(content.at(i));
        if (match.hasMatch()) {
            return match.captured(1);
        }
    }

    return QString();
}

QMultiMap<int, CrystalAtom> BgmnStrParser::parseAtoms() const
{
    QMultiMap<int, CrystalAtom> atoms;
    QJSEngine sEngine;

    static QRegularExpression rxLine("\\bE=[^A-Z]*[A-Z]{1,2}[\\+0-9-]*");
    static QRegularExpression rxL("\\bE=(\\S+)\\s");
    static QRegularExpression rxE("([A-Z]{1,2})(?:[\\+-][0-5])?(\\([^\\)]\\))?");
    static QRegularExpression rxX("x=([^ _\\^]+)");
    static QRegularExpression rxY("y=([^ _\\^]+)");
    static QRegularExpression rxZ("z=([^ _\\^]+)");
    static QRegularExpression rxW("Wyckoff=([a-z])");
    static QRegularExpression rxT("TDS=(\\d+\\.?\\d*)");

    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch rmLine = rxLine.match(content.at(i));

        if (rmLine.hasMatch()) {
            double x = -1.0;
            double y = -1.0;
            double z = -1.0;
            QString w;
            double t = -1.0;

            QRegularExpressionMatch rmX = rxX.match(content.at(i));
            QRegularExpressionMatch rmY = rxY.match(content.at(i));
            QRegularExpressionMatch rmZ = rxZ.match(content.at(i));
            QRegularExpressionMatch rmW = rxW.match(content.at(i));
            QRegularExpressionMatch rmT = rxT.match(content.at(i));

            if (rmX.hasMatch()) x = sEngine.evaluate(rmX.captured(1)).toNumber();
            if (rmY.hasMatch()) y = sEngine.evaluate(rmY.captured(1)).toNumber();
            if (rmZ.hasMatch()) z = sEngine.evaluate(rmZ.captured(1)).toNumber();
            if (rmW.hasMatch()) w = rmW.captured(1);
            if (rmT.hasMatch()) t = rmT.captured(1).toDouble();

            QStringList elems;
            QRegularExpressionMatch rmL = rxL.match(content.at(i));

            if (rmL.hasMatch()) {
                elems = rmL.captured(1).split(",");

                for (int j = 0; j < elems.size(); ++j) {
                    QRegularExpressionMatch rmE = rxE.match(elems.at(j));

                    if (rmE.hasMatch()) {
                        QString e = rmE.captured(1);
                        QString o = rmE.captured(3);

                        bool occOk = true;
                        double occ = o.isEmpty() ? 1.0 : o.toDouble(&occOk);
                        if (!occOk) occ = 1.0; // if an equation is used for the occupancy, we fall back to 1.0

                        CrystalAtom a(e, x, y, z);
                        a.setWyckoff(w);
                        a.setOccupancy(occ);
                        a.setBiso(t);
                        atoms.insert(i, a);
                    }
                }
            }
        }
    }

    return atoms;
}

QString BgmnStrParser::getHermannMauguin() const
{
    static QRegularExpression rx("HermannMauguin=([^ ]+)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        rm = rx.match(content.at(i));
        if (rm.hasMatch()) return rm.captured(1);
    }

    return QString();
}


int BgmnStrParser::getSpacegroupNo() const
{
    static QRegularExpression rx("SpacegroupNo=(\\d+)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        rm = rx.match(content.at(i));
        if (rm.hasMatch()) return rm.captured(1).toInt();
    }

    return -1;
}

int BgmnStrParser::getSetting() const
{
    static QRegularExpression rx("Setting=(\\d+)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        rm = rx.match(content.at(i));
        if (rm.hasMatch()) return rm.captured(1).toInt();
    }

    return -1;
}

QString BgmnStrParser::getPhaseName() const
{
    static QRegularExpression rx("PHASE=(\\S+)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        rm = rx.match(content.at(i));
        if (rm.hasMatch()) return rm.captured(1);
    }

    return QString();

}

void BgmnStrParser::setRefinementState(const QString &par, int state)
{
    QStringList l = par.split(":");
    if (l.size() < 3) {
        qDebug() << QString("BgmnStrParser::setRefinementState(): Invalid parameter string. Skipping (key=\"%1\")").arg(par);
        return;
    }

    QString cat = l.at(0);
    QString var = l.at(1);
    double lim = l.at(2).isEmpty() ? 0.0 : l.at(2).toDouble();

    QString pttrFlt("[\\+-]?\\d+\\.?[E\\d\\+-]*");
    QString pttrPrm("(?:PARAM=|\\b)");
    QString pttrVal = QString("(%1|ANISO|ANISO4|SPHAR\\d)(?:[\\+-\\.\\d\\^]*\\S*)").arg(pttrFlt);

    QString nme;
    QString val;
    bool ok;

    QRegularExpression rx;
    QRegularExpressionMatch rm;

    if (cat == "UC") {
        if (var == "A")     rx.setPattern(QString("%1(A)=%2").arg(pttrPrm,     pttrVal));
        if (var == "B")     rx.setPattern(QString("%1(B)=%2").arg(pttrPrm,     pttrVal));
        if (var == "C")     rx.setPattern(QString("%1(C)=%2").arg(pttrPrm,     pttrVal));
        if (var == "ALPHA") rx.setPattern(QString("%1(ALPHA)=%2").arg(pttrPrm, pttrVal));
        if (var == "BETA")  rx.setPattern(QString("%1(BETA)=%2").arg(pttrPrm,  pttrVal));
        if (var == "GAMMA") rx.setPattern(QString("%1(GAMMA)=%2").arg(pttrPrm, pttrVal));
    }

    if (cat == "PR") {
        if (var == "GEWICHT") rx.setPattern(QString("%1(GEWICHT(?:\\[\\d+\\])?)=%2").arg(pttrPrm, pttrVal));
        if (var == "B1")      rx.setPattern(QString("%1(B1(?:\\[\\d+\\])?)=%2").arg(pttrPrm,      pttrVal));
        if (var == "k2")      rx.setPattern(QString("%1(k2(?:\\[\\d+\\])?)=%2").arg(pttrPrm,      pttrVal));
        if (var == "k1")      rx.setPattern(QString("%1(k1(?:\\[\\d+\\])?)=%2").arg(pttrPrm,      pttrVal));
    }

    if (cat == "AT") {
        if (var == "x")   rx.setPattern(QString("%1(x)=%2").arg(pttrPrm,   pttrVal));
        if (var == "y")   rx.setPattern(QString("%1(y)=%2").arg(pttrPrm,   pttrVal));
        if (var == "z")   rx.setPattern(QString("%1(z)=%2").arg(pttrPrm,   pttrVal));
        if (var == "TDS") rx.setPattern(QString("%1(TDS)=%2").arg(pttrPrm, pttrVal));
    }

    for (int i = 0; i < content.size(); ++i) {
        rm = rx.match(content.at(i));
        if (!rm.hasMatch()) continue;

        nme = rm.captured(1);
        val = rm.captured(2);

        double d = val.toDouble(&ok);
        if (!ok) d = 0.0;

        if (cat == "UC") {
            if (state == 0) content[i].replace(rx, QString("%1=%2").arg(nme, val));
            if (state >= 1) content[i].replace(rx, QString("PARAM=%1=%2_%3^%4").arg(nme).arg(val).arg((1.0 - lim) * d, 0, 'f', 4).arg((1 + lim) * d, 0, 'f', 4));
        }

        if (cat == "PR") {
            if (var == "GEWICHT") {
                if (state == 0) content[i].replace(rx, QString("%1=SPHAR0").arg(nme));
                if (state == 1) content[i].replace(rx, QString("%1=SPHAR2").arg(nme));
                if (state == 2) content[i].replace(rx, QString("%1=SPHAR4").arg(nme));
                if (state == 3) content[i].replace(rx, QString("%1=SPHAR6").arg(nme));
                if (state == 4) content[i].replace(rx, QString("%1=SPHAR8").arg(nme));
                if (state >= 5) content[i].replace(rx, QString("%1=SPHAR10").arg(nme));
            }

            if (var == "B1") {
                if (state == 0) content[i].replace(rx, QString("%1=%2").arg(nme).arg(d, 0, 'f', 4));
                if (state == 1) content[i].replace(rx, QString("PARAM=%1=%2_0.0^%3").arg(nme).arg(d, 0, 'f', 4).arg(lim));
                if (state >= 2) content[i].replace(rx, QString("%1=ANISO^%2").arg(nme).arg(lim));
            }

            if (var == "k2") {
                if (state == 0) content[i].replace(rx, QString("%1=%2").arg(nme).arg(d, 0, 'f', 6));
                if (state == 1) content[i].replace(rx, QString("PARAM=%1=%2_0.0^%3").arg(nme).arg(d, 0, 'f', 6).arg(lim));
                if (state >= 2) content[i].replace(rx, QString("%1=ANISO4^%2").arg(nme).arg(lim));
            }

            if (var == "k1") {
                if (state == 0) content[i].replace(rx, QString("%1=%2").arg(nme, val));
                if (state >= 1) content[i].replace(rx, QString("PARAM=%1=%2_0^1").arg(nme, val));
            }
        }

        if (cat == "AT") {
            if (var == "TDS") {
                if (state == 0) content[i].replace(rx, QString("%1=%2").arg(nme, val));
                if (state == 1) content[i].replace(rx, QString("PARAM=%1=%2_0.00^%3").arg(nme).arg(d, 0, 'f', 4).arg(lim));
                if (state >= 2) content[i].replace(rx, QString("%1=ANISO^%2").arg(nme).arg(lim));
            } else {
                if (state == 0) content[i].replace(rx, QString("%1=%2").arg(nme, val));
                if (state >= 1) content[i].replace(rx, QString("PARAM=%1=%2_%3^%4").arg(nme, val).arg(d - lim, 0, 'f', 4).arg(d + lim, 0, 'f', 4));
            }
        }
    }
}

QStringList BgmnStrParser::getElements() const
{
    QStringList el;
    QMultiMap<int, CrystalAtom> map = parseAtoms();

    QMultiMapIterator<int, CrystalAtom> it(map);

    while (it.hasNext()) {
        it.next();
        if (!el.contains(it.value().element().toUpper())) {
            el.append(it.value().element().toUpper());
        }
    }

    return el;
}

/*
 * processes an E=... line by adding code for a refined substitution
 */
QString BgmnStrParser::setSubstitution(const QString &l, bool &ok)
{
    QString s(l);
    static QRegularExpression rx("^E=([A-Z\\+\\-\\d]+)(?:\\((\\d\\.?\\d*)\\))?");
    QRegularExpressionMatch rm = rx.match(s);

    if (rm.hasMatch()) {
        QString el = rm.captured(1);
        double d = 1.0;

        if (rm.hasCaptured(2)) {
            d = rm.captured(2).toDouble();
        }

        QString nStr = QString("E=(%1(p),XX(%2-p)) PARAM=p=%2_0^%2").arg(el).arg(d);
        s.replace(rx, nStr);
        ok = true;
        return s;
    }

    ok = false;
    return l;
}

/*
 * processes an E=... line by removing code for a refined substitution
 */
QString BgmnStrParser::revertSubstutition(const QString &l, bool &ok)
{
    QString s(l);
    static QRegularExpression rx("^E=\\(([A-Z\\+\\-\\d]+)\\S+\\s+PARAM=\\S+=(\\d\\.?\\d*)\\S+");
    QRegularExpressionMatch rm = rx.match(s);

    if (rm.hasMatch()) {
        QString el = rm.captured(1);
        double d = 1.0;

        if (rm.hasCaptured(2)) {
            d = rm.captured(2).toDouble();
        }

        QString nStr = QString("E=%1").arg(el);
        if (!qFuzzyCompare(d, 1.0)) nStr += QString("(%1)").arg(d, 0, 'f', 4);
        s.replace(rx, nStr);
        ok = true;
        return s;
    }

    ok = false;
    return l;
}

/*
 * Processes an E=... line by settings all coordinates to refined.
 * e is the fraction of the value used for the lower and upper limit:
 * PARAM=x=d_d-e*d^d+e*d
 */
QString BgmnStrParser::allCoordinatesRefined(const QString &s, double e, bool &ok)
{
    QStringList content = s.split(global::rxLineEnding);
    static QRegularExpression rxE("^E=.+\\sWyckoff");
    static QRegularExpression rxX("(?<!=)(?:x=([\\d\\.E\\+\\-]+))(?=\\s|$)");
    static QRegularExpression rxY("(?<!=)(?:y=([\\d\\.E\\+\\-]+))(?=\\s|$)");
    static QRegularExpression rxZ("(?<!=)(?:z=([\\d\\.E\\+\\-]+))(?=\\s|$)");

    int n = 0;
    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch rm = rxE.match(content.at(i));
        if (!rm.hasMatch()) continue;

        QString l = content.at(i);

        rm = rxX.match(l);
        if (rm.hasMatch()) {
            double d = rm.captured(1).toDouble();
            l.replace(rxX, QString("PARAM=x=%1_%2^%3").arg(d, 0, 'f', 4).arg(d-e, 0, 'f', 4).arg(d+e, 0, 'f', 4));
            ++n;
        }

        rm = rxY.match(l);
        if (rm.hasMatch()) {
            double d = rm.captured(1).toDouble();
            l.replace(rxY, QString("PARAM=y=%1_%2^%3").arg(d, 0, 'f', 4).arg(d-e, 0, 'f', 4).arg(d+e, 0, 'f', 4));
            ++n;
        }

        rm = rxZ.match(l);
        if (rm.hasMatch()) {
            double d = rm.captured(1).toDouble();
            l.replace(rxZ, QString("PARAM=z=%1_%2^%3").arg(d, 0, 'f', 4).arg(d-e, 0, 'f', 4).arg(d+e, 0, 'f', 4));
            ++n;
        }

        content[i] = l;
    }

    ok = n > 0;
    return content.join("\n");
}

/*
 * Processes an E=... line by settings all coordinates to fixed.
 */
QString BgmnStrParser::allCoordinatesFixed(const QString &s, bool &ok)
{
    QStringList content = s.split(global::rxLineEnding);
    static QRegularExpression rxE("^E=.+\\sWyckoff");
    static QRegularExpression rxX("PARAM=x=([\\d\\.E\\+\\-]+)\\S*");
    static QRegularExpression rxY("PARAM=y=([\\d\\.E\\+\\-]+)\\S*");
    static QRegularExpression rxZ("PARAM=z=([\\d\\.E\\+\\-]+)\\S*");

    int n = 0;
    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch rm = rxE.match(content.at(i));
        if (!rm.hasMatch()) continue;

        QString l = content.at(i);

        rm = rxX.match(l);
        if (rm.hasMatch()) {
            l.replace(rxX, QString("x=%1").arg(rm.captured(1)));
            ++n;
        }

        rm = rxY.match(l);
        if (rm.hasMatch()) {
            l.replace(rxY, QString("y=%1").arg(rm.captured(1)));
            ++n;
        }

        rm = rxZ.match(l);
        if (rm.hasMatch()) {
            l.replace(rxZ, QString("z=%1").arg(rm.captured(1)));
            ++n;
        }

        content[i] = l;
    }

    ok = n > 0;
    return content.join("\n");
}

/*
 * Processes an E=... line by settings all tds to refined
 * e is the upper limit. Anisotropic values are ignored.
 */
QString BgmnStrParser::allTdsRefined(const QString &s, double e, bool &ok)
{
    QStringList content = s.split(global::rxLineEnding);
    static QRegularExpression rxE("^E=.+\\sWyckoff");
    static QRegularExpression rxT("(?<!=)(?:TDS=([\\d\\.E\\+\\-]+))(?=\\s|$)");

    int n = 0;
    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch rm = rxE.match(content.at(i));
        if (!rm.hasMatch()) continue;

        QString l = content.at(i);

        rm = rxT.match(l);
        if (rm.hasMatch()) {
            double d = rm.captured(1).toDouble();

            // We can't use an upper limit that is smaller than the value. Make sure it is always greater.
            double ulim = e;
            while (ulim < d) ulim += e;

            l.replace(rxT, QString("PARAM=TDS=%1_%2^%3").arg(d, 0, 'f', 4).arg(double(0), 0, 'f', 0).arg(ulim, 0, 'f', 2));
            ++n;
        }

        content[i] = l;
    }

    ok = n > 0;
    return content.join("\n");}

/*
 * Processes an E=... line by settings all tds to fixed.
 * Anisotropic values are ignored.
 */
QString BgmnStrParser::allTdsFixed(const QString &s, bool &ok)
{
    QStringList content = s.split(global::rxLineEnding);
    static QRegularExpression rxE("^E=.+\\sWyckoff");
    static QRegularExpression rxT("PARAM=TDS=([\\d\\.E\\+\\-]+)\\S*");

    int n = 0;
    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch rm = rxE.match(content.at(i));
        if (!rm.hasMatch()) continue;

        QString l = content.at(i);

        rm = rxT.match(l);
        if (rm.hasMatch()) {
            l.replace(rxT, QString("TDS=%1").arg(rm.captured(1)));
            ++n;
        }

        content[i] = l;
    }

    ok = n > 0;
    return content.join("\n");
}

CrystalStructure BgmnStrParser::getCrystalStructure() const
{

    QString phase = getPhaseName();
    CrystalStructure structure(phase);
    CrystalUnitCell ucell = parseUnitCell();
    QList<CrystalAtom> atoms = parseAtoms().values();

    structure.setAtoms(atoms);
    structure.setUnitCell(ucell);
    return structure;
}

CrystalUnitCell BgmnStrParser::parseUnitCell() const
{
    const QString hm = getHermannMauguin();
    const int sgNo = getSpacegroupNo();
    const int setting = getSetting();

    double a  = 0.0;
    double b  = 0.0;
    double c  = 0.0;
    double al = 0.0;
    double be = 0.0;
    double ga = 0.0;

    static QRegularExpression rxA("[\\s=]A=(\\d+\\.?\\d*)");
    static QRegularExpression rxB("[\\s=]B=(\\d+\\.?\\d*)");
    static QRegularExpression rxC("[\\s=]C=(\\d+\\.?\\d*)");
    static QRegularExpression rxAl("[\\s=]ALPHA=(\\d+\\.?\\d*)");
    static QRegularExpression rxBe("[\\s=]BETA=(\\d+\\.?\\d*)");
    static QRegularExpression rxGa("[\\s=]GAMMA=(\\d+\\.?\\d*)");

    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        rm = rxA.match(content.at(i));
        if (rm.hasMatch()) a = rm.captured(1).toDouble();

        rm = rxB.match(content.at(i));
        if (rm.hasMatch()) b = rm.captured(1).toDouble();

        rm = rxC.match(content.at(i));
        if (rm.hasMatch()) c = rm.captured(1).toDouble();

        rm = rxAl.match(content.at(i));
        if (rm.hasMatch()) al = rm.captured(1).toDouble();

        rm = rxBe.match(content.at(i));
        if (rm.hasMatch()) be = rm.captured(1).toDouble();

        rm = rxGa.match(content.at(i));
        if (rm.hasMatch()) ga = rm.captured(1).toDouble();
    }

    CrystalUnitCell cell;
    cell.setItNumber(sgNo);
    cell.setSpaceGroupHMBgmn(hm);
    cell.setSettingNumber(setting);
    cell.setCell(a, b, c, al, be, ga);

    return cell;
}
