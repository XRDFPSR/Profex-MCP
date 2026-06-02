/***************************************************************************
                          bgmndiahandler2.cpp  -  description
                             -------------------
    begin                : Fri Sep 10 22:22:00 CEST 2021
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


#include "bgmndiahandler2.h"
#include <QDateTime>
#include <QDebug>
// #include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/functions.h"

BgmnDiaHandler2::BgmnDiaHandler2(QObject *parent) : QObject(parent)
{
    settings = SettingsManager::getInstance();
    scanControl = nullptr;
    pollTimer = new QTimer(this);
    connect(pollTimer, SIGNAL(timeout()), this, SLOT(poll()));
    iRingBufferSize = 5;
    Rwp = 100.0;
    rDenominator = -1.0;
}

void BgmnDiaHandler2::init(GraphDataController *g, const QString &s)
{
    scanControl = g;
    diaFile = s;

    if (QFile::exists(s)) QFile::remove(s);

    lastFileSize = -1;
    minFileSize = -1;
    curFileSize = -1;
    rDenominator = -1.0;
    wi.clear();
    constantSizePolls = settings->value("config/diaSizePollCounter", 2).toInt();

    qDebug() << QString("BgmnDiaHandler2::init(): Monitoring file %1").arg(s);
}

void BgmnDiaHandler2::startPolling()
{
    cRingBufferSize = 0;
    pollIntervalsToLoad = 0;
    sizeRingBuffer = QVector<qint64>(iRingBufferSize, -1);

    if (!scanControl) return;

    currentSizePolls = 0;
    fileLastModified = QDateTime();
    pollIntervalMS = settings->value("config/diaSizePollIntervalMS", 1).toInt();
    pollTimer->start(pollIntervalMS);

    qDebug() << QString("BgmnDiaHandler2::startPolling(): Starting poll timer at interval %1 ms").arg(pollTimer->interval());
}

void BgmnDiaHandler2::stopPolling()
{
    pollTimer->stop();
}

void BgmnDiaHandler2::poll()
{
    pollIntervalsToLoad++;

    QFileInfo diaFileInfo(diaFile);
    if (!diaFileInfo.exists()) return;

    // qDebug() << QString("*** polling");
    QDateTime lmod = diaFileInfo.lastModified();
    if (lmod == fileLastModified) {
        // qDebug() << QString("    last modified time hasn't changed (%1), skipping reload.").arg(lmod.toString("hh:mm:ss.zzz"));
        return;
    }

    curFileSize = diaFileInfo.size();

    if ((curFileSize == lastFileSize) && (curFileSize >= minFileSize)) {
        ++currentSizePolls;
        // qDebug() << QString("    currentSizePolls adjusted to %1").arg(currentSizePolls);

        if (currentSizePolls >= constantSizePolls) {
            // we stop the timer to avoid it firing while the file is read
            pollTimer->stop();

            fileLastModified = lmod;

            // qDebug() << QString("    reloading file...");
            if (reloadDelayed()) {
                // qDebug() << QString("    ... success");
                adjustPollInterval();
            } else {
                // qDebug() << QString("    ... failed");
            }

            pollTimer->start(pollIntervalMS);
        }
    } else {
        // qDebug() << QString("    file size is still changing (%1, expected %2), skipping reload.").arg(curFileSize).arg(lastFileSize);
        lastFileSize = curFileSize;
    }
}

bool BgmnDiaHandler2::reloadDelayed()
{
    int loaded = -1;
    int linesRead, linesTotal;

    if (QFileInfo(scanControl->fileName()) == QFileInfo(diaFile)) {
        loaded = scanControl->reloadDiaFile(true, false, linesRead, linesTotal);
    } else {
        loaded = scanControl->loadDiaFile(diaFile, true, scanControl->getSampleId(), false, linesRead, linesTotal);
    }

    // the dia file can shrink a little bit due to changed info in the header
    // the shrinking should not be more than 24 bytes, therefore we set the
    // minimum file size to 24 less than the current file size

    if (scanControl->count() > 0) {
        scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
        minFileSize = lastFileSize - 24;
    } else {
        minFileSize = curFileSize - 24;
    }

    return loaded > 0;
}

void BgmnDiaHandler2::adjustPollInterval()
{
    if (!sizeRingBuffer.size()) return;

    sizeRingBuffer[cRingBufferSize] = pollIntervalsToLoad;
    cRingBufferSize = cRingBufferSize < iRingBufferSize - 1 ? cRingBufferSize + 1 : 0;

    qint64 meanIntervals = 0;
    int n = 0;

    for (int i = 0; i < sizeRingBuffer.size(); ++i) {
        if (sizeRingBuffer.at(i) > 0) {
            meanIntervals += sizeRingBuffer.at(i);
            n++;
        }
    }

    if (n == 0) return;

    meanIntervals /= n;

    int adjustFactor = 5;
    bool doReset = false;

    if (meanIntervals > 100) {
        // polling the file size more than 100 times between reloads is unnecessary
        pollIntervalMS *= adjustFactor;
        qDebug() << QString("BgmnDiaHandler2::adjustPollInterval(): Poll interval increased to %1 ms (polled %2 times)").arg(pollIntervalMS).arg(meanIntervals);
        doReset = true;
    } else if (meanIntervals <= 10 && pollIntervalMS > 1) {
        // polling less than 10 times may miss the completed file
        pollIntervalMS = pollIntervalMS > adjustFactor ? pollIntervalMS / adjustFactor : 1;
        qDebug() << QString("BgmnDiaHandler2::adjustPollInterval(): Poll interval decreased to %1 ms (polled %2 times)").arg(pollIntervalMS).arg(meanIntervals);
        doReset = true;
    }

    if (doReset) {
        sizeRingBuffer = QVector<qint64>(iRingBufferSize, -1);
    }

    pollIntervalsToLoad = 0;
}

double BgmnDiaHandler2::getRwp()
{
    if (scanControl->count() < 2) return 100.0;
    if (!wi.size())               return 100.0;

    if (rDenominator < 0.0) {
        rDenominator = global::Functions::getRdenom(scanControl->at(0), wi);
    }

    double rwpNumerator = global::Functions::getRwpNumer(scanControl->at(0), scanControl->at(1), wi);
    return std::sqrt(rwpNumerator / rDenominator);
}

double BgmnDiaHandler2::getRexp(int p)
{
    if (!scanControl->count()) return 100.0;
    if (!wi.size()) return 100.0;

    if (rDenominator <= 0.0) {
        rDenominator = global::Functions::getRdenom(scanControl->at(0), wi);
    }

    int m = scanControl->first()->size();
    return std::sqrt((m - p)/rDenominator);
}
