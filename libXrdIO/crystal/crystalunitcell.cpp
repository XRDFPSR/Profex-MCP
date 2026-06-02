/***************************************************************************
                          crystalunitcell.cpp  -  description
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

#include "crystalunitcell.h"
#include "../parser/spgrdatparser.h"

#include <math.h>
#include <QDebug>
#include <QRegularExpression>
#include <QtMath>

CrystalUnitCell::CrystalUnitCell()
{
    // initialize all global variables
    reset();
}

/*
 * initializes or resets all global variables
 */
void CrystalUnitCell::reset()
{
    _axes[0] = -1.0;
    _axes[1] = -1.0;
    _axes[2] = -1.0;

    _angles[0] = -1.0;
    _angles[1] = -1.0;
    _angles[2] = -1.0;

    _axes_esd[0] = -1.0;
    _axes_esd[1] = -1.0;
    _axes_esd[2] = -1.0;

    _angles_esd[0] = -1.0;
    _angles_esd[1] = -1.0;
    _angles_esd[2] = -1.0;

    _spgr_name_bgmn = QString();
    _spgr_name_cif = QString();

    _it_num = 0;
    _setting_num = 0;
    _axis_unit = NM;
}

/*
 * sets the unit cell axis lengths
 */
void CrystalUnitCell::setAxes(double a, double b, double c, double ea, double eb, double ec)
{
    _axes[0] = a;
    _axes[1] = b;
    _axes[2] = c;
    _axes_esd[0] = ea;
    _axes_esd[1] = eb;
    _axes_esd[2] = ec;
}

/*
 * sets the unit cell angles
 */
void CrystalUnitCell::setAngles(double al, double be, double ga, double eal, double ebe, double ega)
{
    _angles[0] = al;
    _angles[1] = be;
    _angles[2] = ga;
    _angles_esd[0] = eal;
    _angles_esd[1] = ebe;
    _angles_esd[2] = ega;
}

/*
 * sets the unit cell axes and angles
 */
void CrystalUnitCell::setCell(double a,  double b,  double c,  double al,  double be,  double ga,
                              double ea, double eb, double ec, double eal, double ebe, double ega)
{

    setAxes(a, b, c, ea, eb, ec);
    setAngles(al, be, ga, eal, ebe, ega);
    fixCell();
}

/*
 * sets the space group HM symbol in BGMN nomenclature
 */
void CrystalUnitCell::setSpaceGroupHMBgmn(const QString &s)
{
    _spgr_name_bgmn = s;
    _spgr_name_cif = spgrBgmnToCif(s);
}

/*
 * sets the international tables number of the space group
 */
void CrystalUnitCell::setItNumber(int i)
{
    _it_num = i;
    _spgr_cryst_system_cif = spgrCrystalSystemCif(_it_num);
}

void CrystalUnitCell::setSettingNumber(int i)
{
    _setting_num = i;
}

/*
 * sets the space group HM symbol in CIF nomenclature.
 * Note that the BGMN nomenclature is erased. This is because
 * the translation from CIF to BGMN is very complicated (if not impossible)
 * and requires user input.
 */
void CrystalUnitCell::setSpaceGroupHMCif(const QString &s)
{
    _spgr_name_bgmn = QString();
    _spgr_name_cif = s;
}

/*
 * returns the unit cell axis lengths. returns true if successful, or false if an error
 * (e.g. due to undefined values) occurs
 */
bool CrystalUnitCell::axes(double &a, double &b, double &c)
{
    bool ok = fixCell();

    if (ok) {
        a = _axes[0];
        b = _axes[1];
        c = _axes[2];
    }

    return ok;
}

/*
 * returns the unit cell angles. returns true if successful, or false if an error
 * (e.g. due to undefined values) occurs
 */
bool CrystalUnitCell::angles(double &al, double &be, double &ga)
{
    bool ok = fixCell();

    if (ok) {
        al = _angles[0];
        be = _angles[1];
        ga = _angles[2];
    }

    return ok;
}

/*
 * returns the ESDs of unit cell axis lengths. returns true if successful, or false if an error
 * (e.g. due to undefined values) occurs
 * note: ESDs for non-refined parameters are set to std::numeric_limits<double>::min()
 */
bool CrystalUnitCell::axes_esd(double &ea, double &eb, double &ec)
{
    bool ok = fixCell();

    if (ok) {
        ea = _axes_esd[0];
        eb = _axes_esd[1];
        ec = _axes_esd[2];
    }

    return ok;
}

/*
 * returns the ESDs of unit cell angles. returns true if successful, or false if an error
 * (e.g. due to undefined values) occurs
 * note: ESDs for non-refined parameters are set to std::numeric_limits<double>::min()
 */
bool CrystalUnitCell::angles_esd(double &eal, double &ebe, double &ega)
{
    bool ok = fixCell();

    if (ok) {
        eal = _angles_esd[0];
        ebe = _angles_esd[1];
        ega = _angles_esd[2];
    }

    return ok;
}

/*
 * returns the coordinates of a, b, and c axis in cartesian coordinates
 */
bool CrystalUnitCell::axes_cartesian(double &ax, double &ay, double &az,
                                     double &bx, double &by, double &bz,
                                     double &cx, double &cy, double &cz)
{
    bool ok = fixCell();

    QMatrix4x4 m = f2cMatrix();

    QVector3D cva = m.map(QVector3D(1.0, 0.0, 0.0));
    QVector3D cvb = m.map(QVector3D(0.0, 1.0, 0.0));
    QVector3D cvc = m.map(QVector3D(0.0, 0.0, 1.0));

    ax = cva.x();
    ay = cva.y();
    az = cva.z();

    bx = cvb.x();
    by = cvb.y();
    bz = cvb.z();

    cx = cvc.x();
    cy = cvc.y();
    cz = cvc.z();

    return ok;
}

/*
 * automatically determines missing unit cell parameters based on the lattice system
 * e.g. angles that must be 90 will be set to 90
 * axes that must be equal will be set to the same value etc.
 */
bool CrystalUnitCell::fixCell()
{
    // 001 - 002: triclinic
    // 003 - 015: monoclinic
    // 016 - 074: orthorhombic
    // 075 - 142: tetragonal
    // 143 - 167: trigonal
    // 168 - 194: hexagonal
    // 195 - 230: cubic

    // if the _it_num is invalid, we need all 6 cell constants, none of them
    // can be reconstructed. So in that case we check for triclinic cell parameters

    bool ok;
    if (_it_num <= 0)        ok = fixTriclinicCell();
    else if (_it_num <=   2) ok = fixTriclinicCell();
    else if (_it_num <=  15) ok = fixMonoclinicCell();
    else if (_it_num <=  74) ok = fixOrthorhombicCell();
    else if (_it_num <= 142) ok = fixTetragonalCell();
    else if (_it_num <= 167) ok = fixTrigonalCell();
    else if (_it_num <= 194) ok = fixHexagonalCell();
    else if (_it_num <= 230) ok = fixCubicCell();
    else                     ok = fixTriclinicCell();

    if (ok) {
        _vol = calcVolume();
        return true;
    }

    return false;
}

double CrystalUnitCell::calcVolume()
{
    double PI = atan(1.0) * 4.0;

    // rounding errors due to limite PI resolution may cause problems. therefore
    // we hard-code the cosine of 90° to 0.0
    double calpha = _angles[0] == 90.0 ? 0.0 : cos(_angles[0] * PI / 180.0);
    double cbeta  = _angles[1] == 90.0 ? 0.0 : cos(_angles[1] * PI / 180.0);
    double cgamma = _angles[2] == 90.0 ? 0.0 : cos(_angles[2] * PI / 180.0);

    double v = _axes[0] * _axes[1] * _axes[2] * sqrt(1.0
                            - pow(calpha, 2.0) - pow(cbeta, 2.0) - pow(cgamma, 2.0)
                            + 2.0 * calpha * cbeta * cgamma);
    return v;
}

/*
 * translates the HM symbol from BGMN notation to CIF notation
 */
QString CrystalUnitCell::spgrBgmnToCif(const QString &s)
{
    QString cif;
    static QRegularExpression rx("([A-Z])(-?[a-z\\d](?:_\\d)?(?:/[a-z])?)(-?[a-z\\d](?:_\\d)?(?:/[a-z])?)?(-?[a-z\\d](?:_\\d)?(?:/[a-z])?)?");
    QRegularExpressionMatch rm = rx.match(s);

    if (rm.hasMatch()) {
        QStringList cpt = rm.capturedTexts();
        cpt.removeFirst(); // holds captured(0), which is not needed

        for (int i = 0; i < cpt.size(); ++i) {
            cpt[i].replace("_", "");
        }

        cif = cpt.join(" ").trimmed();
    } else {
        // changing the nomenclature failed, so use the name read from the LST file
        qDebug() << QString("CrystalUnitCell::spgrBgmnToCif(): Could not translate HM symbol %1 to CIF format").arg(s);
    }

    return cif;
}

/*
 * constructs a transformation matrix from fractional to cartesian coordinates
 */
QMatrix4x4 CrystalUnitCell::f2cMatrix()
{
    float PI = atan(1.0) * 4.0;

    float data[16] = {0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 0.0,
                      0.0, 0.0, 0.0, 1.0};

    // cosines
    float ca = _angles[0] == 90.0 ? 0.0 : cos(_angles[0] * PI / 180.0);
    float cb = _angles[1] == 90.0 ? 0.0 : cos(_angles[1] * PI / 180.0);
    float cc = _angles[2] == 90.0 ? 0.0 : cos(_angles[2] * PI / 180.0);

    // sines
    // float sa = _angles[0] == 90.0 ? 1.0 : sin(_angles[0] * PI / 180.0);
    float sb = _angles[1] == 90.0 ? 1.0 : sin(_angles[1] * PI / 180.0);
    float sc = _angles[2] == 90.0 ? 1.0 : sin(_angles[2] * PI / 180.0);

    float v = _axes[0] * _axes[1] * _axes[2] * sqrt(1.0 - pow(ca, float(2.0)) - pow(cb, float(2.0)) - pow(cc, float(2.0))
                   + 2.0 * ca * cb * cc);

    // reciprocal axes
    // float ra = _axes[1] * _axes[2] * sa / v;
    // float rb = _axes[0] * _axes[2] * sb / v;
    float rc = _axes[0] * _axes[1] * sc / v;

    // reciprocal angle cosines
    float rca = (cb * cc - ca)/(sb * sc);
    // float rcb = (ca * cc - cb)/(sa * sc);
    // float rcc = (ca * cb - cc)/(sa * sb);

    // write the matrix elements
    data[0] = _axes[0];
    data[1] = _axes[1] * cc;
    data[2] = _axes[2] * cb;
    data[5] = _axes[1] * sc;
    data[6] = _axes[2] * rca * sb;
    data[10] = 1.0 / rc;

    return QMatrix4x4(data);
}

/*
 * returns the _space_group_crystal_system tag for cif files
 */
QString CrystalUnitCell::spgrCrystalSystemCif(int it)
{
    QString cs("?");

    if ((it >= 1)   && (it <= 2))   cs = QString("triclinic");
    if ((it >= 3)   && (it <= 15))  cs = QString("monoclinic");
    if ((it >= 16)  && (it <= 74))  cs = QString("orthorhombic");
    if ((it >= 75)  && (it <= 142)) cs = QString("tetragonal");
    if ((it >= 143) && (it <= 167)) cs = QString("trigonal");
    if ((it >= 168) && (it <= 194)) cs = QString("hexagonal");
    if ((it >= 195) && (it <= 230)) cs = QString("cubic");

    return cs;
}

/*
 * returns a string list with all symmetry operations (x y z, -x -y -z, etc)
 */
QList<QStringList> CrystalUnitCell::symmetryOperations()
{
    QList<QStringList> l;

    if (_symOpMatrices.isEmpty()) {
        parseSymmetryOperations();
    }

    // convert symop matrices to stringlists
    for (int i = 0; i < _symOpMatrices.size(); ++i) {
        l.append(_symOpMatrices.at(i).toStringList());
    }

    return l;
}

QList<CrystalSymOp> CrystalUnitCell::symmetryOperationMatrices()
{
    if (_symOpMatrices.isEmpty()) {
        parseSymmetryOperations();
    }

    return _symOpMatrices;
}

bool CrystalUnitCell::parseSymmetryOperations()
{
    // do not check for the setting number because it will be empty
    // if the structure was read from a *.lst file
    if (_spgr_name_bgmn.isEmpty()) {
        qDebug() << QString("CrystalUnitCell::parseSymmetryOperations(): "
                            "Attempting to read SymOps from SPACEGROUP.DAT but _spgr_name_bgmn is invalid: %1")
                        .arg(_spgr_name_bgmn);
        return false;
    }

    SpgrDatParser sParser;
    QList<QStringList> lst = sParser.getSymOps(_spgr_name_bgmn, _setting_num);
    int n = setSymmetryOperations(lst);

    return n > 0;
}

/*
 * return a specific d-spacing for hkl
 */
double CrystalUnitCell::dSpacing(int ih, int ik, int il)
{
    double h = double(ih);
    double k = double(ik);
    double l = double(il);

    double cosal = qCos(qDegreesToRadians(_angles[0]));
    double cosbe = qCos(qDegreesToRadians(_angles[1]));
    double cosga = qCos(qDegreesToRadians(_angles[2]));

    double sinal = qSin(qDegreesToRadians(_angles[0]));
    double sinbe = qSin(qDegreesToRadians(_angles[1]));
    double singa = qSin(qDegreesToRadians(_angles[2]));

    double termh = _axes[1]*_axes[1] * _axes[2]*_axes[2] * h*h * sinal*sinal + 2.0 * _axes[0]*_axes[0] * _axes[1] * _axes[2] * k * l * (cosbe * cosga - cosal);
    double termk = _axes[2]*_axes[2] * _axes[0]*_axes[0] * k*k * sinbe*sinbe + 2.0 * _axes[0] * _axes[1]*_axes[1] * _axes[2] * h * l * (cosal * cosga - cosbe);
    double terml = _axes[0]*_axes[0] * _axes[1]*_axes[1] * l*l * singa*singa + 2.0 * _axes[0] * _axes[1] * _axes[2]*_axes[2] * h * k * (cosal * cosbe - cosga);
    double termdenom = _axes[0]*_axes[0] * _axes[1]*_axes[1] * _axes[2]*_axes[2] * (1 - cosal*cosal - cosbe*cosbe - cosga*cosga + 2.0 * cosal * cosbe * cosga);

    return qSqrt(termdenom / (termh + termk + terml));
}

bool CrystalUnitCell::fixTriclinicCell()
{
    int errors = 0;
    errors += _axes[0] > 0.0 ? 0 : 1;
    errors += _axes[1] > 0.0 ? 0 : 1;
    errors += _axes[2] > 0.0 ? 0 : 1;
    errors += _angles[0] > 0.0 ? 0 : 1;
    errors += _angles[1] > 0.0 ? 0 : 1;
    errors += _angles[2] > 0.0 ? 0 : 1;

    if (errors) {
        dumpFixError("fixTriclinicCell");
        return false;
    }

    return true;
}

bool CrystalUnitCell::fixMonoclinicCell()
{
    int errors = 0;
    errors += _axes[0] > 0.0 ? 0 : 1;
    errors += _axes[1] > 0.0 ? 0 : 1;
    errors += _axes[2] > 0.0 ? 0 : 1;

    if (errors) {
        dumpFixError("fixMonoclinicCell");
        return false;
    }

    if ((_angles[0] > 0.0) && !qFuzzyCompare(_angles[0], 90.0)) {
        _angles[1] = 90.0;
        _angles[2] = 90.0;
        _angles_esd[1] = 1.0;
        _angles_esd[2] = 1.0;
        return true;
    }

    if ((_angles[1] > 0.0) && !qFuzzyCompare(_angles[1], 90.0)) {
        _angles[0] = 90.0;
        _angles[2] = 90.0;
        _angles_esd[0] = 1.0;
        _angles_esd[2] = 1.0;
        return true;
    }

    if ((_angles[2] > 0.0) && !qFuzzyCompare(_angles[2], 90.0)) {
        _angles[0] = 90.0;
        _angles[1] = 90.0;
        _angles_esd[0] = 1.0;
        _angles_esd[1] = 1.0;
        return true;
    }

    dumpFixError("fixMonoclinicCell");
    return false;
}

bool CrystalUnitCell::fixOrthorhombicCell()
{
    int errors = 0;
    errors += _axes[0] > 0.0 ? 0 : 1;
    errors += _axes[1] > 0.0 ? 0 : 1;
    errors += _axes[2] > 0.0 ? 0 : 1;

    if (errors) {
        dumpFixError("fixOrthorhombicCell");
        return false;
    }

    _angles[0] = 90.0;
    _angles[1] = 90.0;
    _angles[2] = 90.0;
    _angles_esd[0] = -1.0;
    _angles_esd[1] = -1.0;
    _angles_esd[2] = -1.0;

    return true;
}

bool CrystalUnitCell::fixTetragonalCell()
{
    if (_axes[2] <= 0.0) {
        // c axis = unique axis, must be provided
        dumpFixError("fixTetragonalCell");
        return false;
    }

    _angles[0] = 90.0;
    _angles[1] = 90.0;
    _angles[2] = 90.0;
    _angles_esd[0] = -1.0;
    _angles_esd[1] = -1.0;
    _angles_esd[2] = -1.0;

    // either a or b must be provided
    if (_axes[0] > 0.0) {
        _axes[1] = _axes[0];
        _axes_esd[1] = _axes_esd[0];
        return true;
    } else if (_axes[1] > 0.0) {
        _axes[0] = _axes[1];
        _axes_esd[0] = _axes_esd[1];
        return true;
    }

    dumpFixError("fixTetragonalCell");
    return false;
}

bool CrystalUnitCell::fixTrigonalCell()
{
    if ((_axes[0] > 0.0) && (_axes[2] > 0.0) && !qFuzzyCompare(_axes[0], _axes[2])) {
        // trigonal setting
        _axes[1]   = _axes[0];
        _angles[0] =  90.0;
        _angles[1] =  90.0;
        _angles[2] = 120.0;

        _axes_esd[1]   = _axes_esd[0];
        _angles_esd[0] = -1.0;
        _angles_esd[1] = -1.0;
        _angles_esd[2] = -1.0;
        return true;
    }

    double ang = 0.0;
    double angEsd = -1.0;
    double ax = 0.0;
    double axEsd = -1.0;

    for (int i = 0; i < 3; ++i) {
        // check if we have one angle > 0 but != 90 and != 120
        // and one axis > 0
        if ((_angles[i] > 0.0) && !qFuzzyCompare(_angles[i], 90.0) && !qFuzzyCompare(_angles[i], 120.0)) {
            ang = _angles[i];
            angEsd = _angles_esd[i];
        }

        if (_axes[i] > 0.0) {
            ax = _axes[i];
            axEsd = _axes_esd[i];
        }
    }

    if (!qFuzzyIsNull(ang) && !qFuzzyIsNull(ax)) {
        // rhombohedral setting
        _axes[0] = ax;
        _axes[1] = ax;
        _axes[2] = ax;
        _axes_esd[0] = axEsd;
        _axes_esd[1] = axEsd;
        _axes_esd[2] = axEsd;
        _angles[0] = ang;
        _angles[1] = ang;
        _angles[2] = ang;
        _angles_esd[0] = angEsd;
        _angles_esd[1] = angEsd;
        _angles_esd[2] = angEsd;
        return true;
    }

    dumpFixError("fixTrigonalCell");
    return false;
}

bool CrystalUnitCell::fixHexagonalCell()
{
    if ((_axes[0] > 0.0) && (_axes[2] > 0.0) && !qFuzzyCompare(_axes[0], _axes[2])) {
        _axes[1]   = _axes[0];
        _angles[0] =  90.0;
        _angles[1] =  90.0;
        _angles[2] = 120.0;

        _axes_esd[1]   = _axes_esd[0];
        _angles_esd[0] = -1.0;
        _angles_esd[1] = -1.0;
        _angles_esd[2] = -1.0;
        return true;
    }

    dumpFixError("fixHexagonalCell");
    return false;
}

bool CrystalUnitCell::fixCubicCell()
{
    _angles[0] = 90.0;
    _angles[1] = 90.0;
    _angles[2] = 90.0;
    _angles_esd[0] = -1.0;
    _angles_esd[1] = -1.0;
    _angles_esd[2] = -1.0;

    if (_axes[0] > 0.0) {
        _axes[1] = _axes[0];
        _axes[2] = _axes[0];
        _axes_esd[1] = _axes_esd[0];
        _axes_esd[2] = _axes_esd[0];
        return true;
    }

    if (_axes[1] > 0.0) {
        _axes[0] = _axes[1];
        _axes[2] = _axes[1];
        _axes_esd[0] = _axes_esd[1];
        _axes_esd[2] = _axes_esd[1];
        return true;
    }

    if (_axes[2] > 0.0) {
        _axes[0] = _axes[2];
        _axes[1] = _axes[2];
        _axes_esd[0] = _axes_esd[2];
        _axes_esd[1] = _axes_esd[2];
        return true;
    }

    dumpFixError("fixCubicCell");
    return false;
}

void CrystalUnitCell::dumpFixError(const QString &s)
{
    qDebug() << QString("CrystalUnitCell::%1: Missing unit cell constants can't be reconstructed:").arg(s);
    qDebug() << QString("    a = %1").arg(_axes[0]);
    qDebug() << QString("    b = %1").arg(_axes[1]);
    qDebug() << QString("    c = %1").arg(_axes[2]);
    qDebug() << QString("    alpha = %1").arg(_angles[0]);
    qDebug() << QString("    beta  = %1").arg(_angles[1]);
    qDebug() << QString("    gamma = %1").arg(_angles[2]);
}

int CrystalUnitCell::setSymmetryOperations(const QList<QStringList> &l)
{
    _symOpMatrices.clear();

    for (int i = 0; i < l.size(); ++i) {
        _symOpMatrices.append(CrystalSymOp(l.at(i)));
    }

    QList<CrystalSymOp> transl = checkSymmetryOperationTranslations(_symOpMatrices, _spgr_name_cif.left(1));
    if (transl.size()) _symOpMatrices.append(transl);
    return _symOpMatrices.size();
}

/*
 * Returns a list of symmetry operations with translations applied.
 * The returned list does NOT contain the original symmetry operations. The
 * original and translated list of symops must be merged elsewhere.
 * The returned list can be empty if translation is P or if the original list
 * already contains all translated symops.
 */
QList<CrystalSymOp> CrystalUnitCell::checkSymmetryOperationTranslations(const QList<CrystalSymOp> &so, const QString &transl)
{
    QString tSymb = transl.toUpper();

    if (tSymb == "P") return QList<CrystalSymOp>();

    if (tSymb == "R") {
        if ((_axes[0] == _axes[1]) && (_axes[0] == _axes[2])) tSymb = "RRH";
        else                                                  tSymb = "RHX";
    }

    QList<CrystalSymOp> out;

    for (int i = 0; i < so.size(); ++i) {
        CrystalSymOp csop = so.at(i);
        QList<CrystalSymOp> csopT = csop.translate(tSymb);

        for (int j = 0; j < csopT.size(); ++j) {
            if (!so.contains(csopT.at(j))) {
                out.append(csopT.at(j));
            }
        }
    }

    int sizeCheck = 0;

    if      (tSymb == "I")   sizeCheck = 1;
    else if (tSymb == "F")   sizeCheck = 3;
    else if (tSymb == "A")   sizeCheck = 1;
    else if (tSymb == "B")   sizeCheck = 1;
    else if (tSymb == "C")   sizeCheck = 1;
    else if (tSymb == "RHX") sizeCheck = 2;
    else if (tSymb == "D")   sizeCheck = 2;

    if (out.size() == sizeCheck * so.size()) return out;

    if (!out.size()) {
        qDebug() << QString("CrystalUnitCell::checkSymmetryOperationTranslations():"
                            " List of translated SymOps is empty."
                            " Maybe the provided list already contained the translated SymOps.");
    } else {
        qDebug() << QString("CrystalUnitCell::checkSymmetryOperationTranslations():"
                        " Size check of translated SymOps list failed");
        qDebug() << QString("    Received size: %1").arg(out.size());
        qDebug() << QString("    Expected size: %1").arg(sizeCheck * so.size());
    }

    return QList<CrystalSymOp>();
}

/*
 * returns true if all cell parameters are > 0.0
 */
bool CrystalUnitCell::isValid() const
{
    double th = 0.0001;
    if (_axes[0]   < th) return false;
    if (_axes[1]   < th) return false;
    if (_axes[2]   < th) return false;
    if (_angles[0] < th) return false;
    if (_angles[1] < th) return false;
    if (_angles[2] < th) return false;
    return true;
}
