/***************************************************************************
                          peakfitterchernyshov.h  -  description
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

#ifndef PEAKFITTERCHERNYSHOV_H
#define PEAKFITTERCHERNYSHOV_H

#include "lorentzparam.h"
#include "../libXrdIO/scan.h"
#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/curveFitting/genericcurve.h"
#include "../libXrdIO/curveFitting/curvefittingmanager.h"
#include <QObject>
#include <QList>

class PeakFitterCurveParameters
{
public:
    PeakFitterCurveParameters();

    void append(double, double, double, double t);
    QList<double> toList() const;
    QStringList toStringList() const;
    inline QList<LorentzParam> items() const {return _data;}
    void take(int);
    inline int size() const {return _data.size();}
    inline void clear() {_data.clear();}
    inline LorentzParam constFirst() const {return _data.constFirst();}
    inline LorentzParam constLast() const {return _data.constLast();}

private:
    LorentzParams _data;
};

class PeakFitterChernyshov : public QObject
{
    Q_OBJECT
public:
    explicit PeakFitterChernyshov(QObject *parent = nullptr);

    void setTargetData(const Scan &s);
    void resetFitOutput();
    QList<Scan> initLorentzParams();
    inline void setTwoTheta(double t) {twoTheta = t;}
    inline Scan getTargetData() const                     {return peakTargetData;}
    inline QList<Scan> getFittedCurves() const            {return fittedCurves;}
    inline QList<double> getResiduals() const             {return residuals;}
    inline LorentzParams getCurveParameters() const {return params;}
    QList<Scan> setCurveParameters(const LorentzParams &);

private:
    SettingsManager *settings;
    // static const QMap<int, QList<double>> initialParameters;
    double calculateError(const Scan &, const QList<Scan> &, QList<double> &);
    double calculateWsum(const Scan &);
    bool initFromResiduals(PeakFitterCurveParameters &values,
                           PeakFitterCurveParameters &loLimits,
                           PeakFitterCurveParameters &upLimits);

    Scan peakTargetData;
    double maxObserved;
    double twoTheta;
    int dataSize;
    bool isInitialized;
    LorentzParams params;
    QList<Scan> fittedCurves;
    QList<double> fittedValues;
    QList<double> residuals;
    QList<std::shared_ptr<GenericCurve>> l2Curves;
    PeakFitterCurveParameters initValues, lowerLimits, upperLimits;
    CurveFittingManager *curveFittingManager;

    void generateFitReport(const QMap<QString, QVariant> &);
    QList<std::shared_ptr<GenericCurve> > initDefaultCurvesSingle(PeakFitterCurveParameters &,
                                                                  PeakFitterCurveParameters &,
                                                                  PeakFitterCurveParameters &);
    bool initPosAndWidth(const QList<double> &, int &maxPos, int &lHalfPos, int &rHalfPos);

public slots:
    void fitL2ToHistogram();

signals:
    void showText(QString);
    void displayCurve(QList<double>, QList<double>);
    void fitComplete(LorentzParams, QList<Scan>);
};


#endif // PEAKFITTERCHERNYSHOV_H
