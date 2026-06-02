/***************************************************************************
                          bgmnsampledisplay.h  -  description
                             -------------------
    begin                : Wed May 23 21:35:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#ifndef BGMNSAMPLEDISPLAY_H
#define BGMNSAMPLEDISPLAY_H

#include <QObject>
#include <QWidget>
#include "bgmndatadisplay.h"
#include "../../../libXrdIO/parser/bgmnsampledata.h"

class BgmnSampleDisplay : public BgmnDataDisplay
{
    Q_OBJECT

public:
    BgmnSampleDisplay(QWidget *parent = 0);

    void reload();
    void displayCurve(int);
    void clearData();
    bool hasData();

    inline void setB1(double d)    {b1 = d;}
    inline void setk1(double d)    {k1 = d;}
    inline void setk2(double d)    {k2 = d;}
    void setPeakPositionsTwoTheta(QVector<double>, double);

    void computeDisplayCurves();

private:
    BgmnSampleData sampDataObject;
    QList<QVector<global::ProfileCurveData> > sampData;
    double waveLength;
    double b1, k1, k2;
    QVector<double> dInvValues;

    QVector<double> getXdata(int curve = -1, int contribution = -1) const;
    QVector<double> getYdata(int curve = -1, int contribution = -1) const;

    bool checkRanges(int);
    QString dataToCsv(int);
};

#endif // BGMNSAMPLEDISPLAY_H
