/***************************************************************************
                          hklsortfilterproxymodel.cpp  -  description
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

#include "hklsortfilterproxymodel.h"
#include <limits>

HklSortFilterProxyModel::HklSortFilterProxyModel(QObject *parent)
    : QSortFilterProxyModel(parent)
{
    _hmin = std::numeric_limits<int>::min();
    _hmax = std::numeric_limits<int>::max();
    _kmin = std::numeric_limits<int>::min();
    _kmax = std::numeric_limits<int>::max();
    _lmin = std::numeric_limits<int>::min();
    _lmax = std::numeric_limits<int>::max();
    _ttmin = std::numeric_limits<double>::lowest();
    _ttmax = std::numeric_limits<double>::max();
    _dmin = std::numeric_limits<double>::lowest();
    _dmax = std::numeric_limits<double>::max();
    _intAbsMin = 0.0;
    _intAbsMax = std::numeric_limits<double>::max();
    _intRelMin = 0.0;
    _intRelMax = std::numeric_limits<double>::max();
    _texMin = 0.0;
    _texMax = std::numeric_limits<double>::max();
    _b1Min  = 0.0;
    _b1Max  = std::numeric_limits<double>::max();
    _b2Min  = 0.0;
    _b2Max  = std::numeric_limits<double>::max();
}

void HklSortFilterProxyModel::setFilters(const QMap<QString, QVariant> &par, const QString &phas)
{
    _phaseFilter.setPattern(phas);

    if (par.contains("hlow")) {
        _hmin = par.value("hlow").toInt();
        _hmax = par.value("hhigh").toInt();
    } else {
        _hmin = std::numeric_limits<int>::min();
        _hmax = std::numeric_limits<int>::max();
    }

    if (par.contains("klow")) {
        _kmin = par.value("klow").toInt();
        _kmax = par.value("khigh").toInt();
    } else {
        _kmin = std::numeric_limits<int>::min();
        _kmax = std::numeric_limits<int>::max();
    }

    if (par.contains("llow")) {
        _lmin = par.value("llow").toInt();
        _lmax = par.value("lhigh").toInt();
    } else {
        _lmin = std::numeric_limits<int>::min();
        _lmax = std::numeric_limits<int>::max();
    }

    if (par.contains("2tmin")) {
        _ttmin = par.value("2tmin").toDouble();
        _ttmax = par.value("2tmax").toDouble();
    } else {
        _ttmin = 0.0;
        _ttmax = std::numeric_limits<double>::max();
    }

    if (par.contains("dmin")) {
        _dmin = par.value("dmin").toDouble();
        _dmax = par.value("dmax").toDouble();
    } else {
        _dmin = 0.0;
        _dmax = std::numeric_limits<double>::max();
    }

    if (par.contains("iabsmin")) {
        _intAbsMin = par.value("iabsmin").toDouble();
        _intAbsMax = par.value("iabsmax").toDouble();
    } else {
        _intAbsMin = 0.0;
        _intAbsMax = std::numeric_limits<double>::max();
    }

    if (par.contains("irelmin")) {
        _intRelMin = par.value("irelmin").toDouble();
        _intRelMax = par.value("irelmax").toDouble();
    } else {
        _intRelMin = 0.0;
        _intRelMax = std::numeric_limits<double>::max();
    }

    if (par.contains("texmin")) {
        _texMin = par.value("texmin").toDouble();
        _texMax = par.value("texmax").toDouble();
    } else {
        _texMin = 0.0;
        _texMax = std::numeric_limits<double>::max();
    }

    if (par.contains("b1min")) {
        _b1Min = par.value("b1min").toDouble();
        _b1Max = par.value("b1max").toDouble();
    } else {
        _b1Min = 0.0;
        _b1Max = std::numeric_limits<double>::max();
    }

    if (par.contains("b2min")) {
        _b2Min = par.value("b2min").toDouble();
        _b2Max = par.value("b2max").toDouble();
    } else {
        _b2Min = 0.0;
        _b2Max = std::numeric_limits<double>::max();
    }

    invalidateFilter();
}

bool HklSortFilterProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    QModelIndex indexPhase = sourceModel()->index(sourceRow, 0, sourceParent);
    QRegularExpressionMatch rmP = _phaseFilter.match(sourceModel()->data(indexPhase, Qt::EditRole).toString());

    if (!rmP.hasMatch()) return false;

    QModelIndex indexH = sourceModel()->index(sourceRow, _colIdx.value("h"), sourceParent);
    if (sourceModel()->data(indexH, Qt::EditRole).toInt() < _hmin) return false;
    if (sourceModel()->data(indexH, Qt::EditRole).toInt() > _hmax) return false;

    QModelIndex indexK = sourceModel()->index(sourceRow, _colIdx.value("k"), sourceParent);
    if (sourceModel()->data(indexK, Qt::EditRole).toInt() < _kmin) return false;
    if (sourceModel()->data(indexK, Qt::EditRole).toInt() > _kmax) return false;

    QModelIndex indexL = sourceModel()->index(sourceRow, _colIdx.value("l"), sourceParent);
    if (sourceModel()->data(indexL, Qt::EditRole).toInt() < _lmin) return false;
    if (sourceModel()->data(indexL, Qt::EditRole).toInt() > _lmax) return false;

    QModelIndex indexTT = sourceModel()->index(sourceRow, _colIdx.value("2theta"), sourceParent);
    if (sourceModel()->data(indexTT, Qt::EditRole).toDouble() < _ttmin) return false;
    if (sourceModel()->data(indexTT, Qt::EditRole).toDouble() > _ttmax) return false;

    QModelIndex indexD = sourceModel()->index(sourceRow, _colIdx.value("dnm"), sourceParent);
    if (sourceModel()->data(indexD, Qt::EditRole).toDouble() < _dmin) return false;
    if (sourceModel()->data(indexD, Qt::EditRole).toDouble() > _dmax) return false;

    QModelIndex indexInt = sourceModel()->index(sourceRow, _colIdx.value("intensity"), sourceParent);
    if (sourceModel()->data(indexInt, Qt::EditRole).toDouble() < _intAbsMin) return false;
    if (sourceModel()->data(indexInt, Qt::EditRole).toDouble() > _intAbsMax) return false;

    QModelIndex indexIntR = sourceModel()->index(sourceRow, _colIdx.value("intensrel"), sourceParent);
    if (sourceModel()->data(indexIntR, Qt::EditRole).toDouble() < _intRelMin) return false;
    if (sourceModel()->data(indexIntR, Qt::EditRole).toDouble() > _intRelMax) return false;

    QModelIndex indexTex = sourceModel()->index(sourceRow, _colIdx.value("texture"), sourceParent);
    if (sourceModel()->data(indexTex, Qt::EditRole).toDouble() < _texMin) return false;
    if (sourceModel()->data(indexTex, Qt::EditRole).toDouble() > _texMax) return false;

    QModelIndex indexB1 = sourceModel()->index(sourceRow, _colIdx.value("b1"), sourceParent);
    if (sourceModel()->data(indexB1, Qt::EditRole).toDouble() < _b1Min) return false;
    if (sourceModel()->data(indexB1, Qt::EditRole).toDouble() > _b1Max) return false;

    QModelIndex indexB2 = sourceModel()->index(sourceRow, _colIdx.value("b2"), sourceParent);
    if (sourceModel()->data(indexB2, Qt::EditRole).toDouble() < _b2Min) return false;
    if (sourceModel()->data(indexB2, Qt::EditRole).toDouble() > _b2Max) return false;

    return true;
}
