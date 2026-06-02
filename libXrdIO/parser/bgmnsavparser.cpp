/***************************************************************************
                          savparser.cpp  -  description
                             -------------------
    begin                : Fri Feb 03 11:00:00 CEST 2012
    copyright            : (C) 2012 by Nicola Doebelin
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
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>
#include <QThread>
#include <QDebug>

#include "bgmnsavparser.h"
#include "bgmnstrparser.h"
#include "bgmnfileio.h"
#include "structs.h"

/*
 * receive the content as a string. the name and location of the
 * control file name must also be known
 */
BgmnSavParser::BgmnSavParser(const QString &str, const QString &name)
{
    settings = SettingsManager::getInstance();

    qprefixRel = settings->value("bgmnProject/quantGoalRelPrefix", "Q").toString();
    sumvarRel  = settings->value("bgmnProject/quantSumRelPrefix",  "sum").toString();
    qprefixAbs = settings->value("bgmnProject/quantGoalAbsPrefix", "Qabs").toString();
    sumvarAbs  = settings->value("bgmnProject/quantSumAbsPrefix",  "sumabs").toString();

    fileName = name;
    setContent(str);
}

/*
 * read the content from file "file"
 */
BgmnSavParser::BgmnSavParser(const QString &file, bool *ok)
{
    settings = SettingsManager::getInstance();

    qprefixRel = settings->value("bgmnProject/quantGoalRelPrefix", "Q").toString();
    sumvarRel  = settings->value("bgmnProject/quantSumRelPrefix",  "sum").toString();
    qprefixAbs = settings->value("bgmnProject/quantGoalAbsPrefix", "Qabs").toString();
    sumvarAbs  = settings->value("bgmnProject/quantSumAbsPrefix",  "sumabs").toString();

    bool b = loadFile(file);
    if (ok) *ok = b;
}

bool BgmnSavParser::loadFile(const QString &file)
{
    fileName = file;
    QString s =BgmnFileIO::readTextFile(file);

    if (!s.isEmpty()) {
        setContent(s);
        return true;
    }

    return false;
}

void BgmnSavParser::setContent(const QString &s)
{
    rxFileNames = "([^$\\n\\r]*)";
    content = s.split(global::rxLineEnding);
}

void BgmnSavParser::setSampleId(const QString &s)
{
    static QRegularExpression rx("^[%\\s]*SampleID[:=\\s]*.*");

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).contains(rx)) {
            content[i] = QString("% SampleID: %1").arg(s);
            return;
        }
    }

    content.prepend(QString("% SampleID: %1").arg(s));
}

void BgmnSavParser::setPresetName(const QString &s)
{
    static QRegularExpression rx("^[%\\s]*Preset[:=\\s]*.*");

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).contains(rx)) {
            content[i] = QString("% Preset: %1").arg(s);
            return;
        }
    }

    content.prepend(QString("% Preset: %1").arg(s));
}

void BgmnSavParser::setDeviceFile(const QString &s)
{
    if (content.isEmpty()) {
        qDebug() << "SavParser::setDeviceFile failed: no savFile loaded";
        return;
    }

    QFileInfo fi(s);
    QRegularExpression rx(QString("\\bVERZERR=%1").arg(rxFileNames));

    int i = content.indexOf(rx, 0);

    if ((i >= 0) && (i < content.size())) {
        content[i] = "VERZERR=" + fi.fileName();
    }
}

void BgmnSavParser::addAmorphous(const QString &s)
{
    addStructureFiles(QStringList(s), false, false);
}

/*
 * Rewrites the STRUC[n] and GOAL[n] sections
 */
void BgmnSavParser::addStructureFiles(const QStringList &s, bool goals, bool multi)
{
    if (content.isEmpty()) {
        qDebug() << "BgmnSavParser::addStructureFiles(): failed, no savFile loaded";
        return;
    }

    // read existing STRUC files
    QStringList strFiles = getStruc();

    // the stringlist s contains full path plus file names, but here we only
    // want to append the file name without the path
    for (int i = 0; i < s.size(); ++i) {
        QFileInfo fi(s.at(i));
        strFiles.append(fi.fileName());
    }

    if (!multi) {
        strFiles.removeDuplicates();
    }

    // now clear all STRUC lines from the file, store the position where they
    // were located
    int l = clearPhases();

    // no previous STRUC lines? Try to locate "% Phases", else just append
    // the phases to the end of the file content
    if (l < 0) {
        if (content.contains("% Phases")) {
            l = content.indexOf("% Phases") + 1;
        } else {
            l = content.size();
        }
    }

    // write back the STRUC files
    for (int i = 0; i < strFiles.size(); ++i) {
        content.insert(l, QString("STRUC[%1]=%2").arg(i + 1).arg(strFiles.at(i)));
        ++l;
    }

    // now write the new GOAL[n] section
    if (goals) {
        writeGoals();
    }
}

/*
 * Accepts the *.str file name and removes the corresponding STRUC[n]=... entry
 * returns the number of removed structure files
 *
 * s = structure file name as specified in STRUC[]=
 * d = project directory
 * goals = manage goals flag
 *
 * also removes all STRUCOUT[n], SimpleSTRUCOUT[n], RESOUT[n], FCFOUT[n],
 * and PDBOUT[n] files
 *
 * this function does not delete the STR files. must be done
 * by the caller.
 */
int BgmnSavParser::removeStructureFile(const QString &s, const QString &d, bool goals)
{
    if (goals) {
        QString istd = internalStandard();

        if (!istd.isEmpty()) {
            BgmnStrParser strParser(d + "/" + s);

            if (strParser.getQuantGoal() == istd) {
                // if the phase to be removed is also declared as
                // internal standard, switch back to no internal standard
                unsetInternalStandard();
            }
        }
    }

    QRegularExpression rxPhase(QString("STRUC\\[(\\d+)\\]=%1").arg(s));
    QRegularExpressionMatch m;
    int indexToRemove = -1;

    // find the line at which rxStruc matches
    int i = content.indexOf(rxPhase, 0);

    // no match? return
    if (i < 0) return 0;

    // match the regular expression
    m = rxPhase.match(content.at(i));

    // extract n
    if (m.hasMatch()) {
        indexToRemove = m.captured(1).toInt();
    } else {
        return 0;
    }

    QRegularExpression rxStruc(QString("STRUC\\[%1\\]=.+").arg(indexToRemove));
    QRegularExpression rxStrOut(QString("STRUCOUT\\[%1\\]=.+").arg(indexToRemove));
    QRegularExpression rxSstrOut(QString("SimpleSTRUCOUT\\[%1\\]=.+").arg(indexToRemove));
    QRegularExpression rxResOut(QString("RESOUT\\[%1\\]=.+").arg(indexToRemove));
    QRegularExpression rxFcfOut(QString("FCFOUT\\[%1\\]=.+").arg(indexToRemove));
    QRegularExpression rxPdbOut(QString("PDBOUT\\[%1\\]=.+").arg(indexToRemove));

    // remove all lines with n=indexToRemove
    if (content.indexOf(rxStruc,   0) >= 0) content.removeAt(content.indexOf(rxStruc));
    if (content.indexOf(rxStrOut,  0) >= 0) content.removeAt(content.indexOf(rxStrOut));
    if (content.indexOf(rxSstrOut, 0) >= 0) content.removeAt(content.indexOf(rxSstrOut));
    if (content.indexOf(rxResOut,  0) >= 0) content.removeAt(content.indexOf(rxResOut));
    if (content.indexOf(rxFcfOut,  0) >= 0) content.removeAt(content.indexOf(rxFcfOut));
    if (content.indexOf(rxPdbOut,  0) >= 0) content.removeAt(content.indexOf(rxPdbOut));

    static QRegularExpression rxLut("STRUC\\[(\\d+)\\]=.+");
    QList<int> lut;

    for (int i = 0; i < content.size(); ++i) {
        m = rxLut.match(content.at(i));
        if (m.hasMatch()) lut.append(m.captured(1).toInt());
    }

    // renumber all entries with [n]
    for (int i = 0; i < lut.size(); ++i) {
        content.replaceInStrings(QString("STRUC[%1]=").arg(lut.at(i)), QString("STRUC[%1]=").arg(i+1), Qt::CaseSensitive);
        content.replaceInStrings(QString("STRUCOUT[%1]=").arg(lut.at(i)), QString("STRUCOUT[%1]=").arg(i+1), Qt::CaseSensitive);
        content.replaceInStrings(QString("SimpleSTRUCOUT[%1]=").arg(lut.at(i)), QString("SimpleSTRUCOUT[%1]=").arg(i+1), Qt::CaseSensitive);
        content.replaceInStrings(QString("RESOUT[%1]=").arg(lut.at(i)), QString("RESOUT[%1]=").arg(i+1), Qt::CaseSensitive);
        content.replaceInStrings(QString("FCFOUT[%1]=").arg(lut.at(i)), QString("FCFOUT[%1]=").arg(i+1), Qt::CaseSensitive);
        content.replaceInStrings(QString("PDBOUT[%1]=").arg(lut.at(i)), QString("PDBOUT[%1]=").arg(i+1), Qt::CaseSensitive);
    }

    // update the goals if requested
    if (goals) {
        writeGoals();
    }

    return 1;
}

/*
 * returns the sampleID string
 */
QString BgmnSavParser::sampleId() const
{
    static QRegularExpression rx("SampleID[:=]?\\s*(.*)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).contains(rx, &rm)) return rm.captured(1).trimmed();
    }

    return QString();
}

/*
 * returns the LAMBDA= string
 */
QString BgmnSavParser::getLambda() const
{
    static QRegularExpression rx("^\\s*LAMBDA=(\\S+)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).contains(rx, &rm)) return rm.captured(1);
    }

    return QString();
}

/*
 * returns the WMIN value as a double
 */
double BgmnSavParser::getWmin() const
{
    static QRegularExpression rx("^\\s*WMIN=(\\d\\.?\\d*)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).contains(rx, &rm)) return rm.captured(1).toDouble();
    }

    return -1.0;
}

/*
 * returns the WMAX value as a double
 */
double BgmnSavParser::getWmax() const
{
    static QRegularExpression rx("^\\s*WMAX=(\\d\\.?\\d*)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).contains(rx, &rm)) return rm.captured(1).toDouble();
    }

    return -1.0;
}

/*
 * returns the SYNCHROTRON value as a double
 */
double BgmnSavParser::getSynchrotron() const
{
    static QRegularExpression rx("^\\s*SYNCHROTRON=(\\d\\.?\\d*)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).contains(rx, &rm)) return rm.captured(1).toDouble();
    }

    return -1.0;
}

/*
 * returns the NEUTRON value as a double
 */
double BgmnSavParser::getNeutron() const
{
    static QRegularExpression rx("^\\s*NEUTRON=(\\d\\.?\\d*)");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).contains(rx, &rm)) return rm.captured(1).toDouble();
    }

    return -1.0;
}

/*
 * warning: Variable length patterns (e.g. \\s*) are not allowed in negative lookbehinds
 */
QStringList BgmnSavParser::getStruc(const QString &dir) const
{
    static QRegularExpression rx("^\\s*(?!%)STRUC\\[\\d+\\]=(.+\\.(str|STR))");
    return addDirToFiles(getAllMatches(rx), dir);
}

QStringList BgmnSavParser::getStrucOut(const QString &dir) const
{
    static QRegularExpression rx("^\\s*(?!%)STRUCOUT\\[\\d+\\]=(.+\\.(str|STR))");
    return addDirToFiles(getAllMatches(rx), dir);
}

QStringList BgmnSavParser::getSimpleStrucOut(const QString &dir) const
{
    static QRegularExpression rx("^\\s*(?!%)SimpleSTRUCOUT\\[\\d+\\]=(.+\\.(str|STR))");
    return addDirToFiles(getAllMatches(rx), dir);
}

QStringList BgmnSavParser::getResOut(const QString &dir) const
{
    static QRegularExpression rx("^\\s*(?!%)RESOUT\\[\\d+\\]=(.+\\.(res|RES))");
    return addDirToFiles(getAllMatches(rx), dir);
}

QStringList BgmnSavParser::getFcfOut(const QString &dir) const
{
    static QRegularExpression rx("^\\s*(?!%)FCFOUT\\[\\d+\\]=(.+\\.(fcf|FCF))");
    return addDirToFiles(getAllMatches(rx), dir);
}

QStringList BgmnSavParser::getPdbOut(const QString &dir) const
{
    static QRegularExpression rx("^\\s*(?!%)PDBOUT\\[\\d+\\]=(.+\\.(pdb|PDB))");
    return addDirToFiles(getAllMatches(rx), dir);
}

QStringList BgmnSavParser::addDirToFiles(const QStringList &l, const QString &d) const
{
    if (d.isEmpty()) return l;

    QStringList lout;
    for (int i = 0; i < l.size(); ++i) {
        lout.append(d + QDir::separator() + l.at(i));
    }

    return lout;
}

/*
 * returns all lines matching rx. Returned lines are trimmed.
 */
QStringList BgmnSavParser::getAllMatches(const QRegularExpression &rx, int m) const
{
    QRegularExpressionMatch match;
    QStringList lst;

    for (int i = 0; i < content.size(); ++i) {
        match = rx.match(content.at(i));
        // we found a matching line
        if (match.hasMatch()) {
            lst.append(match.captured(m).trimmed());
        }
    }

    return lst;
}

/*
 * QHash<STR file, Quant goal>
 */
QHash<QString, QString> BgmnSavParser::getAllStrucQuantGoals(const QString &dir) const
{
    QString fdir = dir;

    if (fdir.isEmpty()) {
        QFileInfo fi(fileName);
        fdir = fi.absolutePath();
    }

    QHash<QString, QString> map;
    QStringList strucs = getStruc(fdir);
    QStringList goals = getGoals();

    for (int i = 0; i < strucs.size(); ++i) {
        BgmnStrParser sparser(strucs.at(i));
        QString g = sparser.getQuantGoal();

        // here we must anticipate how the final quantity goal name in the sav file was constructed
        // from the quantity goal in the str file.
        // For example, in the str file the goal may be defined as:
        //
        //    GOAL:quartz=GEWICHT*ifthenelse(ifdef(d),exp(my*d*3/4),1) //
        //
        // so the GOAl name extracted from the str file would be "quartz". But in the sav file, the
        // final quantity goal may be any of:
        //
        //    GOAL[1]=Qquartz
        //    GOAL[1]=Qabsquartz
        //    GOAL[1]=Qrelquartz
        //    GOAL[1]=quartz/sum
        //
        // The following regexp must capture all those possible modifications from the str goal name.
        QRegularExpression rx(QString("((?:%1|%2)?%3(?:/sum)?)").arg(qprefixRel, qprefixAbs, g));
        int j = goals.indexOf(rx);

        if (j >= 0) {
            map[strucs.at(i)] = goals.at(j);
        } else {
            map[strucs.at(i)] = g;
        }
    }

    return map;
}

/*
 * QHash<STR file, Phase name>
 */
QHash<QString, QString> BgmnSavParser::getAllStrucPhaseNames(const QString &dir) const
{
    QString fdir = dir;

    if (fdir.isEmpty()) {
        QFileInfo fi(fileName);
        fdir = fi.absolutePath();
    }

    QHash<QString, QString> map;
    QStringList strucs = getStruc(fdir);

    for (int i = 0; i < strucs.size(); ++i) {
        BgmnStrParser sparser(strucs.at(i));
        map[strucs.at(i)] = sparser.getPhaseName();
    }

    return map;
}

/*
 * returns the number of lines containing STRUC[n]=s
 */
int BgmnSavParser::hasPhase(const QString &s) const
{
    // convert all strings ::toLower(), to remain case insensitive
    QRegularExpression rx(QString("struc\\[\\d+\\]=%1").arg(s.toLower()));
    QRegularExpressionMatch match;
    int n = 0;

    for (int i = 0; i < content.size(); ++i) {
        match = rx.match(content.at(i).toLower());
        // we found a STRUC[n]= line
        if (match.hasMatch()) {
            ++n;
        }
    }

    return n;
}

/*
 * removes all STRUC[n]=xxx.str lines from the content and returns the line
 * of the first entry. does not remove STRUCOUT, SimpleSTRUCOUT etc.
 */
int BgmnSavParser::clearPhases()
{
    static QRegularExpression rxStruc("STRUC\\[\\d+\\]=.+\\.(?:str|STR)");

    int i = content.indexOf(rxStruc);

    while (content.indexOf(rxStruc) >= 0) {
        content.removeAt(content.indexOf(rxStruc));
    }

    return i;
}

/*
 * writes the file to disk using 'fileName'
 */
bool BgmnSavParser::saveFile()
{
    return BgmnFileIO::writeTextFile(fileName, content.join("\n"));
}

/*
 * returns the entire content as a string
 */
QString BgmnSavParser::getContent() const
{
    return content.join("\n");
}

/*
 * constructs a block of GOAL phase quantities and appends it to content.
 * Removes the previous GOAL block from the sav file content first.
 */
void BgmnSavParser::writeGoals()
{
    qprefixRel = settings->value("bgmnProject/quantGoalRelPrefix", "Q").toString();
    sumvarRel  = settings->value("bgmnProject/quantSumRelPrefix",  "sum").toString();
    qprefixAbs = settings->value("bgmnProject/quantGoalAbsPrefix", "Qabs").toString();
    sumvarAbs  = settings->value("bgmnProject/quantSumAbsPrefix",  "sumabs").toString();

    static QRegularExpression rxIstd("^ISTD=(\\S+)");
    static QRegularExpression rxIstdq("^ISTDQ=(\\d+\\.?\\d*)");

    // buffer parameters we need later on
    QStringList phases(getAllPhaseNames());
    QStringList nonQuantGoals(getNonQuantificationGoals());
    QStringList listd(getAllMatches(rxIstd));
    QStringList listdq(getAllMatches(rxIstdq));

    QString istd  = listd.isEmpty()  ? QString() : listd.first();
    QString istdq = listdq.isEmpty() ? QString() : listdq.first();
    bool setIstd = !(istd.isEmpty() || istdq.isEmpty());

    removeSumsAndGoals();

    QStringList goalblock;
    int n = 0;

    if (setIstd) n = createQuantGoalAbs(phases, istd, false, goalblock);
    else         n = createQuantGoalRel(phases, goalblock);

    ++n;

    // append the non-managed goals
    for (int i = 0; i < nonQuantGoals.size(); ++i) {
        goalblock.append(QString("GOAL[%1]=%2").arg(n).arg(nonQuantGoals.at(i)));
        ++n;
    }

    content.append(QString());
    content.append(goalblock);
}

void BgmnSavParser::removeSumsAndGoals()
{
    if (!content.size()) return;

    // remove all 'sum=', 'sumrel=', and 'sumabs=' lines
    removeAllLines(QRegularExpression("^sum=\\S*$"));
    removeAllLines(QRegularExpression(QString("^%1=\\S*$").arg(sumvarRel)));
    removeAllLines(QRegularExpression(QString("^%1=\\S*$").arg(sumvarAbs)));

    // remove all 'QPhase=', 'QrelPhase=', and 'QabsPhase=' lines
    removeAllLines(QRegularExpression("^Q[^=]+=\\S+"));
    removeAllLines(QRegularExpression(QString("^%1[^=]+=[^\\/]+\\/%2").arg(qprefixRel, sumvarRel)));
    removeAllLines(QRegularExpression(QString("^%1[^=]+=[^\\/]+\\/%2").arg(qprefixAbs, sumvarAbs)));

    // remove all 'Amorph=' and 'QabsAmorph=' lines
    removeAllLines(QRegularExpression("^Amorph=\\S+"));
    removeAllLines(QRegularExpression(QString("^%1Amorph=\\S+").arg(qprefixAbs)));

    // remove all 'GOAL[n]=' lines
    removeAllLines(QRegularExpression("^GOAL\\[\\d*\\]=.+$"));

    // remove all empty lines at the end of the document
    while ((content.last().simplified().isEmpty()) && content.size()) {
        content.removeLast();
    }
}

/*
 * appends the goal code to goalblock, and returns the number of GOALs
 */
int BgmnSavParser::createQuantGoalRel(const QStringList &phases, QStringList &goalblock)
{
    if (phases.isEmpty()) return 0;
    bool qPercent = settings->value("bgmnProject/quantGoals100percent", false).toBool();

    goalblock.append(QString("%1=%2").arg(sumvarRel,
                                          phases.join("+")));

    for (int i = 0; i < phases.size(); ++i) {
        goalblock.append(QString("%1%2=%3%2/%4").arg(qprefixRel,
                                                     phases.at(i),
                                                     qPercent ? "100*" : QString(),
                                                     sumvarRel));
    }

    goalblock.append(QString());

    for (int i = 0; i < phases.size(); ++i) {
        goalblock.append(QString("GOAL[%1]=%2%3").arg(i+1).arg(qprefixRel,
                                                               phases.at(i)));
    }

    return phases.size();
}

/*
 * appends the goal code to goalblock, and returns the number of GOALs.
 * if 'addBoth = true', relative and absolute GOALs will be created
 */
int BgmnSavParser::createQuantGoalAbs(const QStringList &phases, const QString &istd, bool addBoth, QStringList &goalblock)
{
    if (phases.isEmpty()) return 0;
    bool qPercent = settings->value("bgmnProject/quantGoals100percent", false).toBool();

    QStringList amline;
    int n = 0;

    if (addBoth) {
        n = createQuantGoalRel(phases, goalblock);
        goalblock.append(QString());
    }

    if (qPercent) {
        goalblock.append(QString("%1=ISTD*(100-ISTDQ)/ISTDQ").arg(sumvarAbs));
    } else {
        goalblock.append(QString("%1=ISTD*(1-ISTDQ)/ISTDQ").arg(sumvarAbs));
    }

    for (int i = 0; i < phases.size(); ++i) {
        if (phases.at(i) == istd) continue;
        goalblock.append(QString("%1%2=%3%2/%4").arg(qprefixAbs,
                                                     phases.at(i),
                                                     qPercent ? "100*" : QString(),
                                                     sumvarAbs));
        amline.append(phases.at(i));
    }

    if (qPercent) {
        goalblock.append(QString("%1Amorph=100-100*(%2)/%3").arg(qprefixAbs,
                                                                 amline.join("+"),
                                                                 sumvarAbs));
    } else {
        goalblock.append(QString("%1Amorph=1-(%2)/%3").arg(qprefixAbs,
                                                           amline.join("+"),
                                                           sumvarAbs));
    }

    goalblock.append(QString());
    ++n;

    for (int i = 0; i < phases.size(); ++i) {
        if (phases.at(i) != istd) {
            goalblock.append(QString("GOAL[%1]=%2%3").arg(n).arg(qprefixAbs,
                                                                 phases.at(i)));
            ++n;
        }
    }

    goalblock.append(QString("GOAL[%1]=%2Amorph").arg(n).arg(qprefixAbs));

    return n;
}

int BgmnSavParser::removeAllLines(const QRegularExpression &rx)
{
    int n = 0;

    while (content.indexOf(rx) > -1) {
        content.removeAt(content.indexOf(rx));
        ++n;
    }

    return n;
}

QStringList BgmnSavParser::getAllPhaseNames() const
{
    QStringList phases;
    QFileInfo fi(fileName);
    QStringList strFiles = getAllMatches(QRegularExpression("^STRUC\\[\\d+\\]=(.+)\\s*"));

    for (int i = 0; i < strFiles.size(); ++i) {
        BgmnStrParser strParser(fi.absolutePath() + "/" + strFiles.at(i));
        QString qGoal = strParser.getQuantGoal();

        if (!qGoal.isEmpty()) {
            phases.append(strParser.getQuantGoal());
        }
    }

    return phases;
}

QStringList BgmnSavParser::getGoals() const
{
    QRegularExpression rx(QString("^GOAL\\[\\d+\\]=((%1|%2)?[^=]+)").arg(qprefixRel, qprefixAbs));

    QStringList goals;

    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch rm = rx.match(content.at(i));
        if (rm.hasMatch()) goals.append(rm.captured(1));
    }

    return goals;
}

QStringList BgmnSavParser::getNonQuantificationGoals() const
{
    QStringList goals(getAllMatches(QRegularExpression("^GOAL\\[\\d*\\]=([^=]+)")));

    if (goals.isEmpty()) return QStringList();

    QStringList pattern;
    pattern << QString("^Amorph");
    pattern << QString("^[^\\/]+\\/sum(?:=\\d+\\.?\\d*)?");
    pattern << QString("^%1[^\\/]+(?:=\\d+\\.?\\d*)?").arg(qprefixRel);
    pattern << QString("^%2[^\\/]+(?:=\\d+\\.?\\d*)?").arg(qprefixAbs);

    for (int p = 0; p < pattern.size(); ++p) {
        QRegularExpression rx(pattern.at(p));

        while (goals.indexOf(rx) >= 0) {
            goals.removeAt(goals.indexOf(rx));
        }
    }

    return goals;
}

void BgmnSavParser::renumberGoals()
{
    int n = 1;
    static QRegularExpression rx("^GOAL\\[\\d*\\]=(.+)$");
    QRegularExpressionMatch rm;

    for (int i = 0; i < content.size(); ++i) {
        rm = rx.match(content.at(i));

        if (rm.hasMatch()) {
            content[i] = QString("GOAL[%1]=%2").arg(n).arg(rm.captured(1));
            ++n;
        }
    }
}

void BgmnSavParser::setValFile(const QStringList &l)
{
    qDebug() << QString("BgmnSavParser::setValFile(): adding VAL files %1").arg(l.join(";"));

    if (content.isEmpty()) {
        qDebug() << "BgmnSavParser::setValFile(): Failed, no savFile loaded";
        return;
    }

    int n = clearValFiles();

    if (!l.size()) {
        content.insert(n, QString("VAL[1]="));
    } else {
        for (int j = 0; j < l.size(); ++j) {
            content.insert(n+j, QString("VAL[%1]=%2").arg(j+1).arg(l.at(j)));
        }
    }
}

/*
 * clears all VAL[n]= entries, returns the line number of the last one
 */
int BgmnSavParser::clearValFiles()
{
    QRegularExpression rx(QString("VAL\\[\\d+\\]=%1").arg(rxFileNames));

    int i = content.indexOf(rx, 0);
    int n = 0;

    while (i > -1) {
        content.removeAt(i);
        n = i;
        i = content.indexOf(rx, i);
    }

    return n;
}

/*
 * clears all STRUCOUT[n]= entries, returns the line number of the last one
 */
void BgmnSavParser::clearStrucOutFiles()
{
    QRegularExpression rx(QString("STRUCOUT\\[\\d+\\]=%1").arg(rxFileNames));

    int i = content.indexOf(rx, 0);

    while (i > -1) {
        content.removeAt(i);
        i = content.indexOf(rx, i);
    }
}

void BgmnSavParser::generateStrucOutFiles(const QString &s)
{
    clearStrucOutFiles();
    static QRegularExpression rx("STRUC\\[(\\d+)\\]=(.+)\\.(?:str|STR)");

    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch rm = rx.match(content.at(i));
        if (rm.hasMatch()) {
            QString n = rm.captured(1);
            QString bn = rm.captured(2);
            content.insert(i+1, QString("STRUCOUT[%1]=%2-%3.str").arg(n, bn, s));
            ++i;
        }
    }
}

/*
 * Replaces the STRUC[n]=xxx lines with file names provided in l.
 * No checks are performed, and the GOALs section is not touched.
 * If l.size doesn't match the number of STRUC, it will stop at qMin(l.size(), n(STRUC))
 */
void BgmnSavParser::overrideStrucFileNames(const QStringList &l)
{
    if (!l.size()) return;
    static QRegularExpression rx("STRUC\\[(\\d+)\\]=.+\\.(?:str|STR)");
    int n = 0;

    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch rm = rx.match(content.at(i));

        if (rm.hasMatch() && (n < l.size())) {
            qDebug() << QString("replacing line %1 with %2").arg(content.at(i), l.at(n));
            content[i] = QString("STRUC[%1]=%2").arg(rm.captured(1), l.at(n));
            ++n;
        }
    }
}

int BgmnSavParser::generateValFiles(const QString &loadedGraphFile, const QStringList &nativeGraphFormats, int n)
{
    QFileInfo fi(loadedGraphFile);

    if (nativeGraphFormats.contains(fi.suffix().toLower())) {
        qDebug() << QString("BgmnSavParser::generateValFileNames(): File format is native, using %1 as VAL file").arg(fi.filePath());
        setValFile(QStringList(fi.filePath()));
        return 1;
    }

    if (fi.suffix().toLower() == "dia") {
        QString valFileName(QString("%1.xy").arg(fi.completeBaseName()));
        qDebug() << QString("BgmnSavParser::generateValFileNames(): Creating VAL file name %1").arg(valFileName);
        setValFile(QStringList(valFileName));
        return 1;
    }

    if (n == 1) {
        QString valFileName(QString("%1.xy").arg(fi.completeBaseName()));
        qDebug() << QString("BgmnSavParser::generateValFileNames(): Creating VAL file name %1").arg(valFileName);
        setValFile(QStringList(valFileName));
        return 1;
    }

    QStringList valOut;
    int digits = int(log10(double(n))) + 1;

    for (int i = 0; i < n; ++i) {
        QString valFileName(QString("%1-%2.xy").arg(fi.completeBaseName()).arg(i, digits, 10, QLatin1Char('0')));
        qDebug() << QString("BgmnSavParser::generateValFileNames(): Creating VAL file name %1").arg(valFileName);
        valOut.append(valFileName);
    }

    setValFile(valOut);
    return n;
}

void BgmnSavParser::setOutputFile(const QString &s)
{
    qDebug() << QString("BgmnSavParser::setOutputFile(): setting OUT file to %1").arg(s);
    QRegularExpression rx(QString("OUTPUT=%1").arg(rxFileNames));

    int i = content.indexOf(rx, 0);

    if (i > -1) {
        content[i] = QString("OUTPUT=%1").arg(s);
    } else {
        content.append(QStringLiteral("% Peak list output"));
        content.append(QString("OUTPUT=%1").arg(s));
    }
}

void BgmnSavParser::setListFile(const QString &s)
{
    qDebug() << QString("BgmnSavParser::setListFile(): setting LST file to %1").arg(s);
    QRegularExpression rx(QString("LIST=%1").arg(rxFileNames));

    int i = content.indexOf(rx, 0);

    if (i > -1) {
        content[i] = QString("LIST=%1").arg(s);
    } else {
        content.append(QStringLiteral("% Results list output"));
        content.append(QString("LIST=%1").arg(s));
    }
}

void BgmnSavParser::setDiagramFile(const QString &s)
{
    qDebug() << QString("BgmnSavParser::setDiagramFile(): setting DIA file to %1").arg(s);

    // note: We also capture DIAGRAM= (with only 1 M), because it is an error likely to occur from non-german speaking users
    QRegularExpression rx(QString("DIAGRAM+=%1").arg(rxFileNames));

    int i = content.indexOf(rx, 0);

    if (i > -1) {
        content[i] = QString("DIAGRAMM=%1").arg(s);
    } else {
        content.append(QStringLiteral("% Diagram output"));
        content.append(QString("DIAGRAMM=%1").arg(s));
    }
}

void BgmnSavParser::setUntFile(const QString &s)
{
    qDebug() << QString("BgmnSavParser::setUntFile(): setting UNT file to %1").arg(s);

    QRegularExpression rx(QString("UNT=%1").arg(rxFileNames));

    int i = content.indexOf(rx, 0);

    if (i > -1) {
        content[i] = QString("UNT=%1").arg(s);
    } else {
        content.append(QStringLiteral("% Measured Background file"));
        content.append(QString("UNT=%1").arg(s));
    }
}

void BgmnSavParser::setUntcFile(const QString &s)
{
    qDebug() << QString("BgmnSavParser::setUntFile(): setting UNTC file to %1").arg(s);

    QRegularExpression rx(QString("UNTC=%1").arg(rxFileNames));

    int i = content.indexOf(rx, 0);

    if (i > -1) {
        content[i] = QString("UNTC=%1").arg(s);
    } else {
        content.append(QStringLiteral("% Measured Background file"));
        content.append(QString("UNTC=%1").arg(s));
    }
}

void BgmnSavParser::setTubeTailsFile(const QString &s)
{
    qDebug() << QString("BgmnSavParser::setUntFile(): setting TubeTails file to %1").arg(s);

    QRegularExpression rx(QString("TubeTails=%1").arg(rxFileNames));

    int i = content.indexOf(rx, 0);

    if (i > -1) {
        content[i] = QString("TubeTails=%1").arg(s);
    } else {
        content.append(QStringLiteral("% Tube Tails file"));
        content.append(QString("TubeTails=%1").arg(s));
    }
}

/*
 * structure output files *.res, *.fcf etc should always have a file name
 * of format <phase>-<projectBaseName>.ext in order to avoid overwriting
 * by other projects. Here we change the <projectBaseName> to a new
 * name.
 */
void BgmnSavParser::setStrucOutBaseName(const QString &s)
{
    qDebug() << QString("BgmnSavParser::setStrucOutBaseName(): setting Structure output file basename to %1").arg(s);
    static QRegularExpression rxs("STRUC\\[(\\d+)\\]=(.+)\\.(str|STR)");
    static QRegularExpression rxo("(STRUCOUT|SimpleSTRUCOUT|RESOUT|FCFOUT|PDBOUT)\\[(\\d+)\\]=(?:.+)\\.(str|res|fcf|pdb|STR|RES|FCF|PDB)");

    // we have to re-create all structure output file names, because
    // there is no way to parse the previous ones correctly.
    // first we gather STRUC[n]=<phase>.str in a map <n, phase>

    QMap<QString, QString> nameMap;
    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch match = rxs.match(content.at(i));
        if (match.hasMatch()) {
            nameMap.insert(match.captured(1), match.captured(2));
        }
    }

    // now we re-create the file names for STRUCOUT etc.

    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch match = rxo.match(content.at(i));
        if (match.hasMatch()) {
            QString key = match.captured(1);
            QString num = match.captured(2);
            QString pha = nameMap.value(num);
            QString ext = match.captured(3);

            if (key == "SimpleSTRUCOUT") {
                pha += "-simple";
            }

            content[i] = QString("%1[%2]=%3-%4.%5").arg(key, num, pha, s, ext);
        }
    }
}

/*
 * defines the variables ISTD and ISTDQ (Profex-specific) and
 * changes the goal calculation to use phase p as an internal
 * standard with quantity q
 */
void BgmnSavParser::setInternalStandard(const QString &p, double q)
{
    bool qPercent = settings->value("bgmnProject/quantGoals100percent", false).toBool();

    // first we unset an existing internal standard, because the
    // phase name has to be added to the regular GOALs
    unsetInternalStandard();

    static QRegularExpression rxi("^ISTD=(\\S+)");
    static QRegularExpression rxq("^ISTDQ=(\\d\\.\\d*)");
    QRegularExpression rxp(QString("^GOAL\\[\\d*\\]=%1.*").arg(p));

    // if there is a GOAL entry for phase p, we need to remove it
    if (content.indexOf(rxp) >= 0) content.removeAt(content.indexOf(rxp));

    // rewrite or append the ISTD tag
    if (content.indexOf(rxi) < 0) {
        content.append(QString("ISTD=%1").arg(p));
    } else {
        content.replaceInStrings(rxi, QString("ISTD=%1").arg(p));
    }

    // rewrite or append the ISTDQ tag
    if (content.indexOf(rxq) < 0) {
        if (qPercent) {
            content.append(QString("ISTDQ=%1").arg(100.0 * q, 0, 'f', 2));
        } else {
            content.append(QString("ISTDQ=%1").arg(q, 0, 'f', 4));
        }
    } else {
        if (qPercent) {
            content.replaceInStrings(rxq, QString("ISTDQ=%1").arg(100.0 * q, 0, 'f', 2));
        } else {
            content.replaceInStrings(rxq, QString("ISTDQ=%1").arg(q, 0, 'f', 4));
        }
    }

    // call rewriting of sum and GOAL lines
    writeGoals();
}

void BgmnSavParser::unsetInternalStandard()
{
    QRegularExpression rxi("^ISTD=(\\S+)");
    QRegularExpression rxq("^ISTDQ=(\\d+\\.?\\d*)");
    QRegularExpression rxa("^Amorph=\\S*");

    bool rewrite = false;

    // check if ISTD tag exists, if yes, remove it and flag rewriting of the goals.
    // rewriting goals will add the ISTD=phase phase back to the normal goals
    if (content.indexOf(rxi) >= 0) {
        content.removeAt(content.indexOf(rxi));
        rewrite = true;
    }

    // check if ISTDQ tag exists, if yes, remove it and flag rewriting of the goals
    if (content.indexOf(rxq) >= 0) {
        content.removeAt(content.indexOf(rxq));
        rewrite = true;
    }

    // check if Amorph tag exists, if yes, remove it and flag rewriting of the goals
    if (content.indexOf(rxa) >= 0) {
        content.removeAt(content.indexOf(rxa));
        rewrite = true;
    }

    if (rewrite) writeGoals();
}

QString BgmnSavParser::internalStandard() const
{
    static QRegularExpression rxi("^ISTD=(\\S+)");

    int n = content.indexOf(rxi);

    if (n >= 0) {
        QRegularExpressionMatch rmi = rxi.match(content.at(n));
        if (rmi.hasMatch()) return rmi.captured(1);
    }

    return QString();
}

double BgmnSavParser::internalStandardQuantity() const
{
    static QRegularExpression rxq("^ISTDQ=(\\d\\.\\d*)");

    int n = content.indexOf(rxq);

    if (n >= 0) {
        QRegularExpressionMatch rmq = rxq.match(content.at(n));
        if (rmq.hasMatch()) return rmq.captured(1).toDouble();
    }

    return -1.0;
}

QString BgmnSavParser::deviceFile() const
{
    QString s = getParameterValue(QRegularExpression(QString("VERZERR=%1").arg(rxFileNames)), 1);
    return s.trimmed();
}

QStringList BgmnSavParser::valFile() const
{
    QRegularExpression rx(QString("VAL\\[\\d+\\]=%1").arg(rxFileNames));
    QRegularExpressionMatch match;
    QStringList lst;

    for (int i = 0; i < content.size(); ++i) {
        match = rx.match(content.at(i));
        // we found a VAL[n]= line
        if (match.hasMatch()) {
            QString s = match.captured(1).trimmed();
            if (!s.isEmpty()) {
                lst.append(s);
            }
        }
    }

    return lst;
}

QString BgmnSavParser::outputFile() const
{
    return getParameterValue(QRegularExpression(QString("OUTPUT=%1").arg(rxFileNames)), 1).trimmed();
}

QString BgmnSavParser::listFile() const
{
    return getParameterValue(QRegularExpression(QString("LIST=%1").arg(rxFileNames)), 1).trimmed();
}

QString BgmnSavParser::diagramFile() const
{
    return getParameterValue(QRegularExpression(QString("DIAGRAM+(\\[\\d+\\])?=%1").arg(rxFileNames)), 2).trimmed();
}

QString BgmnSavParser::untFile() const
{
    return getParameterValue(QRegularExpression(QString("UNT=%1").arg(rxFileNames)), 1).trimmed();
}

QString BgmnSavParser::untcFile() const
{
    return getParameterValue(QRegularExpression(QString("UNTC=%1").arg(rxFileNames)), 1).trimmed();
}

QString BgmnSavParser::tubeTailsFile() const
{
    return getParameterValue(QRegularExpression(QString("TubeTails=%1").arg(rxFileNames)), 1).trimmed();
}

bool BgmnSavParser::hasTubeTails() const
{
    return !tubeTailsFile().isEmpty();
}

/*
 * Searches and returns a string defined in a regexp. c defines the capture of the
 * regexp to be returned
 */
QString BgmnSavParser::getParameterValue(const QRegularExpression &rx, int c) const
{
    QRegularExpressionMatch match;
    for (int i = 0; i < content.size(); ++i) {
        match = rx.match(content.at(i));
        if (match.hasMatch()) {
            return match.captured(c).trimmed();
        }
    }

    return QString();
}

/*
 * use c <= 0 to autodetect the number of cores
 */
void BgmnSavParser::setNumberOfThreads(int c)
{
    QRegularExpression rx("(?:NTHREADS=)(\\d+)");

    int n = 4; // this is the fallback value

    if (c > 0) {
        n = c;
    } else {
        // autodetect
        int a = QThread::idealThreadCount();
        if (a > 0) {
            n = a;
        } // else use the fallback
    }

    int l = content.indexOf(rx, 0);
    if (l > -1) {
        content.replace(l, QString("NTHREADS=%1").arg(n));
    } else {
        content.append(QString("NTHREADS=%1").arg(n));
    }
}

/*
 * returns the value of NTHREAD, or -1 if not set
 */
int BgmnSavParser::numberOfThreads() const
{
    return getParameterValue(QRegularExpression("(?:[^\\%]*NTHREADS=)(\\d+)"), 1).toInt();
}

/*
 * returns a string of the current sav file, but without any input
 * or output file names, to be used as a template SAV file
 */
QString BgmnSavParser::createTemplate() const
{
    static QRegularExpression rxClear("(VERZERR|DIAGRAMM|VAL\\[\\d+\\]|LIST|OUTPUT)=.*");
    static QRegularExpression rxVal("VAL\\[\\d+\\]");
    QStringList output;

    bool hasVal = false;

    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch match = rxClear.match(content.at(i));
        if (match.hasMatch()) {
            if (match.captured(1).contains(rxVal)) {
                if (!hasVal) {
                    // this is the first val line. we will append it (number hardcoded to 1),
                    // and set the flag to true to ignore any further val lines
                    output << QStringLiteral("VAL[1]=");
                    hasVal = true;
                    continue;
                } else {
                    // a val line was already read before. We only need the first one, so ignore this one
                    continue;
                }
            }

            // only add the first capture, but not the output file name
            output << QString("%1=").arg(match.captured(1));
            continue;
        }

        // no matches, so it is a line that will be used without modification
        output << content.at(i);
    }

    return output.join("\n");
}

double BgmnSavParser::limitOfDetection() const
{
    bool ok;
    double d = getParameterValue(QRegularExpression("^LOD=(\\d\\.?\\d*)"), 1).toDouble(&ok);
    if (ok) return d;
    return -1.0;
}

double BgmnSavParser::limitOfQuantification() const
{
    bool ok;
    double d = getParameterValue(QRegularExpression("^LOQ=(\\d\\.?\\d*)"), 1).toDouble(&ok);
    if (ok) return d;
    return -1.0;
}

double BgmnSavParser::minimumEsd() const
{
    bool ok;
    double d = getParameterValue(QRegularExpression("^MINESD=(\\d\\.?\\d*)"), 1).toDouble(&ok);
    if (ok) return d;
    return -1.0;
}

/*
 * returns the EPSn value, n = 1, 2, 3, 4
 * if not successful, 0.0 will be returned
 */
double BgmnSavParser::getEpsN(int n) const
{
    QRegularExpression rx(QString("EPS%1=(-?\\d+\\.?[\\dE\\+-]*)").arg(n));
    int l = content.indexOf(rx, 0);

    if (l < 0) return 0.0;

    QRegularExpressionMatch rm = rx.match(content.at(l));

    double eps = 0.0;

    if (rm.hasMatch()) {
        bool ok;
        eps = rm.captured(1).toDouble(&ok);
        if (ok) return eps;
    }

    return 0.0;
}

bool BgmnSavParser::hasEpsN(int n) const
{
    QRegularExpression rx(QString("EPS%1=-?\\d+\\.?[\\dE\\+-]*").arg(n));
    return content.indexOf(rx, 0) >= 0;
}
