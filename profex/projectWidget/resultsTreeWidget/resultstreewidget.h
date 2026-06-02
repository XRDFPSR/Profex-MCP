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

#ifndef RESULTSTREEWIDGET_H
#define RESULTSTREEWIDGET_H

#include <QObject>
#include <QWidget>
#include <QTreeWidget>
#include <QTreeWidgetItem>

#include "../../../libXrdIO/settingsmanager.h"

class ResultsTreeWidget : public QTreeWidget
{
public:
    explicit ResultsTreeWidget(QWidget *parent = Q_NULLPTR);

    virtual void clearStats() = 0;
    virtual void clearGlobal() = 0;
    virtual void clearLocal() = 0;
    virtual void resetStats() = 0;
    virtual void setRvalues(double, double) {/* subclass if supported */}
    virtual void initSettings() {/* subclass if supported */}

protected:
    SettingsManager *settings;
};

#endif // RESULTSTREEWIDGET_H
