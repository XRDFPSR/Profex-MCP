/***************************************************************************
                          scantracergraphicsview.cpp  -  description
                             -------------------
    begin                : Wed May 12 19:00:00 CEST 2021
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

#include "scantracergraphicsview.h"
#include <QGuiApplication>


ScanTracerGraphicsView::ScanTracerGraphicsView(QWidget *parent)
    : QGraphicsView(parent)
{
    setBackgroundBrush(QBrush(QGuiApplication::palette().color(QPalette::Base)));
    setMouseTracking(true);
}

void ScanTracerGraphicsView::resizeEvent(QResizeEvent *)
{
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}

void ScanTracerGraphicsView::fitScene()
{
    fitInView(scene()->sceneRect(), Qt::KeepAspectRatio);
}
