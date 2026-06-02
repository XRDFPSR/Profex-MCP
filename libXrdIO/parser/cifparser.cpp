/***************************************************************************
                          cifparser.cpp  -  description
                             -------------------
    begin                : Tue Aug 27 20:30:00 CEST 2013
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

#include "cifparser.h"
#include "../functions.h"
#include "../structs.h"
#include <math.h>
#include <QDebug>
#include <QFile>
#include <QRegularExpression>
#include <QDomDocument>
#include <QFileInfo>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

CifParser::CifParser(const QString &f)
{
    filename = f;
    qDebug() << QString("CifParser::CifParser(): Loading file %1").arg(filename);

    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        qDebug() << QString("CifParser::CifParser(): Could not open file %1").arg(filename);
        return;
    }

    cifOrig = file.readAll();
    file.close();

    cifStripped = stripComments(cifOrig);
    cifList = cifStripped.split(global::rxLineEnding);
    xmlDoc = getXmlDocument();
}

QString CifParser::stripComments(const QString &istr)
{
    QString ostr = istr;
    QRegularExpression rx("#[^\\n]*\\n");

    while (ostr.contains(rx)) {
        ostr.replace(rx, "\n");
    }

    return ostr;
}

QString CifParser::cifString() const
{
    return cifOrig;
}

/*
 * Creating XML document of the following structure
 *
<phase Name="b-TCP">
    <spacegroup Number="76" HermannMauguin="P 41"/>
    <unitcell>
        <axis Name="a" Unit="A">6.6858</axis>
        <axis Name="b" Unit="A">6.6858</axis>
        <axis Name="c" Unit="A">24.147</axis>
        <angle Name="alpha" Unit="deg">90.0</angle>
        <angle Name="beta" Unit="deg">90.0</angle>
        <angle Name="gamma" Unit="deg">90.0</angle>
    </unitcell>
    <atom Name="Ca1" Type="Ca" OxidationState="2+" Multiplicity="4" Wyckoff="a">
        <coordinates>0.1374 0.2313 0.0</coordinates>
        <occupancy>1.0</occupancy>
        <Uiso>0.0</Uiso>
        <Biso>0.0</Biso>
        <Uaniso>
            <U11>0.0044</U11>
            <U22>0.0043</U22>
            <U33>0.00027</U33>
            <U12>0.0025</U12>
            <U13>-0.00010</U13>
            <U23>0.00036</U23>
        </Uaniso>
        <Baniso>
            <B11>0.0</B11>
            <B22>0.0</B22>
            <B33>0.0</B33>
            <B12>0.0</B12>
            <B13>0.0</B13>
            <B23>0.0</B23>
        </Baniso>
    </atom>
    <symOps>
        <symOp x="x" y="y" z="z"/>
        <symOp x="x" y="x+1/2" z="-z-1/2"/>
        <symOp x="..." y="..." z="..."/>
    </symOps>
</phase>
*/
QDomDocument CifParser::getXmlDocument()
{
    QDomDocument doc("phase");
    QDomElement root = doc.createElement("phase");
    root.setAttribute("Name", phaseName());

    QString dbEntryPrefix(getDatabasePrefix());
    QString dbEntryCode = getStringByTag("(?:[_A-Za-z]*)_database_code(?:[_A-Za-z]*)");

    if (!dbEntryPrefix.isEmpty()) {
        dbEntryPrefix += "_";
    }

    if (!dbEntryCode.isEmpty()) {
        root.setAttribute("Reference", dbEntryPrefix + dbEntryCode.replace(QRegularExpression("\\s+"), "_"));
    }

    QString cifFormulaEntry = getStringByTag("_chemical_formula_structural");
    cifFormulaEntry.isEmpty() ? cifFormulaEntry = getStringByTag("_chemical_formula_[a-z]+") : cifFormulaEntry;

    if (!cifFormulaEntry.isEmpty()) {
        root.setAttribute("Formula", cifFormulaEntry.replace(QRegularExpression("\\s+"), "_"));
    }

    doc.appendChild(root);

    QDomElement spgr = doc.createElement("spacegroup");

    QString itNum = getStringByTag("_symmetry_Int_Tables_number");          // old tag name
    if (itNum.isEmpty()) itNum = getStringByTag("_space_group_IT_number");  // new tag name
    spgr.setAttribute("Number", itNum);

    QString itHM = getStringByTag("_symmetry_space_group_name_H-M");        // old tag name
    if (itHM.isEmpty()) itHM = getStringByTag("_space_group_name_H-M_alt"); // new tag name

    // the cif dictionary allows some additions to the HM symbol, e.g. info in () or :origin
    // we need to clip these
    QRegularExpression rxHM("^([A-Za-z0-9_\\s/-]+)");
    QRegularExpressionMatch rmHM = rxHM.match(itHM);

    if (rmHM.hasMatch()) {
        spgr.setAttribute("HermannMauguin", rmHM.captured(1).trimmed());
    } else {
        spgr.setAttribute("HermannMauguin", itHM);
    }

    root.appendChild(spgr);

    // add all unitcell stuff
    QDomElement ucell = doc.createElement("unitcell");
    root.appendChild(ucell);

    // axes a, b, c
    QMap<QString, double> axes;
    axes.insert("a", getDoubleByTag(QString("_cell_length_a")));
    axes.insert("b", getDoubleByTag(QString("_cell_length_b")));
    axes.insert("c", getDoubleByTag(QString("_cell_length_c")));

    QMap<QString, double>::const_iterator itAx = axes.constBegin();
    while (itAx != axes.constEnd()) {
        QDomElement ax = doc.createElement("axis");
        ax.setAttribute("Name", itAx.key());
        ax.setAttribute("Unit", "A");
        ax.appendChild(doc.createTextNode(QString("%1").arg(itAx.value(), 0, 'f', 8)));
        ucell.appendChild(ax);

        ++itAx;
    }

    // angles alpha, beta, gamma
    QMap<QString, double> angles;
    bool ok;
    angles.insert("alpha", getDoubleByTag("_cell_angle_alpha", &ok));
    if (!ok) angles["alpha"] = 90.0;

    angles.insert("beta", getDoubleByTag("_cell_angle_beta", &ok));
    if (!ok) angles["beta"] = 90.0;

    angles.insert("gamma", getDoubleByTag("_cell_angle_gamma", &ok));
    if (!ok) angles["gamma"] = 90.0;

    QMap<QString, double>::const_iterator itAng = angles.constBegin();
    while (itAng != angles.constEnd()) {
        QDomElement ang = doc.createElement("angle");
        ang.setAttribute("Name", itAng.key());
        ang.setAttribute("Unit", "deg");
        ang.appendChild(doc.createTextNode(QString("%1").arg(itAng.value(), 0, 'f', 8)));
        ucell.appendChild(ang);

        ++itAng;
    }

    // add all atom stuff
    QList<Atom> coords = atomBlock("_atom_site_label");
    QList<Atom> tdsaniso = atomBlock("_atom_site_aniso_label");

    qDebug() << QString("CifParser::getXmlDocument(): found %1 atoms:").arg(coords.size());
    QStringList dbgAtmList;

    for (int k = 0; k < coords.size(); ++k) {
        dbgAtmList.append(QString("%1: %2").arg(k+1).arg(coords.at(k).value("_atom_site_label")));
    }

    qDebug() << QString("    %1").arg(dbgAtmList.join(", "));

    QRegularExpression rxOxi("([A-Za-z]+)([\\+-]?\\d*[\\+-]?)");
    QRegularExpression rxMulti("(\\d*)([a-z]+)");
    QRegularExpressionMatch match;

    for (int i = 0; i < coords.size(); ++i) {
        Atom a = coords.at(i);
        QDomElement elAtom = doc.createElement("atom");

        elAtom.setAttribute("Name", a.value("_atom_site_label"));

        // split oxidation state from atom type symbol
        match = rxOxi.match(a.value("_atom_site_type_symbol"));
        if (match.hasMatch()) {
            // try to use _atom_site_type_symbol for E=
            elAtom.setAttribute("Type", match.captured(1));

            if (!match.captured(2).isEmpty()) {
                elAtom.setAttribute("OxidationState", match.captured(2));
            }
        } else {
            // if _atom_site_type_symbol is not available,
            // fall back to _atom_site_label for E=
            elAtom.setAttribute("Type", a.value("_atom_site_label"));
        }

        // split multiplicity from wyckoff symbol
        match = rxMulti.match(a.value("_atom_site_Wyckoff_symbol"));
        if (match.hasMatch()) {
            elAtom.setAttribute("Wyckoff", match.captured(2));

            if (!match.captured(1).isEmpty()) {
                elAtom.setAttribute("Multiplicity", match.captured(1));
            }
        }

        // read site multiplicity (overwrite previous value if this explicit value is found here)
        if (a.count("_atom_site_symmetry_multiplicity")) {
            elAtom.setAttribute("Multiplicity", a.value("_atom_site_symmetry_multiplicity"));
        }

        // read site occupancy
        double socc = 1.0;
        if (a.count("_atom_site_occupancy")) {
            socc = stripStdDev(a.value("_atom_site_occupancy")).toDouble();
        }
        QDomElement elOcc = doc.createElement("occupancy");
        elOcc.appendChild(doc.createTextNode(QString("%1").arg(socc, 0, 'f', 8)));
        elAtom.appendChild(elOcc);

        // read fractional coordinates
        if (a.count("_atom_site_fract_x") && a.count("_atom_site_fract_y") && a.count("_atom_site_fract_z")) {
            QDomElement elCoord = doc.createElement("coordinates");
            double x = stripStdDev(a.value("_atom_site_fract_x")).toDouble();
            double y = stripStdDev(a.value("_atom_site_fract_y")).toDouble();
            double z = stripStdDev(a.value("_atom_site_fract_z")).toDouble();

            QDomText coo = doc.createTextNode(QString("%1 %2 %3").arg(x, 0, 'f', 8).arg(y, 0, 'f', 8).arg(z, 0, 'f', 8));
            elCoord.appendChild(coo);
            elAtom.appendChild(elCoord);
        }

        bool isDummy = false;
        if (a.count("_atom_site_calc_flag")) {
            QString calcFlag = a.value("_atom_site_calc_flag");
            isDummy = (calcFlag.trimmed().toLower() == "dum");
        }

        // read isotropic B values from the *coordinates* list
        double biso = 0.0;
        if (a.count("_atom_site_B_iso_or_equiv")) {
            biso = stripStdDev(a.value("_atom_site_B_iso_or_equiv")).toDouble();
        } else if (a.count("_atom_site_U_iso_or_equiv")) {
            biso = bisoFromUiso(stripStdDev(a.value("_atom_site_U_iso_or_equiv")).toDouble());
        }

        Atom aaniso;
        bool anisofound = false;
        // now search the same atomLabel in the *anisotropic* list
        for (int j = 0; j < tdsaniso.size(); ++j) {
            if (a.value("_atom_site_label") == tdsaniso.at(j).value("_atom_site_aniso_label")) {
                aaniso = tdsaniso.at(j);
                anisofound = true;
                break;
            }
        }

        // if we found an atom with the same label in the aniso list, extract the U and/or B aniso values
        if (anisofound) {
            // check if there are _atom_site_aniso_U_nn tags in the atom
            if (aaniso.count("_atom_site_aniso_U_11") && aaniso.count("_atom_site_aniso_U_22") && aaniso.count("_atom_site_aniso_U_33")) {
                double u11 = stripStdDev(aaniso.value("_atom_site_aniso_U_11")).toDouble();
                double u22 = stripStdDev(aaniso.value("_atom_site_aniso_U_22")).toDouble();
                double u33 = stripStdDev(aaniso.value("_atom_site_aniso_U_33")).toDouble();
                double u12 = stripStdDev(aaniso.value("_atom_site_aniso_U_12")).toDouble();
                double u13 = stripStdDev(aaniso.value("_atom_site_aniso_U_13")).toDouble();
                double u23 = stripStdDev(aaniso.value("_atom_site_aniso_U_23")).toDouble();

                // re-calculate biso from anisotropic U if they are available
                biso = bisoFromUaniso(angles.value("alpha"), angles.value("beta"), angles.value("gamma"),
                                      u11, u22, u33, u12, u13, u23);

                QDomElement uaniso = doc.createElement("Uaniso");

                QDomElement deU11 = doc.createElement("U11");
                deU11.appendChild(doc.createTextNode(QString("%1").arg(u11, 0, 'f', 8)));
                QDomElement deU22 = doc.createElement("U22");
                deU22.appendChild(doc.createTextNode(QString("%1").arg(u22, 0, 'f', 8)));
                QDomElement deU33 = doc.createElement("U33");
                deU33.appendChild(doc.createTextNode(QString("%1").arg(u33, 0, 'f', 8)));
                QDomElement deU12 = doc.createElement("U12");
                deU12.appendChild(doc.createTextNode(QString("%1").arg(u12, 0, 'f', 8)));
                QDomElement deU13 = doc.createElement("U13");
                deU13.appendChild(doc.createTextNode(QString("%1").arg(u13, 0, 'f', 8)));
                QDomElement deU23 = doc.createElement("U23");
                deU23.appendChild(doc.createTextNode(QString("%1").arg(u23, 0, 'f', 8)));

                uaniso.appendChild(deU11);
                uaniso.appendChild(deU22);
                uaniso.appendChild(deU33);
                uaniso.appendChild(deU12);
                uaniso.appendChild(deU13);
                uaniso.appendChild(deU23);

                elAtom.appendChild(uaniso);
            }

            // now do the same for anisotropic B values
            if (aaniso.count("_atom_site_aniso_B_11") && aaniso.count("_atom_site_aniso_B_22") && aaniso.count("_atom_site_aniso_B_33")) {
                double b11 = stripStdDev(aaniso.value("_atom_site_aniso_B_11")).toDouble();
                double b22 = stripStdDev(aaniso.value("_atom_site_aniso_B_22")).toDouble();
                double b33 = stripStdDev(aaniso.value("_atom_site_aniso_B_33")).toDouble();
                double b12 = stripStdDev(aaniso.value("_atom_site_aniso_B_12")).toDouble();
                double b13 = stripStdDev(aaniso.value("_atom_site_aniso_B_13")).toDouble();
                double b23 = stripStdDev(aaniso.value("_atom_site_aniso_B_23")).toDouble();

                // re-calculate biso from anisotropic U if they are available
                biso = bisoFromBaniso(angles.value("alpha"), angles.value("beta"), angles.value("gamma"),
                                      b11, b22, b33, b12, b13, b23);

                QDomElement baniso = doc.createElement("Baniso");

                QDomElement deB11 = doc.createElement("B11");
                deB11.appendChild(doc.createTextNode(QString("%1").arg(b11, 0, 'f', 8)));
                QDomElement deB22 = doc.createElement("B22");
                deB22.appendChild(doc.createTextNode(QString("%1").arg(b22, 0, 'f', 8)));
                QDomElement deB33 = doc.createElement("B33");
                deB33.appendChild(doc.createTextNode(QString("%1").arg(b33, 0, 'f', 8)));
                QDomElement deB12 = doc.createElement("B12");
                deB12.appendChild(doc.createTextNode(QString("%1").arg(b12, 0, 'f', 8)));
                QDomElement deB13 = doc.createElement("B13");
                deB13.appendChild(doc.createTextNode(QString("%1").arg(b13, 0, 'f', 8)));
                QDomElement deB23 = doc.createElement("B23");
                deB23.appendChild(doc.createTextNode(QString("%1").arg(b23, 0, 'f', 8)));

                baniso.appendChild(deB11);
                baniso.appendChild(deB22);
                baniso.appendChild(deB33);
                baniso.appendChild(deB12);
                baniso.appendChild(deB13);
                baniso.appendChild(deB23);

                elAtom.appendChild(baniso);
            }
        }

        // append Biso and Uiso values
        QDomElement elBiso = doc.createElement("Biso");
        QDomElement elUiso = doc.createElement("Uiso");

        elBiso.appendChild(doc.createTextNode(QString("%1").arg(biso, 0, 'f', 8)));
        elUiso.appendChild(doc.createTextNode(QString("%1").arg(biso / (8.0 * M_PI * M_PI), 0, 'f', 8)));

        // append coordinates and B/Uiso to atom
        elAtom.appendChild(elBiso);
        elAtom.appendChild(elUiso);

        // append atom to root
        if (!isDummy) root.appendChild(elAtom);
    }

    QStringList symLst = symOps();

    if (symLst.size()) {
        QDomElement elSymOps = doc.createElement("symOps");

        for (int i = 0; i < symLst.size(); ++i) {
            QRegularExpression rxSymOps("'?([x-z\\d\\,\\/\\+\\s-]+)'?");
            QRegularExpressionMatch rmSymOps = rxSymOps.match(symLst.at(i));

            if (rmSymOps.hasMatch()) {
                QStringList symOpsCoeff = rmSymOps.captured(1).split(QRegularExpression("[,\\s]+"));

                if (symOpsCoeff.size() == 3) {
                    QDomElement elSymOp = doc.createElement("symOp");
                    elSymOp.setAttribute("x", symOpsCoeff.at(0));
                    elSymOp.setAttribute("y", symOpsCoeff.at(1));
                    elSymOp.setAttribute("z", symOpsCoeff.at(2));
                    elSymOps.appendChild(elSymOp);
                }
            }
        }

        root.appendChild(elSymOps);
    }

    return doc;
}

/*
 *  convenience function
 */
QString CifParser::xmlString()
{
    return getXmlDocument().toString();
}

/*
 *  extracts the phase name from the cif file
 *
 * Note: Sometimes the name in the CIF file is ' ?' or a multi-line comment. These can't be used
 * for many applications. That is why we test thoroughly until a valid name can be returned.
 */
QString CifParser::phaseName()
{
    QString out = getStringByTag("_chemical_name_mineral");
    // make sure no illegal chacaters appear (only a-z and 0-9 allowed)
    out.replace(QRegularExpression("[^A-Za-z0-9]"), QString());

    if (out.isEmpty()) {
        out = getStringByTag("_chemical_name_common");
        out.replace(QRegularExpression("[^A-Za-z0-9]"), QString());
    }

    if (out.isEmpty()) {
        out = getStringByTag("_chemical_name");
        out.replace(QRegularExpression("[^A-Za-z0-9]"), QString());
    }

    if (out.isEmpty()) {
        out = getStringByTag("_chemical_name_systematic");
        out.replace(QRegularExpression("[^A-Za-z0-9]"), QString());
    }

    // if no usable name was found in the CIF file, fall back to the file's base name
    if (out.isEmpty()) {
        QFileInfo fi(filename);
        out = fi.completeBaseName();
        out.replace(QRegularExpression("[^A-Za-z0-9]"), QString());
    }

    return out;
}

/*
 * returns a list of atoms, each atom is represented by a QMap<QString,QString> containing
 * its data in the form "parameterName";"value"
 * e.g. an atom could be a QMap of format:
 * "_atom_site_label";"Ca1"
 * "_atom_site_type_symbol";"Ca2+"
 * "_atom_site_fract_x";"0.125"
 * ...
 *
 * To retrieve a list of atoms with coordinates, call this function as:
 *    QList<atom> l = blockData("_atom_site_fract_x");
 *
 * To get a list of atoms with Uaniso values, call it as:
 *    QList<atom> l = blockData("_atom_site_aniso_U_11");
 *
 * merge the two lists afterwards,
 * e.g. by comparing _atom_site_label with _atom_site_aniso_label
 */
QList<CifParser::Atom> CifParser::atomBlock(const QString &s)
{
    QList<Atom> l;
    QStringList data = getLoopBlock(s);

    // nothing found? return empty list
    if (data.size() < 2) {
        return l;
    }

    QStringList headers = data.at(0).split(";");
    for (int i = 1; i < data.size(); ++i) {
        Atom a;
        QStringList vals = data.at(i).split(";");

        for (int j = 0; j < qMin(vals.size(), headers.size()); ++j) {
            a.insert(headers.at(j), stripStdDev(vals.at(j)));
        }

        l.append(a);
    }

    return l;
}

/*
 *  returns the value belonging to a certain tag as a string
 */
QString CifParser::getStringByTag(const QString &tag)
{
    // constructing a regexp of format:
    // tag spaces? newline? 'or"? value 'or"? spaces? newline
    // value is defined as any character not equal to (_  #  \\n  ' ")
    QRegularExpression rx(QString("%1\\s*\\n?['\"]?([^_#\\n'\"']+)['\"']?\\s*\\n").arg(tag));
    QRegularExpressionMatch match;

    match = rx.match(cifStripped);

    if (match.hasMatch()) {
        return match.captured(1).trimmed();
    }

    return QString();
}

/*
 *  returns the value belonging to a certain tag as a double
 */
double CifParser::getDoubleByTag(const QString &tag, bool *ok)
{
    QString s = stripStdDev(getStringByTag(tag));
    return s.toDouble(ok);
}

/*
 * returns the loop block containing the tag "s" as a stringlist of format:
 * label1;  label2;  label3;...
 * a_value1;a_value2;a_value3;...
 * b_value1;b_value2;b_value3;...
 */
QStringList CifParser::getLoopBlock(const QString &s)
{
    QRegularExpression rx(QString("^\\s*%1\\s*$").arg(s));

    // find the line containing the regexp
    int idxTag = cifList.indexOf(rx);
    if (idxTag < 0) return QStringList();

    // now find the line containing "loop_" just before idxCoord
    int idxLoop = 0;

    for (int i = 0; i < idxTag; ++i) {
        if (cifList.at(i).contains(QRegularExpression("^\\s*loop_\\s*$"))) {
            idxLoop = i;
        }
    }

    qDebug() << QString("CifParser::getLoopBlock: Found loop for tag %1 at line %2").arg(s).arg(idxLoop);

    // now store all following lines as header fields in a string list,
    // e.g. "_atom_site_label;_atom_site_type_symbol;...."
    QStringList headers;
    int idxData = idxLoop;

    for (int i = idxLoop + 1; i < cifList.size(); ++i) {
        QString line = cifList.at(i).simplified();
        if (line.left(1) != "_") {

            // ignore comment lines
            if (line.left(1) == "#") {
                continue;
            }

            idxData = i;
            break;
        }

        headers.append(line);
    }

    QStringList out;
    out.append(headers.join(";"));

    for (int i = idxData; i < cifList.size(); ++i) {
        if ((cifList.at(i).isEmpty()) || (cifList.at(i).contains("_"))) {
            break;
        }

        // ignore comment lines
        if (cifList.at(i).trimmed().left(1) == "#") {
            continue;
        }

        QStringList l;
        // match text between '', "", ;;, or spaces
        QRegularExpression rxLine("((?<=')[^']+(?=')|(?<=\")[^\"]+(?=\")|(?<=;)[^;]+(?=;)|(?<= |^)[^ '\";]+(?= |$))");
        QRegularExpressionMatchIterator rxi = rxLine.globalMatch(cifList.at(i));

        while (rxi.hasNext()) {
            QRegularExpressionMatch rmLine = rxi.next();
            l.append(rmLine.captured(1));
        }

        out.append(l.join(";"));
    }

    return out;
}

QString CifParser::getDatabasePrefix()
{
    QRegularExpression rx("_?([A-Za-z]+)?_database_code_?([A-Za-z]+)?");
    QRegularExpressionMatch match;

    match = rx.match(cifStripped);

    if (match.hasMatch()) {
        if (!match.captured(1).isEmpty()) return match.captured(1).toUpper();
        if (!match.captured(2).isEmpty()) return match.captured(2).toUpper();
    }

    return QString();
}

QString CifParser::stripStdDev(const QString &s)
{
    // used to extract values, considering standard deviations (d.ddd(d))
    QRegularExpression rxV("([\\d\\.Ee\\+-]+)(?:\\(\\d*\\))");
    QRegularExpressionMatch match;

    match = rxV.match(s);

    if (match.hasMatch()) {
        return match.captured(1);
    }

    return s;
}

/*
 * conversion from Uiso to Biso
 */
double CifParser::bisoFromUiso(double uiso)
{
    return uiso * M_PI * M_PI * 8.0;
}

/*
 * conversion from beta_ij to Biso, based on Fischer & Tillmanns, 1988
 */
double CifParser::bisoFromBeta(double a, double b, double c, double alpha, double beta, double gamma,
                               double b11, double b22, double b33, double b12, double b13, double b23)
{
    double biso = 0.0;

    // if angles are 0.0, assume they are 90.0 but not mentioned in the cif
    double ca = alpha == 0.0 ? 0.0 : cos(alpha * M_PI / 180.0);
    double cb = beta  == 0.0 ? 0.0 : cos(beta  * M_PI / 180.0);
    double cc = gamma == 0.0 ? 0.0 : cos(gamma * M_PI / 180.0);

    biso = (b11*a*a) + (b22*b*b) + (b33*c*c) + (2.0*b12*a*b*cc) + (2.0*b13*a*c*cb) + (2.0*b23*b*c*ca);
    biso *= 4.0 / 3.0;
    return biso;
}

/*
 * conversion from B_ij to Biso, based on Fischer & Tillmanns, 1988
 */
double CifParser::bisoFromBaniso(double alpha, double beta, double gamma,
                                 double b11, double b22, double b33, double b12, double b13, double b23)
{
    double biso = 0.0;

    // if angles are 0.0, assume they are 90.0 but not mentioned in the cif
    double ca = alpha == 0.0 ? 0.0 : cos(alpha * M_PI / 180.0);
    double cb = beta  == 0.0 ? 0.0 : cos(beta  * M_PI / 180.0);
    double cc = gamma == 0.0 ? 0.0 : cos(gamma * M_PI / 180.0);

    double sa = alpha == 0.0 ? 1.0 : sin(alpha * M_PI / 180.0);
    double sb = beta  == 0.0 ? 1.0 : sin(beta  * M_PI / 180.0);
    double sc = gamma == 0.0 ? 1.0 : sin(gamma * M_PI / 180.0);

    biso = (b11*sa*sa + b22*sb*sb + b33*sc*sc + 2.0*b12*sa*sb*cc + 2.0*b13*sa*cb*sc + 2.0*b23*ca*sb*sc);
    biso /= (1.0 + 2.0*ca*cb*cc - ca*ca - cb*cb - cc*cc);
    biso /= 3.0;

    return biso;
}

/*
 * conversion from U_ij to Biso, based on Fischer & Tillmanns, 1988
 */
double CifParser::bisoFromUaniso(double alpha, double beta, double gamma,
                                 double u11, double u22, double u33, double u12, double u13, double u23)
{
    double biso = 0.0;

    // if angles are 0.0, assume they are 90.0 but not mentioned in the cif
    double ca = alpha == 0.0 ? 0.0 : cos(alpha * M_PI / 180.0);
    double cb = beta  == 0.0 ? 0.0 : cos(beta  * M_PI / 180.0);
    double cc = gamma == 0.0 ? 0.0 : cos(gamma * M_PI / 180.0);

    double sa = alpha == 0.0 ? 1.0 : sin(alpha * M_PI / 180.0);
    double sb = beta  == 0.0 ? 1.0 : sin(beta  * M_PI / 180.0);
    double sc = gamma == 0.0 ? 1.0 : sin(gamma * M_PI / 180.0);

    biso = (u11*sa*sa + u22*sb*sb + u33*sc*sc + 2.0*u12*sa*sb*cc + 2.0*u13*sa*cb*sc + 2.0*u23*ca*sb*sc);
    biso /= (1.0 + 2.0*ca*cb*cc - ca*ca - cb*cb - cc*cc);
    biso *= 8.0 * M_PI * M_PI / 3.0;

    return biso;
}

CrystalStructure CifParser::getCrystalStructure(bool *ok) const
{
    QDomElement root = xmlDoc.documentElement();
    CrystalStructure structure(root.attribute("Name"));
    CrystalUnitCell unitcell;
    unitcell.setAxisUnit(NM);

    if (root.hasAttribute("Reference")) structure.setAuxInfo("Reference", root.attribute("Reference"));
    if (root.hasAttribute("Formula")) structure.setAuxInfo("Formula", root.attribute("Formula"));

    QDomElement spgrEl = root.firstChildElement("spacegroup");

    if (spgrEl.hasAttribute("Number"))         unitcell.setItNumber(spgrEl.attribute("Number").toInt());
    if (spgrEl.hasAttribute("HermannMauguin")) unitcell.setSpaceGroupHMCif(spgrEl.attribute("HermannMauguin"));
    // if (spgrEl.hasAttribute("Lattice"))        structure.setLattice(spgrEl.attribute("Lattice").toInt());

    QDomElement ucellEl = root.firstChildElement("unitcell");
    QDomNodeList lcellAxis = ucellEl.elementsByTagName("axis");
    QDomNodeList lcellAngle = ucellEl.elementsByTagName("angle");

    // extract unit cell parameters: length
    for (int i = 0; i < lcellAxis.size(); ++i) {
        QDomElement aEl = lcellAxis.at(i).toElement();
        if (aEl.hasAttribute("Name")) {
            if (aEl.attribute("Name") == "a") unitcell.setA(aEl.text().toDouble()/10.0);
            if (aEl.attribute("Name") == "b") unitcell.setB(aEl.text().toDouble()/10.0);
            if (aEl.attribute("Name") == "c") unitcell.setC(aEl.text().toDouble()/10.0);
        }
    }

    // extract unit cell parameters: angle
    for (int i = 0; i < lcellAngle.size(); ++i) {
        QDomElement aEl = lcellAngle.at(i).toElement();
        if (aEl.hasAttribute("Name")) {
            if (aEl.attribute("Name") == "alpha") unitcell.setAlpha(aEl.text().toDouble());
            if (aEl.attribute("Name") == "beta")  unitcell.setBeta(aEl.text().toDouble());
            if (aEl.attribute("Name") == "gamma") unitcell.setGamma(aEl.text().toDouble());
        }
    }

    QDomNodeList latoms = root.elementsByTagName("atom");

    // loop over all atoms
    for (int i = 0; i < latoms.size(); ++i) {
        QDomElement aEl = latoms.at(i).toElement();
        QDomElement oEl = aEl.firstChildElement("occupancy");
        QDomElement cEl = aEl.firstChildElement("coordinates");
        QDomElement bEl = aEl.firstChildElement("Biso");

        QString elem(QString("%1%2")
                             .arg(aEl.attribute("Type"))
                             .arg(aEl.attribute("OxidationState")));

        double occ = oEl.text().toDouble();

        QString wyck(aEl.attribute("Wyckoff"));

        double x = -1.0;
        double y = -1.0;
        double z = -1.0;
        double biso = 0.0;

        QStringList lcoord = cEl.text().split(QRegularExpression("\\s+"));

        if (lcoord.size() > 2) {
            x = lcoord.at(0).toDouble();
            y = lcoord.at(1).toDouble();
            z = lcoord.at(2).toDouble();

            global::Functions::normalizeCoordinates(x, y, z);
        }

        if (!bEl.isNull()) {
            biso = bEl.text().toDouble();
        }

        CrystalAtom atom(elem, x, y, z, occ, biso);
        atom.setWyckoff(wyck);
        structure.addAtom(atom);
    }

    // loop over symmetry operators
    QDomNodeList lsymOps = root.elementsByTagName("symOp");
    QList<QStringList> symOperators;

    // loop over all symetry operators
    for (int i = 0; i < lsymOps.size(); ++i) {
        QDomElement symOpsEl = lsymOps.at(i).toElement();

        symOperators.append(QStringList()
                            << symOpsEl.attribute("x")
                            << symOpsEl.attribute("y")
                            << symOpsEl.attribute("z"));
    }

    unitcell.setSymmetryOperations(symOperators);

    bool b = unitcell.fixCell();
    if (ok) *ok = b;

    structure.setUnitCell(unitcell);
    return structure;
}

QStringList CifParser::symOps()
{
    QStringList symLst = getLoopBlock("_space_group_symop_operation_xyz");

    if (symLst.isEmpty()) symLst = getLoopBlock("_symmetry_equiv_pos_as_xyz");
    if (symLst.isEmpty()) return QStringList();

    QStringList lbls = symLst.first().split(";");
    int n = lbls.indexOf("_space_group_symop_operation_xyz");
    if (n < 0) n = lbls.indexOf("_symmetry_equiv_pos_as_xyz");
    if (n < 0) return QStringList();

    QStringList sops;

    for (int i = 1; i < symLst.size(); ++i) {
        QStringList l = symLst.at(i).split(";");
        if (l.size() > n) sops.append(l.at(n));
    }

    return sops;
}
