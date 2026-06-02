/***************************************************************************
                          mainwindow.h  -  description
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSystemTrayIcon>
#include <QSqlDatabase>
#include <QProgressDialog>

#include "backdropwidget.h"
#include "projectwidgetdock.h"
#include "projectWidget/projectwidget.h"
#include "editInstrumentDialog/editinstrumentdialog.h"
#include "tools/spacegroupdialog/spacegroupdialog.h"
#include "tools/atomicscatteringfactordialog/atomicscatteringfactordialog.h"
#include "contexthelpdisplay.h"
#include "preferencesDialog/preferencesdialog.h"
#include "strucImportDialog/strucimportdialog.h"
#include "projectstreewidget.h"
#include "tools/absorptioncoefficientdialog/absorptioncoefficientdialog.h"
#include "tools/elementdatadialog/elementdatadialog.h"
#include "datatable.h"
#include "projectWidget/bgmnrefstructuremanager.h"
#include "tools/scanmathdialog/scanmathdialog.h"
#include "tools/instrumentDialogs/bgmninstrumentprofiledialog.h"
#include "tools/xrdtoolbox/purifyscans/purifyscans.h"
#include "tools/xrdtoolbox/simulatescans/simulatescans.h"
#include "tools/xrdtoolbox/formatresults/formatresults.h"
#include "tools/baselinedialog/baselinedialog.h"
#include "tools/scansmoothdialog/scansmoothdialog.h"
#include "tools/divslitconvertdialog/divslitconvertdialog.h"
#include "tools/browsereferencestructuredialog/browsereferencestructuresdialog.h"
#include "tools/tubetailsimulatordialog/tubetailsimulatordialog.h"
#include "tools/dspacingdialog/calcdspacingdialog.h"
#include "tools/bondlengthdialog/bondlengthdialog.h"
#include "searchInFilesDialog/searchinfilesdialog.h"
#include "../libXrdIO/settingsmanager.h"
#include "../libXrdIO/coddbmanager.h"
#include "../libXrdIO/import/importhandler.h"
#include "presetmenumanager.h"
#include "indexingprogressdialog.h"
#include "statusbarlabel.h"
#include "oqprojectdialog.h"
#include "helptextmanager.h"

namespace Ui {
    class MainWindow;
}

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void loadFileList(const QStringList &);
    void setProfexExec(const QString &);
    void setLogDest(const QString &d) {logDest = d;}
    bool doShowMaximized();
    void postShowInitialization();

protected:
    void changeEvent(QEvent *e);
    void closeEvent(QCloseEvent *e);

private:
    Ui::MainWindow *ui;

    SettingsManager *settings;
    CodDbManager *codManager;
    QString workingDir;
    QString version;
    QString logDest;
    QString lastOpenTextFilter;
    QString lastOpenGraphFilter;
    QString lastOpenProjectFilter;
    QString helpTextGraph;
    QToolBar *referenceToolBar;
    QStackedWidget *projectToolbarStack;
    QStackedWidget *widgetStack;
    BackdropWidget *backdropWidget;

    QDockWidget *dwProjects;
    QDockWidget *dwContextHelp;
    ProjectWidgetDock *dwScans;
    ProjectWidgetDock *dwOutput;
    ProjectWidgetDock *dwChemistry;
    ProjectWidgetDock *dwConvergence;
    ProjectWidgetDock *dwPeakIntegration;
    ProjectWidgetDock *dwPeakList;
    ProjectWidgetDock *dwSearchMatch;
    ProjectWidgetDock *dwResultsTree;
    ProjectWidgetDock *dwPeakFit;
    ProjectsTreeWidget *twProjects;
    PreferencesDialog *preferencesDlg;
    QSystemTrayIcon *sysTrayIcon;
    EditInstrumentDialog *instrdlg;
    SpacegroupDialog *spdlg;
    AtomicScatteringFactorDialog *asfdlg;
    ContextHelpDisplay *chelp;
    StrucImportDialog *stidlg;
    AbsorptionCoefficientDialog *abscoeffdlg;
    BgmnInstrumentProfileDialog *bgmninsprofdlg;
    QString hklBufferDb;
    BgmnRefStructureManager *refStrManager;
    PurifyScans *purifyScansDlg;
    SimulateScans *simScansDlg;
    FormatResults *formatResultsDlg;
    ScanMathDialog *smdlg;
    IndexingProgressDialog *indexProgressDialog;
    BaseLineDialog *blDialog;
    ScanSmoothDialog *sSmoothDlg;
    DivSlitConvertDialog *dsConvDlg;
    SearchInFilesDialog *searchInFilesDlg;
    ElementDataDialog *elScatDataDlg;
    TubeTailSimulatorDialog *ttSimDlg;
    QList<QToolButton*> menuButtons;
    BrowseReferenceStructuresDialog *refstrDlg;
    OqProjectDialog *oqProDlg;
    CalcDspacingDialog *dSpaceDlg;
    HelpTextManager *helpTextMgr;
    BondLengthDialog *bndLengthDlg;

    StatusBarLabel *statusInt;
    StatusBarLabel *statusAng;
    StatusBarLabel *statusD;
    StatusBarLabel *statusCursor;
    StatusBarLabel *statusLambda;
    StatusBarLabel *statusFileName;
    StatusBarLabel *statusOpenProjects;
    ImportHandler *importHandler;
    QMap<QString, int> fExtProjectType;
    QStringList recentFilesGraph;
    QStringList recentFilesText;
    QMenu *menuRecentGraphs;
    QMenu *menuRecentText;
    QMenu *menuPresets;
    QMenu *menuTextBlocks;
    QMenu *menuLocationsStructures;
    QMenu *menuLocationsDevices;
    QMenu *menuLocationsPresets;
    QToolButton *presetButton;
    QToolButton *textBlockButton;
    QAction *logLocationAction;
    PresetMenuManager *presetMenuMgr;
    QProcess *profexEdProcess;
    QProcess *profexStProcess;
    QProcess *profexWpProcess;
    QProcess *profexScProcess;

    void setupGui();
    void initSettings();
    void saveSettings();
    void initConnections();
    void restoreOpenProjects();
    void initProjectConnections(ProjectWidget *);
    void dragEnterEvent( QDragEnterEvent* );
    void dropEvent( QDropEvent* );
    ProjectWidget *createProject(int, const QString &);
    ProjectWidget *getCurrentProject();
    ProjectWidget *getProjectFromTopLevelIndex(int);
    ProjectWidget *getProjectFromUid(const QUuid &);
    bool hasCurrentProject();
    QList<ProjectWidget *> getAllProjects();
    QList<ProjectWidget *> getProjectsByStatus(global::RefinementStatus);
    QList<ProjectWidget *> getProjectSelection(const QString &title, bool skipCurrent = false, bool ignoreType = false, int precheck = 0);
    int countProjects(bool skipCurrent = false, bool ignoreType = false);
    int hasRunningProjects();
    int hasScheduledProjects();
    void setupRecentFiles();
    void closeProject(int, bool);
    void updateNumberOfOpenProject();
    void preferences(const QString &);
    void initTextBlocks(const QMap<QString, QVariant> &);
    QString buildGoalsCsvTable(const QList<DataTable> &);
    void exportSingleCif(const QList<ProjectWidget *> &, const QMap<QString, QVariant> &);
    void exportProjectCif(const QList<ProjectWidget *> &, const QMap<QString, QVariant> &);
    void exportMergedCif(const QList<ProjectWidget *> &, const QMap<QString, QVariant> &);
    void toggleProjectMenusEnabled(bool, const QString &type);
    void clearToolsDialogs();
    void updateToolsDialogs();
    void updateScanMathDlg();
    void updateBaseLineDlg();
    void updateSmoothScanDlg();
    void updateDivSlitConvDlg();
    void updateCalcDspacingDlg();
    void updateBondLengthDlg();
    void startNewBatch();
    void appendToRunningBatch();
    void updateLocationsMenu();
    QString getModuleExe(const QString &);
    void toggleTabDependentActions(ProjectWidget *pw);
    void probeDirectoryAccess(const QString &);

private slots:
    QList<ProjectWidget *> loadTextFiles(const QStringList &);
    QList<ProjectWidget *> loadGraphFiles(const QStringList &, const QString &filter);
    void insertGraphFiles(const QStringList &files, const QString &filter);
    void fileNewProject();
    void fileOpenText();
    void fileOpenGraph();
    void fileOpenProject();
    void fileCloseCurrentProject();
    void fileCloseAllProjects();
    void fileScanBatchConversion();
    void fileImportStructure();
    void setCurrentProject(QTreeWidgetItem*, QTreeWidgetItem*);
    void setCurrentProject(const QUuid &);
    void runRefinement();
    void runBatchRefinement();
    void runBatchFitting();
    void onFittingComplete(ProjectWidget *pw);
    void save();
    void saveAs();
    void saveAll();
    void revertControlFile();
    void abortAllRefinements();
    void abortCurrentRefinement();
    void preferences();
    void applyPreferences();
    void coordinates(double, double, double);
    void statusWavelength(double);
    void addRemovePhase();
    void exportResults();
    void exportQuantities();
    void exportChemistry();
    void exportCifFiles();
    void exportCastepCellFiles();
    void exportProjectInfo();
    void exportCurveFits();
    void applyControl();
    void applySequenceControl();
    void applyCurveFits();
    void print();
    void printAllGraphs();
    void renderAllSvg();
    void helpAbout();
    void helpMouse();
    void helpBgmnVariables();
    void refinementCompleted(ProjectWidget*);
    void spacegrpDat();
    void afaparmDat();
    void editConfiguration();
    void editCurrentConfiguration();
    void showLegend(bool);
    void editorCursorChanged(int, int);
    void contextHelp(QStringList);
    void contextHelpText(QString);
    void undo();
    void redo();
    void raiseTabAllProjects(int);
    void raiseTabCurrentProject(int);
    void setZoomRange();
    void resetZoomRange();
    void fileOpenProjectStrFiles();
    void fileCloseProjectStrFiles();
    void closeProjectStrFiles(const QStringList &, ProjectWidget *);
    void openRecentGraph(QAction *);
    void openRecentText(QAction *);
    void openLocation(QAction *);
    void openLogFileLocation();
    void fileInsertGraphFile();
    void scanYOffsetUp();
    void scanYOffsetDown();
    void scanXOffsetLeft();
    void scanXOffsetRight();
    void scanOffsetReset();
    void removeScan();
    void createTemplate();
    void projectTabChanged(ProjectWidget *, int);
    void createZipArchive();
    void createZipArchiveAs();
    void setInternalStandard();
    void unsetInternalStandard();
    void generateReport();
    void learnProfile();
    void instrumentPeakProfile();
    void applyTextBlock(QAction *action);
    void searchReplace();
    void resetMarginColor();
    void refStrFavoritesToggled(bool);
    void indexBgmnHkl();
    void indexBgmnHklComplete(const QStringList &errFiles);
    void indexFpHkl();
    void absorptionCoefficients();
    void bondLengths();
    void scanMath();
    void applyPeakIntegralsToAll();
    void exportPeakIntegrals();
    void exportPeakList();
    void editCut();
    void editCopy();
    void editPaste();
    void fileOpenProjectArchive();
    void projectOpenFileManager();
    void addBaseLine();
    void smoothScan();
    void divSlitConvertScan();
    void applyPreset(QString);
    void saveBatchScript();
    void diaStripImpurities();
    void diaSimulateCompositions();
    void diaFormatResults();
    void indexReferenceStructures();
    void editProjectStrFiles();
    void runPeakDetection();
    void runSearchMatch();
    void electronDensityMap();
    void traceScanFigures();
    void waterfallPlot();
    void synchrotronConfiguration();
    void amorphousPeak();
    void editFindInFiles();
    void runFindInFiles();
    void raiseProjectFile(QUuid, QString, int);
    void exportToExcel();
    void editExcelExport();
    void elementScatteringData();
    void overrideProjectWavelength();
    void toolsBrowseReferenceStructures();
    void openCurrentStructureFile();
    void updateReferenceStructureList();
    void updatePresetMenu();
    void simulateTubeTails();
    void createOqProject();
    void checkForUpdates();
    void calculateDspacing();
    void applyPeakListFiltersToAll();
    void createSinglePeakRefinement();
};

#endif // MAINWINDOW_H
