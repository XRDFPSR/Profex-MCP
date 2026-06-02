/***************************************************************************
                          peaklistfilteritemhkl.cpp  -  description
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

#include "peaklistfilteritemhkl.h"

PeakListFilterItemHkl::PeakListFilterItemHkl(QTreeWidget *parent, const QString &header)
    : QTreeWidgetItem(parent)
{
    setData(0, Qt::EditRole, header);
    setFlags(flags() | Qt::ItemIsEditable);
    setCheckState(0, Qt::Unchecked);
}

int PeakListFilterItemHkl::getMin()
{
    return data(1, Qt::EditRole).toInt();
}

int PeakListFilterItemHkl::getMax()
{
    return data(2, Qt::EditRole).toInt();
}

bool PeakListFilterItemHkl::isChecked()
{
    return checkState(0) == Qt::Checked;
}
