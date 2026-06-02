/***************************************************************************
                          crystalstructure.cpp  -  description
                             -------------------
    begin                : Sat Aug 22 08:53:00 CEST 2015
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

#include "crystalstructure.h"
#include "crystalhklgenerator.h"
#include "../functions.h"
#include <QDateTime>
#include <QJSEngine>
#include <QFileInfo>
#include <QStringList>
#include <QRegularExpression>
#include <math.h>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif


CrystalStructure::CrystalStructure()
{
    _name = QString();
    _lattice = 1;
    _z = 0;
    _rWp = 0.0;
    _rExp = 0.0;
    _chi2 = 0.0;
    _density = 0.0;
}

CrystalStructure::CrystalStructure(const QString &p)
    : _name(p)
{
    _lattice = 1;
    _z = 0;
    _rWp = 0.0;
    _rExp = 0.0;
    _chi2 = 0.0;
    _density = 0.0;
}

/*
 * clears all data, equivalent to ::reset()
 */
void CrystalStructure::clear()
{
    reset();
}

void CrystalStructure::reset()
{
    _lattice = 1;
    _unitCell.reset();
    _atoms.clear();
    _fstructs.clear();
    _auxinfo.clear();
    _z = 0;
    _rWp = 0.0;
    _rExp = 0.0;
    _chi2 = 0.0;
    _density = 0.0;
}

QString CrystalStructure::toBgmnStr()
{
    double limits = _auxinfo.value("limits", 0.01).toDouble();
    QString refr = _auxinfo.value("Reference", QString()).toString();

    static QRegularExpression rxRepl("[_\\-\\s\\(\\)\\+\\*]");
    static QRegularExpression rxDbCode("^\\d+$");

    // if the phase name is a set of numeric values, we use the DB reference string instead,
    // otherwise BGMN will be confused
    QString nameFixed = _name;
    nameFixed.replace(rxRepl, "");
    if (nameFixed.contains(rxDbCode)) nameFixed = refr;

    QString pfunc = QString("RP=4 k1=0 k2=0 PARAM=B1=0_0^0.01 GEWICHT=SPHAR4 //\n"
                            "GOAL:%1=GEWICHT*ifthenelse(ifdef(d),exp(my*d*3/4),1) //\n").arg(nameFixed);

    QString str;
    int itNum = _auxinfo.value("BgmnSpacegroupNo", 0).toInt();

    str += QString("PHASE=%1 // %2\n").arg(nameFixed, refr);
    if (!refr.isEmpty()) str += QString("Reference=%1 //\n").arg(refr);

    static QRegularExpression rxSpace("\\s+");
    QString sumFormula;
    if (!_auxinfo.value("Formula", QString()).toString().isEmpty()) {
        sumFormula = _auxinfo.value("Formula", QString()).toString();
    } else {
        sumFormula = toSumFormula();
        sumFormula.replace(rxSpace, "_");
    }

    str += QString("Formula=%1 //\n").arg(sumFormula);
    str += _auxinfo.value("BgmnSettingsLine", QString()).toString() + QString(" //\n");

    QString xa = QString::number(_unitCell.a(), 'f', 6);
    QString xb = QString::number(_unitCell.b(), 'f', 6);
    QString xc = QString::number(_unitCell.c(), 'f', 6);
    QString anga = QString::number(_unitCell.alpha(), 'f', 6);
    QString angb = QString::number(_unitCell.beta(),  'f', 6);
    QString angc = QString::number(_unitCell.gamma(), 'f', 6);

    QString xa_ll = QString::number((1.0-limits) * _unitCell.a(), 'f', 6);
    QString xb_ll = QString::number((1.0-limits) * _unitCell.b(), 'f', 6);
    QString xc_ll = QString::number((1.0-limits) * _unitCell.c(), 'f', 6);

    QString xa_ul = QString::number((1.0+limits) * _unitCell.a(), 'f', 6);
    QString xb_ul = QString::number((1.0+limits) * _unitCell.b(), 'f', 6);
    QString xc_ul = QString::number((1.0+limits) * _unitCell.c(), 'f', 6);

    QString anga_ll = QString::number((1.0-limits) * _unitCell.alpha(), 'f', 6);
    QString angb_ll = QString::number((1.0-limits) * _unitCell.beta(),  'f', 6);
    QString angc_ll = QString::number((1.0-limits) * _unitCell.gamma(), 'f', 6);

    QString anga_ul = QString::number((1.0+limits) * _unitCell.alpha(), 'f', 6);
    QString angb_ul = QString::number((1.0+limits) * _unitCell.beta(),  'f', 6);
    QString angc_ul = QString::number((1.0+limits) * _unitCell.gamma(), 'f', 6);

    if (itNum <= 2) {         // triclinic
         str += QString("PARAM=A=%1_%2^%3 ").arg(xa, xa_ll, xa_ul);
         str += QString("PARAM=B=%1_%2^%3 ").arg(xb, xb_ll, xb_ul);
         str += QString("PARAM=C=%1_%2^%3 ").arg(xc, xc_ll, xc_ul);
         str += "//\n";
         str += QString("PARAM=ALPHA=%1_%2^%3 ").arg(anga, anga_ll, anga_ul);
         str += QString("PARAM=BETA=%1_%2^%3 " ).arg(angb, angb_ll, angb_ul);
         str += QString("PARAM=GAMMA=%1_%2^%3 ").arg(angc, angc_ll, angc_ul);
    }
    else if (itNum <= 15) { // monoclinic
        str += QString("PARAM=A=%1_%2^%3 ").arg(xa, xa_ll, xa_ul);
        str += QString("PARAM=B=%1_%2^%3 ").arg(xb, xb_ll, xb_ul);
        str += QString("PARAM=C=%1_%2^%3 ").arg(xc, xc_ll, xc_ul);
        str += "//\n";
        if      (!qFuzzyCompare(_unitCell.alpha(), 90.0)) str += QString("PARAM=ALPHA=%1_%2^%3 ").arg(anga, anga_ll, anga_ul);
        else if (!qFuzzyCompare(_unitCell.beta(), 90.0))  str += QString("PARAM=BETA=%1_%2^%3 " ).arg(angb, angb_ll, angb_ul);
        else                                              str += QString("PARAM=GAMMA=%1_%2^%3 ").arg(angc, angc_ll, angc_ul);
    }
    else if (itNum <= 74) { // orthorhombic
        str += QString("PARAM=A=%1_%2^%3 ").arg(xa, xa_ll, xa_ul);
        str += QString("PARAM=B=%1_%2^%3 ").arg(xb, xb_ll, xb_ul);
        str += QString("PARAM=C=%1_%2^%3 ").arg(xc, xc_ll, xc_ul);
    }
    else if (itNum <= 142) { // tetragonal
        str += QString("PARAM=A=%1_%2^%3 ").arg(xa, xa_ll, xa_ul);
        if (!qFuzzyCompare(_unitCell.a(), _unitCell.b())) str += QString("PARAM=B=%1_%2^%3 ").arg(xb, xb_ll, xb_ul);
        else                                              str += QString("PARAM=C=%1_%2^%3 ").arg(xc, xc_ll, xc_ul);
    }
    else if (itNum <= 176) { // trigonal / rhombohedral
        if (qFuzzyCompare(_unitCell.a(), unitCell().b()) && qFuzzyCompare(_unitCell.a(), unitCell().c())) {
            // rhombohedral
            str += QString("PARAM=A=%1_%2^%3 ").arg(xa, xa_ll, xa_ul);
            str += "//\n";
            str += QString("PARAM=GAMMA=%1_%2^%3 ").arg(angc, angc_ll, angc_ul);
        } else {
            // trigonal
            str += QString("PARAM=A=%1_%2^%3 ").arg(xa, xa_ll, xa_ul);
            if (!qFuzzyCompare(_unitCell.a(), _unitCell.b())) str += QString("PARAM=B=%1_%2^%3 ").arg(xb, xb_ll, xb_ul);
            else                                              str += QString("PARAM=C=%1_%2^%3 ").arg(xc, xc_ll, xc_ul);
        }
    }
    else if (itNum <= 194) { // hexagonal
        str += QString("PARAM=A=%1_%2^%3 ").arg(xa, xa_ll, xa_ul);
        if (!qFuzzyCompare(_unitCell.a(), _unitCell.b())) str += QString("PARAM=B=%1_%2^%3 ").arg(xb, xb_ll, xb_ul);
        else                                              str += QString("PARAM=C=%1_%2^%3 ").arg(xc, xc_ll, xc_ul);
    }
    else {                                 // cubic
        str += QString("PARAM=A=%1_%2^%3 ").arg(xa, xa_ll, xa_ul);
    }

    str += "//\n" + pfunc;

    if (_atoms.size()) {
        QList<QList<CrystalAtom> > atms = mergeAtomsBgmn(_atoms);

        for (int i = 0; i < atms.size(); ++i) {
            QStringList elem;

            for (int j = 0; j < atms.at(i).size(); ++j) {
                if (qFuzzyCompare(atms.at(i).at(j).occupancy(), 1.0)) {
                    // the SOF is 1.0, so we don't add it
                    elem.append(atms.at(i).at(j).element());
                } else {
                    // the SOF is < 1.0 so we add it
                    elem.append(QString("%1(%2)")
                                .arg(atms.at(i).at(j).element())
                                .arg(atms.at(i).at(j).occupancy(), 0, 'f', 4));
                }
            }

            bool encaps = atms.at(i).size() > 1;
            QString line = QString("E=%1%2%3").arg(encaps ? "(" : "").arg(elem.join(",")).arg(encaps ? ")" : "");

            line += QString(" Wyckoff=%1 ").arg(atms.at(i).first().wyckoff());
            line += QString("x=%1 y=%2 z=%3 ")
                    .arg(atms.at(i).first().x(), 0, 'f', 6)
                    .arg(atms.at(i).first().y(), 0, 'f', 6)
                    .arg(atms.at(i).first().z(), 0, 'f', 6);
            double biso = atms.at(i).first().biso() * 0.01;
            line += QString("TDS=%1").arg(qFuzzyIsNull(biso) ? 0.01 : biso, 0, 'f', 6);

            QStringList addEComment = atms.at(i).first().auxInfo("bgmnEComment", QStringList()).toStringList();
            if (addEComment.size()) line += QString(" // %1").arg(addEComment.join(", "));
            line += "\n";

            str += line;
        }
    } else if (_fstructs.size()) {
        str += "FMult=1\n";
        str += "F=ifthenelse(ifdef(F[iref,h,k,l]),F[iref,h,k,l],0)\n";

        for (int i = 0; i < _fstructs.size(); ++i) {
            str += QString("F[1,%1,%2,%3]=%4 // d=%5 nm\n")
                    .arg(_fstructs.at(i).h())
                    .arg(_fstructs.at(i).k())
                    .arg(_fstructs.at(i).l())
                    .arg(_fstructs.at(i).intensity(), 0, 'f', 6)
                    .arg(_fstructs.at(i).d_nm(), 0, 'f', 6);
        }
    } else {
        str += "LeBail=1\n";
    }

    return str;
}

/*
 * returns the crystal structure as a CIF file string.
 *
 * "complete" set to true will write the file header comment
 * "complete" set to false will only return the data_ block
 *
 * "source" is optional, if provided, it will be mentioned in the header
 */
QString CrystalStructure::toCif(bool complete, const QString &source, const QMap<QString, QVariant> &auxData)
{
    if (!_unitCell.fixCell()) {
        qDebug() << QString("CrystalStructure::toCif(): Fixing the unit cell failed. Abort.");
        return QString();
    }

    QFileInfo sourceFi(source);
    QString cif;
    int digits = 5;

    // header
    if (complete) {
        cif += QStringLiteral("###############################################################################\n");
        cif += QString(       "# Crystal structure data for phase %1\n").arg(_name);
        cif += QString(       "# Created with Profex %1.%2.%3\n").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD);
        cif += QString(       "# Export date: %1\n").arg(QDateTime::currentDateTime().toString("MMMM dd, yyyy - hh:mm"));
        if (!source.isNull()) {
            QString elide;
            if (source.length() > 59) elide = QString(" ...");
            cif += QString(   "#\n# Source file:%1%2\n").arg(elide).arg(source.right(61), 61);
        }
        cif += QStringLiteral("###############################################################################\n\n");
        cif += QStringLiteral("data_global\n\n");
        cif += QStringLiteral("_publ_contact_author_name       ? # Name of author for correspondence\n");
        cif += QStringLiteral("_publ_contact_author_address      # Address of author for correspondence\n");
        cif += QStringLiteral(";?\n;\n\n");
    }

    static QRegularExpression rxSpace("\\s+");

    if (!source.isNull()) {
        QFileInfo fiSource(source);
        cif += QString("data_%1-%2\n")
                .arg(fiSource.completeBaseName(),
                     _name.replace(rxSpace, QString()));
    } else {
        cif += QString("data_%1\n").arg(_name.replace(rxSpace, QString()));
    }

    QString placeHolder(QString("? # %1").arg(_name));

    // general info
    cif += QString("\n_chemical_name_systematic         '%1'\n").arg(_name);

    cif += QString("_chemical_formula_structural      %1\n")
            .arg(auxData.value("_chemical_formula_structural", placeHolder).toString());

    cif += QString("_chemical_formula_sum             %1\n")
            .arg(auxData.value("_chemical_formula_sum", placeHolder).toString());

    cif += QString("_chemical_formula_weight          %1\n")
            .arg(auxData.value("_chemical_formula_weight", placeHolder).toString());

    cif += QString("\n_space_group_crystal_system %1\n").arg(_unitCell.crystalSystemCif());
    cif += QString("_space_group_name_H-M_alt  '%1'\n").arg(_unitCell.spaceGroupHMCif());
    cif += QString("_space_group_IT_number      %1 # new notation\n").arg(_unitCell.itNumber(), 3);
    cif += QString("_symmetry_Int_Tables_number %1 # depricated notation,\n").arg(_unitCell.itNumber(), 3);
    cif += QString("                                # maintained for compatibility with old software\n");

    // symmetry operations
    QList<QStringList> symOps = _unitCell.symmetryOperations();

    if (!symOps.isEmpty()) {
        cif += QStringLiteral("\nloop_\n");
        cif += QStringLiteral("   _symmetry_equiv_pos_site_id\n");
        cif += QStringLiteral("   _symmetry_equiv_pos_as_xyz\n");

        for (int i = 0; i < symOps.size(); ++i) {
            QStringList sops(symOps.at(i));
            if (sops.size() < 3) continue;

            cif += QString("   %1    %2,%3,%4\n")
                    .arg(i+1)
                    .arg(sops.at(0).toLower(),
                         sops.at(1).toLower(),
                         sops.at(2).toLower());
        }

        cif += QStringLiteral("\n");
    }

    double au = _unitCell.axisUnit() == NM ? 10.0 : 1.0;

    // unit cell dimensions: construct the esd string
    QString str_esd_a = _unitCell.a_esd()      < 0.0 ? "" : QString("(%1)").arg(au * _unitCell.a_esd() * pow(10.0, digits), 0, 'f', 0);
    QString str_esd_b = _unitCell.b_esd()      < 0.0 ? "" : QString("(%1)").arg(au * _unitCell.b_esd() * pow(10.0, digits), 0, 'f', 0);
    QString str_esd_c = _unitCell.c_esd()      < 0.0 ? "" : QString("(%1)").arg(au * _unitCell.c_esd() * pow(10.0, digits), 0, 'f', 0);
    QString str_esd_al = _unitCell.alpha_esd() < 0.0 ? "" : QString("(%1)").arg(_unitCell.alpha_esd() * pow(10.0, digits), 0, 'f', 0);
    QString str_esd_be = _unitCell.beta_esd()  < 0.0 ? "" : QString("(%1)").arg(_unitCell.beta_esd()  * pow(10.0, digits), 0, 'f', 0);
    QString str_esd_ga = _unitCell.gamma_esd() < 0.0 ? "" : QString("(%1)").arg(_unitCell.gamma_esd() * pow(10.0, digits), 0, 'f', 0);

    // unit cell dimensions: construct CIF string
    cif += QString("_cell_length_a            %1%2\n").arg(au * _unitCell.a(), 0, 'f', digits).arg(str_esd_a);
    cif += QString("_cell_length_b            %1%2\n").arg(au * _unitCell.b(), 0, 'f', digits).arg(str_esd_b);
    cif += QString("_cell_length_c            %1%2\n").arg(au * _unitCell.c(), 0, 'f', digits).arg(str_esd_c);
    cif += QString("_cell_angle_alpha         %1%2\n").arg(_unitCell.alpha(), 0, 'f', digits).arg(str_esd_al);
    cif += QString("_cell_angle_beta          %1%2\n").arg(_unitCell.beta(), 0, 'f', digits).arg(str_esd_be);
    cif += QString("_cell_angle_gamma         %1%2\n").arg(_unitCell.gamma(), 0, 'f', digits).arg(str_esd_ga);

    // experimental data
    cif += QString("\n_diffrn_ambient_temperature      %1\n").arg(auxData.value("_diffrn_ambient_temperature", "? # 295").toString());
    cif += QString("_diffrn_measurement_device_type  %1\n").arg(auxData.value("_diffrn_measurement_device_type", "? # 'Bruker D8 Advance' 'PANalytical XPert Pro'").toString());
    cif += QString("_diffrn_radiation_probe          %1\n").arg(auxData.value("_diffrn_radiation_probe", "? # 'x-ray' 'neutron' 'electron' 'gamma'").toString());

    cif += QStringLiteral("\nloop_\n");
    // cif += QStringLiteral("    _diffrn_radiation_type\n");
    // cif += QStringLiteral("    _diffrn_radiation_wavelength\n");
    // cif += QStringLiteral("           ?    ?    # CuK\\a~1~ 1.54056\n");
    // cif += QStringLiteral("           ?    ?    # CuK\\a~2~ 1.54439\n");

    cif += QStringLiteral("    _diffrn_radiation_wavelength\n");
    QList<QVariant> wls(auxData.value("_diffrn_radiation_wavelength", QList<QVariant>()).toList());

    if (wls.size()) {
        for (int i = 0; i < wls.size(); ++i) {
            cif += QString("           %1\n").arg(wls.at(i).toDouble(), 0, 'f', 6);
        }
    } else {
            cif += QString("           ?\n");
    }

    cif += QStringLiteral("\n_computing_structure_refinement   'BGMN'\n");
    cif += QStringLiteral("_computing_publication_material   'Profex'\n");

    // refinement statistics
    cif += QString("\n_pd_proc_ls_prof_wR_factor        %1\n").arg(qFuzzyIsNull(_rWp) ? "?" : QString::number(_rWp, 'f', 4));
    cif += QString("_pd_proc_ls_prof_wR_expected      %1\n").arg(qFuzzyIsNull(_rExp) ? "?"  : QString::number(_rExp, 'f', 4));
    cif += QString("_refine_ls_goodness_of_fit_all    %1\n").arg(qFuzzyIsNull(_chi2) ? "?"  : QString::number(sqrt(float(_chi2)), 'f', 4));

    cif += QString("\n_pd_block_diffractogram_id      %1\n").arg(sourceFi.completeBaseName());

    // Atoms
    cif += QStringLiteral("\nloop_\n");
    cif += QStringLiteral("   _atom_site_label\n");
    cif += QStringLiteral("   _atom_site_type_symbol\n");
    cif += QStringLiteral("   _atom_site_occupancy\n");
    cif += QStringLiteral("   _atom_site_fract_x\n");
    cif += QStringLiteral("   _atom_site_fract_y\n");
    cif += QStringLiteral("   _atom_site_fract_z\n");
    cif += QStringLiteral("   _atom_site_B_iso_or_equiv\n");

    for (int i = 0; i < _atoms.size(); ++i) {
        CrystalAtom atom = _atoms.at(i);

        double xcoord = atom.x();
        double ycoord = atom.y();
        double zcoord = atom.z();

        // normalize coordinates
        while (xcoord < 0.0) xcoord += 1.0;
        while (ycoord < 0.0) ycoord += 1.0;
        while (zcoord < 0.0) zcoord += 1.0;
        while (xcoord > 1.0) xcoord -= 1.0;
        while (ycoord > 1.0) ycoord -= 1.0;
        while (zcoord > 1.0) zcoord -= 1.0;

        QString sl = QString("%1").arg(atom.name(), -6);
        QString sn = QString("%1").arg(atom.element(), -4);
        QString so = QString("%1").arg(atom.occupancy(), digits + 4, 'f', digits);
        QString sx = QString("%1").arg(xcoord, digits + 4, 'f', digits);
        QString sy = QString("%1").arg(ycoord, digits + 4, 'f', digits);
        QString sz = QString("%1").arg(zcoord, digits + 4, 'f', digits);
        QString st = qFuzzyIsNull(atom.biso()) ? QString("  ?") : QString("%1").arg(atom.biso(), 10, 'f', digits);

        QString sesdo = atom.occupancy_esd() <= 0.0 ? QString("") : QString("(%1)").arg(atom.occupancy_esd() * pow(10.0, digits), 0, 'f', 0);
        QString sesdx = atom.x_esd()         <= 0.0 ? QString("") : QString("(%1)").arg(atom.x_esd() * pow(10.0, digits), 0, 'f', 0);
        QString sesdy = atom.y_esd()         <= 0.0 ? QString("") : QString("(%1)").arg(atom.y_esd() * pow(10.0, digits), 0, 'f', 0);
        QString sesdz = atom.z_esd()         <= 0.0 ? QString("") : QString("(%1)").arg(atom.z_esd() * pow(10.0, digits), 0, 'f', 0);
        QString sesdt = atom.biso_esd()      <= 0.0 ? QString("") : QString("(%1)").arg(atom.biso_esd() * pow(10.0, digits), 0, 'f', 0);

        cif += QString("   %1%2%3%4").arg(sl, sn, so, sesdo);
        cif += QString("%1%2%3%4%5%6").arg(sx, sesdx, sy, sesdy, sz, sesdz);
        cif += QString("%1%2\n").arg(st, sesdt);
    }

    // Anisotropic TDS
    // store in temporary string, and only append it to cif it not empty
    QString cifBeta;
    QString cifUaniso;
    bool appendUaniso = false;
    bool appendBeta = false;

    cifBeta =  QStringLiteral("\nloop_\n");
    cifBeta += QStringLiteral("   _atom_site_aniso_label\n");
    cifBeta += QStringLiteral("   _atom_site_aniso_B_11\n");
    cifBeta += QStringLiteral("   _atom_site_aniso_B_22\n");
    cifBeta += QStringLiteral("   _atom_site_aniso_B_33\n");
    cifBeta += QStringLiteral("   _atom_site_aniso_B_12\n");
    cifBeta += QStringLiteral("   _atom_site_aniso_B_13\n");
    cifBeta += QStringLiteral("   _atom_site_aniso_B_23\n");

    cifUaniso =  QStringLiteral("\nloop_\n");
    cifUaniso += QStringLiteral("   _atom_site_aniso_label\n");
    cifUaniso += QStringLiteral("   _atom_site_aniso_U_11\n");
    cifUaniso += QStringLiteral("   _atom_site_aniso_U_22\n");
    cifUaniso += QStringLiteral("   _atom_site_aniso_U_33\n");
    cifUaniso += QStringLiteral("   _atom_site_aniso_U_12\n");
    cifUaniso += QStringLiteral("   _atom_site_aniso_U_13\n");
    cifUaniso += QStringLiteral("   _atom_site_aniso_U_23\n");

    for (int i = 0; i < _atoms.size(); ++i) {
        CrystalAtom atom = _atoms.at(i);
        QString sl = QString("%1").arg(atom.name(), -6);

        if (atom.has_uaniso()) {
            double u11, u22, u33, u12, u13, u23;
            atom.uaniso(u11, u22, u33, u12, u13, u23);
            appendUaniso = true;

            QString su11 = QString("%1").arg(u11, digits + 4, 'f', digits);
            QString su22 = QString("%1").arg(u22, digits + 4, 'f', digits);
            QString su33 = QString("%1").arg(u33, digits + 4, 'f', digits);
            QString su12 = QString("%1").arg(u12, digits + 4, 'f', digits);
            QString su13 = QString("%1").arg(u13, digits + 4, 'f', digits);
            QString su23 = QString("%1").arg(u23, digits + 4, 'f', digits);

            cifUaniso += QString("   %1%2%3%4%5%6%7\n").arg(sl, su11, su22, su33, su12, su13, su23);
        }

        if (atom.has_baniso()) {
            double b11, b22, b33, b12, b13, b23;
            atom.baniso(b11, b22, b33, b12, b13, b23);
            appendBeta = true;

            QString sb11 = QString("%1").arg(b11, digits + 4, 'f', digits);
            QString sb22 = QString("%1").arg(b22, digits + 4, 'f', digits);
            QString sb33 = QString("%1").arg(b33, digits + 4, 'f', digits);
            QString sb12 = QString("%1").arg(b12, digits + 4, 'f', digits);
            QString sb13 = QString("%1").arg(b13, digits + 4, 'f', digits);
            QString sb23 = QString("%1").arg(b23, digits + 4, 'f', digits);

            cifBeta += QString("   %1%2%3%4%5%6%7\n").arg(sl, sb11, sb22, sb33, sb12, sb13, sb23);
        }
    }

    if (appendUaniso)      cif += cifUaniso;
    else if (appendBeta)   cif += cifBeta;

    if (complete) {
        cif += QStringLiteral("\n# The following lines are used to test the character set of files sent by\n");
        cif += QStringLiteral("# network email or other means. They are not part of the CIF data set.\n");
        cif += QStringLiteral("# abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n");
        cif += QString("# !@#$%^&*()_+{}:\"~<>?|\\-=[];'`,./ \n");
    }

    return cif;
}

/*
 * returns a castep cell format string of the structure.
 * Unit cell is given in cartesian coordinates
 * Atoms must be multiplied and are given in fractional coordinates
 */
QString CrystalStructure::toCastepCell()
{
    double ax, ay, az, bx, by, bz, cx, cy, cz;

    if (!_unitCell.axes_cartesian(ax, ay, az, bx, by, bz, cx, cy, cz)) {
        qDebug() << QString("CrystalStructure::toCastepCell(): Illegal unit cell, exiting");
        return QString();
    }

    QString cell("%BLOCK lattice_cart\n");
    cell += QString("%1%2%3\n").arg(ax, 8, 'f', 4).arg(ay, 8, 'f', 4).arg(az, 8, 'f', 4);
    cell += QString("%1%2%3\n").arg(bx, 8, 'f', 4).arg(by, 8, 'f', 4).arg(bz, 8, 'f', 4);
    cell += QString("%1%2%3\n").arg(cx, 8, 'f', 4).arg(cy, 8, 'f', 4).arg(cz, 8, 'f', 4);
    cell += QStringLiteral("%ENDBLOCK lattice_cart\n\n");

    cell += QStringLiteral("%BLOCK positions_frac\n");

    QList<CrystalAtom> allAtoms = generateAtoms(_atoms);

    for (int i = 0; i < allAtoms.size(); ++i) {
        CrystalAtom at = allAtoms.at(i);
        QString ate = at.element();
        QString elName(ate.left(1).toUpper() + (ate.length() > 1 ? ate.mid(1, 1).toLower() : QString()));
        cell += QString("%1%2%3%4\n").arg(elName, -3).arg(at.x(), 8, 'f', 4).arg(at.y(), 8, 'f', 4).arg(at.z(), 8, 'f', 4);
    }

    cell += QStringLiteral("%ENDBLOCK positions_frac\n\n");

    cell += QStringLiteral("symmetry_generate\n\n");
    cell += QStringLiteral("kpoint_mp_grid 3 3 3\n");

    return cell;
}

/*
 * returns a string with the sum formula of the phase
 */
QString CrystalStructure::toSumFormula(int z)
{
    QMap<QString, double> elements;

    for (int i = 0; i < _atoms.size(); ++i) {
        CrystalAtom a = _atoms.at(i);
        if (elements.contains(a.element())) {
            elements[a.element()] += a.multiplicity() * a.occupancy();
        } else {
            elements[a.element()] = a.multiplicity() * a.occupancy();
        }
        // qDebug() << QString("Parsing: %1 = %2 * %3").arg(a.element()).arg(a.multiplicity()).arg(a.occupancy());
    }

    QString output;
    int res = 4;
    double val = 0.0;

    QMapIterator<QString, double> it(elements);
    QString oxy;

    while (it.hasNext()) {
        it.next();
        val = it.value() / (z == 1 ? 1.0 : double(z));
        res = qFuzzyCompare(floor(float(val)), float(val)) ? 0 : 4;

        if (it.key().toUpper() == "O") {
            oxy = QString("%1%2 ").arg(it.key()).arg(val, 0, 'f', res);
        } else {
            output += QString("%1%2 ").arg(it.key()).arg(val, 0, 'f', res);
        }
    }

    output += oxy; // append the oxygens to the end
    return output.trimmed();
}

QList<CrystalAtom> CrystalStructure::generateAtoms(const QList<CrystalAtom> &primaryAtoms)
{
    QList<CrystalSymOp> symops = _unitCell.symmetryOperationMatrices();
    QList<CrystalAtom> allAtoms;

    for (int i = 0; i < primaryAtoms.size(); ++i) {
        // do not append the primary atom to the list.
        // the x y z operation will do it after normalizing the coordinates
        for (int j = 0; j < symops.size(); ++j) {
            CrystalAtom nat = symops[j].transform(primaryAtoms.at(i));
            global::Functions::normalizeCoordinates(nat.x(), nat.y(), nat.z());
            if (!allAtoms.contains(nat)) {
                allAtoms.append(nat);
            }
        }
    }

    return allAtoms;
}

void CrystalStructure::setRfactors(double w, double e, double c)
{
    _rWp = w;
    _rExp = e;
    _chi2 = c;
}

/*
 * checks for identical coordinates. Some effort is made to keep the
 * original sequence of atoms.
 */
QList<QList<CrystalAtom> > CrystalStructure::mergeAtomsBgmn(const QList<CrystalAtom> &atm)
{
    QList<QList<CrystalAtom> > merged;
    QMap<QString, int> index;

    for (int i = 0; i < atm.size(); ++i) {
        QString tag = QString("%1;%2;%3")
                .arg(atm.at(i).x(), 0, 'f', 6)
                .arg(atm.at(i).y(), 0, 'f', 6)
                .arg(atm.at(i).z(), 0, 'f', 6);

        if (index.contains(tag)) {
            merged[index.value(tag)].append(atm.at(i));
        } else {
            index[tag] = merged.size();
            QList<CrystalAtom> newPos;
            newPos.append(atm.at(i));
            merged.append(newPos);
        }
    }

    checkSOF(merged);
    return merged;
}

/*
 * checks if the site occupancy factor of multi-element sites sums to 1.0
 * if not, normalizes all SOF of this site to a sum of 1.0
 */
void CrystalStructure::checkSOF(QList<QList<CrystalAtom> > &atm)
{
    for (int i = 0; i < atm.size(); ++i) {
        double sof = 0.0;

        for (int j = 0; j < atm.at(i).size(); ++j) {
            sof += atm.at(i).at(j).occupancy();
        }

        if (sof > 1.05) {
            for (int j = 0; j < atm.at(i).size(); ++j) {
                atm[i][j].setOccupancy(atm.at(i).at(j).occupancy() / sof);
            }
        }
    }
}

bool CrystalStructure::hasWyckoff()
{
    int missingWck = 0;

    for (int i = 0; i < _atoms.size(); ++i) {
        if (_atoms.at(i).wyckoff().isEmpty()) missingWck++;
    }

    return missingWck == 0;
}

/*
 * dmin and lambda in Angstrom
 */
QMap<long, QList<CrystalStructureFactor> > CrystalStructure::getHklList(double dmin, double lambda, double b1)
{
    QList<CrystalAtom> allAtoms = generateAtoms(_atoms);

    CrystalHklGenerator hklGen(_unitCell, allAtoms);
    return hklGen.getHklList(dmin, lambda, b1);
}

