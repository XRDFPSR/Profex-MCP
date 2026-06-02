/***************************************************************************
                          synchrotronpeakmanager.cpp  -  description
                             -------------------
    begin                : Wed Dec 11 18:00:00 CEST 2024
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

#include "peakmanagerchernyshov.h"
#include "peakmodelchernyshov.h"
#include <QStackedWidget>
#include "../libXrdIO/structs.h"

PeakManagerChernyshov::PeakManagerChernyshov(ThreadSafePlotter *p, QObject *parent)
    :  QObject{parent}, plotterPeaks(p)
{
    _uid = QUuid::createUuid();

    peakModel = new PeakModelChernyshov;
    peakFitter = new PeakFitterChernyshov;
    parameterStorage = new ParameterStorage;
    peakModel->setParameters(parameterStorage);

    connect(peakFitter, &PeakFitterChernyshov::fitComplete, this, &PeakManagerChernyshov::peakFitComplete);
    connect(peakFitter, &PeakFitterChernyshov::showText,    this, &PeakManagerChernyshov::fitProgress);
}

PeakManagerChernyshov::~PeakManagerChernyshov()
{
    if (peakModel)     delete peakModel;
    if (peakFitter)    delete peakFitter;
    if (parameterStorage) delete parameterStorage;
}

void PeakManagerChernyshov::updateParameters(const ParameterStorage *p)
{
    parameterStorage->update(p);
}

void PeakManagerChernyshov::calculateFwhm(bool withT)
{
    if (parameterStorage->isEmpty()) {
        qDebug() << QString("PeakManager::calculateFwhm(): No peak parameters found. Use ::setPeakParameters() first. Exiting.");
        return;
    }

    peakModel->getFwhm(withT);
}

void PeakManagerChernyshov::generatePeak()
{
    if (parameterStorage->isEmpty()) {
        qDebug() << QString("PeakManager::generatePeak(): No peak parameters found. Use ::setPeakParameters() first. Exiting.");
        return;
    }

    if (peakModel->generatePeak()) {
        calculatedPeakProfile= peakModel->getCalculatedProfile();
    } else {
        qDebug() << QString("PeakManager::generatePeak(): Generating peak failed. Exiting.");
        return;
    }

    // if a convoluted profile exists, fit this one. Else fit the unconvoluted PV function
    peakFitter->setTargetData(calculatedPeakProfile);
}

void PeakManagerChernyshov::getRawProfilePv(QList<double> &x, QList<double> &y) const
{
    peakModel->getRawProfilePv(x, y);
}

void PeakManagerChernyshov::getRawProfileExp(QList<double> &x, QList<double> &y) const
{
    peakModel->getRawProfileExp(x, y);
}

void PeakManagerChernyshov::getRawProfileConv(QList<double> &x, QList<double> &y) const
{
    peakModel->getRawProfileConv(x, y);
}

void PeakManagerChernyshov::setRawProfiles(const QList<double> &pvX, const QList<double> &pvY,
                                           const QList<double> &expX, const QList<double> &expY,
                                           const QList<double> &convX, const QList<double> &convY)
{
    peakModel->setRawProfiles(pvX, pvY, expX, expY, convX, convY);
    calculatedPeakProfile = peakModel->getCalculatedProfile();
    peakFitter->setTargetData(calculatedPeakProfile);
}

void PeakManagerChernyshov::initCurves()
{
    fittedScans = peakFitter->initLorentzParams();
    updatePlot();
}

void PeakManagerChernyshov::setL2CurveParameters(const LorentzParams &lp)
{
    fittedScans = peakFitter->setCurveParameters(lp);
    updatePlot();
}

void PeakManagerChernyshov::fitCurves(QThreadPool *pool)
{
    lorentzCurves.clear();
    peakFitter->setTwoTheta(parameterStorage->getParameter(synchro::TWOTHETA_DEG, -1.0));
    pool->start(new FitTask(peakFitter)); // Use QThreadPool to execute the task
}

void PeakManagerChernyshov::peakFitComplete(LorentzParams lp, QList<Scan> fc)
{
    lorentzCurves = lp;
    fittedScans = fc;

    emit fitProcessCompleted(_uid);
}

void PeakManagerChernyshov::updatePlot() const
{
    resetPlot();
    plotterPeaks->setPeakData(calculatedPeakProfile);
    plotterPeaks->setLorentzCurves(fittedScans);
    plotterPeaks->rescaleAxes(true, true);
}

void PeakManagerChernyshov::resetPlot() const
{
    plotterPeaks->reset();
}

void PeakManagerChernyshov::fitProgress(QString s)
{
    Q_UNUSED(s);
}

QString PeakManagerChernyshov::getCsvString() const
{
    QList<double> pvX, pvY, expX, expY, convX, convY;
    int nPv   = peakModel->getRawProfilePv(pvX, pvY);
    int nExp  = peakModel->getRawProfileExp(expX, expY);
    int nConv = peakModel->getRawProfileConv(convX, convY);
    int n = qMax(nPv, qMax(nExp, nConv));

    QList<Scan> lorentzCurves = peakFitter->getFittedCurves();

    for (int i = 0; i < lorentzCurves.size(); ++i) {
        n = qMax(n, lorentzCurves.at(i).size());
    }

    QString xAxisLabel(QString("%1%2%3 (%4)").arg(global::Delta).arg("2").arg(global::theta).arg(global::degree));
    QStringList header;

    header << xAxisLabel << "Pseudo-Voigt Profile";
    header << xAxisLabel << "Exponential Decay";
    header << xAxisLabel << "Convolved Profile";

    for (int i = 0; i < lorentzCurves.size(); ++i) {
        QString lbl = (i == lorentzCurves.size() - 1) ? QString("L2 Curve Sum") : QString("L2 Curve %1").arg(i + 1);
        header << xAxisLabel << lbl;
    }

    QString out(header.join(";") + "\n");

    for (int i = 0; i < n; ++i) {
        QStringList line;

        bool addPv   = (i < qMin(pvX.size(),   pvY.size()));
        bool addExp  = (i < qMin(expX.size(),  expY.size()));
        bool addConv = (i < qMin(convX.size(), convY.size()));

        line.append(addPv   ? QString::number(pvX.at(i))   : QString());
        line.append(addPv   ? QString::number(pvY.at(i))   : QString());
        line.append(addExp  ? QString::number(expX.at(i))  : QString());
        line.append(addExp  ? QString::number(expY.at(i))  : QString());
        line.append(addConv ? QString::number(convX.at(i)) : QString());
        line.append(addConv ? QString::number(convY.at(i)) : QString());

        for (int j = 0; j < lorentzCurves.size(); ++j) {
            bool addCurve = (i < lorentzCurves.at(j).size());

            if (addCurve) {
                double ttnRad = lorentzCurves.at(j).pDataAngle().at(i);
                double ttDeg  = 2.0 * qRadiansToDegrees(-ttnRad);
                double val    = lorentzCurves.at(j).pDataIntensity().at(i);

                line << QString::number(ttDeg) << QString::number(val);
            } else {
                line << QString() << QString();
            }
        }

        out.append(line.join(";") + "\n");
    }

    return out;
}

/* class FitTask */

FitTask::FitTask(PeakFitterChernyshov *fitter)
    : m_fitter(fitter)
{}

void FitTask::run()
{
    m_fitter->fitL2ToHistogram();
}
