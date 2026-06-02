/***************************************************************************
                          elementscatteringdatamanager.h  -  description
                             -------------------
    begin                : Wed Jul 14 18:25:07 CET 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#ifndef ELEMENTSCATTERINGDATAMANAGER_H
#define ELEMENTSCATTERINGDATAMANAGER_H

#include <QObject>
#include <QMap>
#include "elementscatteringdata.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif


class XRDIO_EXPORT ElementScatteringDataManager
{
public:
    ElementScatteringDataManager();

    QList<ElementScatteringData> getAllScatteringData() const;

    void getElementValuesAtEnergy(double v, QStringList &el, QList<int> &z, QList<double> &f0, QList<double> &f1, QList<double> &f2, QList<double> &mac, QList<double> &lac);

    QString getElementSymbol(int);
    double getF0ForKeV(const QString &, double v);
    double getF0ForKeV(int, double v);
    double getMacForKeV(const QString &, double v);
    double getMacForKeV(int, double v);

    double getF0ForWlNm(const QString &, double v);
    double getF0ForWlNm(int, double v);
    double getMacForWlNm(const QString &, double v);
    double getMacForWlNm(int, double v);

private:
    QMap<int, ElementScatteringData> _data;

    void initData();
    double interpolate(double, const QVector<double> &, const QVector<double> &);
};

#endif // ELEMENTSCATTERINGDATAMANAGER_H
