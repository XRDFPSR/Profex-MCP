/***************************************************************************
                          strfilefilter.h  -  description
                             -------------------
    begin                : Fri May 31 08:55:00 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#ifndef STRFILEFILTER_H
#define STRFILEFILTER_H

#include <QList>
#include <QString>
#include <QStringList>
#include "../libXrdIO/hklphasedata.h"

class StrFileFilter
{
public:
    StrFileFilter();

    static QList<HklPhaseData> eliminateUnindexed(const QList<HklPhaseData> &);
    static QList<HklPhaseData> eliminatePinnedFiles(const QList<HklPhaseData> &, const QStringList &);
    static QList<HklPhaseData> eliminateDuplicates(const QList<HklPhaseData> &, const QStringList &, double, double);

private:
    static bool compareStrFiles(const QString &, const QString &, double, double);
    static QMap<QString, QVariant> parseStr(const QString &);
};

#endif // STRFILEFILTER_H
