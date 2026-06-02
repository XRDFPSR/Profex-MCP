/***************************************************************************
                          bgmnhandler.cpp  -  description
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

#include "bgmnhandler.h"
#include <QByteArray>
#include <QDir>
#include <QDebug>
#include <QFileInfo>

BgmnHandler::BgmnHandler(QUuid id, QObject *parent) :
    QObject(parent)
{
    settings = SettingsManager::getInstance();

    uid = id;
    wasKilled = false;

    process = new QProcess(this);
    connect(process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(completed(int,QProcess::ExitStatus)));
    connect(process, SIGNAL(readyRead()), this, SIGNAL(pollOutput()));
}

BgmnHandler::~BgmnHandler()
{
    delete process;
}

bool BgmnHandler::init()
{
    refinerExec = QFileInfo(settings->value("bgmnProject/bgmnExec", "").toString());

    if (!refinerExec.exists()) {
        if (settings->verboseLevel() > 0) qDebug() << QString("BgmnHandler::init(): BGMN executable could not be found: %1").arg(refinerExec.absoluteFilePath());
        return false;
    }

    return true;
}

bool BgmnHandler::run(const QString &wdir, QStringList l)
{
    if (process->state() == QProcess::Running) {
        if (settings->verboseLevel() > 2) qDebug() << QString("BgmnHandler::run(): Process is already running. Exiting.");
        return false;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();

#ifdef Q_OS_WIN
    //env.insert("PATH", env.value("Path") + ";" +  refinerExec.absolutePath());
    // construct a relative path for EFLECH starting from the workingdir, to avoid
    // errors with encodings.
    QDir wd(wdir);
    QFileInfo fiExec(wd.relativeFilePath(refinerExec.absoluteFilePath()));
    env.insert("EFLECH", fiExec.path());
    if (settings->verboseLevel() > 2) qDebug() << QString("BgmnHandler::run(): Setting EFLECH=%1").arg(fiExec.path());
#else
    //env.insert("PATH", env.value("Path") + ":" +  refinerExec.absolutePath());
    // absolute path for EFLECH works on unix
    env.insert("EFLECH", refinerExec.absolutePath());
#endif

    process->setProcessEnvironment(env);
    process->setWorkingDirectory(wdir);
    args = l;

    wasKilled = false;
    if (settings->verboseLevel() > 2) qDebug() << QString("BgmnHandler::run(): Starting process %1 %2").arg(refinerExec.absoluteFilePath(), args.join(" "));
    process->start(refinerExec.absoluteFilePath(), args);

    /*
     // This stuff runs bgmn on a remote system using ssh. ssh login must be passwordless, and the
     // files must be copied to /home/nic/test with scp prior to this
    QString cmd("/usr/bin/ssh");
    QStringList arguments;
    arguments << "nic@192.168.0.60";
    arguments << "cd /home/nic/test;" << "export EFLECH=/opt/bgmnwin-4.2.22;" << "/opt/bgmnwin-4.2.22/bgmn 1310211.sav;" << "exit";
    qDebug() << QString("Running process: %1 %2").arg(cmd).arg(arguments.join(" "));
    process->start(cmd, arguments);
    */

    return true;
}

bool BgmnHandler::waitForFinished(int ms)
{
    return process->waitForFinished(ms);
}

void BgmnHandler::abort()
{
    if (settings->verboseLevel() > 2) qDebug() << QString("BgmnHandler::abort(): Killing process");
    wasKilled = true;
    process->kill();
}

void BgmnHandler::completed(int, QProcess::ExitStatus exstatus)
{
    if (exstatus == QProcess::CrashExit) {
        if (wasKilled) {
            if (settings->verboseLevel() > 2) qDebug() << QString("BgmnHandler::completed(): External process was aborted");
            emit allComplete(uid, global::RefinementStatus::ABORTED);
        } else {
            if (settings->verboseLevel() > 2) qDebug() << QString("BgmnHandler::completed(): External process has crashed");
            emit allComplete(uid, global::RefinementStatus::CRASH);
        }
    } else {
        if (settings->verboseLevel() > 2) qDebug() << QString("BgmnHandler::completed(): External process completed");
        emit allComplete(uid, global::RefinementStatus::COMPLETED);
    }
}

QString BgmnHandler::readOutput()
{
    return QString(process->readAllStandardOutput());
}

bool BgmnHandler::isRunning()
{
    return process->state() == QProcess::Running;
}
