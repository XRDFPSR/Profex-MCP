/***************************************************************************
                          bondlengthdata.cpp  -  description
                             -------------------
    begin                : Fri Apr 26 15:24:00 CEST 2024
    copyright            : (C) 2024 by Nicola Doebelin
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

#include "bondlengthdata.h"

BondLengthData::BondLengthData() : QObject()
{

}

void BondLengthData::setData(const QHash<QString, CrystalStructure> &data, double filter)
{
    _data.clear();

    int n = 1;

    for (auto it = data.cbegin(); it != data.cend(); ++it) {
        BondLengthPhase b(it.value(), filter);
        _data[it.key()] = b;

        emit setProgress(n, QString("Calculating bond lengths for %1").arg(it.key()));
        ++n;
    }
}

QStringList BondLengthData::tableHeader(const QString &phase) const
{
    if (!_data.contains(phase)) return QStringList();
    return _data.value(phase).getHeaderLabels();
}

QList<QStringList> BondLengthData::tableData(const QString &phase) const
{
    if (!_data.contains(phase)) return QList<QStringList>();

    QList<QList<double> > tabValues = _data.value(phase).getBondLengths();
    QList<QStringList> tabStrings(tabValues.size(), QStringList(tabValues.size(), ""));

    for (int r = 0; r < tabValues.size(); ++r) {
        for (int c = 0; c < tabValues.size(); ++c) {
            double v = tabValues.at(r).at(c);
            // negative values are skipped (empty table cells)
            if (v < -0.5) continue;

            // rounding errors can accumulate. Clip at 0.001
            tabStrings[r][c] = QString("%1").arg(v < 0.001 ? 0.0 : v, 0, 'f', 4);
        }
    }

    return tabStrings;
}

int BondLengthData::rowCount(const QString &phase) const
{
    if (_data.contains(phase)) return _data.value(phase).rowCount();
    return 0;
}

int BondLengthData::columnCount(const QString &phase) const
{
    if (_data.contains(phase)) return _data.value(phase).columnCount();
    return 0;
}

