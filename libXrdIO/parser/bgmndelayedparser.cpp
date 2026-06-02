/***************************************************************************
                          bgmndelayedparser.cpp  -  description
                             -------------------
    begin                : Jul 15 17:25:02 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#include "bgmndelayedparser.h"
#include "bgmnfileio.h"
#include <QFileInfo>
#include <QElapsedTimer>
#include <QThread>

BgmnDelayedParser::BgmnDelayedParser()
{
    settings = SettingsManager::getInstance();
}

BgmnDelayedParser::BgmnDelayedParser(const QString &s, bool &ok)
{
    Q_UNUSED(s);
    Q_UNUSED(ok);
    settings = SettingsManager::getInstance();
}

BgmnDelayedParser::~BgmnDelayedParser()
{}

bool BgmnDelayedParser::load(const QString &s)
{
    if (s.isEmpty()) {
        qDebug() << QString("BgmnDelayedParser::load(): Empty file name, exiting.");
        return false;
    }

    fileName = s;
    int pollIntervalMS = settings->value("config/fileSizePollIntervalMS", 20).toInt();
    int pollTimeoutMS = settings->value("config/fileSizePollTimeoutMS", 1000).toInt();

    if (!waitForStable(s, pollIntervalMS, pollTimeoutMS)) {
        qDebug() << QString("BgmnDelayedParser::load(): File size is not stable, exiting (%1)").arg(s);
        return false;
    }

    return privateLoad(s);
}

bool BgmnDelayedParser::reload()
{
    if (fileName.isEmpty()) {
        qDebug() << QString("BgmnDelayedParser::reload(): Empty file name. Exiting.");
        return false;
    }

    int pollIntervalMS = settings->value("config/fileSizePollIntervalMS", 20).toInt();
    int pollTimeoutMS = settings->value("config/fileSizePollTimeoutMS", 1000).toInt();

    if (!waitForStable(fileName, pollIntervalMS, pollTimeoutMS)) {
        qDebug() << QString("BgmnDelayedParser::reload(): File size is not stable, exiting (%1)").arg(fileName);
        return false;
    }

    return privateReload();
}

bool BgmnDelayedParser::waitForStable(const QString& f, int intervalMS, int timeoutMS)
{
    QFileInfo info(f);
    if (!info.exists()) return false;

    QElapsedTimer timer;
    timer.start();

    qint64 stableCount = 0;
    qint64 lastSize = -1;
    QDateTime lastMTime;

    while (timer.elapsed() < timeoutMS) {
        info.refresh();
        const qint64 size = info.size();
        const QDateTime mtime = info.lastModified();

        if (size == lastSize && mtime == lastMTime) {
            ++stableCount;
            if (stableCount >= 3) {
                return true; // 3 consecutive matches
            }
        } else {
            stableCount = 0;
            lastSize = size;
            lastMTime = mtime;
        }

        QThread::msleep(intervalMS);
    }

    qDebug() << QString("BgmnDelayedParser::waitForStable(): File is still unstable, exiting (%1)").arg(f);
    return false;
}

bool BgmnDelayedParser::readSafely(const QString &f, QString &out, int maxRetries)
{
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        qDebug() << QString("BgmnDelayedParser::readSafely(): Polling file for reading %1").arg(f);
        QFileInfo beforeInfo(f);
        beforeInfo.refresh();
        const auto sizeBefore = beforeInfo.size();
        const auto mtimeBefore = beforeInfo.lastModified();

        QString data = BgmnFileIO::readTextFile(f);
        QFileInfo afterInfo(f);
        afterInfo.refresh();

        if ((afterInfo.size() == sizeBefore) && (afterInfo.lastModified() == mtimeBefore)) {
            out = std::move(data);
            qDebug() << QString("BgmnDelayedParser::readSafely(): File has been read, size is %1").arg(out.size());
            return true; // consistent snapshot
        }

        // File changed while reading; small backoff and retry
        QThread::msleep(50);
    }

    return false;
}

bool BgmnDelayedParser::readSafely(const QString &f, QList<QByteArray> &out, int maxRetries)
{
    for (int attempt = 0; attempt < maxRetries; ++attempt) {
        QFileInfo beforeInfo(f);
        beforeInfo.refresh();
        const auto sizeBefore = beforeInfo.size();
        const auto mtimeBefore = beforeInfo.lastModified();

        QList<QByteArray> data = BgmnFileIO::readBinaryFileLines(f);
        QFileInfo afterInfo(f);
        afterInfo.refresh();

        if ((afterInfo.size() == sizeBefore) && (afterInfo.lastModified() == mtimeBefore)) {
            out = std::move(data);
            return true; // consistent snapshot
        }

        // File changed while reading; small backoff and retry
        QThread::msleep(50);
    }

    qDebug() << QString("BgmnDelayedParser::waitForStable(): File is still unstable, exiting (%1)").arg(f);
    return false;
}
