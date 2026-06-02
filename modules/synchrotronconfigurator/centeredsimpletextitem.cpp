/***************************************************************************
                          centeredsimpletextitem.cpp  -  description
                             -------------------
    begin                : Thu Feb 13 18:00:00 CEST 2025
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

#include "centeredsimpletextitem.h"
#include <QPainter>

CenteredSimpleTextItem::CenteredSimpleTextItem(const QString &text, QGraphicsItem *parent)
    : QGraphicsSimpleTextItem(text, parent)
{
    setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
}

QRectF CenteredSimpleTextItem::boundingRect() const
{
    QRectF rect = QGraphicsSimpleTextItem::boundingRect();
    rect.moveCenter(QPointF(0, 0));
    return rect;
}

void CenteredSimpleTextItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget)
{
    painter->save();
    // Shift drawing so that the original bounding rect's center is at (0,0)
    painter->translate(-QGraphicsSimpleTextItem::boundingRect().center());
    QGraphicsSimpleTextItem::paint(painter, option, widget);
    painter->restore();
}
