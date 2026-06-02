/***************************************************************************
                          peaklistfilteritemdouble.cpp  -  description
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

#include "peaklistfilteritemdouble.h"

PeakListFilterItemDouble::PeakListFilterItemDouble(QTreeWidget *parent, const QString &header)
    : QTreeWidgetItem(parent)
{
    setData(0, Qt::EditRole, header);
    setFlags(flags() | Qt::ItemIsEditable);
    setCheckState(0, Qt::Unchecked);
}

double PeakListFilterItemDouble::getMin()
{
    return data(1, Qt::EditRole).toDouble();
}

double PeakListFilterItemDouble::getMax()
{
    return data(2, Qt::EditRole).toDouble();
}

bool PeakListFilterItemDouble::isChecked()
{
    return checkState(0) == Qt::Checked;
}
