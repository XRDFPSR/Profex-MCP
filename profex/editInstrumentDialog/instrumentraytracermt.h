/***************************************************************************
                          instrumentraytracer.h  -  description
                             -------------------
    begin                : Sat Jul 25 11:00:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#ifndef INSTRUMENTRAYTRACERMT_H
#define INSTRUMENTRAYTRACERMT_H

#include "../libXrdIO/settingsmanager.h"
#include "projectWidget/processhandler.h"
#include <QObject>
#include <QProgressBar>
#include <QPlainTextEdit>
#include <QFileInfoList>
#include <QTime>
#include <QElapsedTimer>

/*
 * Multi-threaded raytracer
 *
 * Calls parallel processes of geomet for each peak separately
 */
class InstrumentRayTracerMT : public QObject
{
    Q_OBJECT

public:
    explicit InstrumentRayTracerMT(QObject *parent = nullptr);
    ~InstrumentRayTracerMT();

    void setProgressBar(QProgressBar *);
    void setOutputEditor(QPlainTextEdit *);
    void setConfigFile(const QString &cont, const QString &fn);
    bool isRunning();

public slots:
    void runProcess();
    void abortProcess();

private:
    SettingsManager *settings;
    QProgressBar *progressBar;
    QPlainTextEdit *outputEditor;
    QElapsedTimer perfTimer;

    QList<ProcessHandler *> geometLst;
    QFileInfoList tempSavFiles;
    QFileInfoList completedGerFiles;
    QStringList tempSavAngles;
    QString tempTubeTails;
    ProcessHandler *makegeq;

    QString geometExec;
    QString makeGeqExec;

    QString savFileContent;
    QString savFileName;

    bool abort;
    int nPeaks;
    int nCompletedPeaks;
    double wmin;
    double wmax;

    void initProcesses();

    QStringList getTempAngles(const QString &);
    QString getTempSavFileContent(const QString &);
    void prepareTempFiles(const QString &);
    void mergeTempGerFiles(const QFileInfoList &, const QString &);
    void removeTempFiles(const QFileInfoList &);
    QString getTubeTailFile(const QString &);

    void getAngularRange(double &wmin, double &wmax);
    double getCurrentAngle(const QString &);
    void applyDistortionCorrection();
    QString getGerName();
    QString getSavVariable(const QString &, const QString &);
    QString bgmnMathToJS(const QString &);
    QString elapsedTimeFormatted(int);

    bool launchGeometProcess(ProcessHandler *geomet);
    void abortRunningGeomet();

private slots:
    void pollOutput();
    void geometComplete();
    void makegeqComplete();

signals:
    void processComplete();
};

#endif // INSTRUMENTRAYTRACERMT_H
