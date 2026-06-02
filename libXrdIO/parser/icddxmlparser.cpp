/***************************************************************************
                          icddxmlparser.cpp  -  description
                             -------------------
    begin                : Fri Mar 13 10:37:00 CEST 2015
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

#include "icddxmlparser.h"
#include "../functions.h"
#include <QFile>
#include <QMap>
#include <QRegularExpression>
#include <QDebug>
#include <math.h>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

IcddXmlParser::IcddXmlParser()
{
}

IcddXmlParser::IcddXmlParser(const QString &f)
{
    fileName = f;
    QFile file(f);

    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << QString("IcddXmlParser::IcddXmlParser(): Could not open file %1").arg(f);
        return;
    }

    parseSourceString(file.readAll(), f);
    file.close();
}

void IcddXmlParser::parseSourceString(const QString &s, const QString &f)
{
    fileName = f;
    icddXML.setContent(s);
    crystalStructure.clear();
    parseXml(icddXML);
}

void IcddXmlParser::parseXml(const QDomDocument &inDoc)
{
    static QRegularExpression rxNotAlphaNum("[^A-Za-z0-9]");
    static QRegularExpression rxSpace("\\s+");
    // static QRegularExpression rxSpacegroup("([^ ]+) ?\\(?(\\d*)\\)?");  // captures things like "P21/n (14)", "P21/n", or "R3cH (161)"
    static QRegularExpression rxSpacegroup("([A-Z][^ RH]+)(?:[RH]? \\((\\d+)\\))?");  // captures things like "P21/n (14)", "P21/n", or "R3cH (161)"
    static QRegularExpression rxUcell("(\\d+\\.?\\d*)\\(?\\d*\\)?");
    static QRegularExpression rxWyck("(\\d+)([a-z])");

    crystalStructure.setName(getPhaseName(inDoc).replace(rxNotAlphaNum, QString()).trimmed());
    CrystalUnitCell unitcell;
    unitcell.setAxisUnit(NM);

    QString dbRef = getStringByTagName("pdf_number", inDoc);
    if (!dbRef.isEmpty()) crystalStructure.setAuxInfo("Reference", dbRef);

    QString formula = getStringByTagName("chemical_formula", inDoc);
    if (!formula.isEmpty()) crystalStructure.setAuxInfo("Formula", formula.replace(rxSpace, "_"));

    QString spaceGroup = getStringByTagName("ac_space_group", inDoc);
    if (spaceGroup.isEmpty()) spaceGroup = getStringByTagName("spgr", inDoc);
    QRegularExpressionMatch rmSpacegroup = rxSpacegroup.match(spaceGroup);

    qDebug() << QString("IcddXmlParser::parseXml(): HM Symbol = %1").arg(rmSpacegroup.captured(1));
    qDebug() << QString("IcddXmlParser::parseXml(): SG Number = %1").arg(rmSpacegroup.captured(2));

    if (rmSpacegroup.hasMatch()) {
        unitcell.setSpaceGroupHMCif(rmSpacegroup.captured(1).trimmed());
        unitcell.setItNumber(rmSpacegroup.captured(2).trimmed().toInt());
    }

    QRegularExpressionMatch rmUcell;

    QString strCellA  = getStringByTagName(hasElement("ac_unit_cell_a",     inDoc) ? "ac_unit_cell_a"     : "cell_a",     inDoc);
    QString strCellB  = getStringByTagName(hasElement("ac_unit_cell_b",     inDoc) ? "ac_unit_cell_b"     : "cell_b",     inDoc);
    QString strCellC  = getStringByTagName(hasElement("ac_unit_cell_c",     inDoc) ? "ac_unit_cell_c"     : "cell_c",     inDoc);
    QString strCellAl = getStringByTagName(hasElement("ac_unit_cell_alpha", inDoc) ? "ac_unit_cell_alpha" : "cell_alpha", inDoc);
    QString strCellBe = getStringByTagName(hasElement("ac_unit_cell_beta",  inDoc) ? "ac_unit_cell_beta"  : "cell_beta",  inDoc);
    QString strCellGa = getStringByTagName(hasElement("ac_unit_cell_gamma", inDoc) ? "ac_unit_cell_gamma" : "cell_gamma", inDoc);

    rmUcell = rxUcell.match(strCellA);
    if (rmUcell.hasMatch()) unitcell.setA(rmUcell.captured(1).toDouble() / 10.0);

    rmUcell = rxUcell.match(strCellB);
    if (rmUcell.hasMatch()) unitcell.setB(rmUcell.captured(1).toDouble() / 10.0);

    rmUcell = rxUcell.match(strCellC);
    if (rmUcell.hasMatch()) unitcell.setC(rmUcell.captured(1).toDouble() / 10.0);

    rmUcell = rxUcell.match(strCellAl);
    if (rmUcell.hasMatch()) unitcell.setAlpha(rmUcell.captured(1).toDouble());

    rmUcell = rxUcell.match(strCellBe);
    if (rmUcell.hasMatch()) unitcell.setBeta(rmUcell.captured(1).toDouble());

    rmUcell = rxUcell.match(strCellGa);
    if (rmUcell.hasMatch()) unitcell.setGamma(rmUcell.captured(1).toDouble());

    // if tds are stored as U, use this factor to convert to Bequiv
    double uisoConv = (getStringByTagName("tf_type", inDoc) == "U") ? M_PI * M_PI * 8.0 : 1.0;

    QDomNodeList atomList = inDoc.elementsByTagName("atomic_coord");

    for (int i = 0; i < atomList.size(); ++i) {
        int     number  = atomList.at(i).firstChildElement("num").text().toInt();
        QString element = atomList.at(i).firstChildElement("atom").text();
        QString name    = QString("%1%2").arg(element).arg(number);

        double x = atomList.at(i).firstChildElement("x").text().toDouble();
        double y = atomList.at(i).firstChildElement("y").text().toDouble();
        double z = atomList.at(i).firstChildElement("z").text().toDouble();
        double occupancy = atomList.at(i).firstChildElement("sof").text().toDouble();
        double biso = atomList.at(i).firstChildElement("idp").text().toDouble() * uisoConv;

        global::Functions::normalizeCoordinates(x, y, z);

        QRegularExpressionMatch rmWyck = rxWyck.match(atomList.at(i).firstChildElement("wyckoff").text());

        QString wyckoff;
        int multiplicity = 1;

        if (rmWyck.hasMatch()) {
            wyckoff = rmWyck.captured(2);
            multiplicity = rmWyck.captured(1).toInt();
        }

        CrystalAtom atom(name, x, y, z, occupancy, biso);
        atom.setElement(element);
        atom.setWyckoff(wyckoff);
        atom.setMultiplicity(multiplicity);
        crystalStructure.addAtom(atom);
    }

    unitcell.fixCell();

    QList<CrystalStructureFactor> fhkl = getStructureFactors(inDoc);
    unitcell.setSymmetryOperations(getSymOps(inDoc, unitcell));

    crystalStructure.setUnitCell(unitcell);
    if (fhkl.size()) crystalStructure.setStructureFactors(fhkl);
}

bool IcddXmlParser::hasElement(const QString &str, const QDomDocument &doc)
{
    return !doc.elementsByTagName(str).isEmpty();
}

QString IcddXmlParser::getStringByTagName(const QString &str, const QDomDocument &doc)
{
    QDomNodeList lst = doc.elementsByTagName(str);
    if (lst.isEmpty()) return QString();
    return lst.at(0).toElement().text();
}

QString IcddXmlParser::getPhaseName(const QDomDocument &doc)
{
    // first try to find the mineral name
    QString phaseName = getStringByTagName("mineralname", doc);
    if (!phaseName.isEmpty()) {
        return phaseName;
    }

    // if mineral name is not available, try chemical name
    phaseName = getStringByTagName("chemical_name", doc);
    if (!phaseName.isEmpty()) {
        return phaseName;
    }

    // fallback: return the file name
    return fileName;
}

QList<CrystalStructureFactor> IcddXmlParser::getStructureFactors(const QDomDocument &doc) const
{
    QList<CrystalStructureFactor> fhkl;

    QDomNodeList lst = doc.elementsByTagName("stick_series");
    if (lst.isEmpty()) return fhkl;

    QDomNodeList lhkl = lst.at(0).toElement().elementsByTagName("intensity");
    if (lhkl.isEmpty()) return fhkl;

    static QRegularExpression rx("(\\d+\\.?\\d*)m?");
    QRegularExpressionMatch rm;

    for (int i = 0; i < lhkl.size(); ++i) {
        QDomElement elDa = lhkl.at(i).firstChildElement("da");
        QDomElement elI  = lhkl.at(i).firstChildElement("intensity");
        QDomElement elH  = lhkl.at(i).firstChildElement("h");
        QDomElement elK  = lhkl.at(i).firstChildElement("k");
        QDomElement elL  = lhkl.at(i).firstChildElement("l");

        double dnm = 0.0;
        double intens = 0.0;
        int h = 0;
        int k = 0;
        int l = 0;

        if (!elDa.isNull()) dnm = elDa.text().toDouble() / 10.0;
        if (!elH.isNull())  h = elH.text().toInt();
        if (!elK.isNull())  k = elK.text().toInt();
        if (!elL.isNull())  l = elL.text().toInt();

        if (!elI.isNull())  {
            rm = rx.match(elI.text());
            if (rm.hasMatch()) {
                intens = rm.captured(1).toDouble();
            }
        }

        if (intens > 0.0) fhkl.append(CrystalStructureFactor(h, k, l, intens, dnm));
    }


    return fhkl;
}

/*
 * The symmetry operators in this file format do not contain the translated ones.
 * We must generate them here.
 */
QList<QStringList> IcddXmlParser::getSymOps(const QDomDocument &doc, const CrystalUnitCell &uc) const
{
    if (!uc.spaceGroupHMCif().size()) return QList<QStringList>();

    QString tr = uc.spaceGroupHMCif().left(1).toUpper();

    if (tr == "R") {
        if (((uc.a() == uc.b()) && (uc.a()) == uc.c())) tr = "RRH";
        else                                            tr = "RHX";
    }

    QDomNodeList l = doc.elementsByTagName("sg_sym_op");
    QList<CrystalSymOp> lsop;

    for (int i = 0; i < l.size(); ++i) {
        QDomElement elOp = l.at(i).firstChildElement("operator");
        CrystalSymOp sOp(elOp.text().split(","));
        lsop.append(sOp);
        lsop.append(sOp.translate(tr));
    }

    QList<QStringList> lout;

    for (int i = 0; i < lsop.size(); ++i) {
        lout.append(lsop.at(i).toStringList());
    }

    return lout;
}
