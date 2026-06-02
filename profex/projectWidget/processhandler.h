/***************************************************************************
                          processhandler.h  -  description
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

#ifndef PROCESSHANDLER_H
#define PROCESSHANDLER_H

#include <QObject>
#include <QString>
#include <QProcess>

/** \brief Generic external process handler with optional Wine support.
 *
 *  Wraps QProcess to execute an external refinement or helper executable.
 *  Supports running under Wine (Windows emulation) and provides signals
 *   for output polling, completion, and abort notification. */
class ProcessHandler : public QObject
{
Q_OBJECT
public:
    explicit ProcessHandler(QObject *parent = 0);
    ~ProcessHandler();

    void init(const QString &f, const QString &e, bool useW = false, const QString &wEx = QString(), const QStringList &arg = QStringList(), const QString &efl = QString());
    bool run();
    void abort();

    QString readOutput();
    bool isRunning();

    inline QString getExec() const {return processExec;}
    inline QString getFile() const {return processFile;}
    inline bool    getUseWine() const {return useWine;}
    inline QString getWineExec() const {return wineExec;}
    inline QStringList getArguments() const {return arguments;}

private:
    QString processExec;
    QString processFile;
    QProcess *process;
    bool useWine;
    QString wineExec;
    QStringList arguments;

private slots:
    void completed();
    void error();

signals:
    void pollOutput();
    void allComplete();
    void aborted();


};

#endif // PROCESSHANDLER_H
