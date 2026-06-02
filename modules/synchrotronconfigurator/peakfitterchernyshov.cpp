/***************************************************************************
                          peakfitterchernyshov.cpp  -  description
                             -------------------
    begin                : Thu Nov 28 18:39:00 CEST 2024
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

#include "peakfitterchernyshov.h"
#include "../../libXrdIO/curveFitting/bgmnl2curve.h"
#include <cmath>

#ifndef M_PI
#define M_PI acos(-1.0)
#endif

PeakFitterChernyshov::PeakFitterChernyshov(QObject *parent)
    : QObject{parent}
{
    settings = SettingsManager::getInstance();
    maxObserved = 0.0;
    dataSize = 0;
    twoTheta = 0.0;
    curveFittingManager = new CurveFittingManager();
    isInitialized = false;
}

QList<Scan> PeakFitterChernyshov::initLorentzParams()
{
    initValues.clear();
    lowerLimits.clear();
    upperLimits.clear();
    resetFitOutput();

    l2Curves = initDefaultCurvesSingle(initValues, lowerLimits, upperLimits);

    curveFittingManager->setScanData(peakTargetData);
    curveFittingManager->setValues(QStringList(), initValues.toList(), lowerLimits.toList(), upperLimits.toList());
    curveFittingManager->setCurves(l2Curves);

    fittedCurves = curveFittingManager->calculateCurve(-1.0, "sumY", QUuid());

    isInitialized = true;
    return fittedCurves;
}

QList<std::shared_ptr<GenericCurve>> PeakFitterChernyshov::initDefaultCurvesSingle(PeakFitterCurveParameters &initVal,
                                                                                   PeakFitterCurveParameters &loLim,
                                                                                   PeakFitterCurveParameters &upLim)
{
    QList<std::shared_ptr<GenericCurve>> curves;

    auto l2Curve = std::make_shared<BgmnL2Curve>();
    l2Curve->setIndices(QList<int>() << 0 << 1 << 2);

    if (initFromResiduals(initVal, loLim, upLim)) {
        fittedValues = initVal.toList();
        curves.append(l2Curve);
    }

    return curves;
}

QList<Scan> PeakFitterChernyshov::setCurveParameters(const LorentzParams &lp)
{
    if (peakTargetData.size() < 2) return fittedCurves;

    initValues.clear();
    lowerLimits.clear();
    upperLimits.clear();
    resetFitOutput();

    double maxIntens  = 1.5 * maxObserved;
    double rangeStart = peakTargetData.pDataAngle().constFirst();
    double rangeEnd   = peakTargetData.pDataAngle().constLast();
    double range      = rangeEnd - rangeStart;
    double stepSize   = peakTargetData.pDataAngle().at(1) - peakTargetData.pDataAngle().at(0);

    for (int i = 0; i < lp.size(); ++i) {
        int idx = 3 * l2Curves.size();

        auto l2Curve = std::make_shared<BgmnL2Curve>();
        l2Curve->setIndices(QList<int>() << idx << idx + 1 << idx + 2);

        initValues.append(lp.at(i).g(), lp.at(i).e(), lp.at(i).q(), 0.5*twoTheta);
        lowerLimits.append(0.0, rangeStart, stepSize, 0.5*twoTheta);
        upperLimits.append(maxIntens, rangeEnd, range / 4.0, 0.5*twoTheta);

        l2Curves.append(l2Curve);
    }

    curveFittingManager->setScanData(peakTargetData);
    curveFittingManager->setValues(QStringList(), initValues.toList(), lowerLimits.toList(), upperLimits.toList());
    curveFittingManager->setCurves(l2Curves);

    fittedCurves = curveFittingManager->calculateCurve(-1.0, "sumY", QUuid());

    isInitialized = true;
    return fittedCurves;
}

void PeakFitterChernyshov::setTargetData(const Scan &s)
{
    peakTargetData = s;
    int m = s.size();

    for (int i = 0; i < m; ++i) {
        // convert x-data to -theta [rad], make sure it is sorted in ascending order
        peakTargetData.pDataAngle()[i] = -s.pDataAngle().at(m - i - 1) * M_PI / 360.0;
        peakTargetData.pDataIntensity()[i] = s.pDataIntensity().at(m - i - 1);
    }
}

void PeakFitterChernyshov::resetFitOutput()
{
    dataSize = peakTargetData.size();
    params.clear();
    fittedCurves.clear();
    fittedValues.clear();
    l2Curves.clear();
    maxObserved = *std::max_element(peakTargetData.pDataIntensity().begin(), peakTargetData.pDataIntensity().end());
    residuals = peakTargetData.pDataIntensity();
    curveFittingManager->setCurves(QList<std::shared_ptr<GenericCurve>>());
    isInitialized = false;
}

void PeakFitterChernyshov::fitL2ToHistogram()
{
    if (!isInitialized) initLorentzParams();

    if (peakTargetData.pDataAngle().size() != peakTargetData.pDataIntensity().size() || peakTargetData.pDataAngle().isEmpty()) {
        qDebug() << "PeakFitterChernyshov::fitL2ToHistogram(): Input data sizes do not match or are empty.";
        return;
    }

    if (qFuzzyIsNull(maxObserved) || (dataSize <= 0)) {
        qDebug() << "PeakFitterChernyshov::fitL2ToHistogram(): Could not determine maximum intensity.";
        return;
    }

    double heightCutoff = settings->value("Fitting/heightCutoff", 1e-9).toDouble();
    double convergenceCutoff = settings->value("Fitting/convergenceCutoff", 1e-6).toDouble();
    double absErrorCutoff = settings->value("Fitting/absoluteErrorCutoff", 1e-6).toDouble();
    double relErrorCutoff = settings->value("Fitting/relativeErrorCutoff", 1e-6).toDouble();
    int maxCurves = settings->value("Fitting/maxCurves", 24).toInt();
    bool eliminate = settings->value("Fitting/allowCurveElimination", false).toBool();

    qDebug() << QString("PeakFitterChernyshov::fitL2ToHistogram(): Using stop criteria height Cutoff = %1, "
                        "convergence cutoff = %2, absolute error cutoff = %3, relative error cutoff = %4, "
                        "maximum curve adaptations = %5")
                    .arg(heightCutoff)
                    .arg(convergenceCutoff)
                    .arg(absErrorCutoff)
                    .arg(relErrorCutoff)
                    .arg(maxCurves);

    double lastError = std::numeric_limits<double>::max();
    bool addCurve = l2Curves.isEmpty();  // if we start with a default set of curves, let's first fit them before adding more
    int itCounter = 0;

    for (int iteration = 0; iteration < maxCurves; ++iteration) {
        int idx = 3 * l2Curves.size();
        ++itCounter;

        if (addCurve) {
            // Define initial values, lower bounds, and upper bounds for the curve
            if (initFromResiduals(initValues, lowerLimits, upperLimits)) {
                auto l2Curve = std::make_shared<BgmnL2Curve>();
                l2Curve->setIndices(QList<int>() << idx << idx + 1 << idx + 2);
                l2Curves.append(l2Curve);
                qDebug() << QString("PeakFitterChernyshov::fitL2ToHistogram(): Adding curve no. %1").arg(l2Curves.size());
            } else {
                qDebug() << QString("PeakFitterChernyshov::fitL2ToHistogram(): No residual peak found. Exiting.");
                break;
            }
        }

        qDebug() << QString("PeakFitterChernyshov::fitL2ToHistogram(): Fitting %1 parameters").arg(initValues.size());
        curveFittingManager->setValues(QStringList(), initValues.toList(), lowerLimits.toList(), upperLimits.toList());
        curveFittingManager->setCurves(l2Curves);
        curveFittingManager->setControls(0, 0.0, convergenceCutoff);

        addCurve = true;

        // Perform fitting
        fittedValues = curveFittingManager->fit();
        generateFitReport(curveFittingManager->getReport());

        if (fittedValues.isEmpty()) {
            qDebug() << "PeakFitterChernyshov::fitL2ToHistogram(): Fitting failed.";
            break;
        }

        fittedCurves = curveFittingManager->calculateCurve(-1.0, "sumY", QUuid());

        // Compute fitting error
        double currentError = calculateError(peakTargetData, fittedCurves, residuals);
        qDebug() << QString("PeakFitterChernyshov::fitL2ToHistogram(): Current error = %1").arg(currentError);

        bool curvesEliminated = false;

        // Check for negligible contributions and remove curves if necessary
        if (eliminate) {
            for (int i = l2Curves.size() - 1; i > 0; --i) {
                double height = fittedValues[i * 3]; // g parameter

                if (height < heightCutoff) {
                    l2Curves.takeAt(i); // Remove the curve, shared_ptr don't need to be deleted manually
                    initValues.take(i);
                    lowerLimits.take(i);
                    upperLimits.take(i);
                    addCurve = false; // do not add a new curve in next iteration
                    curvesEliminated = true;
                    qDebug() << QString("PeakFitterChernyshov::fitL2ToHistogram(): Eliminating curve %1 due to insignificant contribution (g=%2)").arg(i).arg(height);
                }
            }
        }

        // if curves were eliminated, we enforce another fit to optimize the residual curves.
        // if no curves were eliminated, we check if the error indicates convergence.
        if (!curvesEliminated) {
            // stop if the error falls below a threshold
            if (currentError < absErrorCutoff) {
                qDebug() << QString("PeakFitterChernyshov::fitL2ToHistogram(); Converged. Exiting.");
                break;
            }

            // Stop if the error does not improve significantly
            if (std::abs(lastError - currentError) < relErrorCutoff) {
                qDebug() << QString("PeakFitterChernyshov::fitL2ToHistogram(); No improvement of error. Exiting.");
                break;
            }
        }

        lastError = currentError;
    }

    qDebug() << QString("PeakFitterChernyshov::fitL2ToHistogram(); %1 of %2 iterations needed.").arg(itCounter).arg(maxCurves);

    params.setS(1.0);
    params.setT(0.5 * twoTheta);

    for (int i = 0; i < fittedValues.size() - 2; i += 3) {
        // Extract parameters from fitted values
        params.append(LorentzParam(fittedValues[i], fittedValues[i+1], fittedValues[i+2])); // g, e, q
    }

    emit fitComplete(params, fittedCurves);
}

double PeakFitterChernyshov::calculateError(const Scan &data, const QList<Scan> &curves, QList<double> &residuals)
{
    if (data.isEmpty() || curves.isEmpty()) {
        qWarning() << QString("PeakFitterChernyshov::calculateError(): Insufficient data for error calculation.");
        return std::numeric_limits<double>::max();
    }

    double error = 0.0;
    residuals = QList<double>(data.pDataIntensity().size(), 0.0);
    const auto& yData = data.pDataIntensity();
    const auto& yFit  = curves.last().pDataIntensity(); // summary curve

    if (yData.size() != yFit.size()) {
        qWarning() << QString("PeakFitterChernyshov::calculateError(): Fitted curve has non-matching size");
        return std::numeric_limits<double>::max();
    }

    for (int i = 0; i < yData.size(); ++i) {
        double residual = (yData.at(i) - yFit.at(i)) / maxObserved;
        error += residual * residual;
        residuals[i] = residual;
    }

    return error / dataSize;
}

double PeakFitterChernyshov::calculateWsum(const Scan &scan)
{
    double wsum = 0.0;

    for (int i = 0; i < scan.pDataIntensity().size(); ++i) {
        wsum += std::pow(scan.pDataIntensity().at(i), 2.0);
    }

    return wsum;
}

bool PeakFitterChernyshov::initFromResiduals(PeakFitterCurveParameters &values,
                                          PeakFitterCurveParameters &loLimits,
                                          PeakFitterCurveParameters &upLimits)
{
    int maxPosIndex;
    int lHalfPosIndex;
    int rHalfPosIndex;

    if (!initPosAndWidth(residuals, maxPosIndex, lHalfPosIndex, rHalfPosIndex)) {
        // No residual peak found
        return false;
    }

    // Find the highest value in the source data and limit curve heights to 1.5 * maxIntens
    auto maxIntens = std::max_element(peakTargetData.pDataIntensity().begin(), peakTargetData.pDataIntensity().end());
    double maxIntensLim = *maxIntens * 1.5;
    double stepSize = peakTargetData.pDataAngle().at(1) - peakTargetData.pDataAngle().at(0);

    // Approximate g, e, q to the residual peak, assuming it has a Pseudo-Voigt shape (approximation of width (q) suggested by ChatGPT)
    double eEst = peakTargetData.pDataAngle()[maxPosIndex];
    double qEst = stepSize * (qMax(lHalfPosIndex, rHalfPosIndex) - qMin(lHalfPosIndex, rHalfPosIndex)) * (1.0 + 1.66 * 0.5 + 0.3 * 0.25) / 2.352;
    double gEst = residuals.at(maxPosIndex) * qEst / 2.0;

    // existing curves must be initialized with the results of the previous curve fit
    values.clear();

    for (int i = 0; i < fittedValues.size() - 2; i += 3) {
        values.append(fittedValues.at(i), fittedValues.at(i+1), fittedValues.at(i+2), 0.5*twoTheta);
    }

    // append initial values for a new curve
    values.append(gEst, eEst, qEst, 0.5*twoTheta);
    loLimits.append(0.0, peakTargetData.pDataAngle().constFirst(), stepSize, 0.5*twoTheta);
    upLimits.append(maxIntensLim, peakTargetData.pDataAngle().constLast(), (peakTargetData.pDataAngle().constLast() - peakTargetData.pDataAngle().constFirst()) / 4.0, 0.5*twoTheta);

    return true;
}

bool PeakFitterChernyshov::initPosAndWidth(const QList<double> &val, int &maxPos, int &lHalfPos, int &rHalfPos)
{
    auto m = std::max_element(val.begin(), val.end());
    maxPos = std::distance(val.begin(), m);

    if ((maxPos <= 0) || (maxPos >= val.size() - 1)) {
        // no peak data if highest intensity is at the start or end of the scan
        return false;
    }

    lHalfPos = maxPos - 1;
    rHalfPos = maxPos + 1;

    while (lHalfPos >= 0 && val.at(lHalfPos) > 0.5 * val.at(maxPos)) {
        --lHalfPos;
    }

    while (rHalfPos < val.size() - 1 && val.at(rHalfPos) > 0.5 * val.at(maxPos)) {
        ++rHalfPos;
    }

    return true;
}

void PeakFitterChernyshov::generateFitReport(const QMap<QString, QVariant> &r)
{
    QStringList s;

    QMapIterator<QString, QVariant> it(r);
    while (it.hasNext()) {
        it.next();
        s.append(QString("%1: %2").arg(it.key(), it.value().toString()));
    }

    qDebug() << s.join("\n");
}

/* class PeakFitterCurveParameters */

PeakFitterCurveParameters::PeakFitterCurveParameters() {}

void PeakFitterCurveParameters::append(double g, double e, double q, double t)
{
    _data.append(LorentzParam(g, e, q));
    _data.setT(t);
}

QList<double> PeakFitterCurveParameters::toList() const
{
    QList<double> v;

    for (int i = 0; i < _data.size(); ++i) {
        v << _data.at(i).g();
        v << _data.at(i).e();
        v << _data.at(i).q();
    }

    return v;
}

QStringList PeakFitterCurveParameters::toStringList() const
{
    QStringList v;

    for (int i = 0; i < _data.size(); ++i) {
        v << QString("%1").arg(_data.at(i).g(), 0, 'f', 4);
        v << QString("%1").arg(_data.at(i).e(), 0, 'f', 4);
        v << QString("%1").arg(_data.at(i).q(), 0, 'f', 4);
    }

    return v;
}

void PeakFitterCurveParameters::take(int i)
{
    if (i < _data.size()) _data.remove(i);
}

