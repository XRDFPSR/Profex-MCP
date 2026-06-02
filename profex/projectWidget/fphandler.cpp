/***************************************************************************
                          fphandler.cpp  -  description
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

#include "fphandler.h"
#include "../libXrdIO/parser/fppcrparser.h"
#include <QByteArray>
#include <QDebug>

FpHandler::FpHandler(QObject *parent) :
    QObject(parent)
{
    settings = SettingsManager::getInstance();
    currentCycle = 1;
    maxCycles = 0;

    process = new QProcess(this);
    connect(process, SIGNAL(finished(int)), this, SLOT(completed()));
    connect(process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error()));
}

bool FpHandler::init(const QString &pfile)
{
    refinerExec = settings->value("fpProject/fpExec", "").toString();

    if (!QFileInfo::exists(refinerExec)) {
        qDebug() << QString("FpHandler::init(): Fullprof executable could not be found: %1").arg(refinerExec);
        return false;
    }

    pcrFile = pfile;
    currentCycle = 1;

    qDebug() << QString("FpHandler::init(): Initializing process %1").arg(refinerExec);
    qDebug() << QString("FpHandler::init(): with file %1").arg(pcrFile);

    // store the number of cycles from the PCR file
    FpPcrParser pcp(pcrFile);
    maxCycles = pcp.getCycles();

    abortProcess = false;
    return true;
}

bool FpHandler::run()
{
    // check if abort was requested
    if (abortProcess) {
        emit aborted();
        return false;
    }

    // check if there are any cycles left to process
    if (currentCycle > maxCycles) {
        // we're done
        emit allComplete();
        return false;
    }

    // override the number of cycles, will be restored after the processing cycle
    FpPcrParser pcp(pcrFile);
    pcp.setCycles(1);
    pcp.clearChiSquare();
    pcp.save();

    QStringList arg;

    // set up the process and start
    QFileInfo fiPcr(pcrFile);
    process->setWorkingDirectory(fiPcr.absolutePath());
    arg << fiPcr.fileName();
    process->start(refinerExec, arg);
    qDebug() << QString("FpHandler::run(): Process was started %1 %2").arg(process->program()).arg(process->arguments().join(" "));

    currentCycle++;

    return true;
}

// aborts a running process
void FpHandler::abort()
{
    abortProcess = true;
}

// a cycle has completed
void FpHandler::completed()
{
    FpPcrParser pcp(pcrFile);
    pcp.setCycles(maxCycles);
    pcp.save();

    emit cycleComplete(currentCycle, maxCycles);
}

QString FpHandler::readOutput()
{
    QByteArray ba = process->readAllStandardOutput();
    QString str(ba);
    return str;
}

void FpHandler::error()
{
    FpPcrParser pcp(pcrFile);
    pcp.setCycles(maxCycles);
    pcp.save();

    qDebug() << "FpHandler: External process exited with error";
}


bool FpHandler::isRunning()
{
    if (process->state() == QProcess::Running) {
        return true;
    }

    return false;
}
