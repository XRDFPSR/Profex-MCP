/***************************************************************************
                          opticsitem.cpp  -  description
                             -------------------
    begin                : Tue Jul 16 18:00:00 CEST 2020
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

#include "opticsitem.h"
#include <QtMath>
#include <QGraphicsScene>
#include <QSvgRenderer>
#include <QGraphicsDropShadowEffect>
#include <QDebug>

OpticsItem::OpticsItem(const QString &fa, const QString &fi, const QString &m, QGraphicsItem *parent)
    : QGraphicsSvgItem(fi, parent), _activeFile(fa), _inactiveFile(fi), _module(m)
{
    setFlag(QGraphicsItem::ItemIsSelectable, true);
    _isHighlighted = false;

    // for debugging / manual placing of elements
    // setFlag(QGraphicsItem::ItemIsMovable, true);
    // connect(this, SIGNAL(xChanged()), this, SLOT(posChanged()));
    // connect(this, SIGNAL(yChanged()), this, SLOT(posChanged()));
    // connect(this, SIGNAL(zChanged()), this, SLOT(posChanged()));
}

void OpticsItem::setInstalled(bool b)
{
    setVisible(b);
}

void OpticsItem::setHighlighted(bool b)
{
    _isHighlighted = b;
    if (b) renderer()->load(_activeFile);
    else   renderer()->load(_inactiveFile);

    if (b) {
        QGraphicsDropShadowEffect *e = new QGraphicsDropShadowEffect();

        if (_darkMode) e->setColor(QColor(255, 255, 255, 180));

        e->setBlurRadius(scene()->itemsBoundingRect().width() / 200.0);
        e->setOffset(0.0);
        setGraphicsEffect(e);
    } else {
        setGraphicsEffect(nullptr);
    }
}

void OpticsItem::loadShape(const QString &s)
{
    renderer()->load(s);
}

void OpticsItem::posChanged()
{
    qDebug() << QString("x=%1  y=%2").arg(pos().x()).arg(pos().y());
}
