/***************************************************************************
                          bgmnlstparser.cpp  -  description
                             -------------------
    begin                : Mon Jan 20 14:16:07 CEST 2009
    copyright            : (C) 2005 by Nicola Doebelin
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

#include "bgmnlstparser.h"
#include "bgmnfileio.h"
#include "functions.h"

#include <QTextStream>
#include <QFile>
#include <QDir>
#include <QRegularExpression>
#include <QFileInfo>
#include <QLocale>
#include <QDebug>
#include <QtMath>
#include <limits>

BgmnLstParser::BgmnLstParser()
    : BgmnDelayedParser()
{
    stats_rp = 0.0;
    stats_rpb = 0.0;
    stats_r = 0.0;
    stats_rwp = 0.0;
    stats_rexp = 0.0;
    stats_chi2 = 0.0;

    fileName = QString();
    content = QStringList();
    lastModified = QDateTime();
    status = false;
}

BgmnLstParser::BgmnLstParser(const QString &f, bool &ok)
    : BgmnDelayedParser(f, ok)
{
    stats_rp = 0.0;
    stats_rpb = 0.0;
    stats_r = 0.0;
    stats_rwp = 0.0;
    stats_rexp = 0.0;
    stats_chi2 = 0.0;

    status = false;
    ok = load(f);
}

bool BgmnLstParser::isUpToDate()
{
    if (!QFile::exists(fileName)) return false;

    QFileInfo fi(fileName);
    return lastModified == fi.lastModified();
}

bool BgmnLstParser::privateReload()
{
    if (!QFile::exists(fileName)) return false;

    if (isUpToDate()) {
        qDebug() << QString("BgmnLstParser::reload(): File is up to date, doing nothing: %1").arg(fileName);
        return true;
    }

    qDebug() << QString("BgmnLstParser::reload(): File is outdated, reloading it");
    return privateLoad(fileName);
}

/*
 * reads a file from the hard disk and stores the content in (QStringList)content
 */
bool BgmnLstParser::privateLoad(const QString &s)
{
    // reset some variables
    fileName = s;
    stats_rwp = 0.0;
    stats_rexp = 0.0;
    stats_chi2 = 0.0;

    if (!QFile::exists(fileName)) {
        qDebug() << QString("BgmnLstParser::privateLoad(): File doesn't exist, exiting (%1)").arg(fileName);
        return false;
    }

    QString str;
    if (!BgmnDelayedParser::readSafely(fileName, str)) {
        qDebug() << QString("BgmnLstParser::privateLoad(): DelayedLoader returned false, exiting (%1)").arg(fileName);
        status = false;
        return status;
    }

    if (str.isEmpty()) {
        content.clear();
        lastModified = QDateTime();
        qDebug() << QString("BgmnLstParser::privateLoad(): Returned string is empty, exiting (%1)").arg(fileName);
        status = false;
        return status;
    }

    content = str.split(global::rxLineEnding);

    QFileInfo fi(fileName);
    lastModified = fi.lastModified();
    status = true;

    // parse the content
    readRValues();
    parsePhases();
    return status;
}

QStringList BgmnLstParser::getRValues(const QString &sId) const
{
    QFileInfo fi(fileName);
    QStringList rval;

    rval.append(QString("%1;%2;%3;Rwp;%4;;").arg(fileName, fi.completeBaseName(), sId).arg(stats_rwp, 0, 'f', 4));
    rval.append(QString("%1;%2;%3;Rexp;%4;;").arg(fileName, fi.completeBaseName(), sId).arg(stats_rexp, 0, 'f', 4));
    rval.append(QString("%1;%2;%3;Chi2;%4;;").arg(fileName, fi.completeBaseName(), sId).arg(stats_chi2, 0, 'f', 4));
    rval.append(QString("%1;%2;%3;GOF;%4;;").arg(fileName, fi.completeBaseName(), sId).arg(qSqrt(stats_chi2), 0, 'f', 4));

    return rval;
}

QList<global::Result> BgmnLstParser::getStats() const
{
    QList<global::Result> lst;

    lst.append(global::Result("Rp",    stats_rp,   0.0));
    lst.append(global::Result("Rpb",   stats_rpb,  0.0));
    lst.append(global::Result("R",     stats_r,    0.0));
    lst.append(global::Result("Rwp",   stats_rwp,  0.0));
    lst.append(global::Result("Rexp",  stats_rexp, 0.0));
    lst.append(global::Result("Chi^2", stats_chi2, 0.0));
    lst.append(global::Result("GoF",   qSqrt(stats_chi2), 0.0));

    return lst;
}

/*
 * returns the EPSn value, n = 1, 2, 3, 4
 * if not successful, 0.0 will be returned
 */
double BgmnLstParser::getEpsN(int n) const
{
    QRegularExpression rx(QString("^EPS%1=(-?\\d+\\.?\\d*)\\+?-?(\\d+\\.?\\d*)?$").arg(n));
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

bool BgmnLstParser::hasEpsN(int n) const
{
    QRegularExpression rx(QString("^EPS%1=(-?\\d+\\.?\\d*)\\+?-?(\\d+\\.?\\d*)?$").arg(n));
    return content.indexOf(rx, 0) >= 0;
}

/*
 * returns all global goals listed in globalGoals. If globalGoals is empty,
 * returns all goals.
 */
QList<global::Result> BgmnLstParser::getGlobalGoals(const QStringList &globalGoals) const
{
    QList<global::Result> lst;
    bool ok = true;

    QStringList ggRx = globalGoals;
    if (ggRx.isEmpty()) ggRx.append(".*");

    // extract global paramters and Goals
    static QRegularExpression rxGlobalHeader("^Global\\s+parameters\\s+and\\s+GOALs\\s*$");
    int i = content.indexOf(rxGlobalHeader, 0);

    if (i < 0) {
        qDebug() << QString("BgmnLstParser::getGoals(): No line \"Global parameters and GOALs\" found. Exiting");
        return lst;
    }

    static QRegularExpression rxLine("^([^=]+)=([^\\+]+)(?:\\+-)?([\\d\\.]+)?$");
    i += 2; // skip the ***** line

    while ((i < content.size()) && (!content.at(i).trimmed().isEmpty())) {
        QRegularExpressionMatch rmLine = rxLine.match(content.at(i));

        if (rmLine.hasMatch()) {
            QString name = rmLine.captured(1);

            // check if the value is in the include list
            for (int j = 0; j < ggRx.size(); ++j) {
                QRegularExpression rxAddGoal(ggRx.at(j));
                QRegularExpressionMatch rmAddGoal = rxAddGoal.match(name);

                if (rmAddGoal.hasMatch()) {
                    QString valStr = rmLine.captured(2);
                    int valPrec = global::Functions::getFloatPrecision(valStr);
                    double val = valStr.isEmpty() ? 0.0 : valStr.toDouble(&ok);
                    QString err;

                    if (!ok) {
                        val = -1.0;
                        err = rmLine.captured(2);
                    }

                    QString esdStr = rmLine.captured(3);
                    int esdPrec = global::Functions::getFloatPrecision(esdStr);
                    double esd = esdStr.isEmpty() ? 0.0 : esdStr.toDouble(&ok);
                    if (!ok) esd = 0.0;

                    global::Result res(name, val, esd, err, qMax(valPrec, esdPrec));
                    lst.append(res);
                    break;
                }
            }
        }

        ++i;
    }

    return lst;
}

/*
 * returns all global goals in a string list, each field being "filename;basename;id;goalname;value;error"
 */
QStringList BgmnLstParser::getGoalsCsv(const QStringList &globalGoals, const QString &sId) const
{
    QStringList lst("File;Sample;Sample ID;Parameter, Goal;Value;ESD");
    QFileInfo fi(fileName);

    QList<global::Result> goals = getGlobalGoals(globalGoals);

    for (int i = 0; i < goals.size(); ++i) {
        global::Result r = goals.at(i);
        QString s = QString("%1;%2;%3;%4;%L5;%L6")
                            .arg(fileName,
                                 fi.completeBaseName(),
                                 sId,
                                 r.name)
                            .arg(r.value)
                            .arg(r.esd);

        lst << s;
    }

    return lst;
}

QList<global::Result> BgmnLstParser::getLocalGoals(const QString &p, const QStringList &localGoalsPatterns) const
{
    QList<global::Result> data;
    QHash<QString, ValueEsdStr> phaseVariables;
    QStringList lgRx = localGoalsPatterns;

    if (lgRx.isEmpty()) lgRx.append(".*");

    for (int i = 0; i < phaseList.size(); ++i) {
        if (phaseList.at(i).value("Phase").first == p) {
            phaseVariables = phaseList.at(i);
            break;
        }
    }

    if (phaseVariables.isEmpty()) return data;

    bool ok;
    QStringList tags = phaseVariables.keys();

    for (int j = 0; j < lgRx.size(); ++j) {
        QRegularExpression rxParam(QString("(%1)").arg(lgRx.at(j)));
        QRegularExpressionMatch rmParam;

        for (int k = 0; k < tags.size(); ++k) {
            rmParam = rxParam.match(tags.at(k));

            if (rmParam.hasMatch()) {
                ValueEsdStr entry = phaseVariables[rmParam.captured(1)];
                QString strVal = entry.first;
                QString strEsd = entry.second;

                // strVal might contain a value, something like "ERROR" or "SPHARn", or it may be empty.
                // we handle it as follows:
                // - Conversion to double successful: append value
                // - Conversion to double failed: append -1.0 and set error string to captured string
                // - No capture: skip

                if (strVal.isEmpty()) continue;

                double val = strVal.toDouble(&ok);

                if (!ok) {
                    QString errStr = entry.first;
                    if (entry.first == "ERROR") errStr = "n. a.";
                    if (entry.first == "UNDEF") errStr = "n. a.";
                    data.append(global::Result(rmParam.captured(1), -1.0, 0.0, errStr));
                    continue;
                }

                double esd = strEsd.toDouble(&ok);
                if (!ok) esd = 0.0;

                int precVal = global::Functions::getFloatPrecision(strVal);
                int precEsd = global::Functions::getFloatPrecision(strEsd);

                data.append(global::Result(rmParam.captured(1), val, esd, QString(), qMax(precVal, precEsd)));
            }
        }
    }

    return data;
}

/*
 * returns the local (phase) parameters given in params as csv table
 */
QStringList BgmnLstParser::getLocalParametersCsv(const QStringList &params, const QString &sampleId) const
{
    if (params.join(QString()).trimmed().isEmpty()) {
        qDebug() << QString("BgmnLstParser::getLocalParametersCsv(): Empty parameters list, nothing to do.");
        return QStringList();
    }

    QStringList columns;
    QStringList rows;
    QMap<QString, QMap<QString, QString> > table; // <phase <parameter, value>>

    // parse each parameter of each phase, and check if it has to be added to the table. If yes,
    // add it, and also add the phase name and parameter name to the string lists. The stringlists are needed
    // because we want to display both the phases and parameters in the order they appear in the list file.
    // QMaps are sorted alphabetically, QHash arbitrarily, but Q(String)Lists are sorted in the order they
    // are created. therefore we use QStringLists as a temporary storage of the names.

    columns << "File" << "Sample" << "Sample ID" << "Phase" << "Refined Composition";

    for (int i = 0; i < phaseList.size(); ++i) {
        QHash<QString, ValueEsdStr> phase = phaseList.at(i);

        QString uid(QString("%1-%2-%3").arg(phase["File"].first, phase["Sample"].first, phase["Phase"].first));
        rows.append(uid);

        table[uid]["File"]      = phase["File"].first;
        table[uid]["Sample"]    = phase["Sample"].first;
        table[uid]["Sample ID"] = sampleId;
        table[uid]["Phase"]     = phase["Phase"].first;
        table[uid]["Refined Composition"] = getSumFormula(phase["Phase"].first);

        QStringList tags = phase.keys();

        for (int j = 0; j < params.size(); ++j) {
            QRegularExpression rxParam(QString("(%1)").arg(params.at(j)));
            QRegularExpressionMatch rmParam;

            for (int k = 0; k < tags.size(); ++k) {
                rmParam = rxParam.match(tags.at(k));

                if (rmParam.hasMatch()) {
                    QString p(rmParam.captured(1));
                    QString v(fixLocale(phase[p].first));
                    QString e(fixLocale(phase[p].second));

                    if (!columns.contains(p)) columns << p;
                    if (!columns.contains(QString("ESD(%1)").arg(p))) columns << QString("ESD(%1)").arg(p);

                    table[uid][p] = v;
                    table[uid][QString("ESD(%1)").arg(p)] = e;
                }
            }
        }
    }

    // now the internal table is complete. Compose a ;-separated string

    QStringList str(columns.join(";"));

    for (int r = 0; r < rows.size(); ++r) {
        QStringList line;

        for (int c = 0; c < columns.size(); ++c) {
            line << table[rows.at(r)][columns.at(c)];
        }

        str << line.join(";");
    }

    return str;
}
/*
 * extracts refined parameters for each phase and stores them in a QList.
 */
void BgmnLstParser::parsePhases()
{
    phaseList.clear();

    static QRegularExpression rxPhaseName("^Local\\s+parameters\\s+and\\s+GOALs\\s+for\\s+phase\\s+(.*)$");
    static QRegularExpression rxValueLine("^([^=]+)=([^\\+]+)(?:\\+-(\\S+))?$");
    static QRegularExpression rxEndPhase("^Atomic\\s+positions\\s+for\\s+phase\\s+.*$");

    int i = 0;

    while (i < content.size()) {
        // skip to the next phase block
        i = content.indexOf(rxPhaseName, i);

        if (i < 0) break;

        QRegularExpressionMatch rmPhaseName = rxPhaseName.match(content.at(i));
        if (rmPhaseName.hasMatch()) {
            // we found a phase name, so we create a new QMap
            QHash<QString, ValueEsdStr> phase;

            // Some elements must be present, even if they are empty.
            // adding the real values will just overwrite the default values, that's how QMaps work

            phase.insert("A", ValueEsdStr());
            phase.insert("B", ValueEsdStr());
            phase.insert("C", ValueEsdStr());
            phase.insert("ALPHA", ValueEsdStr());
            phase.insert("BETA", ValueEsdStr());
            phase.insert("GAMMA", ValueEsdStr());
            phase.insert("XrayDensity", ValueEsdStr());
            phase.insert("UNIT", ValueEsdStr());

            phase.insert("File", ValueEsdStr(fileName, QString()));
            phase.insert("Phase", ValueEsdStr(rmPhaseName.captured(1), QString()));

            QFileInfo fi(fileName);
            phase.insert("Sample", ValueEsdStr(fi.completeBaseName(), QString()));

            ++i;

            // loop over all parameter lines until the rxEndPhase pattern matches
            while (i < content.size()) {
                QRegularExpressionMatch rmEndPhase = rxEndPhase.match(content.at(i));

                if (rmEndPhase.hasMatch()) {
                    // end of phase reached, exit loop
                    break;
                }

                // some lines contain several parameters, split by ", "
                QStringList line = content.at(i).split(", ");

                for (int j = 0; j < line.size(); ++j) {
                    // if rxValueLine matches, store the values in the map
                    QRegularExpressionMatch rmValueLine = rxValueLine.match(line.at(j));

                    if (rmValueLine.hasMatch()) {
                        phase.insert(rmValueLine.captured(1), ValueEsdStr(rmValueLine.captured(2), rmValueLine.captured(3)));
                    }
                }

                ++i;
            }

            phaseList.append(phase);
        }

        ++i;
    }

    if (phaseList.isEmpty()) {
        qDebug() << QString("BgmnLstParser::parsePhases(): No phases found in List file %1").arg(fileName);
    }
}

/*
 * extracts Rwp and Rexp, calculates chi2, and stores the values in global variables
 * this is the format of the line we are looking for:
 * Rp=7.91%  Rpb=9.82%  R=7.18%  Rwp=10.30% Rexp=8.12%
 */
void BgmnLstParser::readRValues()
{
    static QRegularExpression rxRvalues("^Rp=(\\d+\\.?\\d*)%\\s+Rpb=(\\d+\\.?\\d*)%\\s+R=(\\d+\\.?\\d*)%\\s+Rwp=(\\d+\\.?\\d*)%\\s+Rexp=(\\d+\\.?\\d*)%\\s*$");
    int i = content.indexOf(rxRvalues, 0);

    if (i < 0) {
        qDebug() << QString("BgmnLstParser::readRvalues(): Could not find Rwp and Rexp in file %1").arg(fileName);
        stats_rp   = 0.0;
        stats_rpb  = 0.0;
        stats_r    = 0.0;
        stats_rwp  = 0.0;
        stats_rexp = 0.0;
        stats_chi2 = 0.0;
        return;
    }

    QRegularExpressionMatch rmRvalues = rxRvalues.match(content.at(i));
    if (rmRvalues.hasMatch()) {
        stats_rp   = rmRvalues.captured(1).toDouble();
        stats_rpb  = rmRvalues.captured(2).toDouble();
        stats_r    = rmRvalues.captured(3).toDouble();
        stats_rwp  = rmRvalues.captured(4).toDouble();
        stats_rexp = rmRvalues.captured(5).toDouble();
    }

    stats_chi2 = qFuzzyIsNull(stats_rexp) ? 0.0 : pow(stats_rwp / stats_rexp, 2.0);
}

QList<CrystalAtom> BgmnLstParser::parseAtoms(const QString &p, const QString &unit) const
{
    double unitFactor = unit.toLower() == "angstroem" ? 1.0 : 10.0;
    QList<CrystalAtom> list;

    static QRegularExpression rxPhaseHeader("^Local\\s+parameters\\s+and\\s+GOALs\\s+for\\s+phase.*$");
    static QRegularExpression rxPhaseAtoms("^Atomic\\s+positions\\s+for\\s+phase\\s+(.*)$");
    static QRegularExpression rxValueLine("^(\\d+)\\s+(-?\\d+\\.\\d+)\\s+(-?\\d+\\.\\d+)\\s+(-?\\d+\\.\\d+)\\s+E[^=]*=\\((([A-Z]+[\\+\\-0-9]*\\(\\d+\\.\\d+\\),?)+)\\)$");
    static QRegularExpression rxOccu("([A-Z]+)[\\-\\+\\d]*\\((\\d\\.\\d+)\\)");
    static QRegularExpression rxParam("^(\\S+)=(-?\\d+\\.\\d*)((?:\\+-)?(\\d+\\.?\\d*))?");

    QRegularExpressionMatch rmPhaseHeader;
    QRegularExpressionMatch rmPhaseAtoms;
    QRegularExpressionMatch rmValueLine;
    QRegularExpressionMatch rmOccu;
    QRegularExpressionMatch rmParam;

    QString parsedPhase;
    int substitutions = 1;
    int labelnum = 1;

    for (int i = 0; i < content.size(); ++i) {
        if (content.at(i).trimmed().isEmpty()) {
            continue;
        }

        // we are at the beginning of a new phase, but we want to ignore the header
        // so set the phase string to QString::null, atom counter to 1, and continue
        rmPhaseHeader = rxPhaseHeader.match(content.at(i));
        if (rmPhaseHeader.hasMatch()) {
            parsedPhase = QString();
            labelnum = 1;
            continue;
        }

        // found a new phase name
        rmPhaseAtoms = rxPhaseAtoms.match(content.at(i));
        if (rmPhaseAtoms.hasMatch()) {
            parsedPhase = rmPhaseAtoms.captured(1);
            continue;
        }

        // if the phase name does not match the one we are looking for, continue
        if (parsedPhase != p) {
            continue;
        }

        // if arrived here, the phase name is correct

        // found a atom position? add it to the list
        rmValueLine = rxValueLine.match(content.at(i).trimmed());

        if (rmValueLine.hasMatch()) {
            // get the multiplicity of the position
            double m = rmValueLine.captured(1).toDouble();

            // get fractional coordinates
            double fx = rmValueLine.captured(2).toDouble();
            double fy = rmValueLine.captured(3).toDouble();
            double fz = rmValueLine.captured(4).toDouble();

            // there could be substitutions, e.g. E=(CA(0.5000),MG(0.5000))
            // so split all elements and treat them individually
            // store the number of elements on this line for later
            QStringList e = rmValueLine.captured(5).split(",");
            substitutions = e.size();

            // loop over each species (e.g. CA and MG)
            for (int j = 0; j < e.size(); ++j) {
                // extract the occupancy
                rmOccu = rxOccu.match(e.at(j));

                // store the atom in the list
                if (rmOccu.hasMatch()) {
                    QString el = rmOccu.captured(1).toUpper(); // make sure the atom name is in upper case
                    double oc = rmOccu.captured(2).toDouble();

                    CrystalAtom atom(el, fx, fy, fz, oc);
                    atom.setName(QString("%1%2").arg(el).arg(labelnum));
                    atom.setMultiplicity(int(m));
                    atom.setOccupancy_esd(0.0);
                    list.append(atom);
                }
            }

            ++labelnum;
            continue;
        }

        // a refined parameter line, e.g. x=0.1234+-0.0040
        rmParam = rxParam.match(content.at(i).trimmed());
        if (rmParam.hasMatch()) {
            // here we are only interested in parameters following an atom line.
            // if the latest phase has no atoms yet, continue
            if (list.isEmpty()) continue;

            QString pname = rmParam.captured(1);

            // regular parameter line, no anisotropic TDS
            double pval = rmParam.captured(2).toDouble();
            double pesd = rmParam.captured(4).isNull() ? -1.0 : rmParam.captured(4).toDouble();

            int pos = list.size() - substitutions;
            while (pos < 0) ++pos;

            while (pos < list.size()) {
                if (pname == "x") {
                    list[pos].setFcoord_x(pval);
                    list[pos].setFcoord_x_esd(pesd);
                }

                if (pname == "y") {
                    list[pos].setFcoord_y(pval);
                    list[pos].setFcoord_y_esd(pesd);
                }

                if (pname == "z") {
                    list[pos].setFcoord_z(pval);
                    list[pos].setFcoord_z_esd(pesd);
                }

                if (pname == "TDS") {
                    list[pos].setBiso(pval*unitFactor*unitFactor);
                    list[pos].setBiso_esd(pesd*unitFactor*unitFactor);
                }

                if (pname == "p") {
                    // Important: only write back the ESD, not the actual value!
                    list[pos].setOccupancy_esd(pesd);
                }

                ++pos;
            }
        }

        if (content.at(i).trimmed().left(9) == "TDS=ANISO") {
            // anisotropic TDS detected, parse the whole block following the line.
            // it looks like this:
            //
            // TDS=ANISO, vibrational matrice for 1st atomic position:
            //  (beta dimensionless, U in nm**2)
            //   beta[i,j]=(beta11, beta12, beta13 U[i,j]=(U11, U12, U13
            //              beta21, beta22, beta23         U21, U22, U23
            //              beta31, beta32, beta33)        U31, U32, U33)
            //
            // note that the matrix is symmetric,
            //      beta12 == beta21
            //      beta13 == beta31
            //      beta23 == beta32
            // so we only need one of them

            // jump to line "(beta dimensionless, U in nm**2)"
            i += 2;

            // check if we can parse the line "beta[i,j]" plus the next two ones
            if (i >= content.size() - 2) {
                qDebug() << QString("*** file is too short, continuing");
                continue;
            }

            static QRegularExpression rxTDS1j("^\\s*beta\\[i,j\\]=\\("
                                              "(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+)"
                                              "\\s*U\\[i,j\\]=\\("
                                              "(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+)");
            static QRegularExpression rxTDS2j("^\\s*(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+)\\s+"
                                              "(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+)");
            static QRegularExpression rxTDS3j("^\\s*(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+)\\)\\s+"
                                              "(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+),\\s*(-?\\d+\\.\\d+)\\)");

            QRegularExpressionMatch rmTDS1j = rxTDS1j.match(content.at(i));
            ++i;
            QRegularExpressionMatch rmTDS2j = rxTDS2j.match(content.at(i));
            ++i;
            QRegularExpressionMatch rmTDS3j = rxTDS3j.match(content.at(i));

            double b11 = 0.0;
            double b22 = 0.0;
            double b33 = 0.0;
            double b12 = 0.0;
            double b13 = 0.0;
            double b23 = 0.0;

            double u11 = 0.0;
            double u22 = 0.0;
            double u33 = 0.0;
            double u12 = 0.0;
            double u13 = 0.0;
            double u23 = 0.0;

            if (rmTDS1j.hasMatch()) {
                b11 = rmTDS1j.captured(1).toDouble();
                b12 = rmTDS1j.captured(2).toDouble();
                b13 = rmTDS1j.captured(3).toDouble();
            }

            if (rmTDS2j.hasMatch()) {
                b22 = rmTDS2j.captured(2).toDouble();
                b23 = rmTDS2j.captured(3).toDouble();
            }

            if (rmTDS3j.hasMatch()) {
                b33 = rmTDS3j.captured(3).toDouble();
            }

            if (rmTDS1j.hasMatch()) {
                u11 = rmTDS1j.captured(4).toDouble()*unitFactor*unitFactor;
                u12 = rmTDS1j.captured(5).toDouble()*unitFactor*unitFactor;
                u13 = rmTDS1j.captured(6).toDouble()*unitFactor*unitFactor;
            }

            if (rmTDS2j.hasMatch()) {
                u22 = rmTDS2j.captured(5).toDouble()*unitFactor*unitFactor;
                u23 = rmTDS2j.captured(6).toDouble()*unitFactor*unitFactor;
            }

            if (rmTDS3j.hasMatch()) {
                u33 = rmTDS3j.captured(6).toDouble()*unitFactor*unitFactor;
            }

            int pos = list.size() - substitutions;
            while (pos < 0) ++pos;

            while (pos < list.size()) {
                list[pos].setBaniso(b11, b22, b33, b12, b13, b23);
                list[pos].setUaniso(u11, u22, u33, u12, u13, u23);
                ++pos;
            }
        }
    }

    return list;
}

CrystalUnitCell BgmnLstParser::parseUnitCell(const QString &p, const QString &unit) const
{
    CrystalUnitCell cell;
    cell.setAxisUnit(AA); // everything will be converted to AA

    QMap<QString, QVariant> localParams = getAllLocalParameters(p, false);

    if (!localParams.contains("SpacegroupNo")) {
        qDebug() << QString("BgmnLstParser::parseUnitCell(): No IT Space Group Number found, exiting");
        return cell;
    }

    double multi = unit.toLower() == "angstroem" ? 1.0 : 10.0;

    cell.setItNumber(localParams.value("SpacegroupNo").toStringList().first().toInt());
    cell.setSpaceGroupHMBgmn(localParams.value("HermannMauguin").toStringList().first());

    // these string lists all have size 2 and hold unit cell dimensions and their esds.
    // we have to use stringlists to split the value from the esd at the "+-" sign,
    // conversion to double is done later
    QStringList sla, slb, slc, slal, slbe, slga;

    if (localParams.contains("A")) {
        sla = localParams.value("A").toStringList();
        if (sla.size() < 2) sla << "na";
    } else {
        sla << "0.0" << "na";
    }

    if (localParams.contains("B")) {
        slb = localParams.value("B").toStringList();
        if (slb.size() < 2) slb << "na";
    } else {
        slb = sla;
    }

    if (localParams.contains("C")) {
        slc = localParams.value("C").toStringList();
        if (slc.size() < 2) slc << "na";
    } else {
        slc = sla;
    }

    if (localParams.contains("ALPHA")) {
        slal = localParams.value("ALPHA").toStringList();
        if (slal.size() < 2) slal << "na";
    } else {
        slal << "0.0" << "na";
    }

    if (localParams.contains("BETA")) {
        slbe = localParams.value("BETA").toStringList();
        if (slbe.size() < 2) slbe << "na";
    } else {
        slbe << "0.0" << "na";
    }

    if (localParams.contains("GAMMA")) {
        slga = localParams.value("GAMMA").toStringList();
        if (slga.size() < 2) slga << "na";
    } else {
        slga << "0.0" << "na";
    }

    // store the axis values in double arrays [value, esd]
    // if the esd is not available (set to "na" above), set it to std::numeric_limits<double>::min()
    cell.setCell(sla[0].toDouble() * multi,
                 slb[0].toDouble() * multi,
                 slc[0].toDouble() * multi,
                 slal[0].toDouble(),
                 slbe[0].toDouble(),
                 slga[0].toDouble(),
                 sla[1] == "na" ? std::numeric_limits<double>::min() : sla[1].toDouble() * multi,
                 slb[1] == "na" ? std::numeric_limits<double>::min() : slb[1].toDouble() * multi,
                 slc[1] == "na" ? std::numeric_limits<double>::min() : slc[1].toDouble() * multi,
                 slal[1] == "na" ? std::numeric_limits<double>::min() : slal[1].toDouble(),
                 slbe[1] == "na" ? std::numeric_limits<double>::min() : slbe[1].toDouble(),
                 slga[1] == "na" ? std::numeric_limits<double>::min() : slga[1].toDouble());

    return cell;
}

QStringList BgmnLstParser::getPhaseNames() const
{
    static QRegularExpression rx("^Local\\s+parameters\\s+and\\s+GOALs\\s+for\\s+phase\\s+(.*)$");
    QStringList lines = content.filter(rx);
    QStringList phNames;

    for (int i = 0; i < lines.size(); ++i) {
        QRegularExpressionMatch rm = rx.match(lines.at(i));
        if (rm.hasMatch()) phNames.append(rm.captured(1));
    }

    return phNames;
}

QString BgmnLstParser:: getPhaseNameFromLineNumber(int l) const
{
    if (l >= content.size()) return QString();
    if (l <= 10)             return QString(); // the first 10 lines in LST files are always present

    static QRegularExpression rxg("^Global parameters and GOALs$");
    static QRegularExpression rxp("^Local\\s+parameters\\s+and\\s+GOALs\\s+for\\s+phase\\s+(.*)$");

    for (int i = l; i > 10; --i) {
        QRegularExpressionMatch rm = rxg.match(content.at(i));
        if (rm.hasMatch()) return QString();
        rm = rxp.match(content.at(i));
        if (rm.hasMatch()) return rm.captured(1);
    }

    return QString();
}

QMap<QString, QVariant> BgmnLstParser::getAllLocalParameters(const QString &ph, bool strip) const
{
    QMap<QString, QVariant> map;
    map["Phase"] = ph;

    // beginning of the block of interest
    int n = content.indexOf(QRegularExpression(QString("^Local\\s+parameters\\s+and\\s+GOALs\\s+for\\s+phase\\s+%1$").arg(ph)), 0);
    if (n < 0) return map;

    // end of the block of interest
    int e = content.indexOf(QRegularExpression(QString("^Atomic\\s+positions\\s+for\\s+phase\\s+%1$").arg(ph)), n + 1);
    if (e < 0) return map;

    static QRegularExpression rx("^([^=]+)=([^\\+]+)(?:(?:\\+-)(.*))?$");
    static QRegularExpression rg("MeanValue\\((\\S+)\\)=(\\d+\\.?\\d*)");
    static QRegularExpression ri("^([^=]+)=(?:ISOTROPIC)=([^\\+]+)(?:(?:\\+-)(.*))?$$");

    QRegularExpressionMatch rmx;
    QRegularExpressionMatch rmg;
    QRegularExpressionMatch rmi;

    for (int i = n; i < e; ++i) {
        rmx = rx.match(content.at(i));
        rmg = rg.match(content.at(i));
        rmi = ri.match(content.at(i));

        QString par;
        QString val;
        QString esd;

        if (rmx.hasMatch()) {
            par = rmx.captured(1).trimmed();
            val = rmx.captured(2).trimmed();
            esd = rmx.captured(3).trimmed();
        }

        if (rmg.hasMatch()) {
            par = rmg.captured(1).trimmed();
            val = rmg.captured(2).trimmed();
            esd = QString();
        }

        if (rmi.hasMatch()) {
            par = rmi.captured(1).trimmed();
            val = rmi.captured(2).trimmed();
            esd = rmi.captured(3).trimmed();
        }

        QStringList data(val);
        if (strip) data << esd;
        map.insert(par, data);
    }

    return map;
}

CrystalStructure BgmnLstParser::getCrystalStructure(const QString &p) const
{
    QHash<QString, ValueEsdStr> phase;

    for (int i = 0; i < phaseList.size(); ++i) {
        if (phaseList.at(i).value("Phase").first.trimmed() == p.trimmed()) {
            phase = phaseList.at(i);
            break;
        }
    }

    QString unit = phase.value("UNIT", QPair<QString,QString>("NM", QString())).first.toLower();
    CrystalStructure structure(p);
    CrystalUnitCell ucell = parseUnitCell(p, unit);
    QList<CrystalAtom> atoms = parseAtoms(p, unit);

    structure.setAtoms(atoms);
    structure.setUnitCell(ucell);
    structure.setRfactors(stats_rwp, stats_rexp, stats_chi2);
    structure.setDensity(phase.value("XrayDensity").first.toDouble());
    return structure;
}

/*
 * returns the end date and time of the refinement read from the 3rd line of the LST file
 */
QDateTime BgmnLstParser::refinementDateTime() const
{
    // example of the parsed string:
    // "Start: Fri Jul 10 14:15:59 2015; End: Fri Jul 10 14:16:16 2015"
    static QRegularExpression rx("Start:[^;]+;\\s+End:\\s+[A-Za-z]+\\s+([A-Za-z]+)\\s+(\\d+)\\s+(\\d+):(\\d+):(\\d+)\\s+(\\d+)");
    //                                                                 (1) month      (2) day   (3)h   (4)m   (5)s      (6)year

    int n = content.indexOf(rx);
    if (n < 0) return QDateTime();

    QRegularExpressionMatch rm = rx.match(content.at(n));

    // translate the month name to month number. Don't use Qt's QDate classes, because they are localized
    QStringList mth;
    mth << "jan" << "feb" << "mar" << "apr" << "may" << "jun" << "jul" << "aug" << "sep" << "oct" << "nov" << "dec";
    int MM = mth.indexOf(rm.captured(1).toLower()) + 1;
    int dd = rm.captured(2).toInt();
    int hh = rm.captured(3).toInt();
    int mm = rm.captured(4).toInt();
    int ss = rm.captured(5).toInt();
    int yy = rm.captured(6).toInt();

    QDate date;
    QTime time;

    date.setDate(yy, MM, dd);
    time.setHMS(hh, mm, ss, 0);

    return QDateTime(date, time, Qt::LocalTime);
}

/*
 * converts a string that potentially contains a floating point number to
 * a string with the locale's decimal sign
 *
 * if no decimal sign is found or if the temporary conversion to double failed,
 * the original string is returned unmodified.
 *
 * this is primarily used for CSV output, because some programs expect the csv
 * values to be localized.
 */
QString BgmnLstParser::fixLocale(const QString &str) const
{
    static QRegularExpression rx("[\\.,]");

    // find the decimal sign
    int i = str.indexOf(rx, 0);
    if (i < 0) return str;

    // number of decimals
    int dec = str.length() - i - 1;
    dec = dec > 0 ? dec : 6;

    // convert to double
    bool ok;
    double d = str.toDouble(&ok);

    // convert back to string with locale's decimal sign
    if (ok) return QString("%L1").arg(d, 0, 'f', dec);
    return str;
}

QString BgmnLstParser::getSumFormula(const QString &ph, int z) const
{
    CrystalStructure cstruc = getCrystalStructure(ph);
    return cstruc.toSumFormula(z);
}

int BgmnLstParser::lineOfPhase(const QString &p) const
{
    if (p.isEmpty()) return 10; // hardcoded block of global goals
    return content.indexOf(QRegularExpression(QString("^Local\\s+parameters\\s+and\\s+GOALs\\s+for\\s+phase\\s+%1$").arg(p)), 10);
}
