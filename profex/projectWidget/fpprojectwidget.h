/***************************************************************************
                          fpprojectwidget.h  -  description
                             -------------------
    begin                : Mon Jan 30 11:00:00 CEST 2012
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

#ifndef FPPROJECTWIDGET_H
#define FPPROJECTWIDGET_H

#include "projectwidget.h"
#include "fphandler.h"
#include "resultsTreeWidget/fpresultstreewidget.h"
#include "../../libXrdIO/parser/fpsumparser.h"

class FpProjectWidget : public ProjectWidget
{
    Q_OBJECT
public:
    FpProjectWidget(QWidget *parent = 0);

    void initSettings();
    QString type() const {return "FP";}

    void abort();
    virtual void runPeakDetection();

    void addRemovePhase();
    QStringList getQuantitiesCsv();
    bool isRunning();
    QString getSampleID();

    void load(const QString &txt, const QString &grph, const QString &uid);

    void saveCifFiles(bool complete, const QMap<QString, QVariant> &auxDataGlobal = QMap<QString, QVariant>());
    void saveCastepCellFiles();
    void setInternalStandard();
    void unsetInternalStandard();
    void generateReport();
    int convertRawDataFile();
    void applyTextBlock(const QString &);
    bool hasControlFile();

    ResultsTreeWidget * getResultsTree() {return dynamic_cast<ResultsTreeWidget*>(resultsTree);}

private:
    FpHandler *fpHandler;
    FpResultsTreeWidget *resultsTree;

    void initFpSettings();
    void updateGui();
    QList<phase> compileQuantities();
    void setHighlighting(ControlFileEdit *);
    int parseProjectFiles() {return 1;}
    void updateResults();

public slots:
    void runRefinement();

private slots:
    void cycleComplete(int cycle, int total);
};

#endif // FPPROJECTWIDGET_H
