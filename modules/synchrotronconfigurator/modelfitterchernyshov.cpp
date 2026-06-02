/***************************************************************************
                          modelfitterchernyshov.cpp  -  description
                             -------------------
    begin                : Sun Jan 19 14:05:00 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#include <QDebug>
#include <limits>
#include "modelfitterchernyshov.h"
#include "../libXrdIO/curveFitting/fwhmmodelchernyshov.h"
#include "../libXrdIO/curveFitting/fwhmmodelchernyshovfocused.h"
#include "../libXrdIO/curveFitting/curvefittingmanager.h"
#include "../libXrdIO/scan.h"

ModelFitterChernyshov::ModelFitterChernyshov(QObject *parent)
    : QObject{parent}
{}

void ModelFitterChernyshov::setTargetData(const QList<double> &x, const QList<double> &y)
{
    _dataX = x;
    _dataY = y;
}

void ModelFitterChernyshov::fitModel()
{
    if (!_parameterStorage) return;

    bool complete = true;
    if (!_parameterStorage->contains(synchro::CHERNYSHOV_D))           complete = false;
    if (!_parameterStorage->contains(synchro::CHERNYSHOV_P))           complete = false;
    if (!_parameterStorage->contains(synchro::CHERNYSHOV_T))           complete = false;
    if (!_parameterStorage->contains(synchro::CHERNYSHOV_C))           complete = false;
    if (!_parameterStorage->contains(synchro::CHERNYSHOV_PHI))         complete = false;
    if (!_parameterStorage->contains(synchro::CHERNYSHOV_PHI_FOCUSED)) complete = false;

    if (!complete) {
        qDebug() << QString("ModelFitterChernyshov::fitModel(): Curve parameters are incomplete. Exiting.");
        return;
    }

    double valD   = _parameterStorage->getParameter(synchro::CHERNYSHOV_D, -1.0);
    double valP   = _parameterStorage->getParameter(synchro::CHERNYSHOV_P, -1.0);
    double valT   = _parameterStorage->getParameter(synchro::CHERNYSHOV_T, -1.0);
    double valC   = _parameterStorage->getParameter(synchro::CHERNYSHOV_C, -1.0);
    double valPhi = _parameterStorage->getParameter(synchro::CHERNYSHOV_PHI, -1.0);

    bool fitD   = _parameterStorage->getParameter(synchro::CHERNYSHOV_D_CHECKED, false);
    bool fitP   = _parameterStorage->getParameter(synchro::CHERNYSHOV_P_CHECKED, false);
    bool fitT   = _parameterStorage->getParameter(synchro::CHERNYSHOV_T_CHECKED, false);
    bool fitC   = _parameterStorage->getParameter(synchro::CHERNYSHOV_C_CHECKED, false);
    bool fitPhi = _parameterStorage->getParameter(synchro::CHERNYSHOV_PHI_CHECKED, false);
    bool phifoc = _parameterStorage->getParameter(synchro::CHERNYSHOV_PHI_FOCUSED, false);

    QList<double> initValues;
    QList<double> lowerLimits;
    QList<double> upperLimits;

    initValues << valD;
    initValues << valP;
    initValues << valT;
    initValues << valC;
    if (!phifoc) initValues << valPhi;

    double dLowest = std::numeric_limits<double>::lowest();
    double dMax    = std::numeric_limits<double>::max();

    lowerLimits << (fitD   ? 0.0     : valD);
    lowerLimits << (fitP   ? 0.0     : valP);
    lowerLimits << (fitT   ? 0.0     : valT);
    lowerLimits << (fitC   ? 0.0     : valC);
    if (!phifoc) lowerLimits << (fitPhi ? dLowest : valPhi);

    upperLimits << (fitD   ? dMax : valD);
    upperLimits << (fitP   ? dMax : valP);
    upperLimits << (fitT   ? dMax : valT);
    upperLimits << (fitC   ? dMax : valC);
    if (!phifoc) upperLimits << (fitPhi ? dMax : valPhi);

    Scan scan;
    scan.setDataAng(_dataX);
    scan.setDataInt(_dataY);

    CurveFittingManager *curveFittingManager = new CurveFittingManager();
    curveFittingManager->setScanData(scan);
    curveFittingManager->setValues(QStringList(), initValues, lowerLimits, upperLimits);
    curveFittingManager->setControls(0, 0.0, 1e-5);

    if (phifoc) {
        auto modelCurve = std::make_shared<FwhmModelChernyshovFocused>();
        modelCurve->setIndices(QList<int>() << 0 << 1 << 2 << 3);
        _modelCurves.append(modelCurve);
    } else {
        auto modelCurve = std::make_shared<FwhmModelChernyshov>();
        modelCurve->setIndices(QList<int>() << 0 << 1 << 2 << 3 << 4);
        _modelCurves.append(modelCurve);
    }

    curveFittingManager->setCurves(_modelCurves);
    _fittedValues = curveFittingManager->fit();
    generateFitReport(curveFittingManager->getReport());

    if (_fittedValues.size() != (phifoc ? 4 : 5)) {
        qWarning() << "ModelFitterChernyshov::fitModel(): Fitting failed.";
    } else {
        _fittedCurves = curveFittingManager->calculateCurve(-1.0, "sumY", QUuid());
        _parameterStorage->setValue(synchro::CHERNYSHOV_D, _fittedValues.at(0));
        _parameterStorage->setValue(synchro::CHERNYSHOV_P, _fittedValues.at(1));
        _parameterStorage->setValue(synchro::CHERNYSHOV_T, _fittedValues.at(2));
        _parameterStorage->setValue(synchro::CHERNYSHOV_C, _fittedValues.at(3));
        if (!phifoc) _parameterStorage->setValue(synchro::CHERNYSHOV_PHI, _fittedValues.at(4));
    }

    delete curveFittingManager;
}

void ModelFitterChernyshov::generateFitReport(const QMap<QString, QVariant> &r)
{
    QStringList s;

    QMapIterator<QString, QVariant> it(r);
    while (it.hasNext()) {
        it.next();
        s.append(QString("%1: %2").arg(it.key(), it.value().toString()));
    }

    qDebug() << s.join("\n");
}
