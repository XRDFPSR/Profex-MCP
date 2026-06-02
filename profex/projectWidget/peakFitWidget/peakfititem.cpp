/***************************************************************************
                          peakfititem.cpp  -  description
                             -------------------
    begin                : Tue Aug 09 12:58:00 CEST 2022
    copyright            : (C) 2022 by Nicola Doebelin
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

#include "peakfititem.h"

PeakFitItem::PeakFitItem(const QUuid &u, int type)
    : QTreeWidgetItem(type), _uid(u)
{

}
