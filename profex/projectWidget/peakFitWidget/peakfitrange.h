/***************************************************************************
                          peakfitrange.h  -  description
                             -------------------
    begin                : Fri Jul 29 21:34:00 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#ifndef PEAKFITRANGE_H
#define PEAKFITRANGE_H

#include <QUuid>
#include <QList>
#include <QObject>
#include "peakfititem.h"
#include "../libXrdIO/curveFitting/genericcurve.h"
#include "../libXrdIO/curveFitting/curvefittingmanager.h"
#include "../libXrdIO/structs.h"

class PeakFitRange : public QObject, public PeakFitItem
{
    Q_OBJECT

public:
    PeakFitRange(const QUuid &u, int type = Type, QObject *parent = nullptr);
    ~PeakFitRange();

    void abortFit();

    void setLowerLimit(double);
    void setUpperLimit(double);
    inline double lowerLimit() const {return _lowerLimit;}
    inline double upperLimit() const {return _upperLimit;}
    QList<std::shared_ptr<GenericCurve> > getCurves();
    QStringList getVariableNames(int) const;
    QSet<QString> getUsedVariableNames() const;

    inline void setRangeNumber(int i) {_rangeNumber = i;}
    inline int rangeNumber() const {return _rangeNumber;}

    inline int curveCount() const {return childCount() < 3 ? 0 : childCount() - 2;}
    std::shared_ptr<GenericCurve> curve(int);
    QList<Scan> getCalculatedCurves(double);

    bool removeCurve(const QUuid &);

    void setControls(int, double, double);
    void setScan(const Scan *);
    void setVariableValues(const QStringList &, const QVector<double> &, const QVector<double> &, const QVector<double> &);
    void updateCurveVariables(const QMap<QString, global::CurveFitVariable> &);
    void updateAreas();

    QVector<double> fit();
    QMap<QString, QVariant> report();

private:
    QTreeWidgetItem *_itemLowerLimit;
    QTreeWidgetItem *_itemUpperLimit;
    double _lowerLimit;
    double _upperLimit;
    CurveFittingManager *_cfitManager;
    int _rangeNumber;

signals:
    void sigFitComplete();
};

#endif // PEAKFITRANGE_H
