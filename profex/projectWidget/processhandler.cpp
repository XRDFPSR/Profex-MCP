/***************************************************************************
                          processhandler.cpp  -  description
                             -------------------
    begin                : Tue Jan 01 16:00:00 CEST 2013
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

#include "processhandler.h"
#include <QByteArray>
#include <QDebug>
#include <QFileInfo>

ProcessHandler::ProcessHandler(QObject *parent) :
    QObject(parent)
{
    process = new QProcess(this);
    connect(process, SIGNAL(finished(int)), this, SLOT(completed()));
    connect(process, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(error()));
    connect(process, SIGNAL(readyRead()), this, SIGNAL(pollOutput()));
}

ProcessHandler::~ProcessHandler()
{
    delete process;
}
/*
 * initializes a generic process from the BGMN program suite:
 * - f: file to process
 * - e: process executable
 * - useW: set to true if e should be executed with wine
 * - wEx: executable of wine
 */
void ProcessHandler::init(const QString &f, const QString &e, bool useW, const QString &wEx, const QStringList &arg, const QString &efl)
{
    processFile = f;
    processExec = e;
    useWine = useW;
    wineExec = wEx;
    arguments = arg;

    QFileInfo fi(processExec);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    if (!efl.isEmpty()) {
        env.insert("EFLECH", efl);
    } else {
        env.insert("EFLECH", fi.absolutePath());
    }

#ifdef Q_OS_WIN
    env.insert("PATH", env.value("Path") + ";" + fi.absolutePath());
#else
    env.insert("PATH", env.value("Path") + ":" + fi.absolutePath());
#endif

    // wine is only used on unix
#ifndef Q_OS_UNIX
    useWine = false;
#endif

    process->setProcessEnvironment(env);
}

/*
 * runs the previously initialized process
 */
bool ProcessHandler::run()
{
    // check if the process is already running
    if (process->state() == QProcess::Running) {
        qDebug() << QString("ProcessHandler::run(): Process is already running. Exiting.");
        return false;
    }

    QFileInfo fi(processFile);
    QStringList arg;

    if (useWine) {
        arg << processExec;
    }

    arg << fi.fileName();

    // add any other arguments, if present
    for (int i = 0; i < arguments.size(); ++i) {
        arg << arguments.at(i);
    }

    // start the process
    process->setWorkingDirectory(fi.absolutePath());

    if (useWine) {
        process->start(wineExec, arg);
    } else {
        process->start(processExec, arg);
    }

    return process->waitForStarted(10000);
}

/*
 * aborts a running process
 */
void ProcessHandler::abort()
{
    process->kill();
}

/*
 * emits a signal when a cycle has completed
 */
void ProcessHandler::completed()
{
    emit allComplete();
}

/*
 * reads and returns the output of the process
 */
QString ProcessHandler::readOutput()
{
    QByteArray ba = process->readAllStandardOutput();
    QString str(ba);
    return str;
}

/*
 * emits a signal when an error occurred
 * also reads the remaining output
 */
void ProcessHandler::error()
{
    qDebug() << QString("ProcessHandler::error(): External process exited with error: %1").arg(process->errorString());
    qDebug() << QString("ProcessHandler::error(): Command: %1").arg(process->program()).arg(process->arguments().join(" "));
    qDebug() << QString("ProcessHandler::error(): Environment variables: %1").arg(process->environment().join(" "));
    emit aborted();
    readOutput();
}

/*
 * returns true if the process is running
 */
bool ProcessHandler::isRunning()
{
    if (process->state() == QProcess::Running) {
        return true;
    }

    return false;
}
