/***************************************************************************
                          treewidgetsupportpeaks.cpp  -  description
                             -------------------
    begin                : Thu Dec 19 18:00:00 CEST 2024
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

#ifndef TREEWIDGETSUPPORTPEAKS_H
#define TREEWIDGETSUPPORTPEAKS_H

#include <QObject>
#include <QTreeWidget>
#include <QUuid>

class TreeWidgetSupportPeaks : public QTreeWidget
{
    Q_OBJECT
public:
    TreeWidgetSupportPeaks(QWidget *parent = nullptr);

private:

private slots:

signals:
};

#endif // TREEWIDGETSUPPORTPEAKS_H
