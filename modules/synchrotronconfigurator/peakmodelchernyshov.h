/***************************************************************************
                          synchrotronfwhmcalculator.h  -  description
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

#ifndef PEAKMODELCHERNYSHOV_H
#define PEAKMODELCHERNYSHOV_H

#include "parameterstorage.h"
#include "../libXrdIO/scan.h"
#include <QList>
#include <QMap>

#ifndef M_PI
#define M_PI acos(-1.0)
#endif

class PeakModelChernyshov
{
public:
    PeakModelChernyshov();
    ~PeakModelChernyshov();

    void setParameters(ParameterStorage *);
    bool generatePeak();
    bool getFwhm(bool withT);
    Scan getCalculatedProfile();
    int getRawProfilePv(QList<double> &pvX, QList<double> &pvY) const;
    int getRawProfileExp(QList<double> &expX, QList<double> &expY) const;
    int getRawProfileConv(QList<double> &convX, QList<double> &convY) const;
    void setRawProfiles(const QList<double> &pvX, const QList<double> &pvY, const QList<double> &expX, const QList<double> &expY, const QList<double> &convX, const QList<double> &convY);

private:
    ParameterStorage *parameterStorage;
    QList<double> _pvX, _pvY;
    QList<double> _expX, _expY;
    QList<double> _convX, _convY;

    bool computeProfile();
    void profileToPlottable(const QList<double> &x, const QList<double> &y, QList<double> &px, QList<double> &py, int n);
    void createXvalues();
    void pseudoVoigtY(double a, double p, double h, double s);
    void detectorTransparency(QList<double> &y, double mu, double t, double ttheta);
    void exponentialX(double tt, double D);
    void exponentialY(double mu, double t, double ttheta, bool applyCutoff);
    void convolvedX();
    void convolvedY(double mu, double t, double ttheta);
    void exgaussY(double tt, double D, double mu, double h);
    double calculateProfileArea(const QList<double> &x, const QList<double> &y);
    double log_erfc(double);
    double exGaussian_log(double x, double lambda, double sigma);
};

#endif // PEAKMODELCHERNYSHOV_H
