/***************************************************************************
                          functions.h  -  description
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

#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "structs.h"
#include "scan.h"
#include <QObject>
#include <QColor>
#include <QRegularExpression>
#include <QPoint>
#include <QPointF>
#include <QKeySequence>
#include <QKeyCombination>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

namespace global {

class XRDIO_EXPORT Functions
{
public:
    Functions();

    static QColor colorToDarkMode(const QColor &);

    /*
     * expects d and wl in angstrom
     * returns t in degrees
     */
    static double dToTwoTheta(double d, double wl);

    /*
     * expects wl in angstrom and t in degrees
     * returns d in angstrom
     */
    static double twoThetaToD(double t, double wl);

    /*
     * expects d and wl in angstrom
     * returns t in degrees
     */
    static double qToTwoTheta(double d, double wl);

    /*
     * expects wl in angstrom and t in degrees
     * returns Q in 1/angstrom
     */
    static double twoThetaToQ(double t, double wl);

    /*
     * a fuzzy compare method that is more tolerant to certain rounding errors
     */
    static bool fuzzyCompareFractions(double, double);

    /*
     * returns true if the coordinate is special, i.e. a round number such
     * as 0, 1/2, 5/6 etc.
     */
    static bool isSpecialCoordinate(double);

    /*
     * returns the 2theta offset due to BGMN displacement parameters EPS1, EPS2, EPS3
     */
    static double angularCorrection(double tt, double eps1 = 0.0, double eps2 = 0.0, double eps3 = 0.0);

    /*
     * shifts fractional coordinates into the unit cell and replaces 0.3333 with 1/3 etc
     */
    static void normalizeCoordinates(double &x, double &y, double &z);

    /*
     * calculates the d spacing in the unit of the provided cell constants
     */
    static double dSpacing(double _a, double _b, double _c, double _al, double _be, double _ga, int _h, int _k, int _l);

    /*
     * calculates the volume of the unit cell in the dimensions of the provided cell contstants
     */
    static double cellVolume(double _a, double _b, double _c, double _al, double _be, double _ga);

    static double bisoFromUiso(double uiso);
    static double bisoFromBeta(double a, double b, double c,
                        double alpha, double beta, double gamma,
                        double b11, double b22, double b33,
                        double b12, double b13, double b23);
    static double bisoFromBaniso(double alpha, double beta, double gamma,
                          double b11, double b22, double b33,
                          double b12, double b13, double b23);
    static double bisoFromUaniso(double alpha, double beta, double gamma,
                          double u11, double u22, double u33,
                          double u12, double u13, double u23);

    static double wavelengthToEnergy(double w);
    static double energyToWavelength(double e);

    static QList<global::CharWaveLength> getAllWavelengths();

    static double lpFactor(double d, double wl);

    /*
     * returns the number of decimals in a string containing a float value.
     * if parsing of the string fails, returns -1
     */
    static int getFloatPrecision(const QString &);

    /*
     * return tick mark positions
     */
    static QList<double> scaleAxis1(double, double, int);
    static QList<double> scaleAxis2(double, double, int);
    static QList<double> scaleAxisLog(double, double, int);
    static QList<double> scaleAxisD(double, double, double, int, int);
    static QList<double> scaleAxisQ(double, double, double, int);

    static double getRwp(const Scan *iobs, const Scan *icalc, const QVector<double> &wi);
    static double getRexp(const Scan *iobs, int p, const QVector<double> &wi);
    static double getRdenom(const Scan *iobs, const QVector<double> &wi);
    static double getRwpNumer(const Scan *iobs, const Scan *icalc, const QVector<double> &wi);

    static double distanceFromLine(const QPointF &pt, const QPointF &la, const QPointF &lb, int mode);
    static int digitsForValues(double valA, double valB, int min);

    static double bgmnGrainSize(double b1, double k1 = 0.0);

    static bool linearRegression(const QVector<double> &, const QVector<double> &, double &, double &);

    static bool sumFractions(const QString &, const QString &, int &, int &);
    static QString minimizeFraction(int num, int denom);
    static QString floatToFractionStringNormalized(double);

    static double getScatteringFactor(const QString &el, double twoTheta, double lambda, double b1 = 0.0);
    static double getScatteringFactor(const global::ScatteringFactorFunction &fct, double twoTheta, double lambda, double b1 = 0.0);
    static double getLPFactor(double twoTheta);

    /*
     * x = x values
     * a = area
     * p = center position
     * h = hwhm
     * f = fwhm
     * s = sigma
     * sh = shape (0 = Gauss, 1 = Lorentz)
     */
    static QList<double> getGaussianH(const QList<double> &x, double a, double p, double h);
    static QList<double> getGaussianF(const QList<double> &x, double a, double p, double f);
    static QList<double> getGaussianS(const QList<double> &x, double a, double p, double s);
    static QList<double> getLorentzianH(const QList<double> &x, double a, double p, double h);
    static QList<double> getLorentzianF(const QList<double> &x, double a, double p, double f);
    static QList<double> getPseudoVoigtH(const QList<double> &x, double a, double p, double h, double sh);
    static QList<double> getPseudoVoigtF(const QList<double> &x, double a, double p, double f, double sh);

    /*
     * converts atomic coordinates from fractional to cartesian
     */
    static void fractionalToCartesian(double fx, double fy, double fz,
                                      double uca, double ucb, double ucc,
                                      double ucal, double ucbe, double ucga,
                                      double &cx, double &cy, double &cz);

    static QList<double> convolute(const QList<double> &, const QList<double> &);

    inline static QKeySequence keyCopy()
    {
        return QKeySequence::Copy;
    }

    inline static QKeySequence keyShiftCopy()
    {
        QKeySequence base(QKeySequence::Copy);
        if (base.isEmpty())
            return base;

        QKeyCombination kc = base[0];  // first key of the standard "Copy" shortcut
        auto mods = kc.keyboardModifiers() | Qt::ShiftModifier;
        QKeyCombination withShift(mods, kc.key());

        return QKeySequence(withShift);
    }

    inline static QKeySequence keyAltCopy()
    {
        QKeySequence base(QKeySequence::Copy);
        if (base.isEmpty())
            return base;

        QKeyCombination kc = base[0];
        auto mods = kc.keyboardModifiers() | Qt::AltModifier;
        QKeyCombination withAlt(mods, kc.key());

        return QKeySequence(withAlt);
    }
};

} // end of namespace global

#endif // FUNCTIONS_H
