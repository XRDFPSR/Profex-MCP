/***************************************************************************
                          hklsortfilterproxymodel.h  -  description
                             -------------------
    begin                : Thu Mar 23 20:00:00 CEST 2023
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

#ifndef HKLSORTFILTERPROXYMODEL_H
#define HKLSORTFILTERPROXYMODEL_H

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QObject>

class HklSortFilterProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT

public:
    HklSortFilterProxyModel(QObject *parent = nullptr);

    void setFilters(const QMap<QString, QVariant> &, const QString &);
    inline void setColumnIndices(const QHash<QString, int> &h) {_colIdx = h;}

protected:
    bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

    QRegularExpression _phaseFilter;
    int _hmin, _hmax, _kmin, _kmax, _lmin, _lmax;
    double _ttmin, _ttmax, _dmin, _dmax;
    double _intAbsMin, _intAbsMax, _intRelMin, _intRelMax;
    double _texMin, _texMax, _b1Min, _b1Max, _b2Min, _b2Max;
    QHash<QString, int> _colIdx;
};

#endif // HKLSORTFILTERPROXYMODEL_H
