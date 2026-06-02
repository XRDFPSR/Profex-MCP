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

#include "lutscale2d.h"
#include "math.h"
#include "../../libXrdIO/functions.h"
#include "../../libXrdIO/colorMaps/imageeffects.h"
#include <QPainter>
#include <QSet>
#include <QHash>
#include <QDebug>

LutScale2D::LutScale2D(QWidget *parent) :
    QWidget(parent)
{
    setMinimumWidth(fontMetrics().horizontalAdvance("aaa -mmmmm"));
    _minE = 0.0;
    _maxE = 1.0;
    _scale = 0;
    _gamma = 1.0;
}

void LutScale2D::setRange(double min, double max)
{
    _maxE = max;
    _minE = min;
    updateTicks();
    update();
}

void LutScale2D::setLut(const colorMaps::Lut &l)
{
    _lut = l;
    updateTicks();
    updateGradient();
    update();
}

void LutScale2D::setColorTemp(int i)
{
    _colTemp = i;
    updateTicks();
    updateGradient();
    update();
}

void LutScale2D::setScaling(int i)
{
    _scale = i;
    updateTicks();
    updateGradient();
    update();
}

void LutScale2D::setGamma(double g)
{
    _gamma = g;
    updateTicks();
    updateGradient();
    update();
}

void LutScale2D::updateAll()
{
    updateTicks();
    updateGradient();
    update();
}

void LutScale2D::paintEvent(QPaintEvent *)
{
    QPainter painter(this);

    if (gradient.isNull()) {
        painter.fillRect(this->rect(), Qt::white);
        return;
    }

    painter.drawImage(0, 0, gradient);

    if      (_scale == 0) drawTicksLin(painter);
    else if (_scale == 1) drawTicksSqrt(painter);
    else if (_scale == 2) drawTicksLog(painter);
}

void LutScale2D::resizeEvent(QResizeEvent *)
{
    updateGradient();
}

void LutScale2D::updateGradient()
{
    if ((width() <= 0) || (height() <= 0)) return;

    int w = fontMetrics().horizontalAdvance("aaa");
    QImage img(w, height(), QImage::Format_ARGB32);
    QPainter pxpainter(&img);

    int   bins      = _lut.hasNegativeRange ? _lut.data.size() / 2 : _lut.data.size();
    float binHeight = float(height()) / float(bins);
    int   lutOffset = 0;
    int   lutSign   = 1;

    if (_lut.hasNegativeRange) {
        lutOffset = bins - 1;
        if (_colTemp > 0) lutSign = -1;
    } else if (_colTemp > 0) {
        lutOffset = bins - 1;
        lutSign = -1;
    }

    int n = 0;
    for (int i = lutOffset; (i >= 0) && (i < _lut.data.size()); i += lutSign) {
        QColor c(_lut.data.at(i));
        pxpainter.fillRect(QRectF(0.0, float(height()) - (n * binHeight), float(w), -binHeight), QBrush(c));
        ++n;
    }

    pxpainter.end();

    if (_lut.contour) {
        if (_lut.type == 1) ImageEffects::contourLinesBlackOnColor(&img, _lut.contour);
        if (_lut.type == 2) ImageEffects::contourLinesBlackOnWhite(&img, _lut.contour);
        if (_lut.type == 3) ImageEffects::contourLinesColor(&img, _lut.contour);
    }

    gradient = img;
}

void LutScale2D::updateTicks()
{
    _minTicks = global::Functions::scaleAxis1(_minE, _maxE, 60);
    _majTicks = global::Functions::scaleAxis1(_minE, _maxE, 10);
}

void LutScale2D::drawTicksLin(QPainter &p)
{
    if (abs(_minE - _maxE) <= 1.0) return;

    QSet<int> minPos;
    QHash<int, QString> majPos;

    double miE = qFuzzyCompare(_gamma, 1.0) ? _minE : pow(_minE, _gamma);
    double maE = qFuzzyCompare(_gamma, 1.0) ? _maxE : pow(_maxE, _gamma);

    for (int i = 0; i < _minTicks.size(); ++i) {
        double ctE = qFuzzyCompare(_gamma, 1.0) ? _minTicks.at(i) : pow(_minTicks.at(i), _gamma);
        minPos.insert(int(0.5 + gradient.height() * (1.0 - (ctE - miE)/(maE - miE))));
    }

    for (int i = 0; i < _majTicks.size(); ++i) {
        double ctE = qFuzzyCompare(_gamma, 1.0) ? _majTicks.at(i) : pow(_majTicks.at(i), _gamma);
        int y = int(0.5 + gradient.height() * (1.0 - (ctE - miE)/(maE - miE)));
        QString s = QString("%1").arg(_majTicks.at(i), 6, 'f', 0);
        majPos.insert(y, s);
    }

    drawTicks(p, minPos, majPos);
}

void LutScale2D::drawTicksSqrt(QPainter &p)
{
    if (abs(_minE - _maxE) <= 1.0) return;

    QSet<int> minPos;
    QHash<int, QString> majPos;

    double miE = _minE < 0.0 ? 0.0 : sqrt(_minE);
    double maE = _maxE < 0.0 ? 0.0 : sqrt(_maxE);

    for (int i = 0; i < _minTicks.size(); ++i) {
        double ctE = _minTicks.at(i) < 0.0 ? 0.0 : sqrt(_minTicks.at(i));
        minPos.insert(int(0.5 + gradient.height() * (1.0 - (ctE - miE)/(maE - miE))));
    }

    for (int i = 0; i < _majTicks.size(); ++i) {
        double ctE = _majTicks.at(i) < 0.0 ? 0.0 : sqrt(_majTicks.at(i));
       int y = int(0.5 + gradient.height() * (1.0 - (ctE - miE)/(maE - miE)));
       QString s = QString("%1").arg(_majTicks.at(i), 6, 'f', 0);
       majPos.insert(y, s);
    }

    drawTicks(p, minPos, majPos);
}

void LutScale2D::drawTicksLog(QPainter &p)
{
    if (abs(_minE - _maxE) <= 1.0) return;

    QSet<int> minPos;
    QHash<int, QString> majPos;

    double miE = _minE < 1.0 ? 0.0 : log10(_minE);
    double maE = _maxE < 1.0 ? 0.0 : log10(_maxE);

    for (int i = 0; i < _minTicks.size(); ++i) {
        double ty = _minTicks.at(i) < 1.0 ? 0.0 : log10(_minTicks.at(i));
        minPos.insert(int(0.5 + gradient.height() * (1.0 - (ty - miE)/(maE - miE))));
    }

    for (int i = 0; i < _majTicks.size(); ++i) {
        double ty = _majTicks.at(i) < 1.0 ? 0.0 : log10(_majTicks.at(i));
        QString s = QString("%1").arg(_majTicks.at(i), 6, 'f', 0);
        majPos.insert(int(0.5 + gradient.height() * (1.0 - (ty - miE)/(maE - miE))), s);
    }

    drawTicks(p, minPos, majPos);
}

void LutScale2D::drawTicks(QPainter &p, const QSet<int> &minPos, const QHash<int, QString> &majPos)
{
    int tm = fontMetrics().horizontalAdvance("a");
    int x = gradient.width();
    QSetIterator<int> minIt(minPos);
    QHashIterator<int, QString> majIt(majPos);

    while (minIt.hasNext()) {
        int y = minIt.next();
        p.drawLine(x, y, x + tm, y);
    }

    while (majIt.hasNext()) {
        majIt.next();
        int y = majIt.key();
        p.drawLine(x, y, x + 2*tm, y);

        y += fontMetrics().ascent() / 2;
        if (y > gradient.height())           p.drawText(x + 3*tm, gradient.height(), majIt.value());
        else if (y > fontMetrics().ascent()) p.drawText(x + 3*tm, y, majIt.value());
    }
}
