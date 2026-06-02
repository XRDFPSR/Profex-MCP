/***************************************************************************
                          bgmngeqdisplay.h  -  description
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

#ifndef GEQDISPLAY_H
#define GEQDISPLAY_H

#include <QObject>
#include <QtMath>
#include "bgmndatadisplay.h"
#include "../../../libXrdIO/settingsmanager.h"
#include "../../../libXrdIO/parser/bgmngeqdata.h"

enum GEQTYPE {GEQ_NATIVE, GEQ_DISPLAY};

class BgmnGeqDisplay : public BgmnDataDisplay
{
    Q_OBJECT

public:
    BgmnGeqDisplay(QWidget *parent = Q_NULLPTR);

    void loadFile(const QString &);
    int count() const;
    void displayCurve(int);
    void reload();
    void clearData();
    bool hasData();
    QVector<double> peakPositions();

private:
    QList<QVector<global::ProfileCurveData> > geqData;
    bool normY;
    double globalYmax;
    double stepSizeTwoTheta;

    QVector<double> getXdata(int curve = -1, int subcurve = -1) const;
    QVector<double> getYdata(int curve = -1, int subcurve = -1) const;

    bool checkRanges(int);
    void computeDisplayCurves();
    void appendScan(const QVector<double> &, const QVector<double> &, const QString &, bool fill = false);
    QString dataToCsv(int);
};

#endif // GEQDISPLAY_H
