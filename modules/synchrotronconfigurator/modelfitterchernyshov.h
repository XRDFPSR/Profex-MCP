/***************************************************************************
                          modelfitterchernyshov.h  -  description
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

#ifndef MODELFITTERCHERNYSHOV_H
#define MODELFITTERCHERNYSHOV_H

#include <QObject>
#include <QMap>
#include "parameterstorage.h"
#include "../libXrdIO/curveFitting/genericcurve.h"
#include "../libXrdIO/scan.h"

class ModelFitterChernyshov : public QObject
{
    Q_OBJECT
public:
    explicit ModelFitterChernyshov(QObject *parent = nullptr);

    void setTargetData(const QList<double> &x, const QList<double> &y);
    void setPeakParameters(ParameterStorage *m) {_parameterStorage = m;}
    inline QList<Scan> getFittedCurves() const {return _fittedCurves;}
    inline QList<double> getFittedValues() const {return _fittedValues;}

private:
    QList<double> _dataX;
    QList<double> _dataY;
    ParameterStorage *_parameterStorage;
    QList<std::shared_ptr<GenericCurve>> _modelCurves;
    QList<Scan> _fittedCurves;
    QList<double> _fittedValues;
    void generateFitReport(const QMap<QString, QVariant> &r);

public slots:
    void fitModel();

signals:
};

#endif // MODELFITTERCHERNYSHOV_H
