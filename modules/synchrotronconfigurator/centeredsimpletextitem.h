/***************************************************************************
                          centeredsimpletextitem.h  -  description
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

#ifndef CENTEREDSIMPLETEXTITEM_H
#define CENTEREDSIMPLETEXTITEM_H

#include "QGraphicsSimpleTextItem"

class CenteredSimpleTextItem : public QGraphicsSimpleTextItem {
public:
    explicit CenteredSimpleTextItem(const QString &text, QGraphicsItem *parent = nullptr);

    // Return a bounding rect that is centered at (0,0)
    QRectF boundingRect() const override;

    // Translate the painter so that text is drawn centered at (0,0)
    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget = nullptr) override;
};

#endif // CENTEREDSIMPLETEXTITEM_H
