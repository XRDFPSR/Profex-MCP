/***************************************************************************
                          peakfititem.h  -  description
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

#ifndef PEAKFITITEM_H
#define PEAKFITITEM_H

#include <QTreeWidgetItem>
#include <QUuid>

class PeakFitItem : public QTreeWidgetItem
{
public:
    PeakFitItem(const QUuid &u, int type = Type);

    inline void setName(const QString &s) {setText(0, s);}
    inline QUuid uid() const {return _uid;}
    inline QString name() const {return text(0);}

protected:
    QUuid _uid;
};

#endif // PEAKFITITEM_H
