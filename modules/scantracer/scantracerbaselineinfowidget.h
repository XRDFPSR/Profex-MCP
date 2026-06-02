/***************************************************************************
                          scantracerbaselineinfowidget.h  -  description
                             -------------------
    begin                : Wed June 29 19:00:00 CEST 2021
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

#ifndef SCANTRACERBASELINEINFOWIDGET_H
#define SCANTRACERBASELINEINFOWIDGET_H

#include <QWidget>
#include <QMap>

class ScanTracerBaseLineInfoWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ScanTracerBaseLineInfoWidget(QWidget *parent = nullptr);

    void setDisplayMode(int);

private:
    int _displayMode;
    int _space;
    int _vTitle;
    int _vHint;

    QString _strTitle;
    QString _strStart;
    QString _strPt1;
    QString _strPt2;
    QString _strPt3;
    QString _strNext;

    QString _arrBl;
    QString _arrBr;
    QString _arrTl;

    QMap<double, double> _dummyData;
    QMap<double, double> _dummyBl;

    void paintEvent(QPaintEvent *);

    QPolygonF dummyScanToScreenCoordinates(const QMap<double, double> &);
    QPolygonF dummyBaseLineToScreenCoordinates(const QMap<double, double> &);
};

#endif // SCANTRACERBASELINEINFOWIDGET_H
