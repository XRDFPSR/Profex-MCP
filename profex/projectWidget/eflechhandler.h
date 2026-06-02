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

#ifndef EFLECHHANDLER_H
#define EFLECHHANDLER_H

#include <QObject>
#include <QFileInfo>
#include <QList>
#include <QString>
#include <QProcess>
#include <QUuid>
#include "../libXrdIO/settingsmanager.h"

class EflechHandler : public QObject
{
Q_OBJECT
public:
    explicit EflechHandler(QUuid id, QObject *parent = 0);
    ~EflechHandler();

    bool init(const QString &name, double wmin, double wmax);
    bool run(int, const QString &, QStringList l = QStringList());
    void abort();

    int totalCycles() {return maxCycles;}
    QString readOutput();
    bool isRunning();
    bool waitForFinished(int);
    QUuid getId() {return uid;}
    QString getStrId();
    bool gatherTemporaryFiles(const QList<QFileInfo> &src, const QString &dest, const QString &uid);
    void removeTemporaryFiles(const QString &dir, const QString &uid);
    QVector<QVector<double> > parseEflechParFiles(const QString &dir, const QString &uid);
    QFileInfo getEflechExec() {return eflechExec;}
    QFileInfo getTeilExec() {return teilExec;}
    inline QString getSourceName() {return sourceName;}
    inline double getSourceWmin()  {return sourceWmin;}
    inline double getSourceWmax()  {return sourceWmax;}

private:
    QFileInfo eflechExec;
    QFileInfo teilExec;
    QStringList args;
    int currentCycle;
    int maxCycles;
    QProcess *process;
    SettingsManager *settings;
    QUuid uid;
    QString sourceName;
    double sourceWmin, sourceWmax;

private slots:
    void completed();
    void error();

signals:
    void pollOutput();
    void allComplete(QUuid);
    void aborted();
};

#endif // EFLECHHANDLER_H
