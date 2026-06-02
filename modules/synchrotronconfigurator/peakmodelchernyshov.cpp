/***************************************************************************
                          synchrotronfwhmcalculator.cpp  -  description
                             -------------------
    begin                : Wed Dec 12 18:00:00 CEST 2024
    copyright            : (C) 2024 by Nicola Doebelin
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

#include "peakmodelchernyshov.h"
#include "../libXrdIO/functions.h"

PeakModelChernyshov::PeakModelChernyshov()
{
    parameterStorage = nullptr;
}

PeakModelChernyshov::~PeakModelChernyshov()
{}

void PeakModelChernyshov::setParameters(ParameterStorage *p)
{
    parameterStorage = p;
}

Scan PeakModelChernyshov::getCalculatedProfile()
{
    double profileRange = parameterStorage->getParameter(synchro::RANGE_PROFILE_WFACTOR, -1.0);
    int nSteps = static_cast<int>(profileRange * 50.0); // default: profileRange * 50.0
    QList<double> origX, origY, plotX, plotY;

    if (_convX.size() > _pvX.size()) {
        origX = _convX.mid(_convX.size() / 4, _convX.size() / 2);
        origY = _convY.mid(_convY.size() / 4, _convY.size() / 2);
    } else if (_convX.size() == _pvX.size()) {
        origX = _convX;
        origY = _convY;
    } else {
        origX = _pvX;
        origY = _pvY;
    }

    profileToPlottable(origX, origY, plotX, plotY, nSteps);

    Scan profile;
    profile.pDataAngle()     = plotX;
    profile.pDataIntensity() = plotY;
    return profile;
}

/*
 * Reduces the resolution of the model profile. It has to be computed at high resolution due to
 * the convolution with the exponential decay function. But the resolution of the convolved curve can be reduced
 * to speed up the curve fitting later.
 */
void PeakModelChernyshov::profileToPlottable(const QList<double> &x, const QList<double> &y, QList<double> &px, QList<double> &py, int n)
{
    int sz = qMin(x.size(), y.size());
    int stp = qMax(1, sz / n);

    if (stp < 2) {
        px = x;
        py = y;
        return;
    }

    for (int i = 0; i < sz; i += stp) {
        px.append(x.at(i));
        py.append(y.at(i));
    }
}

int PeakModelChernyshov::getRawProfilePv(QList<double> &pvX, QList<double> &pvY) const
{
    pvX = _pvX;
    pvY = _pvY;

    return qMax(_pvX.size(), _pvY.size());
}

int PeakModelChernyshov::getRawProfileExp(QList<double> &expX, QList<double> &expY) const
{
    expX = _expX;
    expY = _expY;

    return qMax(_expX.size(), _expY.size());
}

int PeakModelChernyshov::getRawProfileConv(QList<double> &convX, QList<double> &convY) const
{
    convX = _convX;
    convY = _convY;

    return qMax(_convX.size(), _convY.size());
}

void PeakModelChernyshov::setRawProfiles(const QList<double> &pvX, const QList<double> &pvY,
                                         const QList<double> &expX, const QList<double> &expY,
                                         const QList<double> &convX, const QList<double> &convY)
{
    _pvX = pvX;
    _pvY = pvY;
    _expX = expX;
    _expY = expY;
    _convX = convX;
    _convY = convY;
}

bool PeakModelChernyshov::generatePeak()
{
    int paramsMissing = 0;

    if (!parameterStorage->contains(synchro::TWOTHETA_DEG))   paramsMissing += 1;
    if (!parameterStorage->contains(synchro::CHERNYSHOV_D))   paramsMissing += 10;
    if (!parameterStorage->contains(synchro::CHERNYSHOV_P))   paramsMissing += 100;
    if (!parameterStorage->contains(synchro::CHERNYSHOV_T))   paramsMissing += 1000;
    if (!parameterStorage->contains(synchro::CHERNYSHOV_C))   paramsMissing += 10000;
    if (!parameterStorage->contains(synchro::CHERNYSHOV_PHI)) paramsMissing += 100000;

    if (!parameterStorage->contains(synchro::CHERNYSHOV_SHAPE)) paramsMissing += 2;

    if (!parameterStorage->contains(synchro::RANGE_PROFILE_WFACTOR)) paramsMissing += 20;
    if (!parameterStorage->contains(synchro::DETECTOR_LAC_MM))       paramsMissing += 200;

    if (paramsMissing > 0) {
        qDebug() << QString("PeakModelChernyshov::generatePeak(): "
                            "Parameters are missing. Use ::setPeakParameters() first. "
                            "Error code = %1.").arg(paramsMissing);
        return false;
    }

    parameterStorage->setValue(synchro::PROFILE_AREA, -1.0);
    bool applyPosCor = parameterStorage->getParameter(synchro::POSITION_CORRECTION_MODE, 1) > 0;
    if (parameterStorage->getParameter(synchro::PROFILE_FWHM_RAD, -1.0) < 0.0) getFwhm(!applyPosCor);

    double profileRange = parameterStorage->getParameter(synchro::RANGE_PROFILE_WFACTOR, -1.0);
    double fwhmRad      = parameterStorage->getParameter(synchro::PROFILE_FWHM_RAD, -1.0);
    double fwhmDeg      = qRadiansToDegrees(fwhmRad);
    double rStart       = qMin(-profileRange * fwhmDeg, profileRange * fwhmDeg);
    double rEnd         = qMax(-profileRange * fwhmDeg, profileRange * fwhmDeg);

    // high resolution stepsize, because the same value must be used for the exponential decay function
    parameterStorage->setValue(synchro::RANGE_PROFILE_START, rStart);
    parameterStorage->setValue(synchro::RANGE_PROFILE_END,   rEnd);
    parameterStorage->setValue(synchro::RANGE_PROFILE_STEP,  fwhmDeg / 100.0); // default: fwhmDeg / 100.0

    if (!computeProfile()) {
        qDebug() << QString("PeakModelChernyshov::generatePeak(): Profile calculation failed.");
        return false;
    }

    return true;
}

/* calculation of FWHM based on Chernyshov et al. (2021)
 * Eqs 11 and 12:
 * https://doi.org/10.1107/S2053273321007506
 */
bool PeakModelChernyshov::getFwhm(bool withT)
{
    bool paramsOk = true;

    if (!parameterStorage->contains(synchro::TWOTHETA_DEG))           paramsOk = false;
    if (!parameterStorage->contains(synchro::CHERNYSHOV_D))           paramsOk = false;
    if (!parameterStorage->contains(synchro::CHERNYSHOV_P))           paramsOk = false;
    if (!parameterStorage->contains(synchro::CHERNYSHOV_T))           paramsOk = false;
    if (!parameterStorage->contains(synchro::CHERNYSHOV_C))           paramsOk = false;
    if (!parameterStorage->contains(synchro::CHERNYSHOV_PHI))         paramsOk = false;
    if (!parameterStorage->contains(synchro::CHERNYSHOV_PHI_FOCUSED)) paramsOk = false;

    if (!paramsOk) {
        qDebug() << QString("PeakModelChernyshov::getFwhm(): Parameters are missing. Use ::setPeakParameters() first. Exiting.");
        return false;
    }

    double ttDeg = parameterStorage->getParameter(synchro::TWOTHETA_DEG, -1.0);
    double D     = parameterStorage->getParameter(synchro::CHERNYSHOV_D, -1.0);
    double p     = parameterStorage->getParameter(synchro::CHERNYSHOV_P, -1.0);
    double t = withT ? parameterStorage->getParameter(synchro::CHERNYSHOV_T, -1.0) : 0.0;
    double c     = parameterStorage->getParameter(synchro::CHERNYSHOV_C, -1.0);
    double phi   = parameterStorage->getParameter(synchro::CHERNYSHOV_PHI, -1.0) * M_PI / 180.0;
    bool phifoc  = parameterStorage->getParameter(synchro::CHERNYSHOV_PHI_FOCUSED, false);
    double alpha = parameterStorage->getParameter(synchro::CHERNYSHOV_ALPHA, 0.0);

    if (!qFuzzyIsNull(alpha)) ttDeg -= alpha;

    double ln2   = 2.0 * std::log(2.0);
    double ttrad = ttDeg * M_PI / 180.0;

    if (phifoc) {
        c *= std::sin(ttrad);
        phi = 0.5 * std::atan(c / D);
    }

    double A = ln2 * (p*p - 2.0 * t*t - c*c) / (D*D);
    double B = ln2 * 2.0 * (t*t + c*c) / (D*D);
    double C = ln2 * (phifoc ? std::pow(phi * (1.0 - std::cos(ttrad)), 2.0) : pow(phi, 2.0));

    double Hsqr    = A * std::pow(std::cos(ttrad), 4.0) + B * std::pow(std::cos(ttrad), 2.0) + C;
    double fwhmRad = std::sqrt(Hsqr);
    double fwhmDeg = qRadiansToDegrees(fwhmRad);

    parameterStorage->setValue(synchro::PROFILE_FWHM_RAD, fwhmRad);
    parameterStorage->setValue(synchro::PROFILE_FWHM_DEG, fwhmDeg);

    return true;
}

bool PeakModelChernyshov::computeProfile()
{
    double h     = parameterStorage->getParameter(synchro::PROFILE_FWHM_DEG, -1.0) * 0.5;
    double s     = parameterStorage->getParameter(synchro::CHERNYSHOV_SHAPE, -1.0);
    double t     = parameterStorage->getParameter(synchro::CHERNYSHOV_T, -1.0);
    double D     = parameterStorage->getParameter(synchro::CHERNYSHOV_D, -1.0);
    double mu    = parameterStorage->getParameter(synchro::DETECTOR_LAC_MM, 0.0);
    double ttdeg = parameterStorage->getParameter(synchro::TWOTHETA_DEG, -1.0);
    bool cutoff  = parameterStorage->getParameter(synchro::DETECTOR_TRANSPARENCY_CUTOFF, false);
    double alpha = parameterStorage->getParameter(synchro::CHERNYSHOV_ALPHA, 0.0);

    int posMode = parameterStorage->getParameter(synchro::POSITION_CORRECTION_MODE, 1);

    bool muOk = (mu > 1e-6);
    bool ttOk = (ttdeg > 1e-6);

    if (!qFuzzyIsNull(alpha)) ttdeg -= alpha;

    // calculate the x-grid for all modes
    createXvalues();

    if (posMode == 0) {
        // no position correction
        pseudoVoigtY(1.0, 0.0, h, s);
    } else if (posMode == 1) { // exGaussian
        _convX = _pvX;
        exgaussY(ttdeg, D, mu, h);
    } else if (posMode == 2) { // convolution PV * ExpDecay
        pseudoVoigtY(1.0, 0.0, h, s);

        if (ttOk) {
            // apply convolution for 2theta > 0
            exponentialX(ttdeg, D);
            exponentialY(mu, t, ttdeg, cutoff);
            convolvedX();
            convolvedY(mu, t, ttdeg);
        } else {
            // no convolution for 2theta = 0
            _convX = _pvX;
            _convY = _pvY;
        }
    } else {
        // no position correction
        pseudoVoigtY(1.0, 0.0, h, s);
        qDebug() << QString("PeakModelChernyshov::computeProfile(): Unsupported position correction mode: %1. Skipping position correction.").arg(posMode);
    }

    if (!muOk) {
        qDebug() << QString("PeakModelChernyshov::computeProfile(): Intensity correction for detector transparency skipped due to missing LAC (mu = %1)").arg(mu);
    }

    if (muOk) detectorTransparency(posMode == 0 ? _pvY : _convY, mu, t, ttdeg);
    double area = calculateProfileArea(posMode == 0 ? _pvX : _convX, posMode == 0 ? _pvY : _convY);
    parameterStorage->setValue(synchro::PROFILE_AREA, area);

    return true;
}

void PeakModelChernyshov::createXvalues()
{
    // these parameters have previously been set as multiples of fwhm. The fallback values
    // should never be necessary, but the function getParameter requires them anyway.
    double start = parameterStorage->getParameter(synchro::RANGE_PROFILE_START, -0.1);
    double end   = parameterStorage->getParameter(synchro::RANGE_PROFILE_END, 0.1);
    double step  = parameterStorage->getParameter(synchro::RANGE_PROFILE_STEP, 0.001);
    int n = int(0.5 + (end - start) / step);

    _pvX.clear();
    _pvX.resize(n);

    for (int i = 0; i < n; ++i) {
        _pvX[i] = start + (double(i)/double(n)) * (end - start);
    }
}

/*
 * a = total area
 * p = center position
 * h = hwhm in deg
 * s = shape
 */
void PeakModelChernyshov::pseudoVoigtY(double a, double p, double h, double s)
{
    _pvY = global::Functions::getPseudoVoigtH(_pvX, a, p, h, s);
}

/*
 * converts the values of _pvX (degrees) to mm at the 2theta position tt (degrees)
 * and the detector distance d
 */
void PeakModelChernyshov::exponentialX(double tt, double D)
{
    _expX.clear();
    _expX.resize(_pvX.size());

    double R0 = D * std::tan(tt * M_PI / 180.0);

    for (int i = 0; i < _pvX.size(); ++i) {
        double R = D * std::tan((_pvX.at(i) + tt) * M_PI / 180.0);
        _expX[i] = R - R0;
    }
}

/*
 * exponential function according to chernyshov 2021 eq 20.
 * ttheta in degrees
 */
void PeakModelChernyshov::exponentialY(double mu, double t, double ttheta, bool applyCutoff)
{
    _expY.resize(_expX.size());

    double sinTheta = std::sin(ttheta * M_PI / 180.0);
    double cutoff = applyCutoff ? t * std::tan(ttheta * M_PI / 180.0) : std::numeric_limits<double>::max();
    double sum = 0.0;

    for (int i = 0; i < _expX.size(); ++i) {
        if (_expX.at(i) >= 0.0 && _expX.at(i) <= cutoff) {
            double y = std::exp(-mu * _expX.at(i) / sinTheta);
            _expY[i] = y;
            sum += y;
        } else {
            _expY[i] = 0.0;
        }
    }

    // Normalize correctly to ensure total absorbed intensity scales with 2θ
    for (double &yi : _expY) {
        yi /= sum;
    }
}

void PeakModelChernyshov::convolvedX()
{
    if (_pvX.size() < 2) return;

    double deltaX = _pvX.at(1) - _pvX.at(0);
    double startX = 2.0 * _pvX.constFirst();

    int sizeConvolved = 2 * _pvX.size() - 1;
    _convX.resize(sizeConvolved);

    for (int i = 0; i < sizeConvolved; ++i) {
        _convX[i] = startX + i * deltaX;
    }
}

void PeakModelChernyshov::convolvedY(double mu, double t, double ttheta)
{
    // Compute standard convolution
    _convY = global::Functions::convolute(_pvY, _expY);

    // Apply absorption correction only if near 2θ = 0°
    if (qFuzzyIsNull(ttheta)) {
        double absorptionFactor = 1.0 - std::exp(-mu * t); // Fraction absorbed

        // Scale the convolved intensity
        for (double &y : _convY) {
            y *= absorptionFactor;
        }
    }
}

/*
 * tt = 2theta in degrees
 * D = detector distance in mm
 * mu = linear absorption coefficient in mm-1
 * h = hwhm in degrees
 */
void PeakModelChernyshov::exgaussY(double tt, double D, double mu, double h)
{
    const double ln2 = std::log(2.0);
    double sigma = h / (std::sqrt(2.0 * ln2));
    double tau = (M_PI * D * mu) / 180.0;
    double lambda = tt < 0.001 ? 0.0 : tau / std::sin(tt *M_PI / 180.0);
    double p0 = qFuzzyIsNull(lambda) ? 0.0 : 1.0 / lambda;

    if (p0 < 0.0001 * h) {
        // fall back to pure gaussian for stable convergence towards 2theta = 0
        _convY = global::Functions::getGaussianS(_convX, 1.0, p0, sigma);
    } else {
        // compute exGaussian
        _convY.resize(_convX.size());

        for (int i = 0; i < _convX.size(); ++i) {
            _convY[i] = exGaussian_log(_convX.at(i), lambda, sigma);
        }
    }
}

void PeakModelChernyshov::detectorTransparency(QList<double> &y, double mu, double t, double ttheta)
{
    if (qFuzzyIsNull(t)) return;
    double xcutoff = t / std::cos(ttheta * M_PI / 180.0);
    double a_abs = 1.0 - std::exp(-mu * xcutoff);

    for (double &yi : y) {
        yi *= a_abs;
    }
}

/*
 * Simpson's rule
 */
double PeakModelChernyshov::calculateProfileArea(const QList<double> &x, const QList<double> &y)
{
    if (x.size() == 0) return 0.0;
    int n = x.size();

    double area = 0.0;
    double h = (x.last() - x.first()) / (n - 1);

    for (int i = 1; i < n - 1; i += 2) {
        area += (y[i - 1] + 4 * y[i] + y[i + 1]) * h / 3.0;
    }

    return area;
}

// Compute log(erfc(z)) with asymptotic approximations for large |z|.
double PeakModelChernyshov::log_erfc(double z) {
    // Threshold for switching to asymptotic expansion.
    const double threshold = 6.0;

    // For large positive z, erfc(z) is very small.
    if (z > threshold) {
        // Asymptotic expansion for erfc(z) for large positive z:
        //   erfc(z) ~ exp(-z^2) / (z * sqrt(pi)) * [ 1 - 1/(2z^2) + 3/(4z^4) - ... ]
        // Taking logarithms:
        //   log(erfc(z)) ~ -z^2 - log(z) - 0.5*log(pi) + log(1 - 1/(2z^2) + 3/(4z^4) ...)
        double inv_z2 = 1.0 / (z * z);
        // Here we include two correction terms; more can be added if needed.
        double series = 1.0 - 0.5 * inv_z2 + 0.75 * inv_z2 * inv_z2;
        return -z * z - std::log(z) - 0.5 * std::log(M_PI) + std::log(series);
    }
    // For large negative z, use the relation erfc(z) = 2 - erfc(-z)
    else if (z < -threshold) {
        double zp = -z; // positive value
        double inv_zp2 = 1.0 / (zp * zp);
        double series = 1.0 - 0.5 * inv_zp2 + 0.75 * inv_zp2 * inv_zp2;
        // Approximate erfc(-z) for large zp:
        double erfc_neg_z = std::exp(-zp * zp) / (zp * std::sqrt(M_PI)) * series;
        double val = 2.0 - erfc_neg_z;
        // Prevent potential log(0) issues
        if(val <= 0.0) return -std::numeric_limits<double>::infinity();
        return std::log(val);
    }
    // For moderate z, use the standard library function.
    else {
        double val = std::erfc(z);
        if(val <= 0.0) return -std::numeric_limits<double>::infinity();
        return std::log(val);
    }
}

double PeakModelChernyshov::exGaussian_log(double x, double lambda, double sigma)
{
    // 1) prefactor = lambda / 2
    // 2) exponent = (lambda/2)*(lambda*sigma^2 - 2*x)
    // 3) z = (lambda*sigma^2 - x)/(sqrt(2)*sigma)
    // => f(x) = prefactor * exp(exponent) * erfc(z)
    //
    // We'll compute ln(f(x)) = ln(prefactor) + exponent + ln(erfc(z)).

    // if lambda <= 0 or sigma <= 0, handle as needed:
    if (lambda <= 0.0 || sigma <= 0.0) return 0.0;

    // Part A: prefactor
    double prefactor = 0.5 * lambda;
    double log_prefactor = std::log(prefactor);

    // Part B: exponent
    double exponent = 0.5 * lambda * (lambda * sigma * sigma - 2.0 * x);

    // Part C: tail factor in log form
    double z = (lambda * sigma * sigma - x) / (std::sqrt(2.0) * sigma);
    double log_tail = log_erfc(z);

    // Sum them in log space
    double log_val = log_prefactor + exponent + log_tail;

    // Exponentiate the sum to get f(x)
    // If log_val is less than about -700, it will underflow to ~0 in double precision.
    // If it's greater than ~+700, it will overflow to inf.
    // You can clip if needed.
    if (log_val > 700.0) log_val = 700.0;
    if (log_val < -700.0) log_val = -700.0;

    return std::exp(log_val);
}
