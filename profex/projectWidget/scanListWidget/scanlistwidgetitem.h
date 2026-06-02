/***************************************************************************
                          scanlistwidgetitem.h  -  description
                             -------------------
    begin                : Thu May 09 12:27:00 CEST 2022
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

#ifndef SCANLISTWIDGETITEM_H
#define SCANLISTWIDGETITEM_H

#include <QTreeWidgetItem>

static const QString scanActiveText("A");
static const QString scanTempText("T");

class ScanListWidgetItem : public QTreeWidgetItem
{
public:
    ScanListWidgetItem(QTreeWidget *parent, int type = 0);
    ScanListWidgetItem(const QStringList &strings, int type = 0);

    void setItemStatus(bool isVisible, bool isActive, bool tempScan);
    void setUid(const QUuid &uid);
    void setName(const QString &);
    void setScaleFactor(double);
    void setXoffset(double);
    void setYoffset(double);
    void setColor(const QColor &c, bool darkMode);

    bool isVisible() const;
    bool isActive() const;
    QUuid getUid() const;
    QString getName() const;

private:
    bool scanIsActive;
};

#endif // SCANLISTWIDGETITEM_H
