/***************************************************************************
                          backdropwidget.cpp  -  description
                             -------------------
    begin                : Tue Jul 14 18:00:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#include "backdropwidget.h"

BackdropWidget::BackdropWidget(QWidget *parent) : QWidget(parent)
{
    svgRenderer = new QSvgRenderer(QString(":/icons/profex5-5.6-backdrop.svg"), this);
}

void BackdropWidget::paintEvent(QPaintEvent *)
{
    float l = float(height()) / 2.0;
    float x = 0.5 * (float(width()) - l);
    float y = 0.5 * (float(height()) - l);
    QRectF r(x, y, l, l);

    QPainter p(this);
    svgRenderer->render(&p, r);
}
