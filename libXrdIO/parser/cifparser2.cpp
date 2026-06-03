/***************************************************************************
                          cifparser2.cpp  -  description
                             -------------------
    begin                : Wed Feb 03 20:30:00 CEST 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#include "cifparser2.h"
#include "bgmnfileio.h"
#include "functions.h"
#include <QRegularExpression>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

CifParser2::CifParser2()
{
}

CifParser2::CifParser2(const QString &f)
{
    cifFile = QFileInfo(f);
    parseSourceString(BgmnFileIO::readTextFile(f), f);
}

void CifParser2::parseSourceString(const QString &s, const QString &f)
{
    cifFile = QFileInfo(f);
    cifStr = s.split(global::rxLineEnding);
    QList<QMap<QString, QStringList> > dataStructure = parseCif(cifStr);
    QList<QMap<QString, QStringList> > dataUnified = unifyDataBlocks(dataStructure);

    for (int i = 0; i < dataUnified.size(); ++i) {
        for (auto it = dataUnified.at(i).cbegin(); it != dataUnified.at(i).cend(); ++it) {
            qDebug() << QString("%1 %2 = %3").arg(i).arg(it.key()).arg(it.value().join(" "));
        }
    }

    for (int i = 0; i < dataUnified.size(); ++i) {
        cifDataCrystStructures.append(toStructure(dataUnified.at(i)));
    }
}

QString CifParser2::cifString() const
{
    return cifStr.join("\n");
}

CrystalStructure CifParser2::getCrystalStructure(int n, bool *ok) const
{
    if (ok) *ok = (n < cifDataCrystStructures.size() && n >= 0);
    return cifDataCrystStructures.at(n);
}

QList<CrystalStructure> CifParser2::getAllCrystalStructures(bool *ok) const
{
    if (ok) *ok = !cifDataCrystStructures.isEmpty();
    return cifDataCrystStructures;
}

QList<QMap<QString, QStringList> > CifParser2::parseCif(const QStringList &l)
{
    QList<QMap<QString, QStringList> > data;

    QStringList contentNoComments = stripComments(l);
    QList<QStringList> dataBlocks = splitDataBlocks(contentNoComments);

    for (int i = 0; i < dataBlocks.size(); ++i) {
        QStringList tokens = tokenize(dataBlocks.at(i));
        data.append(parseTokens(tokens));
    }

    return data;
}

/*
 * Some cif file (e.g. from SpringerMaterials) use a data block for phase information, and one or several
 * data blocks for structure information (e.g. standardized cell, published cell, reduced cell). Each
 * data block by itself is incomplete. Here we unify them to complete blocks and make sure that the
 * published cell appears first in the list.
 */
QList<QMap<QString, QStringList> > CifParser2::unifyDataBlocks(const QList<QMap<QString, QStringList> > &l)
{
    // if only one data block, nothing to unify
    if (l.size() <= 1) return l;

    // check if it is a SpringerMaterials file. If not, do nothing
    if (l.constFirst().value("data", QStringList()).constFirst() == "sm_global") return unifyDataBlocksSpringerMaterials(l);

    // check if the file contains a data_global header (for example CIFs created by Profex
    if (l.constFirst().value("data", QStringList()).constFirst() == "global") return unifyDataBlocksGlobalHeader(l);

    return l;
}

QList<QMap<QString, QStringList> > CifParser2::unifyDataBlocksSpringerMaterials(const QList<QMap<QString, QStringList> > &l)
{
    static QRegularExpression rxStdCell("sm_isp_SD(\\d+)-standardized_unitcell");
    static QRegularExpression rxPubCell("sm_isp_SD(\\d+)-published_cell");
    static QRegularExpression rxRedCell("sm_isp_SD(\\d+)-niggli_reduced_cell");

    int idxStdCell = -1;
    int idxPubCell = -1;
    int idxRedCell = -1;
    QString dbCode;

    for (int i = 0; i < l.size(); ++i) {
        QString d = l.at(i).value("data", QStringList("")).constFirst();

        QRegularExpressionMatch rm;

        if (idxStdCell < 0) {
            rm = rxStdCell.match(d);
            if (rm.hasMatch()) {
                dbCode = rm.captured(1);
                idxStdCell = i;
            }
        }

        if (idxPubCell < 0) {
            rm = rxPubCell.match(d);
            if (rm.hasMatch()) {
                dbCode = rm.captured(1);
                idxPubCell = i;
            }
        }

        if (idxRedCell < 0) {
            rm = rxRedCell.match(d);
            if (rm.hasMatch()) {
                dbCode = rm.captured(1);
                idxRedCell = i;
            }
        }
    }

    QList<QMap<QString, QStringList> > unified;

    if (idxPubCell > 0) { // return the published cell first
        QMap<QString, QStringList> m = l.constFirst();
        m["_database_code_sm"] = QStringList(QString("isp_sd_") + dbCode);
        m.insert(l.at(idxPubCell));
        unified.append(m);
    }

    if (idxStdCell > 0) {
        QMap<QString, QStringList> m = l.constFirst();
        m["_database_code_sm"] = QStringList(QString("isp_sd_") + dbCode);
        m.insert(l.at(idxStdCell));
        unified.append(m);
    }

    if (idxRedCell > 0) {
        QMap<QString, QStringList> m = l.constFirst();
        m["_database_code_sm"] = QStringList(QString("isp_sd_") + dbCode);
        m.insert(l.at(idxRedCell));
        unified.append(m);
    }

    return unified;
}

QList<QMap<QString, QStringList> > CifParser2::unifyDataBlocksGlobalHeader(const QList<QMap<QString, QStringList> > &l)
{
    if (l.size() < 2) return l;

    const QMap<QString, QStringList> header = l.constFirst();
    QList<QMap<QString, QStringList> > unified;

    for (int i = 1; i < l.size(); ++i) {
        QMap<QString, QStringList> block = l.at(i);
        block.insert(header);
        unified.append(block);
    }

    return unified;
}

QStringList CifParser2::stripComments(const QStringList &l)
{
    QStringList nc;
    bool captureCommentHeader = true;
    commentHeader.clear();

    for (int i = 0; i < l.size(); ++i) {
        if (!l.at(i).contains("#")) {
            nc.append(l.at(i));
            captureCommentHeader = false;
        } else {
            int n = l.at(i).indexOf("#");
            if (n > 0) {
                nc.append(l.at(i).left(n));
            } else {
                if (captureCommentHeader) commentHeader.append(l.at(i));
            }
        }
    }

    return nc;
}

QList<QStringList> CifParser2::splitDataBlocks(const QStringList &l)
{
    QList<QStringList> d;
    static QRegularExpression rx("^data_.+");

    int dataStart = l.indexOf(rx, 0);

    while (dataStart >= 0) {
        int dataLen = -1;
        int dataEnd = l.indexOf(rx, dataStart + 1);
        if (dataEnd >= 0) dataLen = dataEnd - dataStart;

        d.append(l.mid(dataStart, dataLen));
        dataStart = dataEnd;
    }

    return d;
}

QStringList CifParser2::tokenize(const QStringList &l)
{
    static QRegularExpression rx("'([^']*)'|\"([^\"]*)\"|([^\\s'\"]+)");

    QStringList tokens;

    for (int i = 0; i < l.size(); ++i) {
        if (l.at(i).left(1) == ";") {
            tokens.append(multiLineToken(l, i));
        } else {
            QRegularExpressionMatch rm = rx.match(l.at(i), 0);

            while (rm.hasMatch()) {
                if      (!rm.captured(1).isNull())  tokens.append(rm.captured(1));
                else if (!rm.captured(2).isNull())  tokens.append(rm.captured(2));
                else if (!rm.captured(3).isNull())  tokens.append(rm.captured(3));
                else                                tokens.append(rm.captured(rm.lastCapturedIndex()));

                rm = rx.match(l.at(i), rm.capturedEnd(0) + 1);
            }
        }
    }

    return tokens;
}

QString CifParser2::multiLineToken(const QStringList &l, int &n)
{
    if (n >= l.size()) return QString();
    QStringList tk;

    if (l.at(n).size() > 1) {
        // append what's following the opening ;
        tk.append(l.at(n).mid(1, -1));
    }

    int i = n + 1;

    while (i < l.size()) {
        // loop till the end of the list
        if (l.at(i).size() > 0) {
            // line contains text
            if (l.at(i).at(0) != QLatin1Char(';')) {
                // append the line
                tk.append(l.at(i));
                ++i;
            } else {
                // closing ; found, exit the loop
                break;
            }
        } else {
            // empty lines will be skipped
            ++i;
        }
    }

    n = i;
    // return the entire multi-line token as one line
    return tk.join(" ");
}

QMap<QString, QStringList> CifParser2::parseTokens(const QStringList &l)
{
    QMap<QString, QStringList> data;
    int i = 0;

    while (i < l.size()) {
        if (l.at(i).left(5) == "data_") {
            data["data"].append(l.at(i).mid(5));
            ++i;
        } else if (l.at(i).left(1) == "_") {
            i = parseParameter(data, l, i);
        } else if (l.at(i).left(5) == "loop_") {
            i = parseLoop(data, l, i);
        } else {
            ++i;
        }

        if (i < 0) break;
    }

    return data;
}

CrystalStructure CifParser2::toStructure(const QMap<QString, QStringList> &m)
{
    CrystalStructure cStructure;
    cStructure.setName(getPhaseName(m));
    cStructure.unitCell().setAxisUnit(NM);

    cStructure.setAuxInfo("Reference", getDatabaseCode(m));
    cStructure.setAuxInfo("Formula", getFormula(m));

    cStructure.unitCell().setItNumber(getITnum(m));
    cStructure.unitCell().setSpaceGroupHMCif(getHMsymb(m));

    double cell_a  = getCellA(m);
    double cell_b  = getCellB(m);
    double cell_c  = getCellC(m);
    double cell_al = getCellAlpha(m);
    double cell_be = getCellBeta(m);
    double cell_ga = getCellGamma(m);

    if ((cell_a  > 0.0) && !qFuzzyIsNull(cell_a)) cStructure.unitCell().setA(cell_a);
    if ((cell_b  > 0.0) && !qFuzzyIsNull(cell_b)) cStructure.unitCell().setB(cell_b);
    if ((cell_c  > 0.0) && !qFuzzyIsNull(cell_c)) cStructure.unitCell().setC(cell_c);
    if ((cell_al > 0.0) && !qFuzzyIsNull(cell_al)) cStructure.unitCell().setAlpha(cell_al);
    if ((cell_be > 0.0) && !qFuzzyIsNull(cell_be)) cStructure.unitCell().setBeta(cell_be);
    if ((cell_ga > 0.0) && !qFuzzyIsNull(cell_ga)) cStructure.unitCell().setGamma(cell_ga);

    cStructure.unitCell().fixCell();

    QList<CrystalAtom> atms = getAtomList(m, cStructure);

    for (int i = 0; i < atms.size(); ++i) {
        cStructure.addAtom(atms.at(i));
    }

    QList<QStringList> sOps = getSymOps(m);
    cStructure.unitCell().setSymmetryOperations(sOps);
    return cStructure;
}

int CifParser2::parseParameter(QMap<QString, QStringList> &m, const QStringList &l, int i)
{
    if (i >= l.size() - 1) return -1;

    QString key = l.at(i);
    QString value = l.at(i+1);

    m[key].append(value);
    return i+1;
}

int CifParser2::parseLoop(QMap<QString, QStringList> &m, const QStringList &l, int i)
{
    QStringList header;
    int n = i+1;

    while (l.at(n).left(1) == "_") {
        if (n >= l.size()) break;
        header.append(l.at(n));
        ++n;
    }

    int k = 0;

    // the loop ends when we detect "loop_", something stating with "_", or the end of the list
    while (n < l.size()) {
        if (l.at(n) == "loop_") return n;
        if (l.at(n).left(1) == "_") return n;

        m[header.at(k)].append(l.at(n));
        ++n;
        ++k;
        if (k >= header.size()) k = 0;
    }

    return n+1;
}

QStringList CifParser2::getParameter(const QString &name, const QMap<QString, QStringList> &m)
{
    // this guarantees that we can always access the QStringList::first() element of the returned
    // list without having to check for the size each time.
    return m.value(name, QStringList(ERRSTR));
}


QString CifParser2::stripStdDev(const QString &s)
{
    // used to extract values, considering standard deviations (d.ddd(d))
    static QRegularExpression rxV("([\\d\\.Ee\\+-]+)(?:\\(\\d*\\))");
    QRegularExpressionMatch match;

    match = rxV.match(s);

    if (match.hasMatch()) {
        return match.captured(1);
    }

    return s;
}

QString CifParser2::getDatabaseCode(const QMap<QString, QStringList> &m)
{
    QString dbName;
    QString dbRec;
    static QRegularExpression rxMatch("_?([A-Za-z]+)?_database_code_?([A-Za-z]+)?");
    static QRegularExpression rxReplace("\\s+");
    QRegularExpressionMatch match;

    QMapIterator<QString, QStringList> it(m);
    while (it.hasNext()) {
        it.next();
        match = rxMatch.match(it.key());

        if (match.hasMatch()) {
            if (!match.captured(1).isEmpty()) dbName = match.captured(1);
            if (!match.captured(2).isEmpty()) dbName = match.captured(2);
            dbRec = it.value().first();
            return dbName + dbRec.replace(rxReplace, "_");
        }
    }

    return QString();
}

QString CifParser2::getPhaseName(const QMap<QString, QStringList> &m)
{
    QString out = getParameter("_chemical_name_mineral", m).constFirst();

    if (out == ERRSTR) out = getParameter("_chemical_name_common", m).constFirst();
    if (out == ERRSTR) out = getParameter("_chemical_name", m).constFirst();
    if (out == ERRSTR) out = getParameter("_chemical_name_systematic", m).constFirst();

    static QRegularExpression rx("[^A-Za-z0-9]");

    // here we can have invalid strings, for example "_chemical_name_mineral ' ?'"
    // after replacing invalid symbols, the string may be empty
    out = out.replace(rx, QString()).simplified();

    // if the phase name is invalid at this point, fall back to the file name
    if ((out == ERRSTR) || out.isEmpty()) {
        out = cifFile.completeBaseName();
        out = out.replace(rx, QString());
    }

    return out;
}

QString CifParser2::getFormula(const QMap<QString, QStringList> &m)
{
    QString out = getParameter("_chemical_formula_sum", m).constFirst();

    if (out == ERRSTR) out = getParameter("_chemical_formula_structural", m).constFirst();
    if (out == ERRSTR) out = getParameter("_chemical_formula_moiety", m).constFirst();
    if (out == ERRSTR) out = getParameter("_chemical_formula_analytical", m).constFirst();

    static QRegularExpression rx("\\s+");
    return out.replace(rx, "_");
}

int CifParser2::getITnum(const QMap<QString, QStringList> &m)
{
    QString out = getParameter("_space_group_IT_number", m).constFirst();                 // new tag name
    if (out == ERRSTR) out = getParameter("_symmetry_Int_Tables_number", m).constFirst(); // old tag name
    if (out == ERRSTR) return 0;
    return out.toInt();
}

QString CifParser2::getHMsymb(const QMap<QString, QStringList> &m)
{
    QString out = getParameter("_space_group_name_H-M_alt", m).constFirst();                 // new tag name
    if (out == ERRSTR) out = getParameter("_symmetry_space_group_name_H-M", m).constFirst(); // old tag name

    // the cif dictionary allows some additions to the HM symbol, e.g. info in () or :origin
    // we need to clip these
    static QRegularExpression rx("^([ABCFIPRabcfipr][abcdmnABCDMN123456_\\s/-]+)");
    QRegularExpressionMatch rm = rx.match(out);

    if (rm.hasMatch()) return rm.captured(1).trimmed();
    return out;
}

/*
 * CIF symops contain translated ones, we must not generate them
 */
QList<QStringList> CifParser2::getSymOps(const QMap<QString, QStringList> &m)
{
    QList<QStringList> out;

    QStringList l;
    if (m.contains("_space_group_symop_operation_xyz")) {
        l = m.value("_space_group_symop_operation_xyz");
    } else if (m.contains("_symmetry_equiv_pos_as_xyz")) {
        l = m.value("_symmetry_equiv_pos_as_xyz");
    }

    static QRegularExpression rx("[, ]+");
    for (int i = 0; i < l.size(); ++i) {
        QStringList s = l.at(i).split(rx);
        if (s.size() == 3) out.append(s);
    }

    return out;
}

double CifParser2::getCellA(const QMap<QString, QStringList> &m)
{
    return getCellParam(m, "_cell_length_a") / 10.0;
}

double CifParser2::getCellB(const QMap<QString, QStringList> &m)
{
    return getCellParam(m, "_cell_length_b") / 10.0;
}

double CifParser2::getCellC(const QMap<QString, QStringList> &m)
{
    return getCellParam(m, "_cell_length_c") / 10.0;
}

double CifParser2::getCellAlpha(const QMap<QString, QStringList> &m)
{
    return getCellParam(m, "_cell_angle_alpha");
}

double CifParser2::getCellBeta(const QMap<QString, QStringList> &m)
{
    return getCellParam(m, "_cell_angle_beta");
}

double CifParser2::getCellGamma(const QMap<QString, QStringList> &m)
{
    return getCellParam(m, "_cell_angle_gamma");
}

double CifParser2::getCellParam(const QMap<QString, QStringList> &m, const QString &s)
{
    QString val = getParameter(s, m).constFirst();

    if (val == ERRSTR) {
        qDebug() << QString("CifParser2::getCellParam(): Parameter %1 not found").arg(s);
        return -1.0;
    }

    return stripStdDev(val).toDouble();
}

QList<CrystalAtom> CifParser2::getAtomList(const QMap<QString, QStringList> &m, const CrystalStructure &cStructure)
{
    QList<CrystalAtom> mout;
    QMap<QString, QMap<int, QString> > data = getAtomDataStructure(m);

    QList<int> atSites = data.value("_atom_site_label").keys();

    for (int i = 0; i < atSites.size(); ++i) {
        int n = atSites.at(i);

        if (getAtomIsDummy(data, n)) continue;

        double x = -1.0;
        double y = -1.0;
        double z = -1.0;

        if (!getAtomCoordinates(data, n, x, y, z)) continue;

        double biso = 0.0;
        if (!getAtomBiso(data, n, biso, cStructure)) biso = 0.0;

        double occ = 1.0;
        if (!getAtomOccupancy(data, n, occ)) occ = 1.0;

        QString l = data.value("_atom_site_label").value(n);
        CrystalAtom atom(l, x, y, z, occ, biso);

        QString el = l;
        if (getAtomType(data, n, el)) atom.setElement(el);

        QString wyck;
        int mult;
        if (getAtomWyckoffAndMultiplicity(data, n, wyck, mult)) {
            atom.setWyckoff(wyck);
            atom.setMultiplicity(mult);
        }

        mout.append(atom);
    }

    return mout;
}

/*
 * for controlled access, we create the following structure first:
 *
 * QMap<QString(_cif_tag), QMap<int(atom number), QString(_cif_value)> >
 *
 * This helps accessing the data because not all atom sites have all cif tags.
 */
QMap<QString, QMap<int, QString> > CifParser2::getAtomDataStructure(const QMap<QString, QStringList> &m)
{
    QMap<QString, QMap<int, QString> > data;

    QStringList laSiteLabel                = m.value("_atom_site_label", QStringList());
    QStringList laSiteTypeSymbol           = m.value("_atom_site_type_symbol", QStringList());
    QStringList laSiteWyckoffSymbol        = m.value("_atom_site_Wyckoff_symbol", QStringList());
    QStringList laSiteSymmetryMultiplicity = m.value("_atom_site_symmetry_multiplicity", QStringList());
    QStringList laSiteOccupancy            = m.value("_atom_site_occupancy", QStringList());
    QStringList laSiteFractX               = m.value("_atom_site_fract_x", QStringList());
    QStringList laSiteFractY               = m.value("_atom_site_fract_y", QStringList());
    QStringList laSiteFractZ               = m.value("_atom_site_fract_z", QStringList());
    QStringList laSiteBiso                 = m.value("_atom_site_B_iso_or_equiv", QStringList());
    QStringList laSiteUiso                 = m.value("_atom_site_U_iso_or_equiv", QStringList());
    QStringList laSiteCalcFlag             = m.value("_atom_site_calc_flag", QStringList());

    int nAt = qMax(laSiteLabel.size(), laSiteTypeSymbol.size());

    // these are essential
    if ((nAt == 0) || laSiteFractX.isEmpty() || laSiteFractY.isEmpty() || laSiteFractZ.isEmpty()) {
        qDebug() << QString("CifParser2::getAtomDataStructure(): Essential parameters are missing. Exiting.");
        return data;
    }

    QStringList laAnisoLabel  = m.value("_atom_site_aniso_label", QStringList());
    QStringList laAnisoB11    = m.value("_atom_site_aniso_B_11", QStringList());
    QStringList laAnisoB12    = m.value("_atom_site_aniso_B_12", QStringList());
    QStringList laAnisoB13    = m.value("_atom_site_aniso_B_13", QStringList());
    QStringList laAnisoB22    = m.value("_atom_site_aniso_B_22", QStringList());
    QStringList laAnisoB23    = m.value("_atom_site_aniso_B_23", QStringList());
    QStringList laAnisoB33    = m.value("_atom_site_aniso_B_33", QStringList());

    QStringList laAnisoU11    = m.value("_atom_site_aniso_U_11", QStringList());
    QStringList laAnisoU12    = m.value("_atom_site_aniso_U_12", QStringList());
    QStringList laAnisoU13    = m.value("_atom_site_aniso_U_13", QStringList());
    QStringList laAnisoU22    = m.value("_atom_site_aniso_U_22", QStringList());
    QStringList laAnisoU23    = m.value("_atom_site_aniso_U_23", QStringList());
    QStringList laAnisoU33    = m.value("_atom_site_aniso_U_33", QStringList());

    for (int i = 0; i < nAt; ++i) {
        if (i < laSiteLabel.size())                data["_atom_site_label"][i]                 = laSiteLabel.at(i);
        if (i < laSiteTypeSymbol.size())           data["_atom_site_type_symbol"][i]           = laSiteTypeSymbol.at(i);
        if (i < laSiteWyckoffSymbol.size())        data["_atom_site_Wyckoff_symbol"][i]        = laSiteWyckoffSymbol.at(i);
        if (i < laSiteSymmetryMultiplicity.size()) data["_atom_site_symmetry_multiplicity"][i] = laSiteSymmetryMultiplicity.at(i);
        if (i < laSiteOccupancy.size())            data["_atom_site_occupancy"][i]             = laSiteOccupancy.at(i);
        if (i < laSiteFractX.size())               data["_atom_site_fract_x"][i]               = laSiteFractX.at(i);
        if (i < laSiteFractY.size())               data["_atom_site_fract_y"][i]               = laSiteFractY.at(i);
        if (i < laSiteFractZ.size())               data["_atom_site_fract_z"][i]               = laSiteFractZ.at(i);
        if (i < laSiteBiso.size())                 data["_atom_site_B_iso_or_equiv"][i]        = laSiteBiso.at(i);
        if (i < laSiteUiso.size())                 data["_atom_site_U_iso_or_equiv"][i]        = laSiteUiso.at(i);
        if (i < laSiteCalcFlag.size())             data["_atom_site_calc_flag"][i]             = laSiteCalcFlag.at(i);
    }

    for (int i = 0; i < laAnisoLabel.size(); ++i) {
        if (i < laAnisoB11.size()) data["_atom_site_aniso_B_11"][i] = laAnisoB11.at(i);
        if (i < laAnisoB12.size()) data["_atom_site_aniso_B_12"][i] = laAnisoB12.at(i);
        if (i < laAnisoB13.size()) data["_atom_site_aniso_B_13"][i] = laAnisoB13.at(i);
        if (i < laAnisoB22.size()) data["_atom_site_aniso_B_22"][i] = laAnisoB22.at(i);
        if (i < laAnisoB23.size()) data["_atom_site_aniso_B_23"][i] = laAnisoB23.at(i);
        if (i < laAnisoB33.size()) data["_atom_site_aniso_B_33"][i] = laAnisoB33.at(i);

        if (i < laAnisoU11.size()) data["_atom_site_aniso_U_11"][i] = laAnisoU11.at(i);
        if (i < laAnisoU12.size()) data["_atom_site_aniso_U_12"][i] = laAnisoU12.at(i);
        if (i < laAnisoU13.size()) data["_atom_site_aniso_U_13"][i] = laAnisoU13.at(i);
        if (i < laAnisoU22.size()) data["_atom_site_aniso_U_22"][i] = laAnisoU22.at(i);
        if (i < laAnisoU23.size()) data["_atom_site_aniso_U_23"][i] = laAnisoU23.at(i);
        if (i < laAnisoU33.size()) data["_atom_site_aniso_U_33"][i] = laAnisoU33.at(i);
    }

    return data;
}

bool CifParser2::getAtomIsDummy(const QMap<QString, QMap<int, QString> > &d, int n)
{
    if (!d.value("_atom_site_calc_flag").contains(n)) return false;
    return d.value("_atom_site_calc_flag").value(n) == "dum";
}

bool CifParser2::getAtomCoordinates(const QMap<QString, QMap<int, QString> > &d, int n, double &x, double &y, double &z)
{
    if (!d.value("_atom_site_fract_x").contains(n)) return false;
    if (!d.value("_atom_site_fract_y").contains(n)) return false;
    if (!d.value("_atom_site_fract_z").contains(n)) return false;

    bool ok;
    x = stripStdDev(d.value("_atom_site_fract_x").value(n)).toDouble(&ok);
    if (!ok) return false;
    y = stripStdDev(d.value("_atom_site_fract_y").value(n)).toDouble(&ok);
    if (!ok) return false;
    z = stripStdDev(d.value("_atom_site_fract_z").value(n)).toDouble(&ok);
    if (!ok) return false;

    global::Functions::normalizeCoordinates(x, y, z);
    return true;
}

bool CifParser2::getAtomType(const QMap<QString, QMap<int, QString> > &d, int n, QString &t)
{
    if (!d["_atom_site_type_symbol"].contains(n)) return false;

    // we store the oxidation state in conventional format "At2+".
    // It is converted to BGMN format "At+2" elsewhere when needed
    static QRegularExpression rx("([A-Za-z]+)([\\+-]?)(\\d*)([\\+-]?)");
    QRegularExpressionMatch rm = rx.match(d.value("_atom_site_type_symbol").value(n));

    if (rm.hasMatch()) {
        // 2, 3, and 4 can be empty. Either 2 or 4 must be empty, therefore
        // there will only be one sign (4 or 2) after the digit (3)
        t = rm.captured(1) + rm.captured(3) + rm.captured(4) + rm.captured(2);
        return true;
    }

    return false;
}

bool CifParser2::getAtomBiso(const QMap<QString, QMap<int, QString> > &d, int n, double &b, const CrystalStructure &cStructure)
{
    bool ok;

    if (d["_atom_site_B_iso_or_equiv"].contains(n)) {
        b = stripStdDev(d.value("_atom_site_B_iso_or_equiv").value(n)).toDouble(&ok);
        return ok;
    }

    if (d["_atom_site_U_iso_or_equiv"].contains(n)) {
        double u = stripStdDev(d.value("_atom_site_U_iso_or_equiv").value(n)).toDouble(&ok);
        b = global::Functions::bisoFromUiso(u);
        return ok;
    }

    if (d["_atom_site_aniso_B_11"].contains(n)) {
        double b11 = stripStdDev(d.value("_atom_site_aniso_B_11").value(n)).toDouble(&ok);
        if (!ok) return false;
        double b12 = stripStdDev(d.value("_atom_site_aniso_B_12").value(n)).toDouble(&ok);
        if (!ok) return false;
        double b13 = stripStdDev(d.value("_atom_site_aniso_B_13").value(n)).toDouble(&ok);
        if (!ok) return false;
        double b22 = stripStdDev(d.value("_atom_site_aniso_B_22").value(n)).toDouble(&ok);
        if (!ok) return false;
        double b23 = stripStdDev(d.value("_atom_site_aniso_B_23").value(n)).toDouble(&ok);
        if (!ok) return false;
        double b33 = stripStdDev(d.value("_atom_site_aniso_B_33").value(n)).toDouble(&ok);
        if (!ok) return false;

        // warning: angles may be wrong because we have not yet determined the crystal system
        b = global::Functions::bisoFromBaniso(cStructure.unitCell().alpha(), cStructure.unitCell().beta(), cStructure.unitCell().gamma(),
                                              b11, b22, b33, b12, b13, b23);
        return true;
    }

    if (d["_atom_site_aniso_U_11"].contains(n)) {
        double u11 = stripStdDev(d.value("_atom_site_aniso_U_11").value(n)).toDouble(&ok);
        if (!ok) return false;
        double u12 = stripStdDev(d.value("_atom_site_aniso_U_12").value(n)).toDouble(&ok);
        if (!ok) return false;
        double u13 = stripStdDev(d.value("_atom_site_aniso_U_13").value(n)).toDouble(&ok);
        if (!ok) return false;
        double u22 = stripStdDev(d.value("_atom_site_aniso_U_22").value(n)).toDouble(&ok);
        if (!ok) return false;
        double u23 = stripStdDev(d.value("_atom_site_aniso_U_23").value(n)).toDouble(&ok);
        if (!ok) return false;
        double u33 = stripStdDev(d.value("_atom_site_aniso_U_33").value(n)).toDouble(&ok);
        if (!ok) return false;

        b = global::Functions::bisoFromUaniso(cStructure.unitCell().alpha(), cStructure.unitCell().beta(), cStructure.unitCell().gamma(),
                                              u11, u22, u33, u12, u13, u23);
        return true;
    }

    return false;
}

bool CifParser2::getAtomOccupancy(const QMap<QString, QMap<int, QString> > &d, int n, double &o)
{
    if (!d.value("_atom_site_occupancy").contains(n)) return false;

    bool ok;
    o = stripStdDev(d.value("_atom_site_occupancy").value(n)).toDouble(&ok);
    if (!ok) o = 1.0;
    return true;
}

bool CifParser2::getAtomWyckoffAndMultiplicity(const QMap<QString, QMap<int, QString> > &d, int n, QString &w, int &m)
{
    if (!d.value("_atom_site_Wyckoff_symbol").contains(n)) return false;

    static QRegularExpression rx("(\\d*)([a-z]+)");
    QRegularExpressionMatch rm = rx.match(d.value("_atom_site_Wyckoff_symbol").value(n));

    if (rm.hasMatch()) {
        bool ok;
        w = rm.captured(2);
        m = rm.captured(1).toInt(&ok);
        if (!ok) m = 1;
        return true;
    }

    return false;
}

