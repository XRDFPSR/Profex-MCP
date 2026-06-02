/***************************************************************************
                          ArrowLineItem.h  -  description
                             -------------------
    begin                : Wed Feb 12 18:00:00 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#ifndef ARROWLINEITEM_H
#define ARROWLINEITEM_H

#include <QGraphicsLineItem>
#include <QPainter>
#include <QtMath>  // for qSin(), qCos(), and M_PI

class ArrowLineItem : public QGraphicsLineItem {
public:
    // You can pass the parent if needed.
    ArrowLineItem(double x1, double y1, double x2, double y2, int s, const QPen &pen, QGraphicsItem *parent = nullptr);

    // Optionally, allow setting the arrow size.
    void setArrowSize(double size);

    // Override the paint method to draw both the line and the arrowhead.
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;

private:
    double arrowSize;
};


#endif // ARROWLINEITEM_H
