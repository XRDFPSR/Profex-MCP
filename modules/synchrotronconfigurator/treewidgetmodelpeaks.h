/***************************************************************************
                          treewidgetmodelpeaks.cpp  -  description
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

#ifndef TREEWIDGETMODELPEAKS_H
#define TREEWIDGETMODELPEAKS_H

#include <QObject>
#include <QTreeWidget>
#include <QUuid>

class TreeWidgetModelPeaks : public QTreeWidget
{
    Q_OBJECT
public:
    TreeWidgetModelPeaks(QWidget *parent = nullptr);

private:
    QMap<QUuid, QTreeWidgetItem *> _items;

private slots:
    void showContextMenu(const QPoint &pos);
    void initCurve(QTreeWidgetItem *);
    void fitCurve(QTreeWidgetItem *);
    void initFromAbove(int idx);
    void initFromBelow(int idx);

signals:
    void sigInitCurves(QUuid);
    void sigFitCurves(QUuid);
    void sigInitFromAbove(int);
    void sigInitFromBelow(int);
};

#endif // TREEWIDGETMODELPEAKS_H
