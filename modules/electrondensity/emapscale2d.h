/***************************************************************************
                          emapscale2d.h  -  description
                             -------------------
    begin                : Wed Sep 24 20:35:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#ifndef EMAPSCALE2D_H
#define EMAPSCALE2D_H

#include <QWidget>
#include "../libXrdIO/colorMaps/lutstructs.h"
#include "../libXrdIO/structs.h"
#include "emapstructs.h"

class EMapScale2D : public QWidget
{
    Q_OBJECT
public:
    explicit EMapScale2D(QWidget *parent = 0);

    void setRange(float, float);
    void setLut(const colorMaps::Lut &l);
    QImage getScale(int);

private:
    float minE, maxE;
    colorMaps::Lut lut;
    QImage gradient;

    static inline QString scaleWidthStr = QString("aaa");
    static inline QString unitStr = QString("[e%1/%2%3]").arg(global::superMinus).arg(global::angstrom).arg(global::superThree);

    int scaleWidth;
    int labelWidth;
    int totalWidth;

    QImage updateGradient(int h);
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void renderWidget(QPaintDevice *, const QImage &grad);
};

#endif // EMAPSCALE2D_H
