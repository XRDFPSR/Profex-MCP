/***************************************************************************
                          projectstreewidget.h  -  description
                             -------------------
    begin                : Fri Oct 12 12:00:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#ifndef PROJECTSTREEWIDGET_H
#define PROJECTSTREEWIDGET_H

#include "../libXrdIO/settingsmanager.h"
#include <QAction>
#include <QTreeWidget>

class ProjectsTreeWidget : public QTreeWidget
{
    Q_OBJECT
public:
    explicit ProjectsTreeWidget(QWidget *parent = nullptr);
    QUuid getProjectUid(const QTreeWidgetItem *);
    QUuid getProjectUid(int);
    inline int count() const {return topLevelItemCount();}

public slots:
    void expandAll();
    void collapseAll();
    void sortProjects(int);

private:
    QBrush bgDefaultBrush;
    QBrush bgHighlighBrush;
    QMenu *pwHeaderMenu;
    SettingsManager *settings;

    void highlightChildItems(int);
    void highlightToplevelItems();
    void initSettings();

private slots:
    void highlightItems(QTreeWidgetItem *cur, QTreeWidgetItem *prev);
    void headerContextMenuRequested(QPoint);
    void projectListContextMenu(QPoint);
    void toggleColumnVisibility(QAction *);
    void treeWidgetHeaderChanged();

signals:
    void runRefinement();
    void closeProject();
    void exportProjectInfo();
};

#endif // PROJECTSTREEWIDGET_H
