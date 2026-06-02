/***************************************************************************
                          prefpagegraphcursors.cpp  -  description
                             -------------------
    begin                : Fri Mar 04 16:00:00 CEST 2022
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

#include "prefpagegraphcursors.h"
#include "ui_prefpagegraphcursors.h"
#include "../libXrdIO/structs.h"

PrefPageGraphCursors::PrefPageGraphCursors(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageGraphCursors)
{
    ui->setupUi(this);
}

PrefPageGraphCursors::~PrefPageGraphCursors()
{
    delete ui;
}

void PrefPageGraphCursors::initUi()
{
    ui->treeWidgetTungstenLines->setColumnCount(2);
    ui->treeWidgetTungstenLines->setHeaderLabels(QStringList() << tr("Emission line") << tr("Wavelength (nm)"));

    QMapIterator<QString, double> iter(global::tungstenLines);

    while (iter.hasNext()) {
        iter.next();
        QTreeWidgetItem *itm = new QTreeWidgetItem(QStringList() << iter.key() << QString("%1").arg(iter.value(), 0, 'f', 6));
        itm->setData(0, Qt::UserRole, iter.key());
        itm->setCheckState(0, Qt::Unchecked);
        ui->treeWidgetTungstenLines->addTopLevelItem(itm);
    }

    initSettings();
}

void PrefPageGraphCursors::initSettings()
{
    QStringList wlines = settings->value("graph/specCursorTungstenLines", QStringList()).toStringList();

    for (int i = 0; i < ui->treeWidgetTungstenLines->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetTungstenLines->topLevelItem(i);

        if (wlines.contains(it->data(0, Qt::UserRole))) {
            it->setCheckState(0, Qt::Checked);
        }
    }

    ui->treeWidgetTungstenLines->header()->restoreState(settings->value("preferencesDialog/graphCursorHeaderState", QByteArray()).toByteArray());
}

void PrefPageGraphCursors::saveSettings()
{
    QStringList wlines;

    for (int i = 0; i < ui->treeWidgetTungstenLines->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetTungstenLines->topLevelItem(i);

        if (it->checkState(0) == Qt::Checked) {
            wlines.append(it->data(0, Qt::UserRole).toString());
        }
    }

    settings->setValue("graph/specCursorTungstenLines", wlines);
    settings->setValue("preferencesDialog/graphCursorHeaderState", ui->treeWidgetTungstenLines->header()->saveState());
}
