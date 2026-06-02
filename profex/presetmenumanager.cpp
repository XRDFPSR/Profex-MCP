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

#include "presetmenumanager.h"
#include "bgmnpresethandler.h"
#include <QDirIterator>
#include <QDomDocument>
#include <QDebug>

PresetMenuManager::PresetMenuManager(QObject *parent) : QObject(parent)
{
    menu = nullptr;
}

PresetMenuManager::~PresetMenuManager()
{
    if (menu) delete menu;
}

void PresetMenuManager::setMenu(QMenu *m)
{
    menu = m;
}

void PresetMenuManager::update()
{
    if (!menu) return;
    menu->clear();

    QMap<QString, QString> presets = BgmnPresetHandler::getPresets(presetRepos, "a");
    QMap<QString, QMenu*> dirMenues;
    QMap<QString, QString>::const_iterator it = presets.constBegin();

    while (it != presets.constEnd()) {
        QString   pName(it.key());
        QFileInfo pFile(it.value());
        QDir      pDir(pFile.absolutePath());
        pDir.cdUp();

        QAction *act = new QAction(pName, nullptr);
        act->setData(QVariant(pFile.absoluteFilePath()));
        connect(act, SIGNAL(triggered()), this, SLOT(presetActionTriggered()));

        if (!dirMenues.contains(pDir.absolutePath())) {
            dirMenues.insert(pDir.absolutePath(), new QMenu(pDir.absolutePath(), nullptr));
        }

        dirMenues.value(pDir.absolutePath())->addAction(act);
        ++it;
    }

    QMap<QString, QMenu*>::const_iterator mit = dirMenues.constBegin();

    while (mit != dirMenues.constEnd()) {
        menu->addMenu(mit.value());
        ++mit;
    }
}

void PresetMenuManager::presetActionTriggered()
{
    QAction *action = qobject_cast<QAction *>(sender());
    if (!action) return;

    emit sigApplyPreset(action->data().toString());
}
