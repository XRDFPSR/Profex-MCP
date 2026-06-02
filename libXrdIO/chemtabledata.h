/***************************************************************************
                          chemtabledata.h  -  description
                             -------------------
    begin                : Sun Feb 21 13:41:07 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#ifndef CHEMTABLEDATA_H
#define CHEMTABLEDATA_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QHash>
#include "chemtablestruct.h"
#include "settingsmanager.h"
#include "structs.h"
#include "crystal/crystalstructure.h"
#include "parser/bgmnsavparser.h"
#include "parser/bgmnlstparser.h"

#if defined XRDIO
#define XRDIO_EXPORT Q_DECL_EXPORT
#else
#define XRDIO_EXPORT Q_DECL_IMPORT
#endif

using namespace global;

typedef QString Phase;

class XRDIO_EXPORT ChemTableData
{
public:
    ChemTableData();

    void setData(const QString &, const QString &, bool);
    void setData(const BgmnSavParser &, const BgmnLstParser &, bool);
    bool reload();
    void clear();
    void initSettings();
    bool hasData(ChemistryMode m) const;
    QString getSavFileName() const {return savFileName;}
    QString getLstFileName() const {return lstFileName;}
    QString getSampleName()  const {return sampleName;}
    QHash<QString, double> getPhaseChemWeights(const QString &, ChemistryMode m) const;
    QHash<QString, double> getPhaseChemEsds(const QString &, ChemistryMode m) const;
    QList<QStringList> getTableData(ChemistryMode m, QStringList &hHeader, QStringList &vHeader) const;

    QStringList getCsvList(ChemistryMode m, bool header, const QString &sampleId = QString());
    QString getCsvTable(ChemistryMode m);
    QString getHtmlTable(ChemistryMode m);

private:
    SettingsManager *settings;
    bool settingsParsed;
    ChemTableStruct tableStructElements;
    ChemTableStruct tableStructAtoms;
    ChemTableStruct tableStructOxides;
    QHash<QString, int> elementNumberLookupTable;
    QHash<QString, int> oxideNumberLookupTable;
    QHash<QString, Oxide> atomElementLookupTable;
    QHash<QString, Oxide> atomOxideLookupTable;
    QMultiMap<Phase, Oxide> phaseWeightDataElement;
    QMultiMap<Phase, Oxide> phaseWeightDataAtomic;
    QMultiMap<Phase, Oxide> phaseWeightDataOxide;
    QString savFileName;
    QString lstFileName;
    QString sampleName;
    QStringList phaseNames;
    QHash<QString, QString> phaseGoalNames;
    QHash<QString, double> phaseQuantityValues;
    QList<global::Result> globalGoals;

    void parsePhasesAndGoals(const BgmnSavParser &, const BgmnLstParser &lparser);
    void initOxideLuts(const QStringList &);
    void initElementLuts(const QStringList &);
    void buildTables();
    void buildTableElements();
    void buildTableAtomic();
    void buildTableOxides();
    QString appendOxideToLUTs(const QStringList &);
    QStringList defaultOxideWeights();
    QStringList defaultElementWeights();
    void setAtomsAndGoals(const QMultiMap<Phase, CrystalStructure> &phasesStructures);
    void setFileNames(const BgmnSavParser &, const BgmnLstParser &);
    QHash<QString, double> normalizeGoals(const QHash<QString, double> &);

    void structuresToAtoms(const QMultiMap<Phase, CrystalStructure> &, // input
                           QMultiMap<Phase, Oxide> &,  // elements
                           QMultiMap<Phase, Oxide> &,  // atomic
                           QMultiMap<Phase, Oxide> &); // oxide

    QList<Oxide> atomsToOxidesNormalized(const QList<CrystalAtom> &atomList);
    QList<Oxide> atomsToElementsNormalized(const QList<CrystalAtom> &atomList);
    QList<Oxide> atomsToAtomicNormalized(const QList<CrystalAtom> &atomList);

    QStringList sortElements(const QStringList &) const;
    QStringList sortOxides(const QStringList &) const;
    QList<QStringList> tableStructToStringList(const ChemTableStruct &, ChemistryMode m, QStringList &vHeader, QStringList &hHeader, bool headerWithUnit = true) const;
    QString doubleToString(double, int) const;

    void dumpTableToDebug(const QStringList &ph, const QStringList &el, const QList<QStringList> &) const;
};

#endif // CHEMTABLEDATA_H
