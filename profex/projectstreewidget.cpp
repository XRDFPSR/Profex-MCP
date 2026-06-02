/***************************************************************************
                          projectstreewidget.cpp  -  description
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

#include "projectstreewidget.h"
#include "projectWidget/projectselecttreeitem.h"
#include <QAction>
#include <QDebug>
#include <QHeaderView>
#include <QMenu>

ProjectsTreeWidget::ProjectsTreeWidget(QWidget *parent) : QTreeWidget(parent)
{
    settings = SettingsManager::getInstance();
    bgDefaultBrush = QTreeWidgetItem().background(0);
    bgHighlighBrush = QBrush(QColor(221, 221, 255));
    setSortingEnabled(false);

    setDragEnabled(true);
    setAcceptDrops(true);
    setDropIndicatorShown(true);
    setDragDropMode(QAbstractItemView::InternalMove);
    setSelectionMode(QAbstractItemView::ExtendedSelection);
    setContextMenuPolicy(Qt::CustomContextMenu);
    QStringList headerLabels = QStringList() << tr("Name") << tr("Status") << tr("Iteration") << QString("%1%2").arg(global::chi, global::superTwo);
    setHeaderLabels(headerLabels);
    setColumnCount(headerLabels.size());

    header()->setSortIndicatorShown(true);
    header()->setSectionsClickable(true);
    header()->setSectionResizeMode(QHeaderView::Interactive);
    header()->setContextMenuPolicy(Qt::CustomContextMenu);

    pwHeaderMenu = new QMenu(this);

    for (int i = 1; i < header()->count(); ++i) {
        QAction *act = new QAction(headerItem()->text(i), pwHeaderMenu);
        act->setCheckable(true);
        act->setData(i);
        pwHeaderMenu->addAction(act);
    }

    initSettings();

    connect(this, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(highlightItems(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(this, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(projectListContextMenu(QPoint)));
    connect(header(), SIGNAL(sectionClicked(int)), this, SLOT(sortProjects(int)));
    connect(header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(headerContextMenuRequested(QPoint)));
    connect(header(), SIGNAL(sectionResized(int,int,int)), this, SLOT(treeWidgetHeaderChanged()));

    connect(pwHeaderMenu, SIGNAL(triggered(QAction*)), this, SLOT(toggleColumnVisibility(QAction*)));
}

void ProjectsTreeWidget::initSettings()
{
    header()->restoreState(settings->value("projectsList/headerState", QByteArray()).toByteArray());

    QList<QVariant> h = settings->value("projectsList/columnVisibility", QList<QVariant>()).toList();

    for (int i = 0; i < header()->count() - 1; ++i) {
        bool b = i >= h.size() ? true : h.at(i).toBool();
        setColumnHidden(i + 1, !b);
        pwHeaderMenu->actions().at(i)->setChecked(b);
    }

}

void ProjectsTreeWidget::expandAll()
{
    for (int i = 0; i < topLevelItemCount(); ++i) {
        topLevelItem(i)->setExpanded(true);
    }
}

void ProjectsTreeWidget::collapseAll()
{
    for (int i = 0; i < topLevelItemCount(); ++i) {
        topLevelItem(i)->setExpanded(false);
    }
}

void ProjectsTreeWidget::highlightItems(QTreeWidgetItem *cur, QTreeWidgetItem *prev)
{
    Q_UNUSED(prev);
    if (!cur) return;

    if (indexOfTopLevelItem(cur) < 0) {
        QTreeWidgetItem *tlitem = cur->parent();
        highlightChildItems(tlitem->indexOfChild(cur));
    } else {
        if (cur->isExpanded()) {
            highlightToplevelItems();
        }
    }
}

void ProjectsTreeWidget::highlightChildItems(int n)
{
    for (int i = 0; i < topLevelItemCount(); ++i) {
        for (int c = 0; c < topLevelItem(i)->childCount(); ++c) {
            if (c == n) {
                topLevelItem(i)->child(c)->setBackground(0, bgHighlighBrush);
                topLevelItem(i)->child(c)->setBackground(1, bgHighlighBrush);
            } else {
                topLevelItem(i)->child(c)->setBackground(0, bgDefaultBrush);
                topLevelItem(i)->child(c)->setBackground(1, bgDefaultBrush);
            }
        }
    }
}

void ProjectsTreeWidget::highlightToplevelItems()
{
    for (int i = 0; i < topLevelItemCount(); ++i) {
        for (int c = 0; c < topLevelItem(i)->childCount(); ++c) {
            topLevelItem(i)->child(c)->setBackground(0, bgDefaultBrush);
            topLevelItem(i)->child(c)->setBackground(1, bgDefaultBrush);
        }
    }
}

void ProjectsTreeWidget::sortProjects(int o)
{
    Qt::SortOrder order = header()->sortIndicatorOrder();
    sortItems(o, order);
}

void ProjectsTreeWidget::headerContextMenuRequested(QPoint p)
{
    pwHeaderMenu->popup(header()->viewport()->mapToGlobal(p));
}

void ProjectsTreeWidget::projectListContextMenu(QPoint p)
{
    QMenu plContextMenu(this);
    plContextMenu.addAction(QIcon::fromTheme("profex-run"), tr("Start refinement"), this, SIGNAL(runRefinement()));
    plContextMenu.addAction(QIcon::fromTheme("profex-document-close"), tr("Close selected projects"), this, SIGNAL(closeProject()));
    plContextMenu.addSeparator();
    plContextMenu.addAction(QIcon::fromTheme("profex-save-as"), tr("Export project data"), this, SIGNAL(exportProjectInfo()));
    plContextMenu.addSeparator();
    plContextMenu.addAction(tr("Expand all"), this, SLOT(expandAll()));
    plContextMenu.addAction(tr("Collapse all"), this, SLOT(collapseAll()));

    plContextMenu.exec(mapToGlobal(p));
}

void ProjectsTreeWidget::toggleColumnVisibility(QAction *a)
{
    setColumnHidden(a->data().toInt(), !a->isChecked());

    QList<QVariant> columnVisibilities;

    for (int i = 1; i < header()->count(); ++i) {
        columnVisibilities.append(QVariant(!isColumnHidden(i)));
    }

    settings->setValue("projectsList/columnVisibility", columnVisibilities);
}

void ProjectsTreeWidget::treeWidgetHeaderChanged()
{
    settings->setValue("projectsList/headerState", header()->saveState());
}

QUuid ProjectsTreeWidget::getProjectUid(const QTreeWidgetItem *it)
{
    const QTreeWidgetItem *pIt = it;
    while (pIt->parent()) pIt = pIt->parent();

    const ProjectSelectTreeItem *selIt = dynamic_cast<const ProjectSelectTreeItem *>(pIt);
    if (selIt) return selIt->getUid();
    return QUuid();
}

QUuid ProjectsTreeWidget::getProjectUid(int n)
{
    if (n >= topLevelItemCount()) return QUuid();

    const ProjectSelectTreeItem *selIt = dynamic_cast<const ProjectSelectTreeItem *>(topLevelItem(n));
    if (selIt) return selIt->getUid();
    return QUuid();
}
