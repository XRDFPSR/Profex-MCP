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

#ifndef FPRESULTSTREEWIDGET_H
#define FPRESULTSTREEWIDGET_H

#include "resultstreewidget.h"

class FpResultsTreeWidget : public ResultsTreeWidget
{
public:
    explicit FpResultsTreeWidget(QWidget *parent = Q_NULLPTR);

    void clearStats();
    void clearGlobal();
    void clearLocal();
    void resetStats();

private:
    QTreeWidgetItem *itStats;
    QTreeWidgetItem *itGlobal;
    QTreeWidgetItem *itLocal;

    void init();
};

#endif // FPRESULTSTREEWIDGET_H
