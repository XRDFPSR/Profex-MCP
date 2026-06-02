/***************************************************************************
                          bgmnlamdisplay.h  -  description
                             -------------------
    begin                : Tue Feb 08 21:10:00 CEST 2018
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

#ifndef LAMDISPLAY_H
#define LAMDISPLAY_H

#include <QObject>
#include <QtMath>
#include "bgmndatadisplay.h"
#include "../../../libXrdIO/parser/bgmnlamdata.h"

enum LAMTYPE {LAM_NATIVE, LAM_ANGSTROM, LAM_TWOTHETA};

class BgmnLamDisplay : public BgmnDataDisplay
{
    Q_OBJECT

public:
    BgmnLamDisplay(QWidget *parent = Q_NULLPTR);

    void loadFile(const QString &);
    void reload();
    void clearData();
    void displayCurve(int);
    bool hasData();

    double getWavelength() const;
    inline int count() const       {return lamData.count();}

private:
    BgmnLamData lamDataObject;
    QList<global::ProfileCurveData> lamData;
    double stepSizeLambda;

    void computeDisplayCurves();
    void appendScan(const QVector<double> &, const QVector<double> &, const QString &, bool fill = false);
    QVector<double> getXdata(int) const;
    QVector<double> getYdata(int) const;
    QString dataToCsv(int);
    bool checkRanges(int);
};

#endif // LAMDISPLAY_H
