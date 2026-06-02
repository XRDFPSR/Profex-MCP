/***************************************************************************
                          peakfitrange.cpp  -  description
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

#include "peakfitrange.h"
#include "peakfitcurve.h"

PeakFitRange::PeakFitRange(const QUuid &u, int type, QObject *parent)
    : QObject(parent), PeakFitItem(u, type)
{
    _itemLowerLimit = nullptr;
    _itemUpperLimit = nullptr;

    _cfitManager  = new CurveFittingManager;
}

PeakFitRange::~PeakFitRange()
{
    if (_cfitManager)  delete _cfitManager;
}

void PeakFitRange::setLowerLimit(double d)
{
    _lowerLimit = d;

    if (!_itemLowerLimit) {
        _itemLowerLimit = new QTreeWidgetItem(QStringList() << QString("Start") << QString() << QString());
        _itemLowerLimit->setTextAlignment(2, Qt::AlignRight);
        insertChild(0, _itemLowerLimit);
    }

    _itemLowerLimit->setText(2, QString::number(d, 'f', 4));
}

void PeakFitRange::setUpperLimit(double d)
{
    _upperLimit = d;

    if (!_itemUpperLimit) {
        _itemUpperLimit = new QTreeWidgetItem(QStringList() << QString("End") << QString() << QString());
        _itemUpperLimit->setTextAlignment(2, Qt::AlignRight);
        insertChild(1, _itemUpperLimit);
    }

    _itemUpperLimit->setText(2, QString::number(d, 'f', 4));
}

QList<std::shared_ptr<GenericCurve >> PeakFitRange::getCurves()
{
    QList<std::shared_ptr<GenericCurve>> lst;

    if (childCount() < 2) return lst;

    for (int i = 2; i < childCount(); ++i) {
        PeakFitCurve *c = dynamic_cast<PeakFitCurve*>(child(i));
        if (c) lst.append(c->getCurve());
    }

    return lst;
}

QStringList PeakFitRange::getVariableNames(int n) const
{
    QStringList lst;

    // child 0 and 1 are not curve items
    if (childCount() < 3)     return lst;
    if (n > childCount() - 3) return lst;

    PeakFitCurve *c = dynamic_cast<PeakFitCurve*>(child(n + 2));
    if (!c) return lst;

    for (int i = 0; i < c->parameterCount(); ++i) {
        PeakFitParameter *pIt = c->parameterItem(i);
        if (pIt) lst.append(pIt->variableName());
    }

    return lst;
}

QSet<QString> PeakFitRange::getUsedVariableNames() const
{
    QSet<QString> set;

    for (int c = 2; c < childCount(); ++c) {
        PeakFitCurve *curve = dynamic_cast<PeakFitCurve*>(child(c));
        if (!curve) continue;

        for (int p = 0; p < curve->parameterCount(); ++p) {
            PeakFitParameter *pIt = curve->parameterItem(p);
            if (pIt) set.insert(pIt->variableName());
        }
    }

    return set;
}

bool PeakFitRange::removeCurve(const QUuid &u)
{
    if (childCount() < 3) return false;

    for (int i = 2; i < childCount(); ++i) {
        PeakFitCurve *c = dynamic_cast<PeakFitCurve*>(child(i));
        if (!c) continue;
        if (c->uid() != u) continue;

        QTreeWidgetItem *it = takeChild(i);
        if (it) {
            delete it;
            return true;
        }
    }

    return false;
}

std::shared_ptr<GenericCurve> PeakFitRange::curve(int i)
{
    if (childCount() < 3)     return nullptr;
    if (i > childCount() - 3) return nullptr;
    PeakFitCurve * pIt = dynamic_cast<PeakFitCurve*>(child(i + 2));
    if (!pIt) return nullptr;
    return pIt->getCurve();
}

void PeakFitRange::setControls(int maxIt, double eps, double difstep)
{
    if (_cfitManager) _cfitManager->setControls(maxIt, eps, difstep);
}

void PeakFitRange::setScan(const Scan *s)
{
    if (_cfitManager) _cfitManager->setScanData(s->mid(_lowerLimit, _upperLimit));
}

void PeakFitRange::setVariableValues(const QStringList &var, const QVector<double> &val, const QVector<double> &lo, const QVector<double> &up)
{
    if (_cfitManager) _cfitManager->setValues(var, val, lo, up);
}

QVector<double> PeakFitRange::fit()
{
    _cfitManager->setRangeName(text(0));
    _cfitManager->setCurves(getCurves());
    QVector<double> vec = _cfitManager->fit();
    emit sigFitComplete();
    return vec;
}

QMap<QString, QVariant> PeakFitRange::report()
{
    return _cfitManager->getReport();
}

void PeakFitRange::updateCurveVariables(const QMap<QString, global::CurveFitVariable> &v)
{
    for (int i = 2; i < childCount(); ++i) {
        PeakFitCurve *cIt = dynamic_cast<PeakFitCurve*>(child(i));
        if (!cIt) continue;

        for (int j = 0; j < cIt->childCount(); ++j) {
            PeakFitParameter *pIt = dynamic_cast<PeakFitParameter*>(cIt->child(j));
            if (!pIt) continue;

            if (v.contains(pIt->variableName())) {
                pIt->setValue(v.value(pIt->variableName()).value);
            }
        }
    }
}

void PeakFitRange::updateAreas()
{
    bool ok;
    QUuid cUid;

    for (int i = 2; i < childCount(); ++i) {
        PeakFitCurve *cIt = dynamic_cast<PeakFitCurve*>(child(i));
        if (!cIt) continue; // just check if it is a curve item
        double a = _cfitManager->getArea(i-2, cUid, ok);

        if (!ok) continue;

        PeakFitParameter *pIt = cIt->areaItem();
        if (pIt) pIt->setValue(a);
    }
}

QList<Scan> PeakFitRange::getCalculatedCurves(double sSize)
{
    _cfitManager->setCurves(getCurves());
    return _cfitManager->calculateCurve(sSize, text(0), _uid);
}

void PeakFitRange::abortFit()
{
    if (_cfitManager) _cfitManager->abortFit();
}

