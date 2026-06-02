/***************************************************************************
                          periodictablewidget.h  -  description
                             -------------------
    begin                : Mon Aug 03 18:21:00 CEST 2020
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

#ifndef PERIODICTABLEWIDGET_H
#define PERIODICTABLEWIDGET_H

#include <QWidget>
#include <QStringList>
#include <QMap>
#include "periodictablebutton.h"
#include "periodictabletogglegroupbutton.h"

class PeriodicTableWidget : public QWidget
{
    Q_OBJECT

public:
    explicit PeriodicTableWidget(QWidget *parent = nullptr);
    ~PeriodicTableWidget();

    QStringList getAll();
    QStringList getOne();
    QStringList getNone();

public slots:
    void allOptional();
    void allMandatory();
    void allOne();
    void allDisabled();
    void disableOptionals();

private:
    QMap<int, PeriodicTableButton*> buttonMap;
    QMap<int, QString> toolTipMap;
    QList<PeriodicTableToggleGroupButton*> toggleButtons;

    QStringList getButtons(int);
    void setAllStatus(int);

private slots:
    void toggleGroup(QString, int, int);
};

#endif // PERIODICTABLEWIDGET_H
