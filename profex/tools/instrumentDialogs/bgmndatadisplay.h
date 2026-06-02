/***************************************************************************
                          bgmndatadisplay.h  -  description
                             -------------------
    begin                : Wed May 23 21:10:00 CEST 2018
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

#ifndef BGMNDATADISPLAY_H
#define BGMNDATADISPLAY_H

#include <QWidget>
#include "3rdparty/qcustomplot/qcustomplot.h"
#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/structs.h"
#include <algorithm>


class BgmnDataDisplay : public QCustomPlot
{
    Q_OBJECT

public:
    BgmnDataDisplay(QWidget *parent = Q_NULLPTR);

    virtual void loadFile(const QString &) { /* subclass if needed */ }
    virtual void reload() = 0;
    virtual void clearData() = 0;

    virtual void toggleSubCurves(bool);
    virtual void displayCurve(int) = 0;
    virtual bool hasData() = 0;

    void setTitle(const QString &);

    QString exportCsv(int);
    void exportPdf(const QString &);
    void exportPng(const QString &, int, int);

protected:
    SettingsManager *settings;
    bool zoomed;
    int currentCurve;
    double xmin;
    double xmax;
    bool showSubCurves;
    QString currentFileName;

    virtual bool checkRanges(int) = 0;
    virtual QString dataToCsv(int) = 0;
    QVector<double> xValues(double, double, double);
    QString valuesToString(const QVector<QVector<double> > &);
    void offsetXvalues(QVector<double> &, double);

    inline double min(const QVector<double> &v) {return *std::min_element(v.constBegin(), v.constEnd());}
    inline double max(const QVector<double> &v) {return *std::max_element(v.constBegin(), v.constEnd());}

protected slots:
    void zoomX(const QCPRange &);
    void zoomReset(QMouseEvent *);
};

#endif // BGMNDATADISPLAY_H
