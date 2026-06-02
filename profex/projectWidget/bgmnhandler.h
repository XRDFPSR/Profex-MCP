/***************************************************************************
                          bgmnhandler.h  -  description
                             -------------------
    begin                : Mon Jan 30 16:00:00 CEST 2012
    copyright            : (C) 2012 by Nicola Doebelin
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

#ifndef BGMNHANDLER_H
#define BGMNHANDLER_H

#include <QObject>
#include <QFileInfo>
#include <QString>
#include <QProcess>
#include <QUuid>
#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/structs.h"

class BgmnHandler : public QObject
{
Q_OBJECT
public:
    explicit BgmnHandler(QUuid id, QObject *parent = 0);
    ~BgmnHandler();

    bool init();
    bool run(const QString &, QStringList l = QStringList());
    void abort();

    int totalCycles() {return maxCycles;}
    QString readOutput();
    bool isRunning();
    bool waitForFinished(int);
    QUuid getId() {return uid;}

private:
    QFileInfo refinerExec;
    QFileInfo outputExec;
    QStringList args;
    int currentCycle;
    int maxCycles;
    QProcess *process;
    SettingsManager *settings;
    QUuid uid;
    bool wasKilled;

private slots:
    void completed(int, QProcess::ExitStatus);

signals:
    void pollOutput();
    void allComplete(QUuid,global::RefinementStatus);
    void aborted();
};

#endif // BGMNHANDLER_H
