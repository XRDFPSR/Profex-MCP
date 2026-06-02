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

#ifndef LSTPARSER_H
#define LSTPARSER_H

#include <QString>
#include <QStringList>
#include <QList>
#include <QHash>
#include <QMap>
#include <QDateTime>
#include "bgmndelayedparser.h"
#include "../structs.h"
#include "../crystal/crystalstructure.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

typedef QPair<QString, QString> ValueEsdStr;

class XRDIO_EXPORT BgmnLstParser : public BgmnDelayedParser
{
public:
    explicit BgmnLstParser();
    explicit BgmnLstParser(const QString &, bool &);

    QList<global::Result> getGlobalGoals(const QStringList &globalGoals = QStringList()) const;
                                                         // returns goals.
                                                         // the string list holds regexp patterns left
                                                         // of "=" to be gathered. If the stringlist is
                                                         // empty, all goals will be returned.

    QList<global::Result> getLocalGoals(const QString &, const QStringList &localGoalsPatterns = QStringList()) const;
                                                         // returns goals.
                                                         // the string list holds regexp patterns left
                                                         // of "=" to be gathered. If the stringlist is
                                                         // empty, all goals will be returned.

    QStringList getRValues(const QString &sId = QString()) const;  // returns all R values as a ;-separated list of format
                                                         // absoluteFilePath;basename;sampleId;Rwp;<value Rwp>;;
                                                         // only Rwp, Rexp, and Chi2 are returned

    double getEpsN(int) const;                           // returns the EPSn value
    bool hasEpsN(int) const;                             // returns TRUE if the EPSn value is found in the lst file

    QStringList getGoalsCsv(const QStringList &globalGoals, const QString &sId) const;  // ;-separated list, the string list holds
                                                         // regexp patterns left of "=" to be skipped
                                                         // sId is the sampleId, it will be written to one of the columns

    QStringList getLocalParametersCsv(const QStringList &, const QString &sampleId = QString()) const;
                                                         // csv table, the stringlist holds the parameter
                                                         // names to be reported, can be empty to get a
                                                         // default set

    QStringList getPhaseNames() const;                   // returns a list of all phase names
    QString getPhaseNameFromLineNumber(int l) const;     // returns the phase name of the block containing l
    int lineOfPhase(const QString &p) const;             // returns the line number where the block of phase p begins

    QMap<QString, QVariant> getAllLocalParameters(const QString &, bool strip = true) const;  // returns all parameters of a phase
                                                         // if strip = true, the errors are cut off the values

    CrystalStructure getCrystalStructure(const QString &p) const; // returns the crystal structure of the provided phase p

    QString getSumFormula(const QString &, int z = 1) const;   // returns a string containing the refined sum formula

    QDateTime refinementDateTime() const;                // returns the date and time of the refinement read from the lst file

    inline double getRwp()  const {return stats_rwp;}
    inline double getRexp() const {return stats_rexp;}
    inline double getChi2() const {return stats_chi2;}

    QList<global::Result> getStats() const;              // returns Rp, Rpb, R, Rwp, Rexp, Chi2, GoF as a list of results

    inline bool hasData() const {return status;}         // returns true if a lst file was parsed, else returns false

    bool isUpToDate();                                   // returns true if the content is up to date, or false if the file needs reloading

protected:
    bool privateLoad(const QString &) override;
    bool privateReload() override;

    void parsePhases();
    void readRValues();
    QString fixLocale(const QString &) const;
    QList<CrystalAtom> parseAtoms(const QString &p, const QString &unit) const; // adds all atoms found for phase p to list
    CrystalUnitCell parseUnitCell(const QString &p, const QString &unit) const; // returns the unit cell

    QStringList content;
    QString fileName;
    QDateTime lastModified;
    QList< QHash<QString, ValueEsdStr> > phaseList;
    double stats_rp;
    double stats_rpb;
    double stats_r;
    double stats_rwp;
    double stats_rexp;
    double stats_chi2;
    bool status;
};

#endif // LSTPARSER_H
