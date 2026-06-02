/***************************************************************************
                          emapscale2d.cpp  -  description
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

#include "emapscale2d.h"
#include "math.h"
#include "../libXrdIO/colorMaps/imageeffects.h"
#include <QPainter>

EMapScale2D::EMapScale2D(QWidget *parent) :
    QWidget(parent)
{
    minE = -1.0;
    maxE = 1.0;
    scaleWidth = fontMetrics().horizontalAdvance(scaleWidthStr);
    labelWidth = fontMetrics().horizontalAdvance("0" + unitStr);
    totalWidth = scaleWidth + labelWidth;
    setMinimumWidth(totalWidth);
}

void EMapScale2D::setRange(float min, float max)
{
    maxE = qMax(fabs(max), fabs(min));
    minE = -maxE;
}

void EMapScale2D::setLut(const colorMaps::Lut &l)
{
    lut = l;
    gradient = updateGradient(height());
    update();
}

QImage EMapScale2D::getScale(int h)
{
    QImage tGrad = updateGradient(h);
    QImage img(totalWidth, tGrad.height(), QImage::Format_ARGB32);
    img.fill(QColor(0, 0, 0, 0));
    renderWidget(&img, tGrad);
    return img;
}

void EMapScale2D::paintEvent(QPaintEvent *)
{
    renderWidget(this, gradient);
}

void EMapScale2D::renderWidget(QPaintDevice *p, const QImage &grad)
{
    QPainter painter(p);

    if (grad.isNull()) return;

    QString _maxVal = QString("%1").arg(maxE, 0, 'f', 2);
    QString _midVal("0.00");
    QString _minVal = QString("%1").arg(minE, 0, 'f', 2);

    int _xMax  = totalWidth - fontMetrics().horizontalAdvance(_maxVal);
    int _xMid  = totalWidth - fontMetrics().horizontalAdvance(_midVal);
    int _xMin  = totalWidth - fontMetrics().horizontalAdvance(_minVal);
    int _xUnit = totalWidth - fontMetrics().horizontalAdvance(unitStr);

    int _yMax  = fontMetrics().ascent();
    int _yMid  = (p->height() + fontMetrics().ascent())/2;
    int _yMin  = p->height();
    int _yUnit = (p->height() + fontMetrics().ascent())/4;

    painter.drawImage(0, 0, grad);
    painter.drawText(_xMax, _yMax, _maxVal);
    painter.drawText(_xUnit, _yUnit, unitStr);
    painter.drawText(_xMid, _yMid, _midVal);
    painter.drawText(_xMin, _yMin, _minVal);
}

void EMapScale2D::resizeEvent(QResizeEvent *)
{
    gradient = updateGradient(height());
}

QImage EMapScale2D::updateGradient(int h)
{
    if (h <= 0) return QImage();

    float f = float(h) / float(lut.data.size());
    QImage img(scaleWidth, h, QImage::Format_ARGB32);
    QPainter pxpainter(&img);

    for (int i = 0; i < lut.data.size(); ++i) {
        pxpainter.fillRect(QRectF(0.0, float(h) - (i * f), float(scaleWidth), -f), QBrush(QColor(lut.data.at(i))));
    }

    pxpainter.end();

    if (lut.contour) {
        if (lut.type == 1) ImageEffects::contourLinesBlackOnColor(&img, lut.contour);
        if (lut.type == 2) ImageEffects::contourLinesBlackOnWhite(&img, lut.contour);
        if (lut.type == 3) ImageEffects::contourLinesColor(&img, lut.contour);
    }

    return img;
}
