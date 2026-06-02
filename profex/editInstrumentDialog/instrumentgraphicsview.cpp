/***************************************************************************
                          instrumentscenegraphicsview.cpp  -  description
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

#include "instrumentgraphicsview.h"
#include <QGuiApplication>

InstrumentGraphicsView::InstrumentGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    setBackgroundBrush(QBrush(QGuiApplication::palette().color(QPalette::Base)));
}

void InstrumentGraphicsView::resizeEvent(QResizeEvent *)
{
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}
