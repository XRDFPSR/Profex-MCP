/***************************************************************************
                          bondlengthdata.h  -  description
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

#ifndef BONDLENGTHDATA_H
#define BONDLENGTHDATA_H

#include <QMap>
#include <QHash>
#include <QObject>

#include "bondlengthphase.h"

class BondLengthData : public QObject
{
    Q_OBJECT

public:
    BondLengthData();

    void setData(const QHash<QString, CrystalStructure> &, double);

    QStringList tableHeader(const QString &) const;
    QList<QStringList> tableData(const QString &) const;
    inline bool contains(const QString &s) const {return _data.contains(s);}
    int rowCount(const QString &phase) const;
    int columnCount(const QString &phase) const;

private:
    QHash<QString, BondLengthPhase> _data;

signals:
    void setProgress(int, QString);
};

#endif // BONDLENGTHDATA_H
