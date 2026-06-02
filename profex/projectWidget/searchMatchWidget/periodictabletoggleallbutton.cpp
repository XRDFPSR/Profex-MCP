/***************************************************************************
                          periodictabletogglegroupall.cpp  -  description
                             -------------------
    begin                : Tue Aug 04 11:05:00 CEST 2020
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

#include "periodictabletoggleallbutton.h"

#include <QStyleFactory>

PeriodicTableToggleAllButton::PeriodicTableToggleAllButton(int type, QWidget *parent)
    :QToolButton(parent)
{
    setStyle(QStyleFactory::create("fusion"));

    QPalette pal = palette();

    if      (type == 1) pal.setColor(QPalette::Button, QColor("#7eeb86"));
    else if (type == 2) pal.setColor(QPalette::Button, QColor("#7ec3eb"));
    else if (type == 3) pal.setColor(QPalette::Button, QColor("#eb7e7e"));
    else if (type == 4) pal.setColor(QPalette::ButtonText, QColor("#eb7e7e"));

    setPalette(pal);
}
