/***************************************************************************
                          codlocalcifmanager.cpp  -  description
                             -------------------
    begin                : Thu Jul 20 18:30:00 CEST 2023
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

#include "codlocalcifmanager.h"
#include "../libXrdIO/bgmnfileio.h"
#include <QFile>

CodLocalCifManager::CodLocalCifManager(QObject *parent)
    : CodCifFileManager{parent}
{

}

void CodLocalCifManager::startCifDownload(const QStringList &r, const QString &u)
{
    QList<CifFile> cifDownloadedData;
    QStringList failedDownloads;

    if (!r.size()) {
        qDebug() << QString("CodLocalCifManager::startCifDownload(): No records selected for download, exiting.");
        emit cifDownloadComplete(cifDownloadedData, failedDownloads);
        return;
    }

    if (u.isEmpty()) {
        qDebug() << QString("CodLocalCifManager::startCifDownload(): URL is invalid, exiting.");
        emit cifDownloadComplete(cifDownloadedData, failedDownloads);
        return;
    }

    for (int i = 0; i < r.size(); ++i) {
        QString cifFile(QString("%1/%2.cif").arg(u, r.at(i)));
        QString cifContent;

        if (QFile::exists(cifFile)) {
            // first try to access the cif file directly for performance reason
            qDebug() << QString("CodLocalCifManager::startCifDownload(): Reading %1").arg(cifFile);
            cifContent = BgmnFileIO::readTextFile(cifFile);
        } else {
            // if direct access doesn't work, search the file
            cifContent = findFile(r.at(i), u);
        }

        if (!cifContent.isEmpty()) {
            cifDownloadedData.append(CifFile(r.at(i), cifContent));
        } else {
            qDebug() << QString("CodLocalCifManager::startCifDownload(): Could not find %1, skipping it").arg(r.at(i));
            failedDownloads.append(r.at(i));
        }

        emit progress(i+1);
    }

    emit cifDownloadComplete(cifDownloadedData, failedDownloads);
}

QString CodLocalCifManager::findFile(const QString &f, const QString &d)
{
    QStringList files;

    QDirIterator it(d, QStringList() << QString("%1.cif").arg(f) << QString("%1.CIF").arg(f), QDir::NoFilter, QDirIterator::Subdirectories);

    while (it.hasNext()) {
        files.append(it.next());
    }

    if (files.size()) {
        qDebug() << QString("CodLocalCifManager::startCifDownload(): Reading %1").arg(files.first());
        return BgmnFileIO::readTextFile(files.first());
    }

    return QString();
}

void CodLocalCifManager::cancel()
{
    /* copying local files can't be cancelled at the moment */
}
