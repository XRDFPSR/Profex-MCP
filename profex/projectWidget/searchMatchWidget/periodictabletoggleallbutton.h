/***************************************************************************
                          periodictabletogglegroupall.h  -  description
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

#ifndef PERIODICTABLETOGGLEALLBUTTON_H
#define PERIODICTABLETOGGLEALLBUTTON_H

#include <QToolButton>

class PeriodicTableToggleAllButton : public QToolButton
{
    Q_OBJECT
public:
    PeriodicTableToggleAllButton(int type, QWidget *parent = nullptr);
};

#endif // PERIODICTABLETOGGLEALLBUTTON_H
