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

#ifndef BGMNRESULTSTREEWIDGET_H
#define BGMNRESULTSTREEWIDGET_H

#include "resultstreewidget.h"
#include "../libXrdIO/structs.h"
#include "../libXrdIO/parser/bgmnsavparser.h"
#include "../libXrdIO/parser/bgmnlstparser.h"

class BgmnResultsTreeWidget : public ResultsTreeWidget
{
    Q_OBJECT

public:
    explicit BgmnResultsTreeWidget(QWidget *parent = nullptr);

    void updateGoals(const BgmnSavParser &);

    void clearStats();
    void clearGlobal();
    void clearLocal();
    void resetStats();
    void initSettings();

    inline void setLstParser(const BgmnLstParser *lp) {lparser = lp;}
    void setRvalues(double _rwp, double _rexp);

private:
    QStringList globalParams;
    QStringList localParams;
    const BgmnLstParser *lparser;
    QAction *actCopyGvalsGesds;
    QAction *actCopyGvals;
    QAction *actCopyGesds;

    QTreeWidgetItem *itStats;
    QTreeWidgetItem *itStatsRwp;
    QTreeWidgetItem *itStatsRexp;
    QTreeWidgetItem *itStatsChi2;
    QTreeWidgetItem *itStatsGooF;
    QTreeWidgetItem *itBkgrCoeff;
    QTreeWidgetItem *itGlobal;
    QTreeWidgetItem *itLocal;

    void parseSettings();

    void setStats(const QString &);
    void setGlobal();
    void setLocal();

    void setQuantItemColor(QTreeWidgetItem *);
    QTreeWidgetItem *parseLocalGoal(global::Result);
    void copyGlobalData(bool v, bool e);

private slots:
    void slotItemExpanded(QTreeWidgetItem *);
    void slotItemCollapsed(QTreeWidgetItem *);
    void headerResized();
    void showContextMenu(const QPoint &);
    void copyGlobalValuesAndEsds();
    void copyGlobalValues();
    void copyGlobalEsds();
};

#endif // RESULTSTREEWIDGET_H
