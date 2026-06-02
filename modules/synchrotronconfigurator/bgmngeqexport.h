/***************************************************************************
                          synchrotronl2interpolator.h  -  description
                             -------------------
    begin                : Sat Dec 28 09:16:00 CEST 2024
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

#ifndef BGMNGEQEXPORT_H
#define BGMNGEQEXPORT_H

#include "lorentzparam.h"
#include <QObject>
#include <QList>
#include <QByteArray>
#include <QMutex>
#include <cmath>


class BgmnGeqExport : public QObject {
    Q_OBJECT
public:
    explicit BgmnGeqExport(QObject *parent = nullptr);

    // Set curve parameters
    void setCurveData(const QList<LorentzParams> &inputData);

    // Set mandatory parameters
    void setInstrumentParameters(double d, double r, double t);
    inline void setGeometry(GEOMETRY g) {geometry = g;}
    inline void setGerFileName(const QString &s) {ger_= s;}

    QByteArray getBinaryFileContent();

signals:
    void interpolationComplete(QByteArray binaryData); // Emitted upon completion

private:
    // Mutex for thread safety
    QMutex mutex_;

    // Input parameters
    QList<LorentzParams> lorentzParams_;
    QString ger_;
    double wmin_;  // Minimum THETA
    double wmax_;  // Maximum THETA
    double wstep_; // Step size
    double d_, r_, t_; // instrument parameters D, R, T
    GEOMETRY geometry;

    // Check if valid input data is available
    bool checkInputData() const;
};

#endif // BGMNGEQEXPORT_H
