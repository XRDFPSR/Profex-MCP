/***************************************************************************
                          bgmnprojectwidget.h  -  description
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

#ifndef BGMNPROJECTWIDGET_H
#define BGMNPROJECTWIDGET_H

#include <QtCore>
#include <QFileInfoList>
#include "projectwidget.h"
#include "bgmnhandler.h"
#include "eflechhandler.h"
#include "bgmnaddremovedialog.h"
#include "strfilebatcheditdialog.h"
#include "bgmnrefstructuremanager.h"
#include "resultsTreeWidget/bgmnresultstreewidget.h"
#include "../libXrdIO/parser/bgmnlstparser.h"
#include "../libXrdIO/parser/bgmnsavparser.h"
#include "../libXrdIO/parser/bgmnstrparser.h"
#include "../libXrdIO/parser/bgmnprotocolparser.h"
#include "filepoller/bgmndiahandler2.h"
#include "msoExport/msoexportexcel.h"
#include "bgmnpresethandlergui.h"

static const QString tempAuxString("temporaryPeakData");

class BgmnPresetHandlerGui;

class BgmnProjectWidget : public ProjectWidget
{
    friend class BgmnPresetHandlerGui;

    Q_OBJECT
public:
    explicit BgmnProjectWidget(QWidget *parent = nullptr);
    ~BgmnProjectWidget();

    void initSettings() override;
    QString type() const override {return "BGMN";}

    void load(const QString &txt, const QString &grph, const QString &uid) override;

    void abort() override;
    void runPeakDetection() override;
    void addRemovePhase() override;
    QStringList getQuantitiesCsv() override;
    QStringList getResultsCsv() override;
    bool isRunning() override;
    QString adjustControlOutputFiles(const QString &, bool) override;
    bool setSequenceInputFiles(const QString &, const QString &) override;
    void prepareForSequenceOrigin() override;
    QStringList openProjectStrFiles() override;
    void closeProjectStrFiles() override;
    void closeProjectStrFiles(const QStringList &) override;
    QStringList getChemistry(bool header) override;

    void createPreset() override;
    void createZipArchive(const QString &) override;
    void getBaselinePreset(QDomDocument &);
    void applyPresetBaseLine(const QDomElement &);
    QFileInfoList getAllProjectFilesList() const override;
    QFileInfoList getAllLoadedScanFilesList() const;

    QFileInfoList getSharedInputProjectFilesList() const override;
    bool gatherSharedInputFiles(const QString &, const QFileInfoList &, bool) override;

    void saveCifFiles(bool bgmnComplete, const QMap<QString, QVariant> &auxDataGlobal = QMap<QString, QVariant>()) override;
    QList<phaseData> getCifData(const QMap<QString, QVariant> &auxDataGlobal = QMap<QString, QVariant>()) override;
    void saveCastepCellFiles() override;
    void setInternalStandard() override;
    void unsetInternalStandard() override;
    void generateReport() override;
    void resetMarginColor() override;
    QString getSampleID() override;
    void toggleRefStrFavorites(bool) override;
    QString getResultsFile() override;
    int applyPreset(const QString &, bool overwrite = true) override;
    void applyXmlSettings(const QString &) override;
    int convertRawDataFile(const QStringList &) override;
    QString getInstrumentConfigFile(bool copy) override;
    void editProjectStrFiles() override;
    void addAmorphousPeak() override;
    void exportToExcel() override;
    void editExcelExport() override;
    void applyTextBlock(const QString &) override;
    QStringList openStructureFileUnderCursor() override;
    QStringList openStructureFileOfActiveGraph();
    QStringList openStructureFileOfSelectItem();
    QStringList openStructureFileOfSavTextBlock(ControlFileEdit *);
    QStringList openStructureFileOfLstTextBlock(ControlFileEdit *);
    bool hasControlFile() override;
    void createSinglePeakRefinement() override;
    QStringList createSinglePeakStrFiles(const QList<Hkl> &, const QString &baseName, bool overwrite);
    inline BgmnLstParser getLstParser() const {return lparser;}

    QHash<QString, CrystalStructure> getStructuresModels();
    QHash<QString, CrystalStructure> getStructuresRefined();
    QStringList getPhaseNames();

    ResultsTreeWidget * getResultsTree() override {return dynamic_cast<ResultsTreeWidget*>(resultsTree);}

private:
    BgmnResultsTreeWidget *resultsTree;
    BgmnDiaHandler2 *diaHandler;
    BgmnHandler *bgmnHandler;
    EflechHandler *eflechHandler;
    BgmnLstParser lparser;
    BgmnProtocolParser protocolParser;
    BgmnAddRemoveDialog *arPhaseDialog;
    StrFileBatchEditDialog *strFileEditDlg;
    BgmnRefStructureManager *refStrManager;
    QElapsedTimer processTimer;
    MsoExportExcel *excelExporter;
    bool hasTubeTails;
    int currentIteration;

    void initBgmnSettings();
    void setHighlighting(ControlFileEdit *) override;
    void bgmnRunProcess();
    void bgmnAbortProcess();

    void eflechRunProcess();
    void eflechAbortProcess();

    void teilRunProcess();
    void teilAbortProcess();

    bool createControlFileFromTemplate(QPlainTextEdit *, BgmnSavParser &, const QString &, bool );
    void displayReferenceStructure(const QString &s) override;
    QStringList getGlobalIncludeList();
    QStringList getLocalIncludeList();
    void compileSumFormula(const QMap<QString, CrystalStructure> &);
    QString defaultControlFile(const QString &geq);
    void getSampleDisplacements(double &e1, double &e2, double &e3) override;
    void matchStrongestPeak();
    int parseProjectFiles() override;
    BgmnSavParser getSavParser(bool *ok = nullptr);
    void setCustomWavelengt(const QString &);
    QStringList removeStrucFiles(BgmnSavParser &, const QStringList &, bool);
    QStringList insertStrucFiles(BgmnSavParser &, const QStringList &, bool);
    QString getPresetName();
    bool checkFilesExist(QFileInfoList &, const QStringList &flist, const QString &dir = QString()) const;
    bool checkFileExists(QFileInfoList &, const QString &file, const QString &dir = QString()) const;
    QVector<Hkl> getReferenceHklLines(const QString &) override;
    QVector<QVector<double> > getReferenceXyPattern(const QString &) override;
    Scan getReferenceScan(const QString &) override;
    QStringList getProjectStrFiles();
    QString getAbsoluteFilePath(const QString &file);
    void clearGui();
    bool eflechSourceDevFile(QString &, QString &);
    bool eflechActiveScanToTemp(const QString &);
    QString eflechTestString();
    void eflechAppendHklData(EflechHandler *, bool replace);
    QString elapsedTimeFormatted(int);
    void updateResults() override;
    QString writeReport();
    void deleteReport();
    void setupSearchMatchWidget(bool instr = true, bool clearPinned = false) override;
    void getEpsValues(double &, double &, double &);
    bool checkTubeTails(const BgmnSavParser &);
    QVector<double> getScanWeighing();
    void setRexpDenom(const BgmnSavParser &);
    QString strFileFromPhaseName(const QString &p);

    void showAddRemovePhases();
    void applyAddRemovePhases();

    QList<BgmnStrParser> getStrParserList();
    QMap<QString, BgmnStrParser> getStrParserMapByFile();
    QMap<QString, BgmnStrParser> getStrParserMapByPhase();

public slots:
    QStringList contextHelp() override;
    void runRefinement() override;
    void runSearchMatch() override;
    void setReferenceStructure(QString);
    void setReferenceRepoList(const QString &currentRepo = QString()) override;
    void setReferenceStructureFileList() override;

private slots:
    void bgmnPollOutput();
    void bgmnComplete(QUuid _uid, global::RefinementStatus _status);

    void eflechPollOutput();
    void eflechComplete();

    void teilPollOutput();
    void teilComplete();

    void searchMatchStatus(int);

    void referencePosClicked(double, double, double) override;
    void applyEditProjectStrFiles();
    void setReferenceXY() override;

    void currentScanChanged(int);
};

#endif // BGMNPROJECTWIDGET_H
