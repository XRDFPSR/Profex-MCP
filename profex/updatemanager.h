/***************************************************************************
                          updatemanager.h  -  description
                             -------------------
    begin                : Thu Apr 12 18:00:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#ifndef UPDATEMANAGER_H
#define UPDATEMANAGER_H

#include <QObject>
#include "../libXrdIO/settingsmanager.h"

#ifdef Q_OS_WIN
inline extern const QString _updaterExe = "maintenancetool.exe";
#else
#ifdef Q_OS_MACOS
inline extern const QString _updaterExe = "MaintenanceTool";
#else
inline extern const QString _updaterExe = "../maintenancetool";
#endif
#endif

class UpdateManager : public QObject
{
    Q_OBJECT
public:
    explicit UpdateManager(const QString &, QObject *parent = nullptr);

    bool checkForInstaller();
    bool hasUpdates();
    void runUpdate();

private:
    SettingsManager *settings;
    QString _appDir;
    QString _updater;

signals:

};

#endif // UPDATEMANAGER_H
