/***************************************************************************
                          instrumentscenegraphicsview.h  -  description
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

#ifndef INSTRUMENTGRAPHICSVIEW_H
#define INSTRUMENTGRAPHICSVIEW_H

#include <QGraphicsView>

class InstrumentGraphicsView : public QGraphicsView
{
public:
    InstrumentGraphicsView(QWidget *parent = nullptr);

private:
    void resizeEvent(QResizeEvent *);
};

#endif // INSTRUMENTGRAPHICSVIEW_H
