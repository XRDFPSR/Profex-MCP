/***************************************************************************
                          fphandler.h  -  description
                             -------------------
    begin                : Thu Apr 14 18:00:00 CEST 2011
    copyright            : (C) 2011 by Nicola Doebelin
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

#ifndef FPHANDLER_H
#define FPHANDLER_H

#include <QObject>
#include <QString>
#include <QProcess>
#include "../libXrdIO/settingsmanager.h"

class FpHandler : public QObject
{
Q_OBJECT
public:
    explicit FpHandler(QObject *parent = 0);

    bool init(const QString &pfile);
    bool run();
    void abort();

    int totalCycles() {return maxCycles;}
    QString readOutput();
    bool isRunning();

private:
    SettingsManager *settings;
    QString refinerExec;
    QString pcrFile;
    int currentCycle;
    int maxCycles;
    QProcess *process;
    bool abortProcess;

private slots:
    void completed();
    void error();

signals:
    void cycleComplete(int step, int total);
    void allComplete();
    void aborted();


};

#endif // FPHANDLER_H
