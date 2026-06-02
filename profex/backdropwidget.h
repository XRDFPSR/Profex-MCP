/***************************************************************************
                          backdropwidget.h  -  description
                             -------------------
    begin                : Tue Jul 14 18:00:00 CEST 2020
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

#ifndef BACKDROPWIDGET_H
#define BACKDROPWIDGET_H

#include <QtSvg>
#include <QWidget>

class BackdropWidget : public QWidget
{
    Q_OBJECT
public:
    explicit BackdropWidget(QWidget *parent = nullptr);

private:
    QSvgRenderer *svgRenderer;

    void paintEvent(QPaintEvent *);
};

#endif // BACKDROPWIDGET_H
