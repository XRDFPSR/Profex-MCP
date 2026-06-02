/***************************************************************************
                          scantracergraphicsview.h  -  description
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

#ifndef SCANTRACERGRAPHICSVIEW_H
#define SCANTRACERGRAPHICSVIEW_H

#include <QGraphicsView>

class ScanTracerGraphicsView : public QGraphicsView
{
public:
    ScanTracerGraphicsView(QWidget *parent = nullptr);

    void fitScene();

private:
    void resizeEvent(QResizeEvent *);

};

#endif // SCANTRACERGRAPHICSVIEW_H
