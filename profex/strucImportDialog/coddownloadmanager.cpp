/***************************************************************************
                          coddownloadmanager.cpp  -  description
                             -------------------
    begin                : Thu Jan 11 18:30:00 CEST 2021
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

#include "coddownloadmanager.h"
#include <QRegularExpression>

CodDownloadManager::CodDownloadManager(QObject *parent) : CodCifFileManager(parent)
{
    connect(&cifWebCtrl, SIGNAL(finished(QNetworkReply*)), this, SLOT(cifDownloaded(QNetworkReply*)));
}

void CodDownloadManager::startCifDownload(const QStringList &r, const QString &u)
{
    cifDownloadedData.clear();
    failedDownloads.clear();

    if (!r.size()) {
        qDebug() << QString("CodDownloadManager::startCifDownload(): No records selected for download, exiting.");
        emit cifDownloadComplete(cifDownloadedData, failedDownloads);
        return;
    }

    if (u.isEmpty()) {
        qDebug() << QString("CodDownloadManager::startCifDownload(): URL is invalid, exiting.");
        emit cifDownloadComplete(cifDownloadedData, failedDownloads);
        return;
    }

    records = r;
    numberOfFiles = r.size();
    currentRecord = QString();
    codUrl = u;

    wasCancelled = false;

    currentRecord = records.takeFirst();
    QUrl url(QString("%1/%2.cif").arg(codUrl, currentRecord));
    qDebug() << QString("CodDownloadManager::startCifDownload(): Downloading %1").arg(url.toString());
    QNetworkRequest request(url);
    cifWebCtrl.get(request);
}

void CodDownloadManager::cifDownloaded(QNetworkReply* pReply)
{
    emit progress(numberOfFiles - records.size() + 1);

    QByteArray dl = pReply->readAll();

    if (dl.contains("data_")) {
        cifDownloadedData.append(CifFile(currentRecord, QString(dl)));
    } else {
        failedDownloads.append(currentRecord);
    }

    pReply->deleteLater();

    if (records.size()) {
        if (wasCancelled) {
            qDebug() << QString("CodDownloadManager::cifDownloaded(): Download was aborted by the user. Exiting.");
            emit cifDownloadComplete(cifDownloadedData, failedDownloads);
        } else {
            currentRecord = records.takeFirst();
            QUrl url(QString("%1/%2.cif").arg(codUrl, currentRecord));
            qDebug() << QString("CodDownloadManager::cifDownloaded: downloading %1").arg(url.toString());
            QNetworkRequest request(url);
            cifWebCtrl.get(request);
        }
    } else {
        qDebug() << QString("CodDownloadManager::cifDownloaded: All downloaded");
        emit cifDownloadComplete(cifDownloadedData, failedDownloads);
    }
}

void CodDownloadManager::cancel()
{
    wasCancelled = true;
}
