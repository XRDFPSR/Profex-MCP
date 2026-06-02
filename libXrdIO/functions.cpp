/***************************************************************************
                          functions.cpp  -  description
                             -------------------
    begin                : Mon Jun 17, 2019
    copyright            : (C) 2019 by Nicola Doebelin
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


#include "functions.h"
#include "3rdparty/alglib/src/fasttransforms.h"
#include <QDebug>
#include <math.h>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

global::Functions::Functions()
{

}

QColor global::Functions::colorToDarkMode(const QColor &c)
{
    // shortcut for black to white conversion
    if (c == QColor(Qt::black)) return QColor(Qt::white);

    // calculate luminance using BT.709 formula
    double lum = 0.2126 * c.redF() + 0.7152 * c.greenF() + 0.0722 * c.blueF();

    qreal h = c.hueF();
    qreal s = c.saturationF();
    qreal v = c.valueF();

    if (lum > 0.5) {
        // darken
        s += (1.0 - s) / 2.0;
        v -= v / 2.0;
    } else {
        // lighten
        s -= s / 2.0;
        v += (1.0 - v) / 2.0;
    }

    return QColor::fromHsvF(h, s, v);
}

/* d and wl must be given in the same unit (nm or A) */
double global::Functions::dToTwoTheta(double d, double wl)
{
    if (qFuzzyIsNull(d)) return -1.0;
    return 360.0 * asin(wl / (2.0 * d)) / M_PI;
}

/* d is returned in the same unit as wl (nm or A) */
double global::Functions::twoThetaToD(double t, double wl)
{
    if (qFuzzyIsNull(t)) return -1.0;
    return wl / (2.0 * sin(t * M_PI / 360.0));
}

double global::Functions::qToTwoTheta(double q, double wl)
{
    if (qFuzzyIsNull(wl)) return -1.0;
    return 360.0 * asin(q * wl / (4.0 * M_PI)) / M_PI;
}

double global::Functions::twoThetaToQ(double t, double wl)
{
    if (qFuzzyIsNull(wl)) return -1.0;
    return 4.0 * M_PI * sin(t * M_PI / 360.0) / wl;
}

bool global::Functions::fuzzyCompareFractions(double a, double b)
{
    double fa = fmod(a, 1.0);
    double fb = fmod(b, 1.0);

    while (fa < 0.0) fa += 1.0;
    while (fb < 0.0) fb += 1.0;

    while (fa > 1.0) fa -= 1.0;
    while (fb > 1.0) fb -= 1.0;

    if ((fa >= 0.3333) && (fa <= 0.3334) && (fb >= 0.3333) && (fb <= 0.3334)) return true; // 1/3
    if ((fa >= 0.6666) && (fa <= 0.6667) && (fb >= 0.6666) && (fb <= 0.6667)) return true; // 2/3
    if ((fa >= 0.1666) && (fa <= 0.1667) && (fb >= 0.1666) && (fb <= 0.1667)) return true; // 1/6
    if ((fa >= 0.8333) && (fa <= 0.8334) && (fb >= 0.8333) && (fb <= 0.8334)) return true; // 5/6
    if ((fa >= 0.0833) && (fa <= 0.0834) && (fb >= 0.0833) && (fb <= 0.0834)) return true; // 1/12
    if ((fa >= 0.4166) && (fa <= 0.4167) && (fb >= 0.4166) && (fb <= 0.4167)) return true; // 5/12
    if ((fa >= 0.5833) && (fa <= 0.5834) && (fb >= 0.5833) && (fb <= 0.5834)) return true; // 7/12
    if ((fa >= 0.9166) && (fa <= 0.9167) && (fb >= 0.9166) && (fb <= 0.9167)) return true; // 11/12

    return qFuzzyCompare(fa, fb);
}

bool global::Functions::isSpecialCoordinate(double c)
{
    double d = c;

    while (d < 0.0)  d += 1.0;
    while (d >= 1.0) d -= 1.0;

    if (qFuzzyIsNull(d))           return true;
    if (qFuzzyCompare(d, 1.0/2.0)) return true;
    if (qFuzzyCompare(d, 1.0/4.0)) return true;
    if (qFuzzyCompare(d, 3.0/4.0)) return true;
    if (qFuzzyCompare(d, 1.0/8.0)) return true;
    if (qFuzzyCompare(d, 3.0/8.0)) return true;
    if (qFuzzyCompare(d, 5.0/8.0)) return true;
    if (qFuzzyCompare(d, 7.0/8.0)) return true;

    if      (fabs(d - 0.3333) < 0.0001) return true; //  1/3
    else if (fabs(d - 0.6667) < 0.0001) return true; //  2/3
    else if (fabs(d - 0.1667) < 0.0001) return true; //  1/6
    else if (fabs(d - 0.8333) < 0.0001) return true; //  5/6
    else if (fabs(d - 0.0833) < 0.0001) return true; //  1/12
    else if (fabs(d - 0.4167) < 0.0001) return true; //  5/12
    else if (fabs(d - 0.5833) < 0.0001) return true; //  7/12
    else if (fabs(d - 0.9167) < 0.0001) return true; // 11/12

    return false;
}

double global::Functions::angularCorrection(double tt, double eps1, double eps2, double eps3)
{
    double deps1 = qFuzzyIsNull(eps1) ? 0.0 : -eps1 * 360.0 / M_PI;
    double deps2 = qFuzzyIsNull(eps2) ? 0.0 : 2.0 * (-eps2 * cos(tt * M_PI / 360.0)) * 180.0 / M_PI;
    double deps3 = qFuzzyIsNull(eps3) ? 0.0 : -eps3 * sin(tt) * 180.0 / M_PI;
    return deps1 + deps2 + deps3;
}

void global::Functions::normalizeCoordinates(double &x, double &y, double &z)
{
    while (x < 0.0) x += 1.0;
    while (y < 0.0) y += 1.0;
    while (z < 0.0) z += 1.0;

    while (x >= 1.0) x -= 1.0;
    while (y >= 1.0) y -= 1.0;
    while (z >= 1.0) z -= 1.0;

    // x/12 are needed in case of translations by 1/2 or 2/3

    if      (fabs(x - 0.3333) < 0.0001) x = 1.0/3.0;
    else if (fabs(x - 0.6667) < 0.0001) x = 2.0/3.0;
    else if (fabs(x - 0.1667) < 0.0001) x = 1.0/6.0;
    else if (fabs(x - 0.8333) < 0.0001) x = 5.0/6.0;
    else if (fabs(x - 0.0833) < 0.0001) x = 1.0/12.0;
    else if (fabs(x - 0.4167) < 0.0001) x = 5.0/12.0;
    else if (fabs(x - 0.5833) < 0.0001) x = 7.0/12.0;
    else if (fabs(x - 0.9167) < 0.0001) x = 11.0/12.0;

    if      (fabs(y - 0.3333) < 0.0001) y = 1.0/3.0;
    else if (fabs(y - 0.6667) < 0.0001) y = 2.0/3.0;
    else if (fabs(y - 0.1667) < 0.0001) y = 1.0/6.0;
    else if (fabs(y - 0.8333) < 0.0001) y = 5.0/6.0;
    else if (fabs(y - 0.0833) < 0.0001) y = 1.0/12.0;
    else if (fabs(y - 0.4167) < 0.0001) y = 5.0/12.0;
    else if (fabs(y - 0.5833) < 0.0001) y = 7.0/12.0;
    else if (fabs(y - 0.9167) < 0.0001) y = 11.0/12.0;

    if      (fabs(z - 0.3333) < 0.0001) z = 1.0/3.0;
    else if (fabs(z - 0.6667) < 0.0001) z = 2.0/3.0;
    else if (fabs(z - 0.1667) < 0.0001) z = 1.0/6.0;
    else if (fabs(z - 0.8333) < 0.0001) z = 5.0/6.0;
    else if (fabs(z - 0.0833) < 0.0001) z = 1.0/12.0;
    else if (fabs(z - 0.4167) < 0.0001) z = 5.0/12.0;
    else if (fabs(z - 0.5833) < 0.0001) z = 7.0/12.0;
    else if (fabs(z - 0.9167) < 0.0001) z = 11.0/12.0;
}

// http://duffy.princeton.edu/sites/default/files/pdfs/links/xtalgeometry.pdf
double global::Functions::dSpacing(double _a, double _b, double _c, double _al, double _be, double _ga, int _h, int _k, int _l)
{
    double sinAl = sin(M_PI * _al / 180.0);
    double sinBe = sin(M_PI * _be / 180.0);
    double sinGa = sin(M_PI * _ga / 180.0);
    double cosAl = cos(M_PI * _al / 180.0);
    double cosBe = cos(M_PI * _be / 180.0);
    double cosGa = cos(M_PI * _ga / 180.0);

    double sinAl2 = pow(sinAl, 2.0);
    double sinBe2 = pow(sinBe, 2.0);
    double sinGa2 = pow(sinGa, 2.0);
    double cosAl2 = pow(cosAl, 2.0);
    double cosBe2 = pow(cosBe, 2.0);
    double cosGa2 = pow(cosGa, 2.0);

    double s11 = _b*_b * _c*_c * sinAl2;
    double s22 = _a*_a * _c*_c * sinBe2;
    double s33 = _a*_a * _b*_b * sinGa2;

    double s12 = _a*_b*_c*_c * (cosAl*cosBe - cosGa);
    double s23 = _a*_a*_b*_c * (cosBe*cosGa - cosAl);
    double s13 = _a*_b*_b*_c * (cosGa*cosAl - cosBe);

    double vol = _a*_b*_c * sqrt(1.0 - cosAl2 - cosBe2 - cosGa2 + 2.0 * cosAl * cosBe * cosGa);

    double dInv2 = (s11*_h*_h + s22*_k*_k + s33*_l*_l + 2.0*s12*_h*_k + 2.0*s23*_k*_l + 2.0*s13*_h*_l) / (vol*vol);
    double d2 = qFuzzyIsNull(dInv2) ? 0.0 : 1.0/dInv2;
    return d2 < 0.0 ? 0.0 : sqrt(d2);
}

// http://duffy.princeton.edu/sites/default/files/pdfs/links/xtalgeometry.pdf
double global::Functions::cellVolume(double _a, double _b, double _c, double _al, double _be, double _ga)
{
    double cosAl = cos(M_PI * _al / 180.0);
    double cosBe = cos(M_PI * _be / 180.0);
    double cosGa = cos(M_PI * _ga / 180.0);

    double cosAl2 = pow(cosAl, 2.0);
    double cosBe2 = pow(cosBe, 2.0);
    double cosGa2 = pow(cosGa, 2.0);

    return _a*_b*_c * sqrt(1.0 - cosAl2 - cosBe2 - cosGa2 + 2.0 * cosAl * cosBe * cosGa);
}


/*
 * conversion from Uiso to Biso
 */
double global::Functions::bisoFromUiso(double uiso)
{
    return uiso * M_PI * M_PI * 8.0;
}

/*
 * conversion from beta_ij to Biso, based on Fischer & Tillmanns, 1988
 */
double global::Functions::bisoFromBeta(double a, double b, double c, double alpha, double beta, double gamma,
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
double global::Functions::bisoFromBaniso(double alpha, double beta, double gamma,
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
double global::Functions::bisoFromUaniso(double alpha, double beta, double gamma,
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

/*
 * converts wavelength [nm] to energy [kev]
 */
double global::Functions::wavelengthToEnergy(double w)
{
    double h = 6.62607015e-34;    // in J*sec
    double c = 2.99792458e17;     // in nm/sec
    double j = 1.602176634e-19;   // energy of 1 eV in J
    return 0.001 * (h * c / w) / j;
}

/*
 * converts energy [keV] to wavelength [nm]
 */
double global::Functions::energyToWavelength(double e)
{
    double h = 6.62607015e-34;
    double c = 2.99792458e17;
    double j = 1.602176634e-19;
    return h * c / (1000.0 * e * j);
}

/*
 * data taken from X-ray data booklet, October 2009, Table 1-2, and converted
 * to nm using global::functions::energyToWavelength()
 */
QList<global::CharWaveLength> global::Functions::getAllWavelengths()
{
    QList<global::CharWaveLength> lst;

    lst.append(global::CharWaveLength(QString("Cr"), 0.2289760, 0.2293664, 0.2084921));
    lst.append(global::CharWaveLength(QString("Fe"), 0.1936042, 0.1940030, 0.1756653));
    lst.append(global::CharWaveLength(QString("Co"), 0.1789010, 0.1792897, 0.1620829));
    lst.append(global::CharWaveLength(QString("Ni"), 0.1657910, 0.1661788, 0.1500173));
    lst.append(global::CharWaveLength(QString("Cu"), 0.1540598, 0.1544430, 0.1392253));
    lst.append(global::CharWaveLength(QString("Ga"), 0.1340118, 0.1344028, 0.1207929));
    lst.append(global::CharWaveLength(QString("Mo"), 0.0709319, 0.0713607, 0.0632305));
    lst.append(global::CharWaveLength(QString("Ag"), 0.0559408, 0.0563813, 0.0497082));
    lst.append(global::CharWaveLength(QString("In"), 0.0512126, 0.0516558, 0.0454556));
    lst.append(global::CharWaveLength(QString("W"),  0.0209015, 0.0213833, 0.0184379));

    return lst;
}

double global::Functions::lpFactor(double d, double wl)
{
    double tt = dToTwoTheta(d, wl);
    double t = 0.5 * tt;

    double L = 1.0 / (sin(M_PI * t / 180.0) * sin(M_PI * tt / 180.0));
    double P = (1.0 + cos(M_PI * tt / 180.0) * cos(M_PI * tt / 180.0)) / 2.0;
    return L * P;
}

int global::Functions::getFloatPrecision(const QString &s)
{
    static QRegularExpression rx("^\\d+[\\.,]?(\\d+)?");
    QRegularExpressionMatch rm = rx.match(s);
    if (rm.hasMatch()) return rm.captured(1).length();
    return -1;
}

/*
 * tick marks for linear scales
 * adapted from ACM ALGORITHM 463
 */
QList <double> global::Functions::scaleAxis1(double mi, double ma, int num)
{
    if (num <= 0) return QList<double>();
    if (qFuzzyCompare(mi, ma)) return QList<double>();
    double min = qMin(mi, ma);
    double max = qMax(mi, ma);

    static double vint[4] = {1.0, 2.0, 5.0, 10.0};
    static double sqr[3] =  {sqrt(2.0), sqrt(10.0), sqrt(50.0)};

    //  find approximate interval size a
    double a = (max - min) / double(num);
    int nal = int(log10(a));

    if (a < 1.0) {
        --nal;
    }

    // a is scaled into variable b between 1 and 10
    double b = a / pow(10.0, double(nal));

    int j = 4;

    // find the closest permissible value for b
    for (int i = 1; i <= 3; ++i) {
        if (b < sqr[i - 1]) {
            j = i;
            break;
        }
    }

    // compute interval size
    double dist = vint[j - 1] * pow(10.0, double(nal));
    double fm1 = min / dist;
    int m1 = int(fm1);

    if (fm1 < 0.0) {
        --m1;
    }

    double r1 = double(m1) + 1.0 - fm1;

    if (qFuzzyIsNull(r1)) {
        ++m1;
    }

    double xminp = dist * m1;

    // compute new minimum and maximum limits
    double fm2 = max / dist;
    int m2 = int(fm2 + 1.0);

    if (fm2 < -1.0) {
        --m2;
    }

    r1 = fm2 - double(m2) + 1.0;

    if (qFuzzyIsNull(r1)) {
        --m2;
    }

    double xmaxp = dist * m2;

    if (xminp > min) xminp = min;
    if (xmaxp < max) xmaxp = max;

    QList<double> vec;

    for (double tm = xminp; tm <= xmaxp; tm += dist) {
        if (tm >= min && tm <= max) {
            vec.append(tm);
        }
    }

    return vec;
}

/*
 * tick marks for linear scales
 * adapted from ACM ALGORITHM 463
 */
QList<double> global::Functions::scaleAxis2(double mi, double ma, int num)
{
    if (num <= 0) return QList<double>();
    if (qFuzzyCompare(mi, ma)) return QList<double>();
    double min = qMin(mi, ma);
    double max = qMax(mi, ma);

    static double vint[5] = {1.0, 2.0, 5.0, 10.0, 20.0};

    //  fine approximate interval size a
    double a = (max - min) / double(num);
    int nal = int(log10(a));

    if (a < 1.0) {
        --nal;
    }

    // a is scaled into variable b between 1 and 10
    double b = a / pow(10.0, double(nal));

    double dist = 0.0;
    double xminp = 0.0;
    double xmaxp = 0.0;
    int m1 = 0;
    int m2 = 0;
    int j = 4;

    // find the closest permissible value for b
    for (int i = 1; i <= 3; ++i) {
        if (b < vint[i - 1]) {
            j = i;
            break;
        }
    }

    while (j <= 5) {
        // compute interval size
        dist = vint[j - 1] * pow(10.0, double(nal));
        double fm1 = min / dist;
        m1 = int(fm1);

        if (fm1 < 0.0) {
            --m1;
        }

        double r1 = double(m1) + 1.0 - fm1;

        if (qFuzzyIsNull(r1)) {
            ++m1;
        }

        xminp = dist * m1;

        // compute new minimum and maximum limits
        double fm2 = max / dist;
        m2 = int(fm2 + 1.0);

        if (fm2 < -1.0) {
            --m2;
        }

        r1 = fm2 - double(m2) + 1.0;

        if (qFuzzyIsNull(r1)) {
            --m2;
        }

        xmaxp = dist * m2;

        // check whether a second pass is required
        if (m2 - m1 <= num) {
            break;
        }

        ++j;
    }

    int nx = (num - m2 + m1) / 2;
    xminp -= nx * dist;
    xmaxp = xminp + num * dist;

    if (xminp > min) xminp = min;
    if (xmaxp < max) xmaxp = max;

    QList<double> vec;

    for (double tm = xminp; tm < xmaxp; tm += dist) {
        if (tm >= min && tm <= max) vec.append(tm);
    }

    return vec;
}

/*
 * tick marks for log10 scales
 * own implementation, critical values for lseg limits
 * were hand-matched to look good
 */
QList<double> global::Functions::scaleAxisLog(double mi, double ma, int num)
{
    if (num <= 0) return QList<double>();
    if (qFuzzyCompare(mi, ma)) return QList<double>();
    double min = qMin(mi, ma);
    double max = qMax(mi, ma);

    QList<double> vec;

    // if a large section is shown (from min to 5*min or larger) we compute
    // tick marks at even logarithmic intervals, e.g. 1, 10, 100
    if (max / min > 5.0) {
        // show exponential tick marks
        double lmin = floor(log10(min));
        double lmax = ceil(log10(max));
        double lseg = (log10(max) - log10(min)) / num;

        for (double d = lmin; d <= lmax; d += 1.0) {
            double pd = pow(10.0, d);

            if (pd >= min && pd <= max) vec.append(pd);

            if ((lseg < 0.5) && (lseg > 0.15)) {
                for (int z = 2; z < 10; z += 2) {
                    // add ticks at 2*pd, 4*pd, 6*pd, 8*pd
                    if (z * pd >= min && z * pd <= max)  vec.append(z * pd);
                }
            }

            if (lseg <= 0.15) {
                for (int z = 1; z < 10; ++z) {
                    // add ticks at 2*pd, 3*pd, 4*pd, ... 9*pd
                    if (z * pd >= min && z * pd <= max)  vec.append(z * pd);
                }
            }

            if (lseg <= 0.025) {
                for (int z = 1; z < 20; ++z) {
                    // add ticks at 1.5*pd, 2*pd, 2.5*pd, ... 9.5*pd
                    if (z * pd/2.0 >= min && z * pd/2.0 <= max)  vec.append(z * pd/2.0);
                }
            }
        }
    } else {
        // if only a small section is shown (max < 5*min) it is easier to just
        // use tick marks in regular linear intervals, e.g. 100, 150, 200, 250
        return scaleAxis1(min, max, num);
    }

    return vec;
}

/*
 * provide min and max in degrees 2theta, wl in Angstrom
 * type = 0: major ticks
 * type = 1: minor ticks
 */
QList<double> global::Functions::scaleAxisD(double mi, double ma, double wl, int type, int tickDensity)
{
    if (qFuzzyCompare(mi, ma)) return QList<double>();
    double min = qMin(mi, ma);
    double max = qMax(mi, ma);
    if ((tickDensity < 1) || (min <= 0.0)) return QList<double>();

    double segmentTT = (max - min)/double(tickDensity);

    QList<double> vSeg;

    for (int i = 0; i < tickDensity; ++i) {
        double dStr = global::Functions::twoThetaToD(max - i * segmentTT, wl);
        double dEnd = global::Functions::twoThetaToD(max - (i + 1) * segmentTT, wl);

        vSeg.append(scaleAxis1(dStr, dEnd, 1));
    }

    if (vSeg.size() < 2) return QList<double>();

    QList<double> vec;

    if (type == 0) {
        vec = vSeg;
    } else {
        vSeg.prepend(vSeg.first() - vSeg.at(1) + vSeg.first());
        vSeg.append(vSeg.last() + vSeg.last() - vSeg.at(vSeg.size() - 2));

        for (int i = 0; i < vSeg.size() - 1; ++i) {
            double d = vSeg.at(i + 1) - vSeg.at(i);

            if (d < 0.099999) {
                for (double m = vSeg.at(i); m <= vSeg.at(i+1); m += 0.01) vec.append(m);
            } else if (d < 0.99999) {
                for (double m = vSeg.at(i); m <= vSeg.at(i+1); m += 0.1) vec.append(m);
            } else if (d < 9.99999) {
                for (double m = vSeg.at(i); m <= vSeg.at(i+1); m += 1.0) vec.append(m);
            } else {
                for (double m = vSeg.at(i); m <= vSeg.at(i+1); m += 10.0) vec.append(m);
            }
        }
    }

    return vec;
}

/*
 * provide min and max in degrees 2theta, wl in Angstrom
 */
QList<double> global::Functions::scaleAxisQ(double mi, double ma, double wl, int tickDensity)
{
    if (qFuzzyCompare(mi, ma)) return QList<double>();
    double min = qMin(mi, ma);
    double max = qMax(mi, ma);
    if ((tickDensity < 1) || (min <= 0.0)) return QList<double>();

    double segmentTT = (max - min)/double(tickDensity);

    QList<double> vSeg;

    for (int i = 0; i < tickDensity; ++i) {
        double dStr = global::Functions::twoThetaToQ(max - i * segmentTT, wl);
        double dEnd = global::Functions::twoThetaToQ(max - (i + 1) * segmentTT, wl);

        vSeg.append(scaleAxis1(dStr, dEnd, 1));
    }

    if (vSeg.size() < 2) return QList<double>();

    std::sort(vSeg.begin(), vSeg.end());
    return vSeg;
}

/*
 * returns Rwp according to eq 5.41 in the BGMN user manual
 */
double global::Functions::getRwp(const Scan *iobs, const Scan *icalc, const QVector<double> &wi)
{
    int m = qMin(iobs->pDataIntensity().size(), icalc->pDataIntensity().size());

    double zaehlwp = 0.0;
    double nenwp = 0.0;

    for (int i = 0; i < m; ++i) {
        double yi  = iobs->pDataIntensity().at(i);
        double yic = icalc->pDataIntensity().at(i);

        if (qFuzzyIsNull(yi)) continue;
        double w = i >= wi.size() ? 1.0 : wi.at(i);

        zaehlwp += w * std::pow(yi - yic, 2.0);
        nenwp   += std::pow(w * yi, 2.0);
    }

    if (qFuzzyIsNull(nenwp)) return 0.0;
    return std::sqrt(zaehlwp / nenwp);
}

/*
 * returns Rexp according to eq 5.42 in the BGMN user manual
 */
double global::Functions::getRexp(const Scan *iobs, int p, const QVector<double> &wi)
{
    int m = iobs->pDataIntensity().size();
    int zaehlwp = m - p;
    double nenwp = 0.0;

    for (int i = 0; i < m; ++i) {
        double yi  = iobs->pDataIntensity().at(i);
        double w = i >= wi.size() ? 1.0 : wi.at(i);
        nenwp += std::pow(w * yi, 2.0);
    }

    if (qFuzzyIsNull(nenwp)) return 0.0;
    return std::sqrt(zaehlwp / nenwp);
}

/*
 * returns the denominator of Rwp and Rexp in eqs. 5.41 and 5.42 of the BGMN user manual.
 * since it doesn't change during the refinement, it may be more efficient to buffer it
 * separately instead of recalculating it each time.
 */
double global::Functions::getRdenom(const Scan *iobs, const QVector<double> &wi)
{
    int m = iobs->pDataIntensity().size();
    double nenwp = 0.0;

    for (int i = 0; i < m; ++i) {
        double yi  = iobs->pDataIntensity().at(i);
        if (qFuzzyIsNull(yi)) continue;
        double w = i >= wi.size() ? 1.0 : wi.at(i);

        nenwp += std::pow(w * yi, 2.0);
    }

    return nenwp;
}

/*
 * returns the numerator of Rwp according to eq 5.41 in the BGMN user manual. can be used
 * if the denominator is buffered.
 */
double global::Functions::getRwpNumer(const Scan *iobs, const Scan *icalc, const QVector<double> &wi)
{
    int m = qMin(iobs->pDataIntensity().size(), icalc->pDataIntensity().size());
    double zaehlwp = 0.0;

    for (int i = 0; i < m; ++i) {
        double yi  = iobs->pDataIntensity().at(i);
        double yic = icalc->pDataIntensity().at(i);
        if (qFuzzyIsNull(yi)) continue;
        double w = i >= wi.size() ? 1.0 : wi.at(i);

        zaehlwp += w * std::pow(yi - yic, 2.0);
    }

    return zaehlwp;
}

/*
 * int mode determines the returned value if the closest distance lies outside of the connection from A to B:
 * mode = 0: returns -1 if the "Lotfusspunkt" is outside of the connection from A to B
 * mode = 1: returns the distance to the closest point (A or B) if the "Lotfusspunkt" is outside
 * mode = 2: returns the distance to "Lotfusspunkt"
 */
double global::Functions::distanceFromLine(const QPointF &pt, const QPointF &la, const QPointF &lb, int mode)
{
    double a = std::sqrt(std::pow(pt.x() - lb.x(), 2.0) + std::pow(pt.y() - lb.y(), 2.0));
    double b = std::sqrt(std::pow(pt.x() - la.x(), 2.0) + std::pow(pt.y() - la.y(), 2.0));
    double c = std::sqrt(std::pow(la.x() - lb.x(), 2.0) + std::pow(la.y() - lb.y(), 2.0));

    double alpha = std::acos((a*a - b*b - c*c)/(-2.0 * b * c));
    double beta  = std::acos((b*b - a*a - c*c)/(-2.0 * a * c));

    double s = 0.5 * (c + b + a);
    double d = (2.0 / c) * std::sqrt(s * (s - c) * (s - b) * (s - a));

    if (alpha >= 0.5 * M_PI) {
        if      (mode == 0) return -1.0;
        else if (mode == 1) return b;
        else                return d;
    } else if (beta >= 0.5 * M_PI) {
        if      (mode == 0) return -1.0;
        else if (mode == 1) return a;
        else                return d;
    }

    return d;
}

/*
 * returns the number of digits required to distinguish valA from valB
 *
 * Example:
 * valA = 1.002
 * valB = 1.004
 *
 * the returned value is 3 because rounded to less significant values, valA and valB cannot be
 * distinguished anymore. This is typically used to label axes.
 *
 * min is the minimum value returned, because sometimes we want labels like 20.00, 30.00...
 */
int global::Functions::digitsForValues(double valA, double valB, int min)
{
    double l = std::log10(std::abs(valA - valB));
    if (l >= 0.0) return min;
    return std::max(min, int(std::abs(l)) + 1);
}

/*
 * Calculates the GrainSize from B1 and k1 using BGMNs formula
 */
double global::Functions::bgmnGrainSize(double b1, double k1)
{
    if (qFuzzyIsNull(b1)) return 0.0;
    if (k1 < 0.0)         return 0.0;

    return (4.0 / (3.0 * M_PI * b1)) * ((1.0 + 2.0 * std::sqrt(k1)))/std::pow(1.0 + std::sqrt(k1), 2.0);
}

/*
 * performs a simple linear regression on the data in x and y.
 * a = slope
 * b = intercept
 */
bool global::Functions::linearRegression(const QVector<double> &x, const QVector<double> &y, double &slope, double &intercept)
{
    int n = qMin(x.size(), y.size());
    if (n < 2) return false;

    double sumX  = 0.0;
    double sumX2 = 0.0;
    double sumY  = 0.0;
    double sumXY = 0.0;

    for (int i = 0; i < n; ++i) {
        sumX  += x.at(i);
        sumY  += y.at(i);
        sumXY += x.at(i) * y.at(i);
        sumX2 += x.at(i) * x.at(i);
    }

    double xMean = sumX / double(n);
    double yMean = sumY / double(n);
    double denom = sumX2 - sumX * xMean;

    if (qFuzzyIsNull(denom)) return false; // it's a vertical line

    slope = (sumXY - sumX * yMean) / denom;
    intercept = yMean - slope * xMean;
    return true;
}

bool global::Functions::sumFractions(const QString &fa, const QString &fb, int &num, int &denom)
{
    static QRegularExpression rx("([+-]?\\d+)/(\\d+)");

    QRegularExpressionMatch rm = rx.match(fa);
    if (!rm.hasMatch()) return false;

    int numA = rm.captured(1).toInt();
    int denA = rm.captured(2).toInt();

    rm = rx.match(fb);

    if (!rm.hasMatch()) {
        if (fb == "0") {
            num = numA;
            denom = denA;
            return true;
        } else {
            return false;
        }
    }

    int numB = rm.captured(1).toInt();
    int denB = rm.captured(2).toInt();

    num = numA * denB + numB * denA;
    denom = denA * denB;
    return true;
}

QString global::Functions::minimizeFraction(int num, int denom)
{
    if (denom == 0) return QString();
    while (num > denom) num -= denom;
    double d = double(num)/double(denom);

    if (qFuzzyCompare(d,  0.0 / 24.0)) return QString();
    if (qFuzzyCompare(d,  1.0 / 24.0)) return QString("1/24");
    if (qFuzzyCompare(d,  2.0 / 24.0)) return QString("1/12");
    if (qFuzzyCompare(d,  3.0 / 24.0)) return QString("1/8");
    if (qFuzzyCompare(d,  4.0 / 24.0)) return QString("1/6");
    if (qFuzzyCompare(d,  5.0 / 24.0)) return QString("5/24");
    if (qFuzzyCompare(d,  6.0 / 24.0)) return QString("1/4");
    if (qFuzzyCompare(d,  7.0 / 24.0)) return QString("7/24");
    if (qFuzzyCompare(d,  8.0 / 24.0)) return QString("1/3");
    if (qFuzzyCompare(d,  9.0 / 24.0)) return QString("3/8");
    if (qFuzzyCompare(d, 10.0 / 24.0)) return QString("5/12");
    if (qFuzzyCompare(d, 11.0 / 24.0)) return QString("11/24");
    if (qFuzzyCompare(d, 12.0 / 24.0)) return QString("1/2");
    if (qFuzzyCompare(d, 13.0 / 24.0)) return QString("13/24");
    if (qFuzzyCompare(d, 14.0 / 24.0)) return QString("7/12");
    if (qFuzzyCompare(d, 15.0 / 24.0)) return QString("5/8");
    if (qFuzzyCompare(d, 16.0 / 24.0)) return QString("2/3");
    if (qFuzzyCompare(d, 17.0 / 24.0)) return QString("17/24");
    if (qFuzzyCompare(d, 18.0 / 24.0)) return QString("3/4");
    if (qFuzzyCompare(d, 19.0 / 24.0)) return QString("19/24");
    if (qFuzzyCompare(d, 20.0 / 24.0)) return QString("5/6");
    if (qFuzzyCompare(d, 21.0 / 24.0)) return QString("7/8");
    if (qFuzzyCompare(d, 22.0 / 24.0)) return QString("11/12");
    if (qFuzzyCompare(d, 23.0 / 24.0)) return QString("23/24");
    if (qFuzzyCompare(d, 24.0 / 24.0)) return QString();

    return QString();
}

QString global::Functions::floatToFractionStringNormalized(double f)
{
    double d = f;
    while (d >  1.0) d -= 1.0;
    while (d < -1.0) d += 1.0;

    if (qFuzzyIsNull(d))        return QString();
    if (qFuzzyCompare(d,  1.0)) return QString();
    if (qFuzzyCompare(d, -1.0)) return QString();

    if (qFuzzyCompare(d,  1.0 / 24.0)) return QString("1/24");
    if (qFuzzyCompare(d,  2.0 / 24.0)) return QString("1/12");
    if (qFuzzyCompare(d,  3.0 / 24.0)) return QString("1/8");
    if (qFuzzyCompare(d,  4.0 / 24.0)) return QString("1/6");
    if (qFuzzyCompare(d,  5.0 / 24.0)) return QString("5/24");
    if (qFuzzyCompare(d,  6.0 / 24.0)) return QString("1/4");
    if (qFuzzyCompare(d,  7.0 / 24.0)) return QString("7/24");
    if (qFuzzyCompare(d,  8.0 / 24.0)) return QString("1/3");
    if (qFuzzyCompare(d,  9.0 / 24.0)) return QString("3/8");
    if (qFuzzyCompare(d, 10.0 / 24.0)) return QString("5/12");
    if (qFuzzyCompare(d, 11.0 / 24.0)) return QString("11/24");
    if (qFuzzyCompare(d, 12.0 / 24.0)) return QString("1/2");
    if (qFuzzyCompare(d, 13.0 / 24.0)) return QString("13/24");
    if (qFuzzyCompare(d, 14.0 / 24.0)) return QString("7/12");
    if (qFuzzyCompare(d, 15.0 / 24.0)) return QString("5/8");
    if (qFuzzyCompare(d, 16.0 / 24.0)) return QString("2/3");
    if (qFuzzyCompare(d, 17.0 / 24.0)) return QString("17/24");
    if (qFuzzyCompare(d, 18.0 / 24.0)) return QString("3/4");
    if (qFuzzyCompare(d, 19.0 / 24.0)) return QString("19/24");
    if (qFuzzyCompare(d, 20.0 / 24.0)) return QString("5/6");
    if (qFuzzyCompare(d, 21.0 / 24.0)) return QString("7/8");
    if (qFuzzyCompare(d, 22.0 / 24.0)) return QString("11/12");
    if (qFuzzyCompare(d, 23.0 / 24.0)) return QString("23/24");

    if (qFuzzyCompare(d,  -1.0 / 24.0)) return QString("-1/24");
    if (qFuzzyCompare(d,  -2.0 / 24.0)) return QString("-1/12");
    if (qFuzzyCompare(d,  -3.0 / 24.0)) return QString("-1/8");
    if (qFuzzyCompare(d,  -4.0 / 24.0)) return QString("-1/6");
    if (qFuzzyCompare(d,  -5.0 / 24.0)) return QString("-5/24");
    if (qFuzzyCompare(d,  -6.0 / 24.0)) return QString("-1/4");
    if (qFuzzyCompare(d,  -7.0 / 24.0)) return QString("-7/24");
    if (qFuzzyCompare(d,  -8.0 / 24.0)) return QString("-1/3");
    if (qFuzzyCompare(d,  -9.0 / 24.0)) return QString("-3/8");
    if (qFuzzyCompare(d, -10.0 / 24.0)) return QString("-5/12");
    if (qFuzzyCompare(d, -11.0 / 24.0)) return QString("-11/24");
    if (qFuzzyCompare(d, -12.0 / 24.0)) return QString("-1/2");
    if (qFuzzyCompare(d, -13.0 / 24.0)) return QString("-13/24");
    if (qFuzzyCompare(d, -14.0 / 24.0)) return QString("-7/12");
    if (qFuzzyCompare(d, -15.0 / 24.0)) return QString("-5/8");
    if (qFuzzyCompare(d, -16.0 / 24.0)) return QString("-2/3");
    if (qFuzzyCompare(d, -17.0 / 24.0)) return QString("-17/24");
    if (qFuzzyCompare(d, -18.0 / 24.0)) return QString("-3/4");
    if (qFuzzyCompare(d, -19.0 / 24.0)) return QString("-19/24");
    if (qFuzzyCompare(d, -20.0 / 24.0)) return QString("-5/6");
    if (qFuzzyCompare(d, -21.0 / 24.0)) return QString("-7/8");
    if (qFuzzyCompare(d, -22.0 / 24.0)) return QString("-11/12");
    if (qFuzzyCompare(d, -23.0 / 24.0)) return QString("-23/24");

    return QString();
}


/*
 * Calculates the scattering factor for element el.
 * twoTheta: position of hkl line in degrees
 * lambda: wavelength in Angstrom
 * b1: Debye-Waller factor
 */
double global::Functions::getScatteringFactor(const QString &el, double twoTheta, double lambda, double b1)
{
    if (!global::ScatteringFactors.contains(el)) return 0.0;
    return getScatteringFactor(global::ScatteringFactors.value(el), twoTheta, lambda, b1);
}

/*
 * Calculates the scattering factor for scattering factor function fct.
 * twoTheta: position of hkl line in degrees
 * lambda: wavelength in Angstrom
 * b1: Debye-Waller factor
 */
double global::Functions::getScatteringFactor(const ScatteringFactorFunction &fct, double twoTheta, double lambda, double b1)
{
    if (qFuzzyIsNull(lambda)) return 0.0;

    double x = sin(twoTheta * M_PI / 360.0) / lambda;
    double y =  fct.a1 * exp(-fct.b1 * x * x);
           y += fct.a2 * exp(-fct.b2 * x * x);
           y += fct.a3 * exp(-fct.b3 * x * x);
           y += fct.a4 * exp(-fct.b4 * x * x);
           y += fct.c;

    // if b (Debye-Waller factor) != 0, apply the TDS correction
    if (!qFuzzyIsNull(b1)) {
        y *= exp(-(b1 * pow(sin(twoTheta * M_PI / 360.0), 2.0)) / (pow(lambda, 2.0)));
    }

    return y;
}

/*
 * Calculates the Lorentz-Polarization factor for position twoTheta
 * twoTheta in degrees
 * formulae from:
 *  http://pd.chem.ucl.ac.uk/pdnn/diff2/polar.htm
 *  http://pd.chem.ucl.ac.uk/pdnn/diff2/loren.htm
 */
double global::Functions::getLPFactor(double twoTheta)
{
    double rtt = qDegreesToRadians(twoTheta);
    double rt  = qDegreesToRadians(0.5 * twoTheta);

    double ctt = std::cos(rtt);
    double ct  = std::cos(rt);
    double st  = std::sin(rt);

    double p = (1.0 + ctt * ctt) / 2.0; // P = (1 + cos^2(2theta)/2
    double l = 1.0 / (st * st * ct);    // L = 1 / (sin^2(theta) * cos(theta))
    return l * p;

}

void global::Functions::fractionalToCartesian(double a, double b, double c,
                                              double al, double be, double ga,
                                              double fx, double fy, double fz,
                                              double &cx, double &cy, double &cz)
{
    double alpha = qDegreesToRadians(al);
    double beta  = qDegreesToRadians(be);
    double gamma = qDegreesToRadians(ga);

    double fxn = fx;
    double fyn = fy;
    double fzn = fz;

    while (fxn < 0.0) fxn += 1.0;
    while (fyn < 0.0) fyn += 1.0;
    while (fzn < 0.0) fzn += 1.0;

    while (fxn >= 1.0) fxn -= 1.0;
    while (fyn >= 1.0) fyn -= 1.0;
    while (fzn >= 1.0) fzn -= 1.0;

    // Calculate the volume of the unit cell
    double v = a * b * c * std::sqrt(1.0 - std::pow(std::cos(alpha), 2.0) - std::pow(std::cos(beta), 2.0)
                                     - std::pow(std::cos(gamma), 2.0) + 2.0 * std::cos(alpha) * std::cos(beta) * std::cos(gamma));

    // Convert fractional coordinates to Cartesian coordinates
    cx = a * fxn + b * std::cos(gamma) * fyn + c * std::cos(beta) * fzn;
    cy = b * std::sin(gamma) * fyn + c * (std::cos(alpha) - std::cos(beta) * std::cos(gamma)) / std::sin(gamma) * fzn;
    cz = v / (a * b * std::sin(gamma)) * fzn;
}

QList<double> global::Functions::convolute(const QList<double> &profile1, const QList<double> &profile2)
{
    int size1 = profile1.size();
    int size2 = profile2.size();

    QList<double> convY(size1 + size2 - 1, 0.0);

    // Convert QList to raw arrays for AlgLib compatibility
    alglib::real_1d_array arr1;
    alglib::real_1d_array arr2;
    alglib::real_1d_array result;

    arr1.setcontent(size1, profile1.data());
    arr2.setcontent(size2, profile2.data());

    // Perform 1D convolution
    alglib::convr1d(arr1, size1, arr2, size2, result);

    // Convert result back to QList<double>
    for (int i = 0; i < result.length(); ++i) {
        convY[i] = result[i];
    }

    return convY;
}

QList<double> global::Functions::getGaussianH(const QList<double> &x, double a, double p, double h)
{
    double s = h / std::sqrt(2.0 * std::log(2.0));
    return getGaussianS(x, a, p, s);
}

QList<double> global::Functions::getGaussianF(const QList<double> &x, double a, double p, double f)
{
    double s = 0.5 * f / std::sqrt(2.0 * std::log(2.0));
    return getGaussianS(x, a, p, s);
}

QList<double> global::Functions::getGaussianS(const QList<double> &x, double a, double p, double s)
{
    QList<double> y;
    y.resize(x.size());

    for (int i = 0; i < x.size(); ++i) {
        y[i] = ((a / (std::sqrt(2.0*M_PI) * s)) * exp(-std::pow((x.at(i) - p), 2.0) / (2.0 * s * s)));
    }

    return y;
}

QList<double> global::Functions::getLorentzianH(const QList<double> &x, double a, double p, double h)
{
    QList<double> y;
    y.resize(x.size());

    for (int i = 0; i < x.size(); ++i) {
        y[i] = ((a / M_PI) * (h / (std::pow(x.at(i) - p, 2.0) + std::pow(h, 2.0))));

    }

    return y;
}

QList<double> global::Functions::getLorentzianF(const QList<double> &x, double a, double p, double f)
{
    return getLorentzianH(x, a, p, 0.5 * f);
}

QList<double> global::Functions::getPseudoVoigtH(const QList<double> &x, double a, double p, double h, double sh)
{
    if (qFuzzyIsNull(sh))       return getLorentzianH(x, a, p, h);
    if (qFuzzyCompare(sh, 1.0)) return getGaussianH(x, a, p, h);

    QList<double> gy = getGaussianH(x, a, p, h);
    QList<double> ly = getLorentzianH(x, a, p, h);

    QList<double> y;
    y.resize(qMin(gy.size(), ly.size()));

    for (int i = 0; i < qMin(gy.size(), ly.size()); ++i) {
        y[i] = (sh * gy.at(i) + (1.0 - sh) * ly.at(i));
    }

    return y;
}

QList<double> global::Functions::getPseudoVoigtF(const QList<double> &x, double a, double p, double f, double sh)
{
    return getPseudoVoigtH(x, a, p, 0.5 * f, sh);
}

