/***************************************************************************
                          peaklistfilteritemhkl.h  -  description
                             -------------------
    begin                : Wed May 03 23:27:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#ifndef PEAKLISTFILTERITEMHKL_H
#define PEAKLISTFILTERITEMHKL_H

#include <QTreeWidgetItem>

class PeakListFilterItemHkl : public QTreeWidgetItem
{
public:
    PeakListFilterItemHkl(QTreeWidget *parent, const QString &header);

    int getMin();
    int getMax();
    bool isChecked();
};

#endif // PEAKLISTFILTERITEMHKL_H
