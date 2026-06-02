/***************************************************************************
                          presetmenumanager.h  -  description
                             -------------------
    begin                : Fri Mar 19 18:29:00 CEST 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#ifndef PRESETMENUMANAGER_H
#define PRESETMENUMANAGER_H

#include <QObject>
#include <QMenu>
#include <QAction>

class PresetMenuManager : public QObject
{
    Q_OBJECT
public:
    explicit PresetMenuManager(QObject *parent = nullptr);
    ~PresetMenuManager();

    void setMenu(QMenu *);
    inline void setPresetRepos(const QStringList &l) {presetRepos = l;}

public slots:
    void update();

private:
    QMenu *menu;
    QStringList presetRepos;

private slots:
    void presetActionTriggered();

signals:
    void sigApplyPreset(QString);
};

#endif // PRESETMENUMANAGER_H
