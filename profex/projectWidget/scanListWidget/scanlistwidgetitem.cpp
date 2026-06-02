/***************************************************************************
                          scanlistwidgetitem.cpp  -  description
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

#include "scanlistwidgetitem.h"
#include "../libXrdIO/functions.h"

ScanListWidgetItem::ScanListWidgetItem(QTreeWidget *parent, int type)
    : QTreeWidgetItem(parent, type)
{
    scanIsActive = false;

    setFlags(flags() | Qt::ItemIsEditable);
    setFlags(flags() & ~Qt::ItemIsDropEnabled);

    if (columnCount() > 0) {
        this->setCheckState(0, Qt::Checked);
        this->setToolTip(0, "Show/hide scan");
    }
}

ScanListWidgetItem::ScanListWidgetItem(const QStringList &strings, int type)
    : QTreeWidgetItem(strings, type)
{
    scanIsActive = false;

    setFlags(flags() | Qt::ItemIsEditable);
    setFlags(flags() & ~Qt::ItemIsDropEnabled);

    if (columnCount() > 0) {
        this->setCheckState(0, Qt::Checked);
        this->setToolTip(0, "Show/hide scan");
    }
}

void ScanListWidgetItem::setItemStatus(bool isVisible, bool isActive, bool tempScan)
{
    scanIsActive = isActive;
    this->setText(0, (tempScan ? scanTempText : QString()) + (isActive ? scanActiveText : QString()));
    this->setCheckState(0, isVisible ? Qt::Checked : Qt::Unchecked);
}

void ScanListWidgetItem::setUid(const QUuid &uid)
{
    this->setData(0, Qt::UserRole, QVariant(uid));
}

void ScanListWidgetItem::setName(const QString &name)
{
    this->setText(1, name);
}

void ScanListWidgetItem::setScaleFactor(double d)
{
    this->setText(2, QString::number(d, 'f', 2));
}

void ScanListWidgetItem::setXoffset(double d)
{
    this->setText(4, QString::number(d, 'f', 4));
}

void ScanListWidgetItem::setYoffset(double d)
{
    this->setText(3, QString::number(d, 'f', 2));
}

void ScanListWidgetItem::setColor(const QColor &c, bool darkMode)
{
    if (!c.isValid()) return;

    this->setForeground(1, QBrush(darkMode
                                ? global::Functions::colorToDarkMode(c)
                                : c));
}

bool ScanListWidgetItem::isVisible() const
{
    return this->checkState(0) == Qt::Checked;
}

bool ScanListWidgetItem::isActive() const
{
    return scanIsActive;
}

QUuid ScanListWidgetItem::getUid() const
{
    return this->data(0, Qt::UserRole).toUuid();
}

QString ScanListWidgetItem::getName() const
{
    return this->text(1);
}
