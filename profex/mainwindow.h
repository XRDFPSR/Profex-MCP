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

/** @brief Main application window for Profex Rietveld refinement software.
 *
 * MainWindow is the central GUI class that manages the entire application
 * workspace. It owns all dock widgets (projects, scans, output, chemistry,
 * convergence, peak integration, etc.), menu bars, toolbars, and
 * status bar indicators. It handles file I/O, project lifecycle
 * (create/open/close/save), refinement execution (single and batch),
 * tool dialogs (peak fitting, baseline correction, scan math, etc.),
 * preferences, import/export, and system tray integration.
 *
 * Signals and slots connect the user interface actions (menu/toolbar
 * clicks) to the underlying refinement logic and project management.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    /** @brief Construct the main window and its child widgets.
     *  @param parent Optional parent widget (normally nullptr for the main window). */
    explicit MainWindow(QWidget *parent = nullptr);
    /** @brief Destructor – saves settings and cleans up dialogs and processes. */
    ~MainWindow();

    /** @brief Load a list of project/text/graph files after startup.
     *  @param list File paths to open. Extension-based auto-detection determines
     *              whether each file is treated as a text scan, graph scan, or project. */
    void loadFileList(const QStringList &list);
    /** @brief Override the path to the Profex executable for child process launches.
     *  @param exe Absolute or relative path to the profex binary. */
    void setProfexExec(const QString &exe);
    /** @brief Set the destination directory for log files written by this session.
     *  @param d Directory path for log output. */
    void setLogDest(const QString &d) {logDest = d;}
    /** @brief Check whether the window should be shown maximised on first display.
     *  @return true if the saved geometry indicates maximised state. */
    bool doShowMaximized();
    /** @brief Perform one-time initialisation after the window is first shown.
     *
     *  Called from showEvent to complete setup that requires a visible widget
     *  (e.g. restoring dock geometries, opening auto-load projects). */
    void postShowInitialization();

protected:
    /** @brief Handle application state changes (e.g. palette, language, window state).
     *  @param e Pointer to the QEvent describing the change. */
    void changeEvent(QEvent *e);
    /** @brief Intercept window close to prompt for unsaved work and stop refinements.
     *  @param e Pointer to the QCloseEvent (accept/ignore to control closing). */
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
    /** @brief Load one or more text-format scan files into new projects.
     *  @param files List of file paths to open.
     *  @return List of ProjectWidget instances created for the loaded files. */
    QList<ProjectWidget *> loadTextFiles(const QStringList &files);
    /** @brief Load one or more graph-format scan files into new projects.
     *  @param files List of file paths to open.
     *  @param filter File-type filter string used by the open dialog.
     *  @return List of ProjectWidget instances created. */
    QList<ProjectWidget *> loadGraphFiles(const QStringList &files, const QString &filter);
    /** @brief Insert graph files into the currently active project.
     *  @param files File paths to insert.
     *  @param filter Format filter used during file selection. */
    void insertGraphFiles(const QStringList &files, const QString &filter);
    /** @brief Create a brand-new empty project. */
    void fileNewProject();
    /** @brief Open text-format scan files via a file dialog. */
    void fileOpenText();
    /** @brief Open graph-format scan files via a file dialog. */
    void fileOpenGraph();
    /** @brief Open an existing project (.pro) file via a file dialog. */
    void fileOpenProject();
    /** @brief Close the currently active project tab. */
    void fileCloseCurrentProject();
    /** @brief Close all open projects, prompting for unsaved changes. */
    void fileCloseAllProjects();
    /** @brief Convert a batch of scan files between formats. */
    void fileScanBatchConversion();
    /** @brief Import a crystal structure from a CIF or other supported format. */
    void fileImportStructure();
    /** @brief Change the active project based on a tree-widget selection change.
     *  @param current The newly selected item.
     *  @param previous The previously selected item. */
    void setCurrentProject(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    /** @brief Change the active project to the one identified by its unique ID.
     *  @param uid The UUID of the project to activate. */
    void setCurrentProject(const QUuid &uid);
    /** @brief Start (or restart) a single refinement on the current project. */
    void runRefinement();
    /** @brief Run refinements for all queued/scheduled projects in sequence. */
    void runBatchRefinement();
    /** @brief Run batch fitting (peak-fitting) for the selected projects. */
    void runBatchFitting();
    /** @brief Called when a fitting run completes on a given project.
     *  @param pw The project whose fitting has just finished. */
    void onFittingComplete(ProjectWidget *pw);
    /** @brief Save the current project's control file. */
    void save();
    /** @brief Save the current project under a new name / location. */
    void saveAs();
    /** @brief Save all open projects that have unsaved changes. */
    void saveAll();
    /** @brief Revert the current project's control file to its last saved state. */
    void revertControlFile();
    /** @brief Abort all currently running refinements immediately. */
    void abortAllRefinements();
    /** @brief Abort only the refinement running in the currently active project. */
    void abortCurrentRefinement();
    /** @brief Open the application preferences dialog. */
    void preferences();
    /** @brief Apply preference changes to all open projects without closing the dialog. */
    void applyPreferences();
    /** @brief Update the status bar with cursor coordinates from the diffraction plot.
     *  @param x 2-theta / Q-space value.
     *  @param y Intensity value.
     *  @param d d-spacing value (Å). */
    void coordinates(double x, double y, double d);
    /** @brief Update the status bar with the current wavelength.
     *  @param lambda Wavelength in Ångström. */
    void statusWavelength(double lambda);
    /** @brief Add or remove a phase from the current project. */
    void addRemovePhase();
    /** @brief Export refinement results to a text/CSV file. */
    void exportResults();
    /** @brief Export refined quantities (phase fractions, etc.) to a file. */
    void exportQuantities();
    /** @brief Export chemistry / elemental composition data. */
    void exportChemistry();
    /** @brief Export CIF files for the selected projects or phases. */
    void exportCifFiles();
    /** @brief Export CASTEP-compatible cell files. */
    void exportCastepCellFiles();
    /** @brief Export project metadata summary (dates, conditions, etc.). */
    void exportProjectInfo();
    /** @brief Export fitted curve data for selected scans. */
    void exportCurveFits();
    /** @brief Apply the control-file edits in the current project (commit changes). */
    void applyControl();
    /** @brief Apply control-file changes for all projects in a batch sequence. */
    void applySequenceControl();
    /** @brief (Re-)apply curve-fit settings stored in the project. */
    void applyCurveFits();
    /** @brief Print the currently displayed graph / plot. */
    void print();
    /** @brief Print all open graph views at once. */
    void printAllGraphs();
    /** @brief Render all graphs to SVG files in the project directory. */
    void renderAllSvg();
    /** @brief Show the About dialog (version, credits, license). */
    void helpAbout();
    /** @brief Show mouse/keyboard shortcut help. */
    void helpMouse();
    /** @brief Open the BGMN variable reference / help page. */
    void helpBgmnVariables();
    /** @brief Called when a BGMN refinement process finishes.
     *  @param pw The project whose refinement completed. */
    void refinementCompleted(ProjectWidget *pw);
    /** @brief Load and display space-group data from the BGMN database. */
    void spacegrpDat();
    /** @brief Load and display atomic scattering factor data (afaparm.dat). */
    void afaparmDat();
    /** @brief Open the BGMN configuration editor (instrument/profile definition). */
    void editConfiguration();
    /** @brief Open the BGMN configuration editor for the current project's instrument. */
    void editCurrentConfiguration();
    /** @brief Toggle the graph legend on or off.
     *  @param visible true to show, false to hide. */
    void showLegend(bool visible);
    /** @brief Respond to changes in the editor cursor position.
     *  @param line Current line number.
     *  @param col  Current column number. */
    void editorCursorChanged(int line, int col);
    /** @brief Display context-sensitive help for a list of topics.
     *  @param topics List of help-key strings to look up. */
    void contextHelp(QStringList topics);
    /** @brief Display context-sensitive help for a single text key.
     *  @param text Help-key string to look up. */
    void contextHelpText(QString text);
    /** @brief Undo the last editing action in the current project. */
    void undo();
    /** @brief Redo the last undone editing action. */
    void redo();
    /** @brief Switch to the given tab index across all open projects.
     *  @param index Tab index (workspace/control/output...). */
    void raiseTabAllProjects(int index);
    /** @brief Switch to the given tab index in the current project only.
     *  @param index Tab index. */
    void raiseTabCurrentProject(int index);
    /** @brief Open a dialog to set the 2-theta zoom range on the graph. */
    void setZoomRange();
    /** @brief Reset the graph zoom range to show the full data extent. */
    void resetZoomRange();
    /** @brief Open structure (.str) files associated with a project. */
    void fileOpenProjectStrFiles();
    /** @brief Close all structure (.str) files for the current project. */
    void fileCloseProjectStrFiles();
    /** @brief Close specific structure files for a given project.
     *  @param files List of structure file paths to close.
     *  @param pw    The project from which to remove the files. */
    void closeProjectStrFiles(const QStringList &files, ProjectWidget *pw);
    /** @brief Open a recently-used graph file via the recent-files menu.
     *  @param action The QAction that triggered this slot. */
    void openRecentGraph(QAction *action);
    /** @brief Open a recently-used text file via the recent-files menu.
     *  @param action The QAction that triggered this slot. */
    void openRecentText(QAction *action);
    /** @brief Open a file from a location bookmark / favourite.
     *  @param action The QAction describing the location to open. */
    void openLocation(QAction *action);
    /** @brief Open the folder containing the current session's log file. */
    void openLogFileLocation();
    /** @brief Insert a graph file into the currently active project. */
    void fileInsertGraphFile();
    /** @brief Shift all scans in the current graph upward by a fixed Y offset. */
    void scanYOffsetUp();
    /** @brief Shift all scans in the current graph downward by a fixed Y offset. */
    void scanYOffsetDown();
    /** @brief Shift all scans in the current graph left (decrease 2θ). */
    void scanXOffsetLeft();
    /** @brief Shift all scans in the current graph right (increase 2θ). */
    void scanXOffsetRight();
    /** @brief Reset all scan offsets to zero. */
    void scanOffsetReset();
    /** @brief Remove the currently selected scan from the project. */
    void removeScan();
    /** @brief Create a template project from the current project's settings. */
    void createTemplate();
    /** @brief Called when the user switches tabs within a project widget.
     *  @param pw The project whose tab changed.
     *  @param index The new tab index. */
    void projectTabChanged(ProjectWidget *pw, int index);
    /** @brief Archive the current project into a ZIP file (default name). */
    void createZipArchive();
    /** @brief Archive the current project into a ZIP file with a chosen name. */
    void createZipArchiveAs();
    /** @brief Set an internal standard phase for quantitative analysis. */
    void setInternalStandard();
    /** @brief Remove the internal standard phase assignment. */
    void unsetInternalStandard();
    /** @brief Generate a printable HTML or PDF report for the current project. */
    void generateReport();
    /** @brief Run instrument profile learning (determine instrument params from standard). */
    void learnProfile();
    /** @brief Open the instrument peak-profile dialog. */
    void instrumentPeakProfile();
    /** @brief Insert a predefined text block at the current editor position.
     *  @param action The QAction identifying which text block to insert. */
    void applyTextBlock(QAction *action);
    /** @brief Open the search-and-replace dialog for the current project's editor. */
    void searchReplace();
    /** @brief Reset the margin colour in the editor to the default. */
    void resetMarginColor();
    /** @brief Toggle the favourites filter on the reference structure browser.
     *  @param toggled true if favourites-only mode is now active. */
    void refStrFavoritesToggled(bool toggled);
    /** @brief Launch BGMN HKL indexing on the current scan. */
    void indexBgmnHkl();
    /** @brief Called when BGMN HKL indexing finishes.
     *  @param errFiles List of file paths that failed indexing. */
    void indexBgmnHklComplete(const QStringList &errFiles);
    /** @brief Launch FullProf (FP) HKL indexing on the current scan. */
    void indexFpHkl();
    /** @brief Open the absorption coefficient calculator dialog. */
    void absorptionCoefficients();
    /** @brief Open the bond-lengths calculator dialog. */
    void bondLengths();
    /** @brief Open the scan math tool dialog. */
    void scanMath();
    /** @brief Apply the current peak-integral limits to all scans in the project. */
    void applyPeakIntegralsToAll();
    /** @brief Export peak integral data to a file. */
    void exportPeakIntegrals();
    /** @brief Export the peak list (positions, widths, areas) to a file. */
    void exportPeakList();
    /** @brief Cut selected text in the current project's editor. */
    void editCut();
    /** @brief Copy selected text in the current project's editor. */
    void editCopy();
    /** @brief Paste text into the current project's editor. */
    void editPaste();
    /** @brief Open a project from a ZIP archive file. */
    void fileOpenProjectArchive();
    /** @brief Open the operating-system file manager at the current project's directory. */
    void projectOpenFileManager();
    /** @brief Open the baseline correction / background subtraction dialog. */
    void addBaseLine();
    /** @brief Open the scan smoothing dialog (Savitzky–Golay, etc.). */
    void smoothScan();
    /** @brief Convert a divergent-slit scan to fixed-slit equivalent. */
    void divSlitConvertScan();
    /** @brief Apply a named preset configuration to the current project.
     *  @param preset Name of the preset to apply. */
    void applyPreset(QString preset);
    /** @brief Save a batch-processing script from the current batch queue. */
    void saveBatchScript();
    /** @brief Open the impurity stripping / phase-purity analysis dialog. */
    void diaStripImpurities();
    /** @brief Open the composition simulation dialog. */
    void diaSimulateCompositions();
    /** @brief Open the result-formatting dialog. */
    void diaFormatResults();
    /** @brief Re-index all available reference structures. */
    void indexReferenceStructures();
    /** @brief Edit the structure (.str) files associated with the current project. */
    void editProjectStrFiles();
    /** @brief Run peak detection on the current scan. */
    void runPeakDetection();
    /** @brief Run search-match (phase identification) on the current scan. */
    void runSearchMatch();
    /** @brief Open the electron-density map calculation dialog. */
    void electronDensityMap();
    /** @brief Trace scan figures / annotations on the graph. */
    void traceScanFigures();
    /** @brief Generate a waterfall (stacked) plot of multiple scans. */
    void waterfallPlot();
    /** @brief Open the synchrotron instrument configuration dialog. */
    void synchrotronConfiguration();
    /** @brief Add/configure an amorphous (background) peak component. */
    void amorphousPeak();
    /** @brief Open the Find-in-Files dialog for searching across projects. */
    void editFindInFiles();
    /** @brief Execute a Find-in-Files operation. */
    void runFindInFiles();
    /** @brief Raise / open a specific file within a project identified by UUID.
     *  @param uid    UUID of the target project.
     *  @param file   File path to open.
     *  @param line   Optional line number to jump to. */
    void raiseProjectFile(QUuid uid, QString file, int line);
    /** @brief Export refinement data to Microsoft Excel format. */
    void exportToExcel();
    /** @brief Configure the Excel export template/options. */
    void editExcelExport();
    /** @brief Open the element scattering data dialog. */
    void elementScatteringData();
    /** @brief Override the wavelength for the current project's instrument. */
    void overrideProjectWavelength();
    /** @brief Open the reference structure browser toolbox. */
    void toolsBrowseReferenceStructures();
    /** @brief Open the currently selected structure file in the system editor. */
    void openCurrentStructureFile();
    /** @brief Refresh the list of reference structures from the database. */
    void updateReferenceStructureList();
    /** @brief Rebuild the preset menu from the presets configuration. */
    void updatePresetMenu();
    /** @brief Open the tube-tail simulator dialog. */
    void simulateTubeTails();
    /** @brief Create a new project from the "Optimal Quantification" (OQ) workflow. */
    void createOqProject();
    /** @brief Check online for a newer version of Profex. */
    void checkForUpdates();
    /** @brief Open the d-spacing calculator dialog. */
    void calculateDspacing();
    /** @brief Apply the current peak-list filter settings to all projects. */
    void applyPeakListFiltersToAll();
    /** @brief Create a single-peak refinement project from the selected peak. */
    void createSinglePeakRefinement();
};

#endif // MAINWINDOW_H
