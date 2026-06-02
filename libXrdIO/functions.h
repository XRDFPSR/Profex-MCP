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
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QKeyCombination>
#endif

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

namespace global {

//! \brief Utility class providing static helper functions for XRD crystallographic computations.
//!
//! The Functions class contains a collection of static methods covering a wide range of
//! tasks commonly needed in X-ray diffraction (XRD) data analysis and Rietveld refinement:
//!   - Conversions between d-spacing, 2-theta, Q, wavelength, and energy.
//!   - Unit cell geometry: d-spacing, volume, fractional-to-Cartesian coordinate transforms.
//!   - Atomic displacement parameter conversions (Biso, Uiso, Beta, anisotropic forms).
//!   - Profile function evaluation (Gaussian, Lorentzian, Pseudo-Voigt).
//!   - R-factor computation (Rwp, Rexp, Rdenom).
//!   - Statistical helpers: fuzzy comparison, fraction arithmetic, linear regression.
//!   - Axis scaling and tick-mark generation for plots.
//!   - Miscellaneous utilities: angular correction, LP factor, grain size, convolution.
class XRDIO_EXPORT Functions
{
public:
    Functions();

    //! \brief Convert a QColor to its dark-mode equivalent.
    //! \param color  The input color.
    //! \return       The dark-mode adjusted color.
    static QColor colorToDarkMode(const QColor &color);

    //! \brief Convert d-spacing to 2-theta angle.
    //! \param d   Lattice d-spacing in angstrom.
    //! \param wl  X-ray wavelength in angstrom.
    //! \return    2-theta angle in degrees.
    static double dToTwoTheta(double d, double wl);

    //! \brief Convert 2-theta angle to d-spacing.
    //! \param t   2-theta angle in degrees.
    //! \param wl  X-ray wavelength in angstrom.
    //! \return    Lattice d-spacing in angstrom.
    static double twoThetaToD(double t, double wl);

    //! \brief Convert d-spacing to Q (momentum transfer).
    //! \param d   Lattice d-spacing in angstrom.
    //! \param wl  X-ray wavelength in angstrom.
    //! \return    Q value in 1/angstrom (2-theta equivalent in degrees).
    static double qToTwoTheta(double d, double wl);

    //! \brief Convert 2-theta angle to Q (momentum transfer).
    //! \param t   2-theta angle in degrees.
    //! \param wl  X-ray wavelength in angstrom.
    //! \return    Q value in 1/angstrom.
    static double twoThetaToQ(double t, double wl);

    //! \brief Compare two floating-point values with enhanced tolerance.
    //!
    //! A fuzzy comparison method that is more tolerant to certain rounding
    //! errors commonly encountered in crystallographic fractional coordinates.
    //! \param a  First value.
    //! \param b  Second value.
    //! \return   \c true if the values are considered equal within tolerance.
    static bool fuzzyCompareFractions(double a, double b);

    //! \brief Check whether a fractional coordinate is a "special" position.
    //!
    //! Returns \c true if the coordinate is a round number such as 0, 1/2, 1/3,
    //! 2/3, 1/4, 3/4, 1/6, 5/6, etc.
    //! \param coord  Fractional coordinate value.
    //! \return       \c true if the coordinate is special.
    static bool isSpecialCoordinate(double coord);

    //! \brief Compute the 2-theta offset due to BGMN specimen displacement.
    //! \param tt    Nominal 2-theta angle in degrees.
    //! \param eps1  BGMN displacement parameter EPS1 (default 0.0).
    //! \param eps2  BGMN displacement parameter EPS2 (default 0.0).
    //! \param eps3  BGMN displacement parameter EPS3 (default 0.0).
    //! \return      Corrected 2-theta offset in degrees.
    static double angularCorrection(double tt, double eps1 = 0.0, double eps2 = 0.0, double eps3 = 0.0);

    //! \brief Shift fractional coordinates into the unit cell and normalise.
    //!
    //! Replaces approximate representations (e.g. 0.3333) with exact fractions
    //! (e.g. 1/3) and wraps coordinates into the [0,1) range.
    //! \param x  Input/output fractional x coordinate.
    //! \param y  Input/output fractional y coordinate.
    //! \param z  Input/output fractional z coordinate.
    static void normalizeCoordinates(double &x, double &y, double &z);

    //! \brief Calculate the d-spacing for a given (hkl) reflection.
    //! \param _a  Unit cell parameter a (in any length unit).
    //! \param _b  Unit cell parameter b.
    //! \param _c  Unit cell parameter c.
    //! \param _al Unit cell angle alpha in degrees.
    //! \param _be Unit cell angle beta in degrees.
    //! \param _ga Unit cell angle gamma in degrees.
    //! \param _h  Miller index h.
    //! \param _k  Miller index k.
    //! \param _l  Miller index l.
    //! \return    d-spacing in the same length unit as the cell parameters.
    static double dSpacing(double _a, double _b, double _c, double _al, double _be, double _ga, int _h, int _k, int _l);

    //! \brief Calculate the volume of a unit cell.
    //! \param _a  Unit cell parameter a.
    //! \param _b  Unit cell parameter b.
    //! \param _c  Unit cell parameter c.
    //! \param _al Unit cell angle alpha in degrees.
    //! \param _be Unit cell angle beta in degrees.
    //! \param _ga Unit cell angle gamma in degrees.
    //! \return    Cell volume in the cube of the input length unit.
    static double cellVolume(double _a, double _b, double _c, double _al, double _be, double _ga);

    //! \brief Convert isotropic U (mean squared displacement, in \u00c5\u00b2) to Biso.
    //! \param uiso  Isotropic U value in \u00c5\u00b2.
    //! \return      Equivalent Biso value in \u00c5\u00b2.
    static double bisoFromUiso(double uiso);

    //! \brief Convert anisotropic Beta tensor to equivalent isotropic Biso.
    //! \param a     Unit cell parameter a.
    //! \param b     Unit cell parameter b.
    //! \param c     Unit cell parameter c.
    //! \param alpha Unit cell angle alpha in degrees.
    //! \param beta  Unit cell angle beta in degrees.
    //! \param gamma Unit cell angle gamma in degrees.
    //! \param b11   Anisotropic Beta tensor component B11.
    //! \param b22   Anisotropic Beta tensor component B22.
    //! \param b33   Anisotropic Beta tensor component B33.
    //! \param b12   Anisotropic Beta tensor component B12.
    //! \param b13   Anisotropic Beta tensor component B13.
    //! \param b23   Anisotropic Beta tensor component B23.
    //! \return      Equivalent isotropic Biso in \u00c5\u00b2.
    static double bisoFromBeta(double a, double b, double c,
                        double alpha, double beta, double gamma,
                        double b11, double b22, double b33,
                        double b12, double b13, double b23);

    //! \brief Convert anisotropic B (Baniso) tensor to equivalent isotropic Biso.
    //! \param alpha Unit cell angle alpha in degrees.
    //! \param beta  Unit cell angle beta in degrees.
    //! \param gamma Unit cell angle gamma in degrees.
    //! \param b11   Anisotropic B tensor component B11.
    //! \param b22   Anisotropic B tensor component B22.
    //! \param b33   Anisotropic B tensor component B33.
    //! \param b12   Anisotropic B tensor component B12.
    //! \param b13   Anisotropic B tensor component B13.
    //! \param b23   Anisotropic B tensor component B23.
    //! \return      Equivalent isotropic Biso in \u00c5\u00b2.
    static double bisoFromBaniso(double alpha, double beta, double gamma,
                          double b11, double b22, double b33,
                          double b12, double b13, double b23);

    //! \brief Convert anisotropic U tensor to equivalent isotropic Biso.
    //! \param alpha Unit cell angle alpha in degrees.
    //! \param beta  Unit cell angle beta in degrees.
    //! \param gamma Unit cell angle gamma in degrees.
    //! \param u11   Anisotropic U tensor component U11 (\u00c5\u00b2).
    //! \param u22   Anisotropic U tensor component U22 (\u00c5\u00b2).
    //! \param u33   Anisotropic U tensor component U33 (\u00c5\u00b2).
    //! \param u12   Anisotropic U tensor component U12 (\u00c5\u00b2).
    //! \param u13   Anisotropic U tensor component U13 (\u00c5\u00b2).
    //! \param u23   Anisotropic U tensor component U23 (\u00c5\u00b2).
    //! \return      Equivalent isotropic Biso in \u00c5\u00b2.
    static double bisoFromUaniso(double alpha, double beta, double gamma,
                          double u11, double u22, double u33,
                          double u12, double u13, double u23);

    //! \brief Convert X-ray wavelength to photon energy.
    //! \param w  Wavelength in angstrom.
    //! \return   Energy in keV.
    static double wavelengthToEnergy(double w);

    //! \brief Convert photon energy to X-ray wavelength.
    //! \param e  Energy in keV.
    //! \return   Wavelength in angstrom.
    static double energyToWavelength(double e);

    //! \brief Retrieve a list of all known characteristic X-ray wavelengths.
    //! \return List of CharWaveLength structures describing anode/edge wavelengths.
    static QList<global::CharWaveLength> getAllWavelengths();

    //! \brief Compute the Lorentz-polarisation (LP) factor.
    //! \param d   Lattice d-spacing in angstrom.
    //! \param wl  X-ray wavelength in angstrom.
    //! \return    LP factor as a dimensionless scaling value.
    static double lpFactor(double d, double wl);

    //! \brief Determine the number of decimal places in a string representation of a float.
    //! \param str  Input string containing a floating-point value.
    //! \return     Number of decimal places, or -1 if parsing fails.
    static int getFloatPrecision(const QString &str);

    //! \brief Generate tick-mark positions for a linear axis (strategy 1).
    //! \param min  Minimum axis value.
    //! \param max  Maximum axis value.
    //! \param n    Desired number of intervals.
    //! \return     List of tick-mark positions.
    static QList<double> scaleAxis1(double min, double max, int n);

    //! \brief Generate tick-mark positions for a linear axis (strategy 2).
    //! \param min  Minimum axis value.
    //! \param max  Maximum axis value.
    //! \param n    Desired number of intervals.
    //! \return     List of tick-mark positions.
    static QList<double> scaleAxis2(double min, double max, int n);

    //! \brief Generate tick-mark positions for a logarithmic axis.
    //! \param min  Minimum axis value.
    //! \param max  Maximum axis value.
    //! \param n    Desired number of intervals.
    //! \return     List of tick-mark positions.
    static QList<double> scaleAxisLog(double min, double max, int n);

    //! \brief Generate tick-mark positions for an axis in d-spacing units.
    //! \param min     Minimum d value.
    //! \param max     Maximum d value.
    //! \param wl      X-ray wavelength in angstrom.
    //! \param nMin    Minimum number of intervals.
    //! \param nMax    Maximum number of intervals.
    //! \return        List of tick-mark positions.
    static QList<double> scaleAxisD(double min, double max, double wl, int nMin, int nMax);

    //! \brief Generate tick-mark positions for an axis in Q (1/\u00c5) units.
    //! \param min   Minimum Q value.
    //! \param max   Maximum Q value.
    //! \param wl    X-ray wavelength in angstrom.
    //! \param n     Desired number of intervals.
    //! \return      List of tick-mark positions.
    static QList<double> scaleAxisQ(double min, double max, double wl, int n);

    //! \brief Compute the weighted-profile R-factor numerator.
    //! \param iobs   Pointer to the observed scan data.
    //! \param icalc  Pointer to the calculated scan data.
    //! \param wi     Vector of weights.
    //! \return       Unadjusted Rwp numerator value.
    static double getRwp(const Scan *iobs, const Scan *icalc, const QVector<double> &wi);

    //! \brief Compute the expected R-factor (Rexp).
    //! \param iobs  Pointer to the observed scan data.
    //! \param p     Number of refined parameters.
    //! \param wi    Vector of weights.
    //! \return      Rexp value.
    static double getRexp(const Scan *iobs, int p, const QVector<double> &wi);

    //! \brief Compute the denominator used in R-factor calculations.
    //! \param iobs  Pointer to the observed scan data.
    //! \param wi    Vector of weights.
    //! \return      Weighted sum of squares of observed intensities.
    static double getRdenom(const Scan *iobs, const QVector<double> &wi);

    //! \brief Compute the unadjusted numerator of Rwp (weighted squared residuals).
    //! \param iobs   Pointer to the observed scan data.
    //! \param icalc  Pointer to the calculated scan data.
    //! \param wi     Vector of weights.
    //! \return       Sum of w_i * (Iobs_i - Icalc_i)\u00b2.
    static double getRwpNumer(const Scan *iobs, const Scan *icalc, const QVector<double> &wi);

    //! \brief Calculate the perpendicular distance from a point to a line segment.
    //! \param pt   Point to measure from.
    //! \param la   First endpoint of the line segment.
    //! \param lb   Second endpoint of the line segment.
    //! \param mode Calculation mode (0 = perpendicular distance, other values may vary).
    //! \return     Distance in the coordinate units of the input points.
    static double distanceFromLine(const QPointF &pt, const QPointF &la, const QPointF &lb, int mode);

    //! \brief Determine the number of significant digits needed to distinguish two values.
    //! \param valA  First value.
    //! \param valB  Second value.
    //! \param min   Minimum number of digits to return.
    //! \return      Recommended number of decimal digits.
    static int digitsForValues(double valA, double valB, int min);

    //! \brief Compute the BGMN grain size from isotropic broadening parameter B1.
    //! \param b1  BGMN isotropic broadening parameter B1.
    //! \param k1  Scherrer constant (default 0.0; if 0, a default value is used internally).
    //! \return    Estimated grain size in the same length unit as the cell.
    static double bgmnGrainSize(double b1, double k1 = 0.0);

    //! \brief Perform a linear regression (y = m*x + b) on two data vectors.
    //! \param x  Vector of x-coordinates.
    //! \param y  Vector of y-coordinates.
    //! \param m  Output: slope of the fitted line.
    //! \param b  Output: intercept of the fitted line.
    //! \return   \c true if the regression succeeded, \c false otherwise.
    static bool linearRegression(const QVector<double> &x, const QVector<double> &y, double &m, double &b);

    //! \brief Add two fraction strings and return the result as numerator/denominator integers.
    //! \param frac1  First fraction string (e.g. "1/2").
    //! \param frac2  Second fraction string (e.g. "1/3").
    //! \param num    Output: numerator of the sum.
    //! \param denom  Output: denominator of the sum.
    //! \return       \c true if parsing and summation succeeded.
    static bool sumFractions(const QString &frac1, const QString &frac2, int &num, int &denom);

    //! \brief Reduce a fraction to its lowest terms.
    //! \param num    Numerator.
    //! \param denom  Denominator.
    //! \return       Reduced fraction as a string (e.g. "2/3").
    static QString minimizeFraction(int num, int denom);

    //! \brief Convert a floating-point value to its normalised fraction string representation.
    //! \param val  Input floating-point value.
    //! \return     Fraction string (e.g. "1/3", "1/2") or the value as-is if no
    //!             suitable fraction is found.
    static QString floatToFractionStringNormalized(double val);

    //! \brief Compute the X-ray scattering factor for a given element.
    //! \param el         Element symbol (e.g. "Si", "Fe").
    //! \param twoTheta   2-theta angle in degrees.
    //! \param lambda     X-ray wavelength in angstrom.
    //! \param b1         Isotropic Biso displacement parameter (\u00c5\u00b2; default 0.0).
    //! \return           Scattering factor in electrons.
    static double getScatteringFactor(const QString &el, double twoTheta, double lambda, double b1 = 0.0);

    //! \brief Compute the X-ray scattering factor from an explicit parameterisation function.
    //! \param fct        Scattering factor function (analytical coefficients).
    //! \param twoTheta   2-theta angle in degrees.
    //! \param lambda     X-ray wavelength in angstrom.
    //! \param b1         Isotropic Biso displacement parameter (\u00c5\u00b2; default 0.0).
    //! \return           Scattering factor in electrons.
    static double getScatteringFactor(const global::ScatteringFactorFunction &fct, double twoTheta, double lambda, double b1 = 0.0);

    //! \brief Compute the Lorentz-polarisation factor for a given 2-theta angle.
    //! \param twoTheta  2-theta angle in degrees.
    //! \return          LP factor value.
    static double getLPFactor(double twoTheta);

    //! \brief Evaluate a Gaussian peak (parameterised by HWHM).
    //! \param x  Vector of x-values at which to evaluate.
    //! \param a  Peak area (integral intensity).
    //! \param p  Centre position in x-units.
    //! \param h  Half-width at half-maximum (HWHM) in x-units.
    //! \return   Vector of evaluated Gaussian intensities.
    static QList<double> getGaussianH(const QList<double> &x, double a, double p, double h);

    //! \brief Evaluate a Gaussian peak (parameterised by FWHM).
    //! \param x  Vector of x-values at which to evaluate.
    //! \param a  Peak area (integral intensity).
    //! \param p  Centre position in x-units.
    //! \param f  Full-width at half-maximum (FWHM) in x-units.
    //! \return   Vector of evaluated Gaussian intensities.
    static QList<double> getGaussianF(const QList<double> &x, double a, double p, double f);

    //! \brief Evaluate a Gaussian peak (parameterised by sigma).
    //! \param x  Vector of x-values at which to evaluate.
    //! \param a  Peak area (integral intensity).
    //! \param p  Centre position in x-units.
    //! \param s  Standard deviation sigma in x-units.
    //! \return   Vector of evaluated Gaussian intensities.
    static QList<double> getGaussianS(const QList<double> &x, double a, double p, double s);

    //! \brief Evaluate a Lorentzian peak (parameterised by HWHM).
    //! \param x  Vector of x-values at which to evaluate.
    //! \param a  Peak area (integral intensity).
    //! \param p  Centre position in x-units.
    //! \param h  Half-width at half-maximum (HWHM) in x-units.
    //! \return   Vector of evaluated Lorentzian intensities.
    static QList<double> getLorentzianH(const QList<double> &x, double a, double p, double h);

    //! \brief Evaluate a Lorentzian peak (parameterised by FWHM).
    //! \param x  Vector of x-values at which to evaluate.
    //! \param a  Peak area (integral intensity).
    //! \param p  Centre position in x-units.
    //! \param f  Full-width at half-maximum (FWHM) in x-units.
    //! \return   Vector of evaluated Lorentzian intensities.
    static QList<double> getLorentzianF(const QList<double> &x, double a, double p, double f);

    //! \brief Evaluate a Pseudo-Voigt peak (parameterised by HWHM).
    //! \param x   Vector of x-values at which to evaluate.
    //! \param a   Peak area (integral intensity).
    //! \param p   Centre position in x-units.
    //! \param h   Half-width at half-maximum (HWHM) in x-units (common to both Gauss and Lorentz).
    //! \param sh  Shape parameter (0 = pure Gaussian, 1 = pure Lorentzian).
    //! \return    Vector of evaluated Pseudo-Voigt intensities.
    static QList<double> getPseudoVoigtH(const QList<double> &x, double a, double p, double h, double sh);

    //! \brief Evaluate a Pseudo-Voigt peak (parameterised by FWHM).
    //! \param x   Vector of x-values at which to evaluate.
    //! \param a   Peak area (integral intensity).
    //! \param p   Centre position in x-units.
    //! \param f   Full-width at half-maximum (FWHM) in x-units (common to both Gauss and Lorentz).
    //! \param sh  Shape parameter (0 = pure Gaussian, 1 = pure Lorentzian).
    //! \return    Vector of evaluated Pseudo-Voigt intensities.
    static QList<double> getPseudoVoigtF(const QList<double> &x, double a, double p, double f, double sh);

    //! \brief Convert atomic coordinates from fractional to Cartesian.
    //! \param fx   Fractional x coordinate.
    //! \param fy   Fractional y coordinate.
    //! \param fz   Fractional z coordinate.
    //! \param uca  Unit cell parameter a.
    //! \param ucb  Unit cell parameter b.
    //! \param ucc  Unit cell parameter c.
    //! \param ucal Unit cell angle alpha in degrees.
    //! \param ucbe Unit cell angle beta in degrees.
    //! \param ucga Unit cell angle gamma in degrees.
    //! \param cx   Output: Cartesian x coordinate.
    //! \param cy   Output: Cartesian y coordinate.
    //! \param cz   Output: Cartesian z coordinate.
    static void fractionalToCartesian(double fx, double fy, double fz,
                                      double uca, double ucb, double ucc,
                                      double ucal, double ucbe, double ucga,
                                      double &cx, double &cy, double &cz);

    //! \brief Perform one-dimensional discrete convolution of two data vectors.
    //! \param a  First input vector (e.g. underlying pattern).
    //! \param b  Second input vector (e.g. kernel / instrument profile).
    //! \return   Convolution result (same size as the larger input).
    static QList<double> convolute(const QList<double> &a, const QList<double> &b);

    inline static QKeySequence keyCopy()
    {
        return QKeySequence::Copy;
    }

    inline static QKeySequence keyShiftCopy()
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QKeySequence base(QKeySequence::Copy);
        if (base.isEmpty())
            return base;

        QKeyCombination kc = base[0];  // first key of the standard "Copy" shortcut
        auto mods = kc.keyboardModifiers() | Qt::ShiftModifier;
        QKeyCombination withShift(mods, kc.key());

        return QKeySequence(withShift);
#else
        return QKeySequence::Copy;
#endif
    }

    inline static QKeySequence keyAltCopy()
    {
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
        QKeySequence base(QKeySequence::Copy);
        if (base.isEmpty())
            return base;

        QKeyCombination kc = base[0];
        auto mods = kc.keyboardModifiers() | Qt::AltModifier;
        QKeyCombination withAlt(mods, kc.key());

        return QKeySequence(withAlt);
#else
        return QKeySequence::Copy;
#endif
    }
};

} // end of namespace global

#endif // FUNCTIONS_H
