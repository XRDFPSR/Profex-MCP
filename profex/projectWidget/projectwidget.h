/***************************************************************************
                          projectwidget.h  -  description
                             -------------------
    begin                : Thu Apr 14 18:00:00 CEST 2011
    copyright            : (C) 2011 by Nicola Doebelin
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

#ifndef PROJECTWIDGET_H
#define PROJECTWIDGET_H

#include <QtCore>
#include <QTableWidget>
#include <QTextEdit>
#include <QStackedWidget>
#include <QLabel>
#include <QTreeWidget>
#include <QComboBox>
#include <QDockWidget>
#include <QDragEnterEvent>
#include <QSyntaxHighlighter>
#include <QToolButton>
#include <QDoubleSpinBox>
#include <QPair>
#include <QPrinter>
#include <QPainter>

#include "controlfileedit.h"
#include "projectselecttreeitem.h"
#include "graphWidget/graphwindow.h"
#include "graphWidget/graphdatamodel.h"
#include "graphWidget/graphdatacontroller.h"
#include "chemTable/chemtablewidget.h"
#include "scanListWidget/scanlistwidget.h"
#include "convergencedisplay.h"
#include "peakintegrationwidget.h"
#include "peakListWidget/peaklistwidget.h"
#include "resultsTreeWidget/resultstreewidget.h"
#include "searchMatchWidget/searchmatchwidget.h"
#include "peakFitWidget/peakfitwidget.h"
#include "../../libXrdIO/structs.h"
#include "../../libXrdIO/settingsmanager.h"


struct phaseData {
    QString project;
    QString phase;
    QVariant data;
};

class ProjectSelectTreeItem;

/** \brief Abstract base class for all refinement project widgets.
 *
 *  Provides the common interface and shared functionality for project
 *   widgets of different refinement engines (BGMN, FullProf, etc.).
 *  Manages the graph view, scan list, control file editors, chemistry
 *   table, convergence display, peak integration, search-match, and
 *   peak-fit sub-widgets. Subclasses must implement refinement-specific
 *   operations such as runRefinement(), abort(), load(), and type(). */
class ProjectWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit ProjectWidget(QWidget *parent = nullptr);
    ~ProjectWidget();

    virtual void initSettings() {}

    virtual QString type() const = 0;
    virtual void load(const QString &txt, const QString &grph, const QString &uid) = 0;
    void openTextFileEditors(const QStringList &txt);

    virtual QStringList insertGraph(const QStringList &, const QString &uid, bool switchTab = true);
    virtual bool saveCurrent();
    virtual bool saveCurrentAs();
    virtual void saveAll();
    virtual void saveEditorFile(const QString &);

    virtual void abort() = 0;
    virtual void runPeakDetection() = 0;

    virtual void addRemovePhase() { /* subclass if needed */ }
    virtual void revertControl();

    virtual void raiseTab(int);
    virtual void raiseTab(const QString &, int l = -1);

    virtual bool isIdenticalTo(const QString &basename, const QString &dir);
    virtual void editProjectStrFiles() {/* subclass if needed */}

    virtual QString getResultsFile()             {return QString(); /* subclass if needed */}
    virtual QStringList getQuantitiesCsv()       {return QStringList(); /* subclass if needed */}
    virtual QStringList getResultsCsv()          {return QStringList(); /* subclass if needed */}
    virtual QStringList getChemistry(bool)       {return QStringList(); /* subclass if needed */}
    QString getPeakDataCsv(bool header) const;
    virtual void saveCifFiles(bool complete, const QMap<QString, QVariant> &auxDataGlobal = QMap<QString, QVariant>());
    virtual QList<phaseData>  getCifData(const QMap<QString, QVariant> &auxDataGlobal = QMap<QString, QVariant>());
    virtual QString getSampleID()                {return QString(); /* subclass if needed */}
    virtual QString getControlFileName();
    virtual QString getInstrumentConfigFile(bool)    {return QString(); /* subclass if needed */}

    virtual void saveCastepCellFiles() = 0;
    virtual void getSampleDisplacements(double &, double &, double &) {/* subclass if needed */}
    virtual void setInternalStandard() = 0;
    virtual void unsetInternalStandard() = 0;
    virtual void resetMarginColor();
    virtual void sendMessageToOutputConsole(const QString &s) {refOutput->appendPlainText(s);}
    virtual void exportToExcel() {/* subclass if supported */}
    virtual void editExcelExport() {/* subclass if supported */}

    virtual QString getControlFileContent();
    virtual QString getGraphFile();

    inline GraphWindow * scanView()            {return graphView;}
    inline GraphDataController * dataControl() {return graphControl;}
    inline GraphDataModel * dataModel()        {return graphModel;}

    inline QUuid uId() const {return id;}
    QString workingDir() const {return projectDir;}
    QString baseName() const {return projectBasename;}

    QString currentFileName() const;
    inline virtual global::RefinementStatus status() const {return refStatus;}
    QStringList getOpenFileNames(const QString &ext = QString());

    inline ProjectSelectTreeItem * projectSelectItem()      const {return pSelectItem;}

    virtual int convertRawDataFile(const QStringList &) {return 0; /* subclass if needed */}

    /*
     * returns a list with all files belonging to a project. This is very much dependent on
     * the type of project (BGMN, FP)
     */
    virtual QFileInfoList getAllProjectFilesList() const {return QFileInfoList(); /* subclass if needed */}

    /*
     * returns a list with all input files belonging to a project. This is very much dependent on
     * the type of project (BGMN, FP). Input files are structure, device, and background files.
     * Not included are: DIA, PAR, VAL, LIST, STRUCOUT, SimpleSTRUCOUT, FCFOUT, RESOUT, and PDBOUT
     */
    virtual QFileInfoList getSharedInputProjectFilesList() const {return QFileInfoList(); /* subclass if needed */}

    /*
     * send and receive peak fit curves
     */
    QDomElement getCurveFitData();
    void applyCurveFitData(const QDomElement &);

    virtual double wavelength();
    virtual void setControlFileContent(const QString &);
    virtual void applyControlFileContent(const QString &);
    virtual bool setSequenceInputFiles(const QString &, const QString &) {return true; /* subclass if needed */}
    virtual bool gatherSharedInputFiles(const QString &, const QFileInfoList &, bool) {return true; /* subclass if needed */}
    virtual void prepareForSequenceOrigin() {/* subclass if needed */}
    virtual void applyXmlSettings(const QString &) {/* subclass if needed */}

    virtual void print();
    virtual void printGraphDirectly(QPrinter &, QPainter &);
    virtual QString renderSvgDirectly();
    virtual bool isRunning() = 0;
    virtual void setShowLegend(bool);
    virtual QVector<double> getZoomRange();
    virtual void setZoomRange(double, double, double, double);
    virtual void resetZoomRange();
    virtual QStringList openProjectStrFiles() {return QStringList(); /* subclass if needed */}
    virtual void closeProjectStrFiles() {/* subclass if needed */}
    virtual void closeProjectStrFiles(const QStringList &) {/* subclass if needed */}
    virtual void createPreset() {/* subclass if needed */}
    virtual void createZipArchive(const QString &) {/* subclass if needed */}
    virtual void generateReport() = 0;
    virtual void applyTextBlock(const QString &) {/* subclass if needed */}
    QDomElement getPeakIntegralRanges();
    virtual void applyPeakIntegralRanges(const QDomElement &);
    QString getPeakIntegralCsv();
    QString getCurveFitReport();
    virtual void addAmorphousPeak() {/* subclass if needed */}
    QMap<QString, QMap<int, QStringList> > searchInOpenFiles(const QString &, const QString &);
    virtual void createSinglePeakRefinement() {/* subclass if needed */}
    virtual bool hasControlFile() = 0;
    void startFit(QThreadPool *pool);

    void scanYOffsetUp(bool);
    void scanYOffsetDown(bool);
    void scanXOffsetLeft();
    void scanXOffsetRight();
    void scanOffsetReset();

    void setPeakListFilter(const QDomElement &);
    QDomElement getPeakListFilter() const;

    void searchReplace();
    virtual QStringList openStructureFileUnderCursor() {return QStringList(); /* subclass if needed */}

    virtual void toggleRefStrFavorites(bool) {/* subclass if needed */}
    bool isRefStrFavorites();
    virtual int applyPreset(const QString &, bool) { return -1; /* subclass if needed */ }
    virtual void overrideProjectWavelength(double, const Scan::WavelengthMode &);

    ScanListWidget * getScanList()                     {return scanList;}
    QWidget * getToolbarWidget()                       {return toolbarWidget;}
    QPlainTextEdit * getRefOutput()                    {return refOutput;}
    ChemTableWidget * getChemOutput()                  {return chemOutput;}
    ConvergenceDisplay * getConvDisplay()              {return convDisplay;}
    PeakIntegrationWidget * getPeakIntegrationWidget() {return peakIntegrationWidget;}
    PeakListWidget * getPeakListWidget()               {return peakListWidget;}
    SearchMatchWidget * getSearchMatchWidget()         {return searchMatchWidget;}
    PeakFitWidget * getPeakFitWidget()                 {return peakFitWidget;}
    virtual ResultsTreeWidget * getResultsTree() = 0;

protected:
    ProjectSelectTreeItem *pSelectItem;
    QComboBox *comboBoxRefStrucRepos;
    QComboBox *comboBoxRefStructures;
    QToolButton *refStrResetButton;
    QToolButton *refStrFavoritesButton;
    QToolButton *refStrAppendHklButton;
    QDoubleSpinBox *refHdispValue;
    QToolButton *refHdispResetButton;
    QToolButton *refHdispZeroButton;
    QMenu *menuHklBase;
    QAction *actHklBaseZero;
    QAction *actHklBaseBkgr;
    QAction *actHklBaseDiff;
    QUuid id;
    SettingsManager *settings;
    GraphWindow *graphView;
    GraphDataController *graphControl;
    GraphDataModel *graphModel;
    QSignalMapper *textChangedSignalMapper;

    ScanListWidget *scanList;
    QWidget *toolbarWidget;
    QPlainTextEdit *refOutput;
    ChemTableWidget *chemOutput;
    ConvergenceDisplay *convDisplay;
    PeakIntegrationWidget *peakIntegrationWidget;
    PeakListWidget *peakListWidget;
    SearchMatchWidget *searchMatchWidget;
    PeakFitWidget *peakFitWidget;

    QStringList deleteFiles;
    QStringList textFormats;
    QStringList graphFormats;
    QStringList nativeGraphFormats;
    QFont edFont;

    QString refinerExec;
    QString projectBasename;
    QString projectDir;
    QString loadedGraphFile;
    QString sampleID;
    QString controlFile;
    QString refinedGraphFile;
    QStringList previousControlContent;
    QString controlFileExtension;

    QMap<QString, ControlFileEdit*> editors;
    QMap<ControlFileEdit*, QSyntaxHighlighter*> highlighters;
    QList<QColor> colorTable;
    bool autoShowOutput;
    bool stopOnConvergence;
    bool graphLiveUpdate;
    int switchToPage;
    int nCpus;
    bool slStateRestored;
    global::RefinementStatus refStatus;
    double lastEps1, lastEps2, lastEps3;

    void initGlobalSettings();
    void loadGraph(const QString &, const QString &uid);
    bool loadTextFile(const QString &);
    bool readFile(const QString &fn, ControlFileEdit*);
    bool writeFile(const QString &fn, ControlFileEdit*);
    bool saveGraphFileAs();
    int  editorsTabIndex(const ControlFileEdit *);
    ControlFileEdit * fileEditor(const QString &, bool);
    ControlFileEdit::ControlFileEditFileType textFileType(const QString &);
    virtual void setHighlighting(ControlFileEdit *) = 0;
    void toggleTabNameSaving(ControlFileEdit *, bool);
    virtual void displayReferenceStructure(const QString &) { /* subclass if needed */ }
    virtual int parseProjectFiles() = 0;
    void printText();
    void printGraph();
    void sampleDisplacementsChanged(double, double, double);
    ControlFileEdit* getCurrentEditor();
    virtual QVector<Hkl> getReferenceHklLines(const QString &) {return QVector<Hkl>(); /* subclass if needed */ }
    virtual QVector<QVector<double> > getReferenceXyPattern(const QString &) {return QVector<QVector<double> >(); /* subclass if needed */ }
    virtual Scan getReferenceScan(const QString &) {return Scan(); /* subclass if needed */}
    virtual void updateResults() = 0;
    virtual void autoLoadTextFiles(const QString &caller); // caller is the file that initiated the auto-loading. we won't load it again
    virtual QStringList autoDetermineTextFiles(const QString &caller);
    virtual QString autoDetermineGraphFile();
    virtual QString adjustControlOutputFiles(const QString &, bool) {return QString(); /* subclass if needed */}
    virtual void setupSearchMatchWidget(bool, bool) {/* subclass if needed */}

    // returns all matching patterns
    QMap<int, QStringList> findInEditorText(const ControlFileEdit *, const QString &);

    // returns all matching patterns in a text file on disk
    QMap<int, QStringList> findInFileText(const QString &, const QString &);

public slots:
    void setStatus(global::RefinementStatus);
    virtual QStringList contextHelp() {return QStringList();}
    void removeCurrentScan();
    virtual void runRefinement() = 0;
    virtual void runSearchMatch() {/* subclass if needed */}
    virtual void setReferenceRepoList(const QString &) {/* subclass if needed */}
    virtual void setReferenceStructureFileList() {/* subclass if needed */}

protected slots:
    virtual void processAborted();
    virtual void closeTab(int index);
    virtual void closeTab(const QString &);
    virtual void tabChanged(int);
    virtual void cursorPositionChanged();
    virtual void textChanged(QObject *);
    virtual void referenceStructureChanged(int);
    virtual void resetRefStructure();
    virtual void refStrFavoritesButtonToggled(bool);
    virtual void refStrAppendHklToGraph();
    virtual void referencePosClicked(double, double, double) { /* subclass if needed */ }
    virtual void dumpCoordinates(double, double, double);
    virtual void loadTextFileFromContextMenu(const QString &);
    virtual void loadGraphFileFromContextMenu(const QString &);
    virtual void sampleHeightDisplacementChanged(double);
    virtual void resetSampleHeightDisplacement();
    virtual void zeroSampleHeightDisplacement();
    virtual void selectReferenceHklFile(const QString &);
    virtual void setReferenceXY() { /* subclass if needed */ }
    virtual void setHklBaseMode(QAction*);
    virtual void rangeDoubleClicked(int, int);
    virtual void onFitCompleted(global::RefinementStatus);
    void resetTextZoom();

signals:
    void savingEnabled(bool);
    void currentTabChanged(ProjectWidget*, int);
    void openNewProject(QStringList);
    void setCoordinates(double, double, double);
    void completed(ProjectWidget*);
    void cursorPosition(int, int);
    void updateWavelength(double);
    void referenceStructuresUpdated(QStringList);
    void referenceStructureFavoritesToggled(bool);
    void indexBgmnReferenceStructures();
    void indexFpReferenceStructures();
    void sigAllPeakIntegrals();
    void sigAllCurveFits();
    void openGraphFileInNewProject(QStringList, QString);
    void sigUpdatePresetMenu();
    void scanListUpdated();
    void parameterSelected(QStringList);
    void moduleHelpText(QString);
    void applyPeakFiltersToAll();
    void fitCompleted(ProjectWidget *);
    void sigFileOpenedExclusively(const QStringList &, ProjectWidget *);
};

#endif // PROJECTWIDGET_H
