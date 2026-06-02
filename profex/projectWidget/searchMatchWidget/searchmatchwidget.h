/***************************************************************************
                          searchmatchwidget.h  -  description
                             -------------------
    begin                : Mon May 20 18:00:00 CEST 2019
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

#ifndef SEARCHMATCHWIDGET_H
#define SEARCHMATCHWIDGET_H

#include <QPlainTextEdit>
#include <QWidget>
#include <QTreeWidgetItem>
#include <QSet>
#include "../bgmnrefstructuremanager.h"
#include "../graphWidget/abstractgraphview.h"
#include "../bgmnhandler.h"
#include "../filepoller/bgmndiahandler2.h"
#include "../libXrdIO/parser/bgmnprotocolparser.h"
#include "periodictabletoggleallbutton.h"
#include "../syntaxHighlighter/bgmnhighlighter.h"

namespace Ui {
class SearchMatchWidget;
}

struct MeanStdDev {
    int n;
    double sum;
    double mean;
    double stdev;
};

struct PhaseStats {
    QString name;
    double fom;
    double gewicht;
    double azero;
    double bzero;
    double czero;
    double arefined;
    double brefined;
    double crefined;
};

enum COLUMN {FILENAME, PHASE, GEWICHT, FOMSOURCE};

class SearchMatchWidget : public AbstractGraphView
{
    Q_OBJECT

public:
    explicit SearchMatchWidget(GraphDataController *, QWidget *parent = 0);
    ~SearchMatchWidget();

    inline void getPreset(QDomDocument &) override {};
    inline void applyPreset(const QDomElement &) override {};

    void initData();
    void clearResults();
    void clearPinned();
    bool isRunning();
    void setWidgets(QPlainTextEdit *);
    void setPinnedPhases(const QList<QPair<QString, QString> > &);
    void setInstrumentFile(const QString &);
    void setLambdaFile(const QString &);
    void setSynchrotron(double);
    inline void setEps2(double d) {defaultEps2 = d;}
    void addReferencePhases(const QList<PeakFile> &);

    QStringList getPinnedPhases();
    QString getDeviceFile();
    bool hasValidInstrumentSelection();
    bool hasValidWavelengthSelection();

private:
    Ui::SearchMatchWidget *ui;
    BgmnRefStructureManager *refStrManager;
    QPlainTextEdit *refOutput;
    QList<HklPhaseData> strFiles;
    BgmnHandler *bgmnHandler;
    BgmnDiaHandler2 *diaHandler;
    QStringList nativeFormats;
    QStringList pinnedPhases;
    MeanStdDev statistics;
    QMenu *contextMenuHeaderResults;
    QMenu *contextMenuHeaderPinned;
    BgmnProtocolParser protocolParser;
    bool hasValidBgmnConfig;
    PeriodicTableToggleAllButton *buttonToggleAllOptional;
    PeriodicTableToggleAllButton *buttonToggleAllMandatory;
    PeriodicTableToggleAllButton *buttonToggleAllOne;
    PeriodicTableToggleAllButton *buttonToggleAllDiscard;
    PeriodicTableToggleAllButton *buttonToggleAllDiscOptional;
    BgmnHighlighter *bhl;

    QFileInfo rawFile;
    QFileInfo valFile;
    QFileInfo diaFile;
    QFileInfo lstFile;
    QFileInfo parFile;
    QFileInfo geqFile;
    QFileInfo untFile;
    QFileInfo strFile;
    QFileInfo localStrFile;
    QString projectGeqFile;
    QString projectLam;
    QString wDir;
    QList<double> fomList;
    bool aborted;
    double defaultEps2;

    void initGui();
    void initSettings();
    void initConnections();
    bool initInstruments();
    bool initLam();
    bool initDbDirs();

    QList<HklPhaseData> getStrFiles();
    bool convertRawFile();
    void startProcess();
    void parseLst();
    PhaseStats calculateFoM(double rwp, QMap<QString, QVariant> localGoals);
    void cellZero(double &a, double &b, double &c);
    void setItemColors();
    void updateMeanAndSd();
    void updateQuantities(const QList<QMap<QString, QVariant> > &l, QTreeWidgetItem *);
    QColor calcFoMColor(double);
    double getPhaseValue(const QString &, const QMap<QString, QVariant> &);
    void changeParameterState(const QString &strFile);

public slots:
    void updateView();
    void run();
    void abort();

private slots:
    void pollOutput();
    void complete(QUuid, global::RefinementStatus);
    void itemChanged(QTreeWidgetItem *, QTreeWidgetItem *);
    void itemClicked(QTreeWidgetItem *, int);
    void itemDoubleClicked(QTreeWidgetItem *, int);
    void pin();
    void unpin();
    void showQuantities(QTreeWidgetItem *, QTreeWidgetItem *);
    void showQuantities(QTreeWidgetItem *, int);
    void save();

    void customMenuRequestedResults(QPoint);
    void toggleColumnHiddenResults(QAction *);
    void customMenuRequestedPinned(QPoint);
    void toggleColumnHiddenPinned(QAction *);
    void headerResultsChanged();
    void headerPinnedChanged();
    void headerDirectoriesChanged();
    void favButtonToggled(bool);
    void itMaxChanged(int);
    void lambdaChanged(QString);
    void synchrotronValueChanged(double);
    void instrumentChanged(QString);
    void anisoToggled(bool);
    void wmaxToggled(bool);
    void wmaxChanged(double);
    void wminToggled(bool);
    void wminChanged(double);
    void eps2Toggled(bool);
    void ruToggled(bool);
    void ruChanged(int);
    void splitterChanged(int, int);
    void crystallinityChanged(int);
    void ucRefineementChanged(int);
    void directorySelectionChanged(QTreeWidgetItem*,int);
    void clearMatchSelection();
    void showHelp();

signals:
    void refStructureSelected(QString);
    void refStructureReset();
    void exitStatus(int);
    void helpText(QString);
};

#endif // SEARCHMATCHWIDGET_H
