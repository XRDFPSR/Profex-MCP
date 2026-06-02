/***************************************************************************
                          bgmnconvolutiondisplay.h  -  description
                             -------------------
    begin                : Fri Feb 09 17:10:00 CEST 2018
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

#ifndef CONVOLUTIONDISPLAY_H
#define CONVOLUTIONDISPLAY_H

#include <QObject>
#include <QtMath>
#include "bgmndatadisplay.h"

class BgmnConvolutionDisplay : public BgmnDataDisplay
{
    Q_OBJECT

public:
    BgmnConvolutionDisplay(QWidget *parent = Q_NULLPTR);

    inline void setLamFile(const QString &s) {currentLamFile = s;}
    inline void setGeqFile(const QString &s) {currentGeqFile = s;}

    void reload();
    void clearData();
    int count(int) const;
    void displayCurve(int);
    bool hasData();

    QString dataToCsv(int);

    inline void setB1(double d)    {b1 = d;}
    inline void setk1(double d)    {k1 = d;}
    inline void setk2(double d)    {k2 = d;}

private:
    QString currentLamFile;
    QString currentGeqFile;
    double stepSizeTwoTheta;
    double b1, k1, k2, twoTheta;
    QVector<double> dvalues;

    QList<global::ProfileCurveData> lamData;
    QList<global::ProfileCurveData> geqData;
    QList<global::ProfileCurveData> convData;
    QList<global::ProfileCurveData> sampData;

    void setDvals(QVector<double>, double);

    void addData(QList<global::ProfileCurveData> &m, const QVector<double> &x, const QVector<double> &y, double ymax);

    QVector<double> getXdata(int n, int c) const;
    QVector<double> getYdata(int n, int c) const;

    void computeDisplayCurves();
    global::ProfileCurveData convolute(const QVector<double> &gx, const QVector<double> &gy, const QVector<double> &lx, const QVector<double> &ly);
    bool checkRanges(int);

signals:
    void setProgress(int);
};

#endif // CONVOLUTIONDISPLAY_H
