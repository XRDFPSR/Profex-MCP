/***************************************************************************
                          modelmanagerchernyshov.h  -  description
                             -------------------
    begin                : Fri Jan 17 18:00:00 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#ifndef MODELMANAGERCHERNYSHOV_H
#define MODELMANAGERCHERNYSHOV_H

#include <QObject>
#include <QTreeWidget>
#include "qcustomplot/qcustomplot.h"
#include "peakmanagerchernyshov.h"
#include "threadsafeplotter.h"
#include "bgmngeqexport.h"
#include "parameterstorage.h"
#include "structssc.h"
#include "../libXrdIO/settingsmanager.h"

class ModelManagerChernyshov : public QObject
{
    Q_OBJECT
public:
    ModelManagerChernyshov(QCustomPlot *, QCustomPlot *, QTreeWidget *, QTreeWidget *, QObject *parent = nullptr);
    ~ModelManagerChernyshov();

    int addModelPeaks();
    void loadSupportPeaks(const QList<double> &ttDeg, const QList<double> &hwhmDeg, const QList<double> &sh, double meanSh);
    void exportCurves(const QString &);
    void fitAllProfiles();
    void computeFwhm();
    void fitFundamentalParameters();
    void saveOutputFiles(const QString &workingDir, const QString &baseName);
    inline ParameterStorage* getParameters() {return parameterStorage;}

    void setSupportPeaks(const QList<synchro::SupportPeak> &);
    void setProfiles(const QList<synchro::Profile> &);
    QList<synchro::SupportPeak> getSupportPeaks() const;
    QList<synchro::Profile> getProfiles() const;
    bool hasAllFittedCurves() const;

private:
    SettingsManager *settings;
    QCustomPlot *plotFwhm;
    QCustomPlot *plotPeaks;
    QTreeWidget *treeSupportPeaks;
    QTreeWidget *treeModelPeaks;
    ThreadSafePlotter *peakFitPlotter;
    ParameterStorage *parameterStorage;
    QMap<QUuid, PeakManagerChernyshov*> peakManagers;
    BgmnGeqExport *geqExporter;
    QList<QUuid> fitThreadPool;
    int fitThreadsRunning;
    QProgressDialog *progressDlg;
    bool fitAborted;

    void generateSupportPeaks(const QList<double> &, const QList<double> &, const QList<double> &, double meanSh);
    int generateModelPeaks(const QList<double> &, double);
    void updateFwhmPeakPlot();
    QList<double> calculatePeakPositions(double st, double ed, double iv);
    void saveGerFile(const QString &);
    void saveGeqFile(const QString &);
    void saveTplFile(const QString &);
    int getSupportPeakData(QList<double> &ttDeg, QList<double> &fwhmRad, QList<double> &shape);
    void rescaleFwhmAxes(bool x = true, bool y = true);
    QUuid getCurrentUid(bool *ok = nullptr) const;

private slots:
    void fitCompleted(QUuid);
    void peakItemChanged(QTreeWidgetItem*,QTreeWidgetItem*);
    void peakDataChanged(QTreeWidgetItem*,int);
    void initSingleProfile(QUuid);
    void initFromAbove(int idx);
    void initFromBelow(int idx);
    void fitSingleProfile(QUuid);
    void abortFit();

signals:
    void sigFitsComplete();
};

#endif // MODELMANAGERCHERNYSHOV_H
