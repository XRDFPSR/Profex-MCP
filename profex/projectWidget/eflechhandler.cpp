/***************************************************************************
                          eflechhandler.cpp  -  description
                             -------------------
    begin                : Thu Jan 17 15:00:00 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#include "eflechhandler.h"
#include "../../libXrdIO/parser/eflechparparser.h"
#include <QByteArray>
#include <QDir>
#include <QDebug>
#include <QFileInfo>
#include <QRegularExpression>

EflechHandler::EflechHandler(QUuid id, QObject *parent) :
    QObject(parent)
{
    settings = SettingsManager::getInstance();
    uid = id;
    sourceName = QString();
    sourceWmin = 0.0;
    sourceWmax = 0.0;
    process = new QProcess(this);
    connect(process, SIGNAL(finished(int)), this, SLOT(completed()));
    connect(process, SIGNAL(errorOccurred(QProcess::ProcessError)), this, SLOT(error()));
    connect(process, SIGNAL(readyRead()), this, SIGNAL(pollOutput()));
}

EflechHandler::~EflechHandler()
{
    delete process;
}

bool EflechHandler::init(const QString &name, double wmin, double wmax)
{
    sourceName = name;
    sourceWmin = wmin;
    sourceWmax = wmax;

    teilExec = QFileInfo(settings->value("bgmnProject/teilExec", "").toString());
    eflechExec = QFileInfo(settings->value("bgmnProject/eflechExec", "").toString());

    if (!eflechExec.exists() || !teilExec.exists()) {
        qDebug() << QString("EflechHandler::init(): EFLECH or TEIL executable could not be found:");
        qDebug() << QString("  %1").arg(teilExec.absoluteFilePath());
        qDebug() << QString("  %1").arg(eflechExec.absoluteFilePath());
        return false;
    }

    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert("EFLECH", eflechExec.absolutePath());

#ifdef Q_OS_WIN
    env.insert("PATH", env.value("Path") + ";" +  eflechExec.absolutePath());
#else
    env.insert("PATH", env.value("Path") + ":" +  eflechExec.absolutePath());
#endif

    process->setProcessEnvironment(env);
    return true;
}

/*
 * type = 0: run "teil"
 * type = 1: run "eflech"
 */
bool EflechHandler::run(int type, const QString &wdir, QStringList l)
{
    if (process->state() == QProcess::Running) {
        qDebug() << QString("EflechHandler::run(): Process is already running. Exiting.");
        return false;
    }

    process->setWorkingDirectory(wdir);
    args = l;

    switch (type) {
    case 0:
        qDebug() << QString("EflechHandler::run(): Starting process %1 %2").arg(teilExec.absoluteFilePath()).arg(args.join(" "));
        process->start(teilExec.absoluteFilePath(), args);
        break;
    case 1:
        qDebug() << QString("EflechHandler::run(): Starting process %1 %2").arg(eflechExec.absoluteFilePath()).arg(args.join(" "));
        process->start(eflechExec.absoluteFilePath(), args);
        break;
    }

    return true;
}

bool EflechHandler::waitForFinished(int ms)
{
    return process->waitForFinished(ms);
}

void EflechHandler::abort()
{
    qDebug() << "EflechHandler::abort(): Killing process";
    process->kill();
}

void EflechHandler::completed()
{
    emit allComplete(uid);
}

QString EflechHandler::readOutput()
{
    return QString(process->readAllStandardOutput());
}

void EflechHandler::error()
{
    emit aborted();
    qDebug() << "EflechHandler::error(): External process exited with error";
    readOutput();
}

bool EflechHandler::isRunning()
{
    return (process->state() == QProcess::Running);
}

QString EflechHandler::getStrId()
{
    QRegularExpression rx("{([a-zA-Z\\d-]+)}");
    QRegularExpressionMatch rm = rx.match(uid.toString());
    if (rm.hasMatch()) return rm.captured(1);
    return QString();
}

bool EflechHandler::gatherTemporaryFiles(const QList<QFileInfo> &src, const QString &dest, const QString &uid)
{
    bool ok = true;

    for (int i = 0; i < src.size(); ++i) {
        QString dfile(QDir::fromNativeSeparators(dest + "/" + uid + "." + src.at(i).suffix()));
        if (QFile::copy(src.at(i).absoluteFilePath(), dfile)) {
            qDebug() << QString("EflechHandler::gatherTemporaryFiles(): File copied to %1").arg(dfile);
        } else {
            qDebug() << QString("EflechHandler::gatherTemporaryFiles(): Copying %1 to %1 failed.").arg(src.at(i).absoluteFilePath()).arg(dfile);
            ok = false;
        }
    }

    return ok;
}

void EflechHandler::removeTemporaryFiles(const QString &dir, const QString &uid)
{
    QDir wdir(dir);
    QStringList filter(QString("%1*.*").arg(uid));
    QFileInfoList filist = wdir.entryInfoList(filter, QDir::Files);

    for (int i = 0; i < filist.size(); ++i) {
        QFile f(filist.at(i).absoluteFilePath());
        f.remove();
    }
}

QVector<QVector<double> > EflechHandler::parseEflechParFiles(const QString &dir, const QString &uid)
{
    QStringList filter;
    filter << QString("%1-*.par").arg(uid) << QString("%1-*.PAR").arg(uid);

    QDir wdir(dir);
    QFileInfoList filist = wdir.entryInfoList(filter, QDir::Files);

    QVector<QVector<double> > vec = QVector<QVector<double> >(4, QVector<double>());
    EflechParParser eParParser;

    for (int i = 0; i < filist.size(); ++i) {
        if (eParParser.load(filist.at(i).absoluteFilePath())) {
            QVector<QVector<double> > tvec = eParParser.getPeakData();
            vec[0].append(tvec[0]);
            vec[1].append(tvec[1]);
            vec[2].append(tvec[2]);
            vec[3].append(tvec[3]);
        }
    }

    return vec;
}
