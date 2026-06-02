/***************************************************************************
                          resultstreewidget.h  -  description
                             -------------------
    begin                : Thu June 06 21:00:00 CEST 2019
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


#include "fpresultstreewidget.h"

FpResultsTreeWidget::FpResultsTreeWidget(QWidget *parent) :
    ResultsTreeWidget(parent)
{
    itStats  = new QTreeWidgetItem(this, QStringList(tr("Statistics")));
    itLocal  = new QTreeWidgetItem(this, QStringList(tr("Phases")));

    itStats->setFirstColumnSpanned(true);
    itLocal->setFirstColumnSpanned(true);

    addTopLevelItem(itStats);
    addTopLevelItem(itLocal);
}

void FpResultsTreeWidget::clearStats()
{
}

void FpResultsTreeWidget::clearGlobal()
{
    if (itGlobal) qDeleteAll(itGlobal->takeChildren());
}

void FpResultsTreeWidget::clearLocal()
{
    if (itLocal) qDeleteAll(itStats->takeChildren());
}

void FpResultsTreeWidget::resetStats()
{
    if (itStats) {
        for (int i = 0; i < itStats->childCount(); ++i) {
            itStats->child(i)->setText(1, QString());
        }
    }
}
