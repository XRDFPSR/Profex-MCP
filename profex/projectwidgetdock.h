/***************************************************************************
                          projectwidgetdock.cpp  -  description
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

#ifndef PROJECTWIDGETDOCK_H
#define PROJECTWIDGETDOCK_H

#include <QDockWidget>
#include <QStackedWidget>

class ProjectWidgetDock : public QDockWidget
{
    Q_OBJECT

public:
    explicit ProjectWidgetDock(QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());
    explicit ProjectWidgetDock(const QString &title, QWidget *parent = nullptr, Qt::WindowFlags flags = Qt::WindowFlags());

    int addWidget(QWidget *);
    void removeWidget(QWidget *);
    int indexOf(QWidget *) const;

public slots:
    void setCurrentIndex(int);
    void setCurrentWidget(QWidget *);
    void toggleVisibility(bool);

private:
    QStackedWidget *widgetStack;
    bool isShown;
};

#endif // PROJECTWIDGETDOCK_H
