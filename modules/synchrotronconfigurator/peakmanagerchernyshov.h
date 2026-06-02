/***************************************************************************
                          synchrotronpeakmanager.h  -  description
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

#ifndef PEAKMANAGERCHERNYSHOV_H
#define PEAKMANAGERCHERNYSHOV_H

#include "peakfitterchernyshov.h"
#include "peakmodelchernyshov.h"
#include "threadsafeplotter.h"
#include "parameterstorage.h"
#include "../../libXrdIO/scan.h"
#include <QObject>
#include <QUuid>


class PeakManagerChernyshov : public QObject
{
    Q_OBJECT
public:
    PeakManagerChernyshov(ThreadSafePlotter *, QObject *parent = nullptr);
    ~PeakManagerChernyshov();

    inline QUuid uid() const {return _uid;}
    inline LorentzParams getL2CurveParameters() const {return lorentzCurves;}
    void updatePlot() const;
    void resetPlot() const;
    void generatePeak();
    void calculateFwhm(bool withT);
    void setL2CurveParameters(const LorentzParams &);
    QString getCsvString() const;

    void getRawProfilePv(QList<double> &pvX, QList<double> &pvY) const;
    void getRawProfileExp(QList<double> &expX, QList<double> &expY) const;
    void getRawProfileConv(QList<double> &convX, QList<double> &convY) const;

    void setRawProfiles(const QList<double> &pvX, const QList<double> &pvY,
                        const QList<double> &expX, const QList<double> &expY,
                        const QList<double> &convX, const QList<double> &convY);
    inline QList<Scan> getFittedCurves() const {return fittedScans;}
    inline ParameterStorage* getParameters() {return parameterStorage;}
    void updateParameters(const ParameterStorage *);

private:
    PeakFitterChernyshov *peakFitter;
    ThreadSafePlotter *plotterPeaks;
    PeakModelChernyshov *peakModel;
    ParameterStorage *parameterStorage;

    Scan calculatedPeakProfile;
    QUuid _uid;
    LorentzParams lorentzCurves;
    QList<Scan> fittedScans;

    int _maxCurves;
    int _maxIterations;

public slots:
    void initCurves();
    void fitCurves(QThreadPool *);

private slots:
    void fitProgress(QString);
    void peakFitComplete(LorentzParams,QList<Scan>);

signals:
    void fitProcessCompleted(QUuid);
    void statusMessage(QUuid,QStringList);
};

class FitTask : public QRunnable {
public:
    explicit FitTask(PeakFitterChernyshov *fitter);
    void run() override;

private:
    PeakFitterChernyshov *m_fitter;
};

#endif // PEAKMANAGERCHERNYSHOV_H
