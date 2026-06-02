/***************************************************************************
                          coddownloadmanager.h  -  description
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

#ifndef CODDOWNLOADMANAGER_H
#define CODDOWNLOADMANAGER_H

#include <QObject>
#include <QByteArray>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include "codciffilemanager.h"

class CodDownloadManager : public CodCifFileManager
{
    Q_OBJECT
public:
    explicit CodDownloadManager(QObject *parent = nullptr);

    void startCifDownload(const QStringList &r, const QString &u);

private:
    QString codUrl;
    QStringList records;
    QString currentRecord;
    QList<CifFile> cifDownloadedData;
    int numberOfFiles;
    bool wasCancelled;
    QNetworkAccessManager cifWebCtrl;
    QStringList failedDownloads;

public slots:
    void cancel();

private slots:
    void cifDownloaded(QNetworkReply* pReply);
};

#endif // CODDOWNLOADMANAGER_H
