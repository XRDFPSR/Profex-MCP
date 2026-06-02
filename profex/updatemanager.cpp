/***************************************************************************
                          updatemanager.cpp  -  description
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

#include "updatemanager.h"
#include <QFile>
#include <QDir>
#include <QDebug>
#include <QProcess>

UpdateManager::UpdateManager(const QString &d, QObject *parent)
    : _appDir(d), QObject{parent}
{
    settings = SettingsManager::getInstance();
    _updater = _appDir + QDir::separator() + _updaterExe;
}

bool UpdateManager::hasUpdates()
{
    if (!checkForInstaller()) return false;

    QProcess process;
    process.start(_updater, QStringList() << "check-updates");
    process.waitForFinished();

    if (process.error() != QProcess::UnknownError) {
        qDebug() << "UpdateManager::hasUpdates(): An error occurred.";
        return false;
    }

    QString data(process.readAllStandardOutput());

    if (data.contains("Warning: There are currently no updates available.")) {
        qDebug() << "UpdateManager::hasUpdates(): No updates available";
        return false;
    }

    qDebug() << data;
    return true;
}

void UpdateManager::runUpdate()
{
    QProcess process;
    process.startDetached(_updater, QStringList() << "--start-updater");
}

bool UpdateManager::checkForInstaller()
{
    bool b = QFile::exists(_updater);

    if (b) {
        qDebug() << QString("UpdateManager::checkForInstaller(): Installer was found (%1)").arg(_updater);
    } else {
        qDebug() << QString("UpdateManager::checkForInstaller(): No installer found (%1)").arg(_updater);
    }

    return b;
}
