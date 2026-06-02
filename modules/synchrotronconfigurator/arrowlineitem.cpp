/***************************************************************************
                          ArrowLineItem.cpp  -  description
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

#include "arrowlineitem.h"

ArrowLineItem::ArrowLineItem(double x1, double y1, double x2, double y2, int s, const QPen &pen, QGraphicsItem *parent)
    : QGraphicsLineItem(x1, y1, x2, y2, parent), arrowSize(s)
{
    // Set a default pen (customize as needed)
    setPen(pen);
}

// Optionally, allow setting the arrow size.
void ArrowLineItem::setArrowSize(double size)
{
    arrowSize = size;
}

// Override the paint method to draw both the line and the arrowhead.
void ArrowLineItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    // Draw the dashed line using the base class implementation.
    QGraphicsLineItem::paint(painter, option, widget);

    QLineF line = this->line();
    if (qFuzzyCompare(line.length(), 0))
        return;  // Nothing to do for a zero-length line.

    // Compute the unit direction vector from p1 to p2.
    QPointF diff = line.p2() - line.p1();
    qreal lineLength = std::hypot(diff.x(), diff.y());
    QPointF unitVector = diff / lineLength;

    // Define the arrowhead offset angle in radians (e.g., 30°).
    const double offsetAngle = M_PI / 6;

    // Helper lambda to rotate a vector by a given angle.
    auto rotateVector = [](const QPointF &vec, double angle) -> QPointF {
        return QPointF(vec.x() * cos(angle) - vec.y() * sin(angle),
                       vec.x() * sin(angle) + vec.y() * cos(angle));
    };

    // Rotate the unit vector by ±offsetAngle.
    QPointF rotatedPlus = rotateVector(unitVector,  offsetAngle);
    QPointF rotatedMinus = rotateVector(unitVector, -offsetAngle);

    // Compute the two arrowhead points by subtracting the scaled rotated vectors
    // from the endpoint. This places the arrowhead “behind” the endpoint.
    QPointF arrowP1 = line.p2() - rotatedPlus * arrowSize;
    QPointF arrowP2 = line.p2() - rotatedMinus * arrowSize;

    // Build the arrowhead polygon.
    QPolygonF arrowHead;
    arrowHead << line.p2() << arrowP1 << arrowP2;

    // Save the painter state.
    painter->save();

    // Set a solid pen (and brush) for the arrowhead.
    QPen solidPen = pen();
    solidPen.setStyle(Qt::SolidLine);
    painter->setPen(solidPen);
    painter->setBrush(solidPen.color());

    // Draw the arrowhead.
    painter->drawPolygon(arrowHead);

    // Restore the painter state.
    painter->restore();
}
