/***************************************************************************
                          projectwidgetdock.h  -  description
                             -------------------
    begin                : Thu Feb 20 18:07:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#include "projectwidgetdock.h"
#include "projectWidget/graphWidget/abstractgraphview.h"
#include <QGuiApplication>

ProjectWidgetDock::ProjectWidgetDock(QWidget *parent, Qt::WindowFlags flags) :
    QDockWidget(parent, flags)
{
    widgetStack = new QStackedWidget(this);
    this->setWidget(widgetStack);

    isShown = true;
    connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(toggleVisibility(bool)));
}

ProjectWidgetDock::ProjectWidgetDock(const QString &title, QWidget *parent, Qt::WindowFlags flags) :
    QDockWidget(title, parent, flags)
{
    widgetStack = new QStackedWidget(this);
    this->setWidget(widgetStack);

    isShown = true;
    connect(this, SIGNAL(visibilityChanged(bool)), this, SLOT(toggleVisibility(bool)));
}

int ProjectWidgetDock::addWidget(QWidget *w)
{
    int i = widgetStack->addWidget(w);

    AbstractGraphView *graphView = dynamic_cast<AbstractGraphView*>(w);
    if (graphView) graphView->setShown(isShown);

    return i;
}

void ProjectWidgetDock::removeWidget(QWidget *w)
{
    widgetStack->removeWidget(w);
}

int ProjectWidgetDock::indexOf(QWidget *w) const
{
    return widgetStack->indexOf(w);
}

void ProjectWidgetDock::setCurrentIndex(int i)
{
    widgetStack->setCurrentIndex(i);
}

void ProjectWidgetDock::setCurrentWidget(QWidget *w)
{
    widgetStack->setCurrentWidget(w);
}

void ProjectWidgetDock::toggleVisibility(bool b)
{
    isShown = b;

    if (b) qApp->setOverrideCursor(Qt::WaitCursor);

    for (int i = 0; i < widgetStack->count(); ++i) {
        AbstractGraphView *graphView = dynamic_cast<AbstractGraphView*>(widgetStack->widget(i));
        if (graphView) {
            graphView->setShown(b);
            if (b) {
                graphView->updateView();
                qApp->processEvents();
            }
        }
    }

    if (b) qApp->restoreOverrideCursor();
}
