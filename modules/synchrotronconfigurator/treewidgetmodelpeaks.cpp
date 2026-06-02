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

#include "treewidgetmodelpeaks.h"
#include <QUuid>
#include <QMenu>
#include <QAction>

TreeWidgetModelPeaks::TreeWidgetModelPeaks(QWidget *parent)
    : QTreeWidget(parent)
{
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
}

void TreeWidgetModelPeaks::showContextMenu(const QPoint &pos)
{
    QTreeWidgetItem *item = this->itemAt(pos);
    if (!item) return;

    int idx = indexOfTopLevelItem(item);
    bool hasAbove = idx > 0;
    bool hasBelow = idx < topLevelItemCount() - 1;

    QMenu menu(this);

    QAction *actInit = new QAction(tr("Initialize"), this);
    QAction *actFit  = new QAction(tr("Fit"), this);
    QAction *actInitAbove = new QAction(tr("Initialize from previous"), this);
    QAction *actInitBelow = new QAction(tr("Initialize from next"), this);

    menu.addAction(actInit);
    menu.addAction(actFit);
    if (hasAbove) menu.addAction(actInitAbove);
    if (hasBelow) menu.addAction(actInitBelow);

    connect(actInit, &QAction::triggered, [this, item]() {initCurve(item);});
    connect(actFit,  &QAction::triggered, [this, item]() {fitCurve(item);});
    connect(actInitAbove, &QAction::triggered, [this, idx]() {initFromAbove(idx);});
    connect(actInitBelow, &QAction::triggered, [this, idx]() {initFromBelow(idx);});

    menu.exec(this->viewport()->mapToGlobal(pos));
}

void TreeWidgetModelPeaks::initCurve(QTreeWidgetItem *item)
{
    if (item) emit sigInitCurves(item->data(0, Qt::UserRole).toUuid());
}

void TreeWidgetModelPeaks::fitCurve(QTreeWidgetItem *item)
{
    if (item) emit sigFitCurves(item->data(0, Qt::UserRole).toUuid());
}

void TreeWidgetModelPeaks::initFromAbove(int idx)
{
    if (idx > 1) emit sigInitFromAbove(idx);
}

void TreeWidgetModelPeaks::initFromBelow(int idx)
{
    if (idx < topLevelItemCount() - 1) emit sigInitFromBelow(idx);
}
