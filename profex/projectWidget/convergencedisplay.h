/***************************************************************************
                          convergencedisplay.h  -  description
                             -------------------
    begin                : Wed Mar 02 20:00:00 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#ifndef CONVERGENCEDISPLAY_H
#define CONVERGENCEDISPLAY_H

#include "3rdparty/qcustomplot/qcustomplot.h"
#include "../libXrdIO/functions.h"
#include "../libXrdIO/settingsmanager.h"

#include <QRegularExpression>
#include <QVector>
#include <QMenu>
#include <QContextMenuEvent>
#include <QAction>

class ConvergenceDisplay : public QCustomPlot
{
    Q_OBJECT
public:
    ConvergenceDisplay(QWidget *parent = Q_NULLPTR);
    ~ConvergenceDisplay();

    void setValues(double, double, double);
//    void appendBgmnOutput(const QString &);
    void setRexpDenom(int, double);
    void clear();

private:
    SettingsManager *settings;
    QRegularExpression rxBgmnFirst;
    QRegularExpression rxBgmn;
    QRegularExpression rxBgmnP;

    QVector<double> rXdata;
    QVector<double> rwpYdata;
    QVector<double> rexpYdata;
    QVector<double> chi2Ydata;

    QAction *actionLegendVisible;
    QAction *actionSaveAscii;
    QAction *actionSavePdf;

    double bgmnFirstValue;
    double chi2accept;
    double yZoom;
    int    lineWidth;

    void setBgmnFirstValue(double);
    void setBgmnValue(double, double, double);
    double zoomedYmax();
    double zoomedYmin();

    QString dataToAscii();
    void contextMenuEvent(QContextMenuEvent *);

private slots:
    void exportAscii();
    void exportPdf();
    void toggleLegend();
    void mousePressEvent(QMouseEvent *event);
    void wheelEvent(QWheelEvent *event);
};

#endif // CONVERGENCEDISPLAY_H
