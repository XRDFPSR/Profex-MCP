/***************************************************************************
                          lutscale2d.h  -  description
                             -------------------
    begin                : Wed Jul 28 20:35:00 CEST 2021
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

#ifndef LUTSCALE2D_H
#define LUTSCALE2D_H

#include <QWidget>
#include "../../libXrdIO/colorMaps/lutstructs.h"

class LutScale2D : public QWidget
{
    Q_OBJECT
public:
    explicit LutScale2D(QWidget *parent = 0);

    void setRange(double, double);
    void setLut(const colorMaps::Lut &l);
    void setColorTemp(int);
    void setScaling(int);
    void setGamma(double);
    void updateAll();

private:
    double _minE, _maxE;
    colorMaps::Lut _lut;
    QImage gradient;
    int _colTemp;
    int _scale;
    double _gamma;
    QList<double> _minTicks;
    QList<double> _majTicks;

    void updateTicks();
    void updateGradient();
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void drawTicksLin(QPainter &);
    void drawTicksSqrt(QPainter &);
    void drawTicksLog(QPainter &);
    void drawTicks(QPainter &, const QSet<int> &, const QHash<int, QString> &);
};

#endif // LUTSCALE2D_H
