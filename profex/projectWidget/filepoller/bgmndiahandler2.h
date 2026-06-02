/***************************************************************************
                          bgmndiahandler2.h  -  description
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

#ifndef BGMNDIAHANDLER2_H
#define BGMNDIAHANDLER2_H

#include <QThread>
#include <QTimer>
#include <QTime>
#include <QFile>
#include "../graphWidget/graphdatacontroller.h"
#include "../../../libXrdIO/settingsmanager.h"

class BgmnDiaHandler2 : public QObject
{
    Q_OBJECT
public:
    explicit BgmnDiaHandler2(QObject *parent = 0);

    void init(GraphDataController *, const QString &s);
    inline void setScanWeighing(const QVector<double> &w) {wi = w;}
    inline bool hasScanWeighing() const                   {return wi.size() > 0;}
    QString monitoredFile() const {return diaFile;}
    double getRwp();
    double getRexp(int p);

public slots:
    void startPolling();
    void stopPolling();
    void poll();

private:
    GraphDataController *scanControl;
    QTimer *pollTimer;
    QString diaFile;
    SettingsManager *settings;
    qint64 lastFileSize;
    qint64 minFileSize;
    qint64 curFileSize;
    int constantSizePolls;
    int currentSizePolls;
    int iRingBufferSize;
    int cRingBufferSize;
    int pollIntervalMS;
    qint64 pollIntervalsToLoad;
    QVector<qint64> sizeRingBuffer;
    QDateTime fileLastModified;
    double Rwp;
    double rDenominator;
    QVector<double> wi;

    bool reloadDelayed();
    void adjustPollInterval();

signals:
    void startPollTimer(int);
    void stopPollTimer();
};

#endif // BGMNDIAHANDLER2_H
