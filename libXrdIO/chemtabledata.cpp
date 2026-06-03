/***************************************************************************
                          chemtabledata.cpp  -  description
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

#include "chemtabledata.h"
#include <QDebug>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QtMath>

ChemTableData::ChemTableData()
{
    settings = SettingsManager::getInstance();
    tableStructElements = ChemTableStruct(ELEMENT);
    tableStructAtoms = ChemTableStruct(ATOMIC);
    tableStructOxides = ChemTableStruct(OXIDE);
    settingsParsed = false;
}

/********************************************************************
 * Read default data into data structures:
 ********************************************************************/

void ChemTableData::initSettings()
{
    QStringList oxideSettingsData = settings->value("chemistry/oxides", QStringList()).toStringList();

    if (oxideSettingsData.size() < 3) {
        qDebug() << QStringLiteral("ChemTableData::readSettings(): StringList is too short.");
        oxideSettingsData = defaultOxideWeights();
    }

    initElementLuts(defaultElementWeights());
    initOxideLuts(oxideSettingsData);
    settingsParsed = true;
}

/*
 * creates two lookup tables:
 *
 * atomElementLookupTable: QHash<QString, Oxide> to look up the atomic weight,
 *                         key is the atom name in all upper case
 *
 * elementNumberLookupTable: QHash<QString, int> to look up the element number.
 */
void ChemTableData::initElementLuts(const QStringList &elementData)
{
    if (elementData.isEmpty()) return;

    atomElementLookupTable.clear();
    elementNumberLookupTable.clear();

    for (int i = 0; i < elementData.size() - 1; i += 2) {
        QString atomName     = elementData.at(i);
        double elementWeight = elementData.at(i+1).toDouble();
        int elementNumber    = elementNumberLookupTable.size();

        elementNumberLookupTable.insert(atomName, elementNumber);

        if (elementWeight > 0.0) {
            atomElementLookupTable.insert(atomName.toUpper(), Oxide(atomName, elementWeight, 0.0));
        } else {
            qDebug() << QString("ChemTableData::initElementList(): No molecular weight defined for element %1").arg(atomName);
        }
    }
}

/*
 * creates two lookup tables:
 *
 * atomOxideLookupTable: QHash<QString, Oxide> to look up the molecular weight,
 *                       key is the atom name in all upper case
 *
 * oxideNumberLookupTable: QHash<QString, int> to look up the oxide number.
 */
void ChemTableData::initOxideLuts(const QStringList &oxideSettingsData)
{
    atomOxideLookupTable.clear();
    oxideNumberLookupTable.clear();
    QStringList missing;

    for (int i = 0; i < oxideSettingsData.size() - 2; i += 3) {
        QString missingEl = appendOxideToLUTs(oxideSettingsData.mid(i, 3));
        if (!missingEl.isEmpty()) missing << missingEl;
    }

    if (missing.size()) {
        qDebug() << QString("ChemTableData::initOxideList(): No oxide weight defined in preferences for elements %1").arg(missing.join(", "));
    }
}

QString ChemTableData::appendOxideToLUTs(const QStringList &oxideData)
{
    if (oxideData.isEmpty()) return QString();

    QString atomName    = oxideData.at(0);
    QString oxideName   = oxideData.size() > 1 ? oxideData.at(1)            : atomName;
    double  oxideWeight = oxideData.size() > 2 ? oxideData.at(2).toDouble() : 0.0;
    int     oxideNumber = oxideNumberLookupTable.size();

    if (atomName.trimmed() == "O") return QString();

    oxideNumberLookupTable.insert(oxideName, oxideNumber);

    if (oxideWeight > 0.0) {
        atomOxideLookupTable.insert(atomName.toUpper(), Oxide(oxideName, oxideWeight, 0.0));
        return QString();
    }

    return atomName;
}

QStringList ChemTableData::defaultOxideWeights()
{
    QStringList atomList = global::atoms.split(";", Qt::KeepEmptyParts);
    QStringList defaultOxideList;

    for (int i = 0; i < atomList.size() - 4; i += 5) {
        defaultOxideList << atomList.at(i+1) << atomList.at(i+3) << atomList.at(i+4);
    }

    return defaultOxideList;
}

QStringList ChemTableData::defaultElementWeights()
{
    QStringList atomList = global::atoms.split(";", Qt::KeepEmptyParts);
    QStringList defaultElementList;

    for (int i = 0; i < atomList.size() - 4; i += 5) {
        defaultElementList << atomList.at(i+1) << atomList.at(i+2);
    }

    return defaultElementList;
}

/********************************************************************
 * Public API:
 ********************************************************************/

void ChemTableData::setData(const QString &savFile, const QString &lstFile, bool recalc)
{
    bool ok;
    BgmnSavParser sparser(savFile);
    BgmnLstParser lparser(lstFile, ok);
    if (ok) setData(sparser, lparser, recalc);
}

void ChemTableData::setData(const BgmnSavParser &sparser, const BgmnLstParser &lparser, bool recalc)
{
    setFileNames(sparser, lparser);
    if (!recalc) return;

    if (!settingsParsed) initSettings();
    parsePhasesAndGoals(sparser, lparser);

    phaseNames = lparser.getPhaseNames();
    QMultiMap<QString, CrystalStructure> phaseStructures;

    for (int i = 0; i < phaseNames.size(); ++i) {
        phaseStructures.insert(phaseNames.at(i), lparser.getCrystalStructure(phaseNames.at(i)));
    }

    setAtomsAndGoals(phaseStructures);
    buildTables();
}

bool ChemTableData::reload()
{
    if (lstFileName.isEmpty() || savFileName.isEmpty()) return false;

    setData(savFileName, lstFileName, true);
    return true;
}

void ChemTableData::clear()
{
    phaseWeightDataElement.clear();
    phaseWeightDataAtomic.clear();
    phaseWeightDataOxide.clear();
}

QHash<QString, double> ChemTableData::getPhaseChemWeights(const QString &phase, ChemistryMode m) const
{
    QList<Oxide> phOxides;
    QHash<QString, double> oxHash;

    switch (m) {
    case UNKNOWN:
        break;
    case ELEMENT:
        phOxides = phaseWeightDataElement.values(phase);
        break;
    case ATOMIC:
        phOxides = phaseWeightDataAtomic.values(phase);
        break;
    case OXIDE:
        phOxides = phaseWeightDataOxide.values(phase);
        break;
    }

    for (int i = 0; i < phOxides.size(); ++i) {
        oxHash.insert(phOxides.at(i).name, phOxides.at(i).weight);
    }

    return oxHash;
}

QHash<QString, double> ChemTableData::getPhaseChemEsds(const QString &phase, ChemistryMode m) const
{
    QList<Oxide> phOxides;
    QHash<QString, double> oxHash;

    switch (m) {
    case UNKNOWN:
        break;
    case ELEMENT:
        phOxides = phaseWeightDataElement.values(phase);
        break;
    case ATOMIC:
        phOxides = phaseWeightDataAtomic.values(phase);
        break;
    case OXIDE:
        phOxides = phaseWeightDataOxide.values(phase);
        break;
    }

    for (int i = 0; i < phOxides.size(); ++i) {
        oxHash.insert(phOxides.at(i).name, phOxides.at(i).esd);
    }

    return oxHash;
}

bool ChemTableData::hasData(ChemistryMode m) const
{
    switch (m) {
    case ELEMENT: return tableStructElements.hasData();
    case ATOMIC:  return tableStructAtoms.hasData();
    case OXIDE:   return tableStructOxides.hasData();
    default:      return false;
    }
}

QList<QStringList> ChemTableData::getTableData(ChemistryMode m, QStringList &vHeader, QStringList &hHeader) const
{
    QList<QStringList> lst;

    switch (m) {
    case ELEMENT:
        lst = tableStructToStringList(tableStructElements, ELEMENT, vHeader, hHeader, true);
        break;
    case ATOMIC:
        lst = tableStructToStringList(tableStructAtoms, ATOMIC, vHeader, hHeader, true);
        break;
    case OXIDE:
        lst = tableStructToStringList(tableStructOxides, OXIDE, vHeader, hHeader, true);
        break;
    default:
        break;
    }

    dumpTableToDebug(vHeader, hHeader, lst);
    return lst;
}

/********************************************************************
 * Private API:
 ********************************************************************/

void ChemTableData::setFileNames(const BgmnSavParser &sparser, const BgmnLstParser &lparser)
{
    savFileName = sparser.controlFile();
    QFileInfo fi(lparser.getFileName());
    lstFileName = fi.absoluteFilePath();
    sampleName = fi.completeBaseName();
}

void ChemTableData::setAtomsAndGoals(const QMultiMap<Phase, CrystalStructure> &phasesStructures)
{
    structuresToAtoms(phasesStructures, phaseWeightDataElement, phaseWeightDataAtomic, phaseWeightDataOxide);
}

void ChemTableData::parsePhasesAndGoals(const BgmnSavParser &sparser, const BgmnLstParser &lparser)
{
    globalGoals = lparser.getGlobalGoals();

    QHash<QString, QString> phaseFromStrFile = sparser.getAllStrucPhaseNames(); // QHash<STR file, Phase name>
    QHash<QString, QString> goalFromStrFile  = sparser.getAllStrucQuantGoals(); // QHash<STR file, Goal name>
    QHash<QString, double>  goalValueFromName;                                  // QHash<Goal name, Goal value>
    QHash<QString, double>  goalValueFromPhase;                                 // QHash<Phase, Goal value>
    phaseGoalNames.clear();                                                     // QHash<Phase, Goal name>

    for (int i = 0; i < globalGoals.size(); ++i) {
        goalValueFromName[globalGoals.at(i).name] = globalGoals.at(i).value;
    }

    QHashIterator<QString, QString> it(phaseFromStrFile);

    while (it.hasNext()) {
        it.next();
        QString _phaseName = it.value();
        QString _goalName  = goalFromStrFile.value(it.key());
        double  _goalValue = goalValueFromName.value(_goalName);

        phaseGoalNames[_phaseName]     = _goalName;
        goalValueFromPhase[_phaseName] = _goalValue;

        qDebug() << QString("ChemTableWidget::parsePhasesAndGoals(): Phase=%1 Goal=%2 Value=%3").arg(_phaseName, _goalName).arg(_goalValue);
    }

    phaseQuantityValues = normalizeGoals(goalValueFromPhase);
}

void ChemTableData::structuresToAtoms(const QMultiMap<Phase, CrystalStructure> &structures,
                                      QMultiMap<Phase, Oxide> &phWeightListElement,
                                      QMultiMap<Phase, Oxide> &phWeightListAtom,
                                      QMultiMap<Phase, Oxide> &phWeightListOxide)
{
    phWeightListElement.clear();
    phWeightListAtom.clear();
    phWeightListOxide.clear();

    QStringList phases = structures.keys();

    for (int i = 0; i < phases.size(); ++i) {
        CrystalStructure phaseStructure = structures.value(phases.at(i));

        QList<Oxide> phaseDataElementNormalized = atomsToElementsNormalized(phaseStructure.atoms());
        QList<Oxide> phaseDataAtomNormalized    = atomsToAtomicNormalized(phaseStructure.atoms());
        QList<Oxide> phaseDataOxideNormalized   = atomsToOxidesNormalized(phaseStructure.atoms());

        // store the results in the phase map
        for (int j = 0; j < phaseDataElementNormalized.size(); ++j) {
            phWeightListElement.insert(phases.at(i), phaseDataElementNormalized.at(j));
        }

        // store the results in the phase map
        for (int j = 0; j < phaseDataAtomNormalized.size(); ++j) {
            phWeightListAtom.insert(phases.at(i), phaseDataAtomNormalized.at(j));
        }

        // store the results in the phase map
        for (int j = 0; j < phaseDataOxideNormalized.size(); ++j) {
            phWeightListOxide.insert(phases.at(i), phaseDataOxideNormalized.at(j));
        }
    }
}

QList<Oxide> ChemTableData::atomsToElementsNormalized(const QList<CrystalAtom> &atomList)
{
    double sum = 0.0;
    QHash<QString, Oxide> elementList;

    for (int i = 0; i < atomList.size(); ++i) {
        CrystalAtom atom = atomList.at(i);

        if (!atomElementLookupTable.contains(atom.element())) {
            continue;
        }

        QString elementName = atomElementLookupTable[atom.element().toUpper()].name;

        double val = atom.occupancy() * atom.multiplicity() * atomElementLookupTable[atom.element()].weight;
        double esd = atom.occupancy_esd() * atom.multiplicity() * atomElementLookupTable[atom.element()].weight;
        sum += val;

        if (elementList.contains(elementName)) {
            elementList[elementName].weight += val;
            elementList[elementName].esd = qSqrt(qPow(elementList[elementName].esd, 2.0) + qPow(esd, 2.0));
        } else {
            elementList.insert(elementName, Oxide(elementName, val, esd));
        }
    }

    // normalize to sum
    QHash<QString, Oxide>::iterator it = elementList.begin();
    while (it != elementList.end()) {
        it.value().weight /= sum;
        it.value().esd    /= sum;
        ++it;
    }

    return elementList.values();
}

QList<Oxide> ChemTableData::atomsToAtomicNormalized(const QList<CrystalAtom> &atomList)
{
    double sum = 0.0;
    QHash<QString, Oxide> elementList;

    for (int i = 0; i < atomList.size(); ++i) {
        CrystalAtom atom = atomList.at(i);

        QString elementName = atomElementLookupTable[atom.element().toUpper()].name;

        double val = atom.occupancy() * atom.multiplicity();
        double esd = atom.occupancy_esd() * atom.multiplicity();
        sum += val;

        if (elementList.contains(elementName)) {
            elementList[elementName].weight += val;
            elementList[elementName].esd = qSqrt(qPow(elementList[elementName].esd, 2.0) + qPow(esd, 2.0));
        } else {
            elementList.insert(elementName, Oxide(elementName, val, esd));
        }
    }

    // normalize to sum
    QHash<QString, Oxide>::iterator it = elementList.begin();
    while (it != elementList.end()) {
        it.value().weight /= sum;
        it.value().esd    /= sum;
        ++it;
    }

    return elementList.values();
}

QList<Oxide> ChemTableData::atomsToOxidesNormalized(const QList<CrystalAtom> &atomList)
{
    double sum = 0.0;
    QHash<QString, Oxide> oxideList;

    for (int i = 0; i < atomList.size(); ++i) {
        CrystalAtom atom = atomList.at(i);

        if (!atomOxideLookupTable.contains(atom.element())) {
            continue;
        }

        QString oxideName = atomOxideLookupTable[atom.element().toUpper()].name;

        // extract the number of cations, e.g. the "2" in "Al2O3",
        // because we must divide the oxide weight by this number

        double sc = 1.0;
        static QRegularExpression rx("([A-Z][a-z]?)(\\d*)O?\\d*");
        QRegularExpressionMatch match = rx.match(oxideName);

        if (match.hasMatch()) {
            bool ok;
            sc = match.captured(2).toDouble(&ok);
            if (!ok) sc = 1.0;
        }

        double val = atom.occupancy() * atom.multiplicity() * atomOxideLookupTable[atom.element()].weight / (qFuzzyIsNull(sc) ? 1.0 : sc);
        double esd = atom.occupancy_esd() * atom.multiplicity() * atomOxideLookupTable[atom.element()].weight / (qFuzzyIsNull(sc) ? 1.0 : sc);
        sum += val;

        if (oxideList.contains(oxideName)) {
            oxideList[oxideName].weight += val;
            oxideList[oxideName].esd = qSqrt(qPow(oxideList[oxideName].esd, 2.0) + qPow(esd, 2.0));
        } else {
            oxideList.insert(oxideName, Oxide(oxideName, val, esd));
        }
    }

    // normalize to sum
    QHash<QString, Oxide>::iterator it = oxideList.begin();
    while (it != oxideList.end()) {
        it.value().weight /= sum;
        it.value().esd    /= sum;
        ++it;
    }

    return oxideList.values();
}

void ChemTableData::buildTables()
{
    buildTableElements();
    buildTableAtomic();
    buildTableOxides();
}

void ChemTableData::buildTableElements()
{
    QStringList elem;

    for (int i = 0; i < phaseNames.size(); ++i) {
        elem.append(getPhaseChemWeights(phaseNames.at(i), ELEMENT).keys());
    }

    tableStructElements.clear();
    tableStructElements.setPhaseLabels(phaseNames);
    tableStructElements.setElementLabels(sortElements(elem));
    tableStructElements.setPhaseQuantities(phaseQuantityValues);

    QMapIterator<Phase, Oxide> it(phaseWeightDataElement);

    while (it.hasNext()) {
        it.next();
        tableStructElements.addValue(it.key(), it.value().name, it.value().weight);
        tableStructElements.addEsd(it.key(), it.value().name, it.value().esd);
    }
}

void ChemTableData::buildTableAtomic()
{
    QStringList atom;

    for (int i = 0; i < phaseNames.size(); ++i) {
        atom.append(getPhaseChemWeights(phaseNames.at(i), ATOMIC).keys());
    }

    tableStructAtoms.clear();
    tableStructAtoms.setPhaseLabels(phaseNames);
    tableStructAtoms.setElementLabels(sortElements(atom));
    tableStructAtoms.setPhaseQuantities(phaseQuantityValues);

    QMapIterator<Phase, Oxide> it(phaseWeightDataAtomic);

    while (it.hasNext()) {
        it.next();
        tableStructAtoms.addValue(it.key(), it.value().name, it.value().weight);
        tableStructAtoms.addEsd(it.key(), it.value().name, it.value().esd);
    }
}

void ChemTableData::buildTableOxides()
{
    QStringList oxid;

    for (int i = 0; i < phaseNames.size(); ++i) {
        oxid.append(getPhaseChemWeights(phaseNames.at(i), OXIDE).keys());
    }

    tableStructOxides.clear();
    tableStructOxides.setPhaseLabels(phaseNames);
    tableStructOxides.setElementLabels(sortOxides(oxid));
    tableStructOxides.setPhaseQuantities(phaseQuantityValues);

    QMapIterator<Phase, Oxide> it(phaseWeightDataOxide);

    while (it.hasNext()) {
        it.next();
        tableStructOxides.addValue(it.key(), it.value().name, it.value().weight);
        tableStructOxides.addEsd(it.key(), it.value().name, it.value().esd);
    }
}

/*
 * sorts elements according to the periodic table, also removes duplicates
 */
QStringList ChemTableData::sortElements(const QStringList &l) const
{
    QMap<int, QString> elements;

    for (int i = 0; i < l.size(); ++i) {
        elements[elementNumberLookupTable.value(l.at(i))] = l.at(i);
    }

    return elements.values();
}

/*
 * sorts oxides according to the periodic table, also removes duplicates
 */
QStringList ChemTableData::sortOxides(const QStringList &l) const
{
    QMap<int, QString> oxides;

    for (int i = 0; i < l.size(); ++i) {
        oxides[oxideNumberLookupTable.value(l.at(i))] = l.at(i);
    }

    return oxides.values();
}

/*
 * makes sure that the quantity goals are normalized to 1.0
 */
QHash<QString, double> ChemTableData::normalizeGoals(const QHash<QString, double> &vals)
{
    QHash<QString, double> norm;

    QHashIterator<QString, double> ita(vals);
    double sum = 0.0;

    while (ita.hasNext()) {
        ita.next();
        sum += ita.value();
    }

    QHashIterator<QString, double> itb(vals);

    while (itb.hasNext()) {
        itb.next();
        norm[itb.key()] = itb.value() / sum;
    }

    return norm;
}

QList<QStringList> ChemTableData::tableStructToStringList(const ChemTableStruct &tStruc, ChemistryMode m, QStringList &vHeader, QStringList &hHeader, bool headerWithUnit) const
{
    int digits = 2;
    QString unit = (m == ATOMIC ? "(atm-%)" : "(wt-%)");
    QList<QStringList> tlst;
    QStringList elLbls = tStruc.elementLabels();

    vHeader = tStruc.phaseLabels();
    hHeader = QStringList("Phase Quantity (wt-%)");

    // add the unit to the element labels and append the labels to the hHeader
    for (int e = 0; e < elLbls.size(); ++e) {
        if (headerWithUnit) {
            hHeader << QString("%1 %2").arg(elLbls.at(e), unit);
        } else {
            hHeader << elLbls.at(e);
        }
    }

    // compose the cells
    for (int r = 0; r < vHeader.size(); ++r) {
        QStringList line;
        double quant = tStruc.getPhaseQuantity(vHeader.at(r));
        line << doubleToString(quant, digits);

        for (int c = 0; c < hHeader.size() - 1; ++c) {
            double val = tStruc.getValue(r, c);
            line << doubleToString(val, digits);
        }

        tlst << line;
    }

    // add the total line
    if ((m == ELEMENT) || (m == OXIDE)) {
        vHeader << QString("Weighted Total");
        QStringList line;
        double sumQ = tStruc.getSumQuantities();
        line << doubleToString(sumQ, digits);

        for (int c = 0; c < hHeader.size() - 1; ++c) {
            double totQ = tStruc.getTotal(c);
            line << doubleToString(totQ, digits);
        }

        tlst << line;
    }

    return tlst;
}

QString ChemTableData::doubleToString(double val, int digits) const
{
    return (val < 0.0 ? QString("-") : QString("%L1").arg(100.0 * val, 0, 'f', digits));
}

void ChemTableData::dumpTableToDebug(const QStringList &ph, const QStringList &el, const QList<QStringList> &l) const
{
    int rows = l.size();
    int cols = el.size();
    int lengPhase = 0;
    int lengQuant = el.constFirst().length();
    int lengOxide = 12;

    for (int p = 0; p < ph.size(); ++p) {
        lengPhase = qMax(lengPhase, ph.at(p).length());
    }

    QStringList hHeader(QString("%1").arg("Phase", -lengPhase));

    for (int e = 0; e < el.size(); ++e) {
        hHeader << QString("%1").arg(el.at(e), e == 0 ? lengQuant : lengOxide);
    }

    qDebug() << hHeader.join(" ");

    for (int r = 0; r < rows; ++r) {
        QStringList line(QString("%1").arg(ph.at(r), -lengPhase));

        for (int c = 0; c < cols; ++c) {
            line << QString("%1").arg(l.at(r).at(c), c == 0 ? lengQuant : lengOxide);
        }

        qDebug() << line.join(" ");
    }
}

QStringList ChemTableData::getCsvList(ChemistryMode m, bool header, const QString &sampleId)
{
    switch (m) {
    case ELEMENT:
        if (!tableStructElements.hasData()) return QStringList();
        break;
    case ATOMIC:
        if (!tableStructAtoms.hasData()) return QStringList();
        break;
    case OXIDE:
        if (!tableStructOxides.hasData()) return QStringList();
        break;
    default:
        return QStringList();
        break;
    }

    if (!settingsParsed) initSettings();

    QString sep(";");
    QStringList lst;

    if (header) {
        QStringList columnHeaders(QString("File"));
        columnHeaders << QString("Sample");
        columnHeaders << QString("Sample ID");
        columnHeaders << QString("Phase");
        columnHeaders << QString("Phase Quantity (wt-%)");

        switch (m) {
        case ELEMENT:
            columnHeaders << QString("Element");
            columnHeaders << QString("Quantity (wt-%)");
            break;
        case ATOMIC:
            columnHeaders << QString("Element");
            columnHeaders << QString("Quantity (atm-%)");
            break;
        case OXIDE:
            columnHeaders << QString("Oxide");
            columnHeaders << QString("Quantity (wt-%)");
            break;
        default:
            return lst;
            break;
        }

        lst += columnHeaders.join(sep);
    }

    QList<QStringList> data;
    QStringList vHeader, hHeader;

    switch (m) {
    case ELEMENT:
        data = tableStructToStringList(tableStructElements, m, vHeader, hHeader, false);
        break;
    case ATOMIC:
        data = tableStructToStringList(tableStructAtoms, m, vHeader, hHeader, false);
        break;
    case OXIDE:
        data = tableStructToStringList(tableStructOxides, m, vHeader, hHeader, false);
        break;
    default:
        return lst;
        break;
    }

    for (int r = 0; r < data.size(); ++r) {
        const QStringList &row = data[r];

        for (int c = 1; c < row.size(); ++c) {
            QStringList line(lstFileName);  // File name with path
            line << sampleName;             // Sample name
            line << sampleId;               // Sample ID
            line << vHeader.at(r);          // Phase name
            line << row.constFirst();       // Phase quantity
            line << hHeader.at(c);          // Element name
            line << row.at(c);              // Element value

            lst << line.join(sep);
        }
    }

    return lst;
}

QString ChemTableData::getCsvTable(ChemistryMode m)
{
    switch (m) {
    case ELEMENT:
        if (!tableStructElements.hasData()) return QString();
        break;
    case ATOMIC:
        if (!tableStructAtoms.hasData()) return QString();
        break;
    case OXIDE:
        if (!tableStructOxides.hasData()) return QString();
        break;
    default:
        return QString();
        break;
    }

    if (!settingsParsed) initSettings();

    QString sep = settings->value("config/clipboardFieldSeparator", ";").toString();
    QStringList lst;

    QList<QStringList> data;
    QStringList vHeader, hHeader;

    switch (m) {
    case ELEMENT:
        data = tableStructToStringList(tableStructElements, m, vHeader, hHeader, true);
        break;
    case ATOMIC:
        data = tableStructToStringList(tableStructAtoms, m, vHeader, hHeader, true);
        break;
    case OXIDE:
        data = tableStructToStringList(tableStructOxides, m, vHeader, hHeader, true);
        break;
    default:
        return QString();
        break;
    }

    lst << QString("\"Phase\"%1\"%2\"").arg(sep, hHeader.join("\"" + sep + "\""));

    for (int r = 0; r < data.size(); ++r) {
        lst << QString("\"%1\"%2%3").arg(vHeader.at(r), sep, data.at(r).join(sep));
    }

    return lst.join("\n");
}

QString ChemTableData::getHtmlTable(ChemistryMode m)
{
    switch (m) {
    case ELEMENT:
        if (!tableStructElements.hasData()) return QString();
        break;
    case ATOMIC:
        if (!tableStructAtoms.hasData()) return QString();
        break;
    case OXIDE:
        if (!tableStructOxides.hasData()) return QString();
        break;
    default:
        return QString();
        break;
    }

    if (!settingsParsed) initSettings();

    QList<QStringList> data;
    QStringList vHeader, hHeader;

    switch (m) {
    case ELEMENT:
        data = tableStructToStringList(tableStructElements, m, vHeader, hHeader, true);
        break;
    case ATOMIC:
        data = tableStructToStringList(tableStructAtoms, m, vHeader, hHeader, true);
        break;
    case OXIDE:
        data = tableStructToStringList(tableStructOxides, m, vHeader, hHeader, true);
        break;
    default:
        return QString();
        break;
    }

    QStringList lst("<table id=\"tableChemComposition\">");

    lst << QString("    <tr>");
    lst << QString("        <th>Phase</th>");

    for (int c = 0; c < hHeader.size(); ++c) {
        lst << QString("        <th>%1</th>").arg(hHeader.at(c));
    }

    lst << QString("    </tr>");

    for (int r = 0; r < data.size(); ++r) {
        const QStringList &row = data[r];

        lst << "    <tr>";
        lst << QString("        <td>%1</td>").arg(vHeader.at(r));

        for (int c = 0; c < row.size(); ++c) {
            lst << QString("        <td>%1</td>").arg(row.at(c));
        }

        lst << "    </tr>";
    }

    lst << "</table><br>";
    return lst.join("\n");
}
