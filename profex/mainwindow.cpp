/***************************************************************************
                          mainwindow.cpp  -  description
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

#include "mainwindow.h"
#include "projectWidget/projectwidget.h"
#include "projectWidget/fpprojectwidget.h"
#include "projectWidget/bgmnprojectwidget.h"
#include "exportgraphdialog.h"
#include "strucImportDialog/strucimportdialog.h"
#include "zoomrangedialog.h"
#include "projectselectdialog.h"
#include "projectWidget/projectselecttreeitem.h"
#include "helpaboutdialog.h"
#include "learnprofiledialog.h"
#include "projectWidget/bgmnbackendconfig.h"
#include "../libXrdIO/structs.h"
#include "cifexportdialog.h"
#include "projectWidget/bgmnindexingerrordialog.h"
#include "ui_mainwindow.h"
#include "../quazip/JlCompress.h"
#include "wavelengthselectdialog.h"
#include "tools/browsereferencestructuredialog/browsereferencestructuresdialog.h"
#include "updatemanager.h"

#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QProgressDialog>
#include <QDir>
#include <QDesktopServices>
#include <QMapIterator>
#include <QStandardPaths>
#include <QWidgetAction>
#include <QThread>
#include <QElapsedTimer>
#include <QIcon>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    bool devel = false;
    settings = SettingsManager::getInstance();
    refStrManager = BgmnRefStructureManager::getInstance();
    codManager = CodDbManager::getInstance();

    // set the program version
    version = QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD);

    if (devel) {
        QLocale loc(QLocale::C);
        QDateTime dt = loc.toDateTime(QString(__DATE__).simplified(), "MMM dd yyyy");
        version += QString("-beta%1").arg(dt.toString("yyMMdd"));
    }

    // print some info
    qDebug() << QString("Profex version %1").arg(version);
    qDebug() << QString("Running executable %1").arg(qApp->applicationFilePath());
    qDebug() << QString("Qt compile version %1").arg(QT_VERSION_STR);
    qDebug() << QString("Qt runtime version %2").arg(qVersion());
    qDebug() << QString("Running on %1 (%2)").arg(QSysInfo::prettyProductName(), QSysInfo::currentCpuArchitecture());

    settings->initialize();

    // create dock widgets
    setupGui();

    // temporarily deactivate some actions in stable releases
    // until they are in a usable condition
    if (!devel) {
        ui->actionLearn_Profile->setVisible(false);
        ui->menu_Process_DIA_Files->setVisible(false);
    }

#ifndef Q_OS_WIN
    ui->actionExport_to_Excel->setVisible(false);
    ui->actionEdit_Excel_Export->setVisible(false);
#endif

    // create some dialogs as null pointers for lazy loading
    preferencesDlg = nullptr;
    spdlg = nullptr;
    asfdlg = nullptr;
    stidlg = nullptr;
    instrdlg = nullptr;
    purifyScansDlg = nullptr;
    simScansDlg = nullptr;
    formatResultsDlg = nullptr;
    indexProgressDialog = nullptr;
    abscoeffdlg = nullptr;
    bgmninsprofdlg = nullptr;
    smdlg = nullptr;
    blDialog = nullptr;
    sSmoothDlg = nullptr;
    dsConvDlg = nullptr;
    searchInFilesDlg = nullptr;
    elScatDataDlg = nullptr;
    refstrDlg = nullptr;
    ttSimDlg = nullptr;
    oqProDlg = nullptr;
    dSpaceDlg = nullptr;
    helpTextMgr = nullptr;
    bndLengthDlg = nullptr;
    importHandler = new ImportHandler();

    profexEdProcess = nullptr;
    profexStProcess = nullptr;
    profexWpProcess = nullptr;
    profexScProcess = nullptr;

    UpdateManager updMgr(qApp->applicationDirPath(), this);
    ui->actionCheck_for_updates->setEnabled(updMgr.checkForInstaller());

    // load the settings
    initSettings();
    initConnections();

    toggleProjectMenusEnabled(false, QString());

    restoreState(settings->value("window/state", QByteArray()).toByteArray(), 0);
    restoreGeometry(settings->value("window/geometry", QByteArray()).toByteArray());

    probeDirectoryAccess(settings->getAppDataLocation());
    probeDirectoryAccess(settings->getTempLocation());
}

void MainWindow::postShowInitialization()
{
    int idx = settings->value("config/indexHkl", 0).toInt();

    if (idx == -1) {
        if (settings->verboseLevel() > 2) qDebug() << QString("MainWindow::postShowInitialization(): Initial indexing of reference structures requested by the user. Running it now, but setting the preference to false.");
        settings->setValue("config/indexHkl", 0);
        indexBgmnHkl();
    } else if (idx == 1) {
        if (settings->verboseLevel() > 2) qDebug() << QString("MainWindow::postShowInitialization(): Indexing of reference structures requested.");
        indexBgmnHkl();
    } else {
        if (settings->verboseLevel() > 2) qDebug() << QString("MainWindow::postShowInitialization(): Indexing of reference structures skipped.");
    }

    restoreOpenProjects();
}

MainWindow::~MainWindow()
{
    // properly delete all projects and related widgets
    while (widgetStack->count() > 1) {
        ProjectWidget *pw = static_cast<ProjectWidget*>(widgetStack->widget(1));

        if (pw) {
            dwScans->removeWidget(pw->getScanList());
            dwOutput->removeWidget(pw->getRefOutput());
            dwChemistry->removeWidget(pw->getChemOutput());
            dwPeakIntegration->removeWidget(pw->getPeakIntegrationWidget());
            dwConvergence->removeWidget(pw->getConvDisplay());
            dwPeakList->removeWidget(pw->getPeakListWidget());
            dwSearchMatch->removeWidget(pw->getSearchMatchWidget());
            dwResultsTree->removeWidget(pw->getResultsTree());
            dwPeakFit->removeWidget(pw->getPeakFitWidget());

            projectToolbarStack->removeWidget(pw->getToolbarWidget());
            widgetStack->removeWidget(pw);
            delete pw;
        }
    }

    twProjects->clear();

    // delete global dialogs
    // check if lazy-loaded dialogs were created
    if (preferencesDlg)      delete preferencesDlg;
    if (stidlg)              delete stidlg;
    if (spdlg)               delete spdlg;
    if (asfdlg)              delete asfdlg;
    if (instrdlg)            delete instrdlg;
    if (indexProgressDialog) delete indexProgressDialog;
    if (abscoeffdlg)         delete abscoeffdlg;
    if (bgmninsprofdlg)      delete bgmninsprofdlg;
    if (smdlg)               delete smdlg;
    if (blDialog)            delete blDialog;
    if (sSmoothDlg)          delete sSmoothDlg;
    if (dsConvDlg)           delete dsConvDlg;
    if (purifyScansDlg)      delete purifyScansDlg;
    if (simScansDlg)         delete simScansDlg;
    if (formatResultsDlg)    delete formatResultsDlg;
    if (elScatDataDlg)       delete elScatDataDlg;
    if (refstrDlg)           delete refstrDlg;
    if (ttSimDlg)            delete ttSimDlg;
    if (presetMenuMgr)       delete presetMenuMgr;
    if (oqProDlg)            delete oqProDlg;
    if (dSpaceDlg)           delete dSpaceDlg;
    if (helpTextMgr)         delete helpTextMgr;
    if (bndLengthDlg)        delete bndLengthDlg;

    if (profexEdProcess) {
        profexEdProcess->terminate();
        profexEdProcess->waitForFinished(10000);
        delete profexEdProcess;
    }

    if (profexStProcess) {
        profexStProcess->terminate();
        profexStProcess->waitForFinished(10000);
        delete profexStProcess;
    }

    if (profexWpProcess) {
        profexWpProcess->terminate();
        profexWpProcess->waitForFinished(10000);
        delete profexWpProcess;
    }

    if (profexScProcess) {
        profexScProcess->terminate();
        profexScProcess->waitForFinished(10000);
        delete profexScProcess;
    }

    if (importHandler)           delete importHandler;
    if (menuRecentGraphs)        delete menuRecentGraphs;
    if (menuRecentText)          delete menuRecentText;
    if (menuTextBlocks)          delete menuTextBlocks;
    if (menuLocationsStructures) delete menuLocationsStructures;
    if (menuLocationsDevices)    delete menuLocationsDevices;
    if (menuLocationsPresets)    delete menuLocationsPresets;
    if (logLocationAction)       delete logLocationAction;

    delete ui;

    // delete the singletons
    refStrManager->destroy();
    settings->destroy();
    codManager->destroy();

    qDebug() << QString("Mainwindow::~MainWindow(): Exiting... good bye!");
}

bool MainWindow::doShowMaximized()
{
    return settings->value("window/maximized", false).toBool();
}

void MainWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void MainWindow::closeEvent(QCloseEvent *e)
{
    if (settings->verboseLevel() > 2) qDebug() << QString("MainWindow::closeEvent(): Saving settings...");

    saveSettings();
    settings->sync();

    if (settings->verboseLevel() > 2) qDebug() << QString("Mainwindow::closeEvent(): Settings saved");

    // note: We have to delete the sysTrayIcon when closing the main window. Else
    // it keeps on running after closing the main window on some systems (KDE5),
    // the destructor is never called, and the event loop never exits.
    if (sysTrayIcon) delete sysTrayIcon;
    e->accept();
}

/*
 * Creates dock widgets, toolbar, and status bar
 */
void MainWindow::setupGui()
{
    bool guiDarkMode = settings->isDarkMode();
    QString iconTheme = guiDarkMode ? QString("profex-dark") : QString("profex-light-colored");

    qDebug() << QString("MainWindow::setupGui(): Available GUI styles:      %1").arg(QStyleFactory::keys().join(", "));
    qDebug() << QString("MainWindow::setupGui(): Current GUI style:         %1").arg(QApplication::style()->name());
    qDebug() << QString("MainWindow::setupGui(): Palette::Base color value: %1").arg(QGuiApplication::palette().color(QPalette::Base).value());
    qDebug() << QString("MainWindow::setupGui(): Setting GUI color mode:    %1").arg(guiDarkMode ? "dark" : "light");
    qDebug() << QString("MainWindow::setupGui(): Setting icon theme:        %1").arg(iconTheme);

    QIcon::setThemeName(iconTheme);

    setDockNestingEnabled(true);

    // construct the main GUI
    ui->setupUi(this);
    setWindowTitle(QString("Profex %1").arg(version));
    widgetStack = new QStackedWidget(this);
    setCentralWidget(widgetStack);
    setAcceptDrops(true);

    backdropWidget = new BackdropWidget;
    widgetStack->addWidget(backdropWidget);

    // create the systray icon
    sysTrayIcon = new QSystemTrayIcon(QIcon(":/icons/profex5.png"), this);
    sysTrayIcon->setToolTip(tr("Idle"));
    sysTrayIcon->show();

    // create widgetstack in project toolbar
    projectToolbarStack = new QStackedWidget(Q_NULLPTR);
    referenceToolBar = addToolBar(tr("Reference Structure Toolbar"));
    referenceToolBar->setObjectName("tbProject");
    referenceToolBar->addWidget(projectToolbarStack);

    // Project list
    dwProjects = new QDockWidget(tr("Projects"), this);
    dwProjects->setObjectName("dwProjects");
    twProjects = new ProjectsTreeWidget(this);
    dwProjects->setWidget(twProjects);
    addDockWidget(Qt::LeftDockWidgetArea, dwProjects);
    ui->menu_Window->addAction(dwProjects->toggleViewAction());

    // Scans list
    dwScans = new ProjectWidgetDock(tr("Plot Options"), this);
    dwScans->setObjectName("dwScans");
    addDockWidget(Qt::LeftDockWidgetArea, dwScans);
    ui->menu_Window->addAction(dwScans->toggleViewAction());
    dwScans->toggleVisibility(true);

    tabifyDockWidget(dwProjects, dwScans);
    dwProjects->raise();

    // Refinement log output
    dwOutput = new ProjectWidgetDock(tr("Refinement Protocol"), this);
    dwOutput->setObjectName("dwOutput");
    addDockWidget(Qt::BottomDockWidgetArea, dwOutput);
    ui->menu_Window->addAction(dwOutput->toggleViewAction());
    dwOutput->toggleVisibility(true);

    // Chemistry output
    dwChemistry = new ProjectWidgetDock(tr("Chemical Composition"), this);
    dwChemistry->setObjectName("dwChemistry");
    addDockWidget(Qt::BottomDockWidgetArea, dwChemistry);
    ui->menu_Window->addAction(dwChemistry->toggleViewAction());
    dwChemistry->close();
    dwChemistry->toggleVisibility(false);

    // Context help
    dwContextHelp = new QDockWidget(tr("Context Help"), this);
    dwContextHelp->setObjectName("dwContextHelp");
    chelp = new ContextHelpDisplay(dwContextHelp);
    dwContextHelp->setWidget(chelp);
    addDockWidget(Qt::RightDockWidgetArea, dwContextHelp);
    ui->menu_Window->addAction(dwContextHelp->toggleViewAction());
    dwContextHelp->close();

    // Convergence graph
    dwConvergence = new ProjectWidgetDock(tr("Convergence Progress"), this);
    dwConvergence->setObjectName("dwConvergence");
    addDockWidget(Qt::LeftDockWidgetArea, dwConvergence);
    ui->menu_Window->addAction(dwConvergence->toggleViewAction());
    dwConvergence->toggleVisibility(true);

    // Peak Integration tables
    dwPeakIntegration = new ProjectWidgetDock(tr("Peak Integrals"), this);
    dwPeakIntegration->setObjectName("dwPeakIntegration");
    addDockWidget(Qt::BottomDockWidgetArea, dwPeakIntegration);
    ui->menu_Window->addAction(dwPeakIntegration->toggleViewAction());
    dwPeakIntegration->close();
    dwPeakIntegration->toggleVisibility(false);

    // Peak list widget
    dwPeakList = new ProjectWidgetDock(tr("Peak List"), this);
    dwPeakList->setObjectName("dwPeakList");
    addDockWidget(Qt::RightDockWidgetArea, dwPeakList);
    ui->menu_Window->addAction(dwPeakList->toggleViewAction());
    dwPeakList->close();
    dwPeakList->toggleVisibility(false);

    // Search/Match widget
    dwSearchMatch = new ProjectWidgetDock(tr("Search/Match Phases"), this);
    dwSearchMatch->setObjectName("dwSearchMatch");
    addDockWidget(Qt::RightDockWidgetArea, dwSearchMatch);
    ui->menu_Window->addAction(dwSearchMatch->toggleViewAction());
    dwSearchMatch->close();
    dwSearchMatch->toggleVisibility(false);
    ui->actionRun_Search_Match->setEnabled(false);
    connect(dwSearchMatch, SIGNAL(visibilityChanged(bool)), ui->actionRun_Search_Match, SLOT(setEnabled(bool)));

    // results tree widget
    dwResultsTree = new ProjectWidgetDock(tr("Refined Parameters"), this);
    dwResultsTree->setObjectName("dwResultsTree");
    addDockWidget(Qt::BottomDockWidgetArea, dwResultsTree);
    ui->menu_Window->addAction(dwResultsTree->toggleViewAction());
    dwResultsTree->toggleVisibility(true);

    dwPeakFit = new ProjectWidgetDock(tr("Peak Fitting"), this);
    dwPeakFit->setObjectName("dwPeakFit");
    addDockWidget(Qt::RightDockWidgetArea, dwPeakFit);
    ui->menu_Window->addAction(dwPeakFit->toggleViewAction());
    dwPeakFit->close();
    dwPeakFit->toggleVisibility(false);

    tabifyDockWidget(dwProjects, dwScans);
    tabifyDockWidget(dwResultsTree, dwChemistry);
    tabifyDockWidget(dwChemistry, dwPeakIntegration);
    tabifyDockWidget(dwPeakList, dwSearchMatch);
    dwProjects->raise();
    dwResultsTree->raise();

    // create widgets in status bar
    statusLambda = new StatusBarLabel(statusBar());
    statusAng = new StatusBarLabel(statusBar());
    statusInt = new StatusBarLabel(statusBar());
    statusD = new StatusBarLabel(statusBar());
    statusCursor = new StatusBarLabel(statusBar());
    statusFileName = new StatusBarLabel(statusBar());
    statusOpenProjects = new StatusBarLabel(statusBar());

    statusLambda->setMinWidth("L = m.mmmmmm");
    statusAng->setMinWidth("2T = mmmmmmmm");
    statusInt->setMinWidth("I = mmmmmmmmmmm");
    statusD->setMinWidth("d = mmmmmmmm");
    statusCursor->setMinWidth("Line: mmmm, Column: mmm");
    statusFileName->setMinWidth("0");
    statusOpenProjects->setMinWidth("999 Projects");

    statusBar()->addPermanentWidget(statusOpenProjects, 0);
    statusBar()->addPermanentWidget(statusLambda, 0);
    statusBar()->addPermanentWidget(statusAng, 0);
    statusBar()->addPermanentWidget(statusInt, 0);
    statusBar()->addPermanentWidget(statusD, 0);
    statusBar()->addPermanentWidget(statusCursor, 0);
    statusBar()->addWidget(statusFileName, 0);

    menuRecentGraphs = new QMenu(QString(tr("Recent Graph Files")), this);
    menuRecentText = new QMenu(QString(tr("Recent Text Files")), this);
    ui->menu_File->insertMenu(ui->actionPrint, menuRecentGraphs);
    ui->menu_File->insertMenu(ui->actionPrint, menuRecentText);
    ui->menu_File->insertSeparator(ui->actionPrint);

    menuPresets = new QMenu(QString(tr("Refinement Presets")), this);
    menuPresets->setIcon(QIcon::fromTheme("profex-preset"));
    ui->menuProject->insertMenu(ui->actionCreatePreset, menuPresets);

    presetMenuMgr = new PresetMenuManager;
    presetMenuMgr->setMenu(menuPresets);
    presetMenuMgr->setPresetRepos(settings->value("bgmnProject/presetDirectory", QStringList()).toStringList());
    presetMenuMgr->update();
    connect(presetMenuMgr, SIGNAL(sigApplyPreset(QString)), this, SLOT(applyPreset(QString)));

    menuLocationsStructures = new QMenu(tr("Structure Repositories"), this);
    menuLocationsDevices    = new QMenu(tr("Device Repositories"), this);
    menuLocationsPresets    = new QMenu(tr("Preset Repositories"), this);
    ui->menu_Locations->addMenu(menuLocationsStructures);
    ui->menu_Locations->addMenu(menuLocationsDevices);
    ui->menu_Locations->addMenu(menuLocationsPresets);

#ifndef Q_OS_LINUX
    logLocationAction = new QAction(tr("Log File"), this);
    logLocationAction->setData("LOGFILE");
    ui->menu_Locations->addSeparator();
    ui->menu_Locations->addAction(logLocationAction);
#else
    logLocationAction = nullptr;
#endif

    // create a tool button in the projectToolBar showing the menuPresets menu
    presetButton = new QToolButton(this);
    presetButton->setToolTip(tr("Create the control file from a refinement preset"));
    presetButton->setText(tr("Presets"));
    presetButton->setMenu(menuPresets);
    presetButton->setPopupMode(QToolButton::InstantPopup);
    presetButton->setIcon(QIcon::fromTheme("profex-preset"));
    ui->projectToolBar->addWidget(presetButton);
    menuButtons.append(presetButton);

    menuTextBlocks = new QMenu(QString(tr("Insert Text Block")), this);
    ui->menu_Edit->insertMenu(ui->actionRevertControlFile, menuTextBlocks);

    // create a tool button in the projectToolBar showing the textBlocks menu
    textBlockButton = new QToolButton(this);
    textBlockButton->setText(tr("Text blocks"));
    textBlockButton->setToolTip(tr("Insert text block at cursor position"));
    textBlockButton->setMenu(menuTextBlocks);
    textBlockButton->setPopupMode(QToolButton::InstantPopup);
    textBlockButton->setIcon(QIcon::fromTheme("profex-text-block"));
    ui->projectToolBar->addWidget(textBlockButton);
    menuButtons.append(textBlockButton);
}

/*
 * initialize the program's settings
 */
void MainWindow::initSettings()
{
    if (settings->verboseLevel() > 2) {
        if (settings->format() == QSettings::NativeFormat)  qDebug() << QString("MainWindow::initSettings(): Settings format Native");
        if (settings->format() == QSettings::IniFormat)     qDebug() << QString("MainWindow::initSettings(): Settings format INI");
        if (settings->format() == QSettings::InvalidFormat) qDebug() << QString("MainWindow::initSettings(): Settings format Invalid");
        qDebug() << QString("MainWindow::initSettings(): Reading settings from %1").arg(settings->fileName());
    }

    workingDir = settings->value("config/workingdir", QDir::homePath()).toString();
    lastOpenTextFilter = settings->value("config/lastOpenTextFilter", "").toString();
    lastOpenGraphFilter = settings->value("config/lastOpenGraphFilter", "").toString();
    lastOpenProjectFilter = settings->value("config/lastOpenProjectFilter", "").toString();

    recentFilesGraph = settings->value("config/recentFilesGraph", QStringList()).toStringList();
    recentFilesText = settings->value("config/recentFilesText", QStringList()).toStringList();
    setupRecentFiles();

    QMap<QString, QVariant> textBlocksFallback;
    for (int i = 0; i < 2*global::defaultTextBlockNumber; i += 2) {
        textBlocksFallback[global::defaultTextBlocks[i]] = global::defaultTextBlocks[i+1];
    }

    QMap<QString, QVariant> textBlocks = settings->value("config/textBlocks", textBlocksFallback).toMap();
    initTextBlocks(textBlocks);

    // default file extensions associated with the different project types
    QStringList fpFext;
    QStringList bgmnFext;

    fpFext << "prf" << "pcr" << "sum";
    bgmnFext << "dia" << "sav" << "lst" << "par" << "val";

    fpFext = settings->value("fpProject/fileExtensions", fpFext).toStringList();
    bgmnFext = settings->value("bgmnProject/fileExtensions", bgmnFext).toStringList();

    for (int i = 0; i < fpFext.size(); ++i) {
        fExtProjectType.insert(fpFext.at(i), 0);
    }

    for (int i = 0; i < bgmnFext.size(); ++i) {
        fExtProjectType.insert(bgmnFext.at(i), 1);
    }

    // no project has indexed the hkl files yet, so we have to initialize the "config/sessionHklIndexed"
    // flag to false. It is correct to *write* this settings value here. It is an initialization.
    settings->setValue("config/sessionHklIndexed", false);

    // initialize the reference structure manager
    refStrManager->init();
    hklBufferDb = refStrManager->getHklBufferFileName();
    connect(refStrManager, SIGNAL(signalUpdateComplete(QStringList)), this, SLOT(indexBgmnHklComplete(QStringList)));

    if (settings->value("config/useLocalCodDb", false).toBool()) {
        QString localCodDb = settings->value("config/localCodDbFile", QString()).toString();
        qDebug() << QString("MainWindow::initSettings(): Initializing COD %1").arg(localCodDb);
        codManager->init(localCodDb, nullptr);
    } else {
        qDebug() << QString("MainWindow::initSettings(): Initialization of COD skipped due to preference setting");
    }

    // read some more settings and try to guess reasonable defaults
    BgmnBackendConfig bgCfg;
    bool hasValidConfig = bgCfg.checkExecutables();
    bool hasValidRepos  = bgCfg.checkRepositories();

    if (!hasValidConfig || !hasValidRepos) {
        QString msgTitle(tr("BGMN Backend Configuration"));
        QString msgText(tr("No valid BGMN configuration found. Would you like to run auto-configuration?"));

        int autoConf = QMessageBox::question(this, msgTitle, msgText, QMessageBox::Yes, QMessageBox::No);

        if (autoConf == QMessageBox::Yes) {
            if (!hasValidConfig) hasValidConfig = bgCfg.guessExecutables();
            if (!hasValidRepos)  hasValidRepos  = bgCfg.guessRepositories();

            if (hasValidConfig && hasValidRepos) {
                msgText = QString(tr("Auto-configuration was successful.\n\n"
                                     "Would you like to index the structure file repository now?\n"
                                     "This will take up to several minuts."));

                int autoIndex = QMessageBox::question(this, msgTitle, msgText, QMessageBox::Yes, QMessageBox::No);

                if (autoIndex == QMessageBox::Yes) {
                    settings->setValue("config/indexHkl", -1);
                }
            } else {
                msgText = QString(tr("Auto-detection of %1%2%3 failed. Please verify the configuration in the following dialog.")
                                  .arg(!hasValidConfig ? "BGMN" : "",
                                  (!hasValidConfig && !hasValidRepos) ? " and " : "",
                                  !hasValidRepos ? "repositories" : ""));
                QMessageBox::warning(this, msgTitle, msgText);

                if (!hasValidConfig) {
                    preferences("Backend Configuration"); // if config or both failed
                } else if (!hasValidRepos) {
                    preferences("Repositories");          // if only repos failed
                }
            }
        }
    }

    ui->actionLegend->setChecked(settings->value("graph/drawLegend", true).toBool());
    ui->actionHkl_Indices->setChecked(settings->value("graph/drawHkl", true).toBool());
    updateLocationsMenu();

    Qt::ToolButtonStyle toolStyle = static_cast<Qt::ToolButtonStyle>(settings->value("window/toolButtonLayout", Qt::ToolButtonIconOnly).toInt());

    ui->mainToolBar->setToolButtonStyle(toolStyle);
    ui->projectToolBar->setToolButtonStyle(toolStyle);
    ui->refinementToolBar->setToolButtonStyle(toolStyle);
    referenceToolBar->setToolButtonStyle(toolStyle);

    for (int i = 0; i < menuButtons.size(); ++i) {
        menuButtons.at(i)->setToolButtonStyle(toolStyle);
    }

    // clear old data from the spacegroup.dat dialog. It will be re-loaded from the settings whe
    // the dialog is opened the next time. We don't reload it here, because it would delay
    // program start even if we don't need the dialog at all
    if (spdlg) spdlg->clear();

    chelp->parseHelpFiles();
}

/*
   store the program's settings
*/
void MainWindow::saveSettings()
{
        settings->setValue("window/state", saveState(0));
        settings->setValue("window/geometry", saveGeometry());
        settings->setValue("window/size", size());
        settings->setValue("window/maximized", isMaximized());

        settings->setValue("config/workingdir", workingDir);
        settings->setValue("config/lastOpenTextFilter", lastOpenTextFilter);
        settings->setValue("config/lastOpenGraphFilter", lastOpenGraphFilter);
        settings->setValue("config/lastOpenProjectFilter", lastOpenProjectFilter);

        QStringList rfGraph;
        QStringList rfText;

        if (recentFilesGraph.size()) {
            rfGraph = recentFilesGraph.mid(qMax(0, recentFilesGraph.length() - 10),
                                           qMin(recentFilesGraph.length(), 10));
        }

        if (recentFilesText.size()) {
            rfText = recentFilesText.mid(qMax(0, recentFilesText.length() - 10),
                                         qMin(recentFilesText.length(), 10));
        }

        settings->setValue("config/recentFilesGraph", rfGraph);
        settings->setValue("config/recentFilesText", rfText);

        settings->setValue("graph/drawLegend", ui->actionLegend->isChecked());
        settings->setValue("graph/drawHkl", ui->actionHkl_Indices->isChecked());

        // collect all open graph files, so we can restore the open projects
        // next time.
        QStringList openGraphFiles;
        QList<ProjectWidget *> openProjects = getAllProjects();
        for (int i = 0; i < openProjects.size(); ++i) {
            openGraphFiles.append(openProjects.at(i)->getGraphFile());
        }

        settings->setValue("config/openProjects", openGraphFiles);
        settings->setValue("config/ProfexVersion", version);
}

/*
 * initialize the Mainwindow's connections
 */
void MainWindow::initConnections()
{
    connect(statusLambda, SIGNAL(sigDoubleClicked()), this, SLOT(overrideProjectWavelength()));
    connect(statusFileName, SIGNAL(sigDoubleClicked()), this, SLOT(projectOpenFileManager()));

    connect(twProjects, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(setCurrentProject(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(twProjects, SIGNAL(runRefinement()), this, SLOT(runRefinement()));
    connect(twProjects, SIGNAL(closeProject()), this, SLOT(fileCloseCurrentProject()));
    connect(twProjects, SIGNAL(exportProjectInfo()), this, SLOT(exportProjectInfo()));

    connect(menuRecentGraphs, SIGNAL(triggered(QAction*)), this, SLOT(openRecentGraph(QAction*)));
    connect(menuRecentText, SIGNAL(triggered(QAction*)), this, SLOT(openRecentText(QAction*)));
    connect(menuTextBlocks, SIGNAL(triggered(QAction*)), this, SLOT(applyTextBlock(QAction*)));
    connect(menuLocationsStructures, SIGNAL(triggered(QAction*)), this, SLOT(openLocation(QAction*)));
    connect(menuLocationsDevices,    SIGNAL(triggered(QAction*)), this, SLOT(openLocation(QAction*)));
    connect(menuLocationsPresets,    SIGNAL(triggered(QAction*)), this, SLOT(openLocation(QAction*)));
    if (logLocationAction) connect(logLocationAction, SIGNAL(triggered()), this, SLOT(openLogFileLocation()));

    // Shortcuts Ctrl+n (n = 1 - 9) raises tab n in the current project
    for (int i = 1; i < 10; ++i) {
        QKeySequence shortcut = QKeySequence(QString("Ctrl+%1").arg(i));
#ifdef Q_OS_MAC
        shortcut = QKeySequence(QString("Meta+%1").arg(i));
#endif
        QShortcut *s = new QShortcut(shortcut, this);
        connect(s, &QShortcut::activated, this, [=]() {
            raiseTabCurrentProject(i - 1);
        });
    }
}

void MainWindow::probeDirectoryAccess(const QString &dir)
{
    if (!settings) {
        qDebug() << QString("MainWindow::probeDirectoryAccess(): Settings not initialized, exiting.");
        return;
    }

    QStringList errors;

    if (!settings->probeDirectoryWriteAccess(dir, errors)) {
        QString errorString(tr("Profex requires write access to the following directory:\n\n"));
        errorString += dir;
        errorString += QString(tr("\n\nError messages were generated:\n\n"));
        errorString += errors.join("\n");
        errorString += QString(tr("\n\nPlease make sure Profex has write access."));

        QMessageBox::warning(this, tr("Probing directory access"), errorString);
    }
}

void MainWindow::restoreOpenProjects()
{
    if (settings->value("config/restoreProjects", false).toBool()) {
        QStringList openProjects = settings->value("config/openProjects", QStringList()).toStringList();

        int npr = openProjects.size();

        if (npr > 20) {
            QMessageBox msgBox;
            msgBox.setText(QString(tr("%1 projects will be restored. This may take a while.\nHow do you want to proceed?")).arg(npr));
            QPushButton *allButton = msgBox.addButton(tr("Load all"), QMessageBox::AcceptRole);
            QPushButton *tenButton = msgBox.addButton(tr("Load first 10"), QMessageBox::RejectRole);
            msgBox.addButton(tr("Skip loading"), QMessageBox::DestructiveRole);

            msgBox.exec();

            if (msgBox.clickedButton() == allButton) {
                loadGraphFiles(openProjects, QString());
                if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::restoreOpenProjects(): %1 projects should be restored, user confirmed loading all").arg(npr);
            } else if (msgBox.clickedButton() == tenButton) {
                loadGraphFiles(openProjects.mid(0, 10), QString());
                if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::restoreOpenProjects(): %1 projects should be restored, but user clicked first 10 only").arg(npr);
            } else {
                if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::restoreOpenProjects(): %1 projects should be restored, but user clicked none").arg(npr);
            }
        } else {
            if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::restoreOpenProjects(): Restoring %1 projects").arg(openProjects.size());
            loadGraphFiles(openProjects, QString());
        }
    } else {
        if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::restoreOpenProjects(): Restoring not requested in settings.");
    }
}

/*
 * populates the Edit->TextBlocks menu with actins
 */
void MainWindow::initTextBlocks(const QMap<QString, QVariant> &blocks)
{
    menuTextBlocks->clear();

    QMap<QString, QVariant>::ConstIterator it = blocks.constBegin();

    while (it != blocks.constEnd()) {
        QAction *action = new QAction(it.key(), this);
        action->setData(it.value());
        menuTextBlocks->addAction(action);
        ++it;
    }
}

/*
 * slot to open preferences dialog
 */
void MainWindow::preferences()
{
    preferences(QString());
}

/*
 * opens the preferences dialog, raises page with title "str". Use
 * empty string to raise the first page. Str is the text shown in the
 * tree widget item of the preferences dialog's navigation list.
 */
void MainWindow::preferences(const QString &str)
{
    qApp->setOverrideCursor(Qt::WaitCursor);

    if (!preferencesDlg) {
        preferencesDlg = new PreferencesDialog(this);
        connect(preferencesDlg, SIGNAL(preferencesApplied()), this, SLOT(applyPreferences()));
    } else {
        preferencesDlg->updatePreferences();
    }

    if (!str.isEmpty()) {
        preferencesDlg->raisePage(str);
    }

    qApp->restoreOverrideCursor();

    // apply settings in real time
    if (preferencesDlg->exec() == QDialog::Accepted) {
        qApp->processEvents();
        applyPreferences();
    }
}

void MainWindow::applyPreferences()
{
    // write settings to disk. It would not be necessary here (will be done in the
    // destructor anyway), but if the program crashes, the settings applied here
    // will be persistent
    settings->sync();

    // settings for the main window
    Qt::ToolButtonStyle tbStyle = static_cast<Qt::ToolButtonStyle>(settings->value("window/toolButtonLayout", 4).toInt());
    ui->mainToolBar->setToolButtonStyle(tbStyle);
    ui->projectToolBar->setToolButtonStyle(tbStyle);
    ui->refinementToolBar->setToolButtonStyle(tbStyle);
    referenceToolBar->setToolButtonStyle(tbStyle);

    if (stidlg) stidlg->initSettings();

    for (int i = 0; i < menuButtons.size(); ++i) {
        menuButtons.at(i)->setToolButtonStyle(tbStyle);
    }

    chelp->parseHelpFiles();
    updateLocationsMenu();

    QMap<QString, QVariant> textBlocks = settings->value("config/textBlocks", QMap<QString, QVariant>()).toMap();
    if (!textBlocks.isEmpty()) initTextBlocks(textBlocks);

    /* removed in 5.2.2
    QString newIcons = settings->value("config/iconTheme", QString()).toString();

    if ((newIcons != QIcon::themeName()) && (!newIcons.isEmpty())) {
        QIcon::setThemeName(newIcons);
        update();
    }
    */

    int n = twProjects->count();
    QProgressDialog pdlg(tr("Applying preferences to projects..."), tr("Abort"), 0, n, this);
    pdlg.setWindowModality(Qt::WindowModal);
    pdlg.setMinimumDuration(0);

    // settings for projects
    for (int i = 0; i < n; ++i) {
        ProjectWidget *pw = getProjectFromTopLevelIndex(i);
        pdlg.setValue(i);

        if (pw) {
            pw->initSettings();
            if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::preferences(): Updating settings of project %1").arg(pw->baseName());
        }

        if (pdlg.wasCanceled()) {
            if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::preferences(): Applying preferences "
                                                                  "to projects was aborted by the user. Leaving %1 "
                                                                  "projects not updated.").arg(n - i);
            break;
        }
    }

    pdlg.close();

    if (settings->value("bgmnProject/strIndexingRequired", false).toBool()) {
        if (QMessageBox::question(this,
                                  tr("Index structure files"),
                                  tr("Structure repositories have changed.\nDo you want to re-index the structure files?"))
                == QMessageBox::Yes) {
            indexBgmnHkl();
        }
    }
}

void MainWindow::updateLocationsMenu()
{
    QStringList strDirs = settings->value("bgmnProject/structureDatabaseIndexed", QStringList()).toStringList();
    strDirs.append(settings->value("bgmnProject/structureDatabaseNonIndexed", QStringList()).toStringList());
    QStringList devDirs = settings->value("bgmnProject/deviceDatabase", QStringList()).toStringList();
    QStringList prsDirs = settings->value("bgmnProject/presetDirectory", QStringList()).toStringList();

    QList<QAction *> lStr = menuLocationsStructures->actions();
    QList<QAction *> lDev = menuLocationsDevices->actions();
    QList<QAction *> lPrs = menuLocationsPresets->actions();

    menuLocationsStructures->clear();
    menuLocationsDevices->clear();
    menuLocationsPresets->clear();

    while (!lStr.isEmpty()) {
        QAction *act = lStr.takeFirst();
        if (act) delete act;
    }

    while (!lDev.isEmpty()) {
        QAction *act = lDev.takeFirst();
        if (act) delete act;
    }

    while (!lPrs.isEmpty()) {
        QAction *act = lPrs.takeFirst();
        if (act) delete act;
    }

    for (int i = 0; i < strDirs.size(); ++i) {
        QAction *act = new QAction(strDirs.at(i), this);
        act->setData(strDirs.at(i));
        menuLocationsStructures->addAction(act);
    }

    for (int i = 0; i < devDirs.size(); ++i) {
        QAction *act = new QAction(devDirs.at(i), this);
        act->setData(devDirs.at(i));
        menuLocationsDevices->addAction(act);
    }

    for (int i = 0; i < prsDirs.size(); ++i) {
        QAction *act = new QAction(prsDirs.at(i), this);
        act->setData(prsDirs.at(i));
        menuLocationsPresets->addAction(act);
    }
}

void MainWindow::openLocation(QAction *act)
{
    QUrl url = QUrl::fromLocalFile(act->data().toString());

    if (QDesktopServices::openUrl(url)) {
        qDebug() << QString("MainWindow::openLocation(): Opening %1").arg(url.toDisplayString());
    } else {
        qDebug() << QString("MainWindow::openLocation(): Opening %1 failed").arg(url.toDisplayString());
    }
}

void MainWindow::openLogFileLocation()
{
    QFileInfo fi(logDest);
    QUrl url = QUrl::fromLocalFile(fi.absolutePath());

    if (QDesktopServices::openUrl(url)) {
        if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::openLogFileLocation(): Opening %1").arg(url.toDisplayString());
    } else {
        if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::openLogFileLocation(): Opening %1 failed").arg(url.toDisplayString());
    }
}

/*
 * raises tab t on the current project
 */
void MainWindow::raiseTabCurrentProject(int t)
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->raiseTab(t);
}

/*
 * raises tab t on all open projects
 */
void MainWindow::raiseTabAllProjects(int t)
{
    QString s = getCurrentProject()->tabText(t);

    for (int i = 0; i < twProjects->count(); ++i) {
        ProjectWidget *pw = getProjectFromTopLevelIndex(i);
        if (pw) pw->raiseTab(s);
    }
}

/*
 * this function accepts a list of files and tries to determine if the files
 * are text or graph files. It is used to parse the command line and drag&drop
 * events
 */
void MainWindow::loadFileList(const QStringList &l)
{
    QStringList graphExtensions(importHandler->extensions());

    QStringList gfiles;
    QStringList tfiles;

    for (int i = 0; i < l.size(); ++i) {
        QFileInfo f(l.at(i));
        if (f.exists()) {
            if (graphExtensions.contains(f.suffix().toLower())) {
                gfiles.append(l.at(i));
            } else {
                tfiles.append(l.at(i));
            }
        }
    }

    if (l.size()) {
        // filter is an empty string, because files of different formats
        // may be loaded
        loadGraphFiles(gfiles, QString());
        loadTextFiles(tfiles);
    }
}

void MainWindow::dragEnterEvent(QDragEnterEvent *e)
{
    if (e->mimeData()->hasFormat("text/uri-list")) {
        e->acceptProposedAction();
    }
}

void MainWindow::dropEvent(QDropEvent *e)
{
    QList<QUrl> u = e->mimeData()->urls();
    QStringList l;

    for (int i = 0; i < u.size(); ++i) {
        l.append(u.at(i).toLocalFile());
    }

    if (!l.size()) return;

    if (e->modifiers().testFlag(Qt::ControlModifier)) {
        insertGraphFiles(l, QString());
        return;
    }

    if (e->modifiers().testFlag(Qt::NoModifier)) {
        loadFileList(l);
        return;
    }
}

/*
 * creates a new project
 */
void MainWindow::fileNewProject()
{
    /* TODO */
}

/*
 * Open a text file.
 */
void MainWindow::fileOpenText()
{
    QStringList filters;
    filters << tr("BGMN Control File (*.SAV *.sav)");
    filters << tr("Fullprof Control File (*.PCR *.pcr)");
    filters << tr("Structure File (*.STR *.str)");
    filters << tr("Results Summary (*.SUM *.sum)");
    filters << tr("Results List (*.LST *.lst)");
    filters << tr("Output File (*.OUT *.out)");
    filters << tr("All Files (*.*)");

    QFileDialog fdlg(this, tr("Open File"), workingDir);
    fdlg.setAcceptMode(QFileDialog::AcceptOpen);
    fdlg.setNameFilters(filters);
    fdlg.setFileMode(QFileDialog::ExistingFiles);
    fdlg.setOption(QFileDialog::HideNameFilterDetails, false);
    fdlg.selectNameFilter(lastOpenTextFilter);

    if (!lastOpenTextFilter.isEmpty()) {
        fdlg.selectNameFilter(lastOpenTextFilter);
    }

    if (fdlg.exec() == QDialog::Accepted) {
        QStringList fn = fdlg.selectedFiles();
        lastOpenTextFilter = fdlg.selectedNameFilter();
        loadTextFiles(fn);

        recentFilesText += fn;
        setupRecentFiles();
    }
}

/*
 * open a raw scan file
 */
void MainWindow::fileOpenGraph()
{
    QStringList filters;
    filters.append(importHandler->rawFilters());

    QFileDialog fdlg(this, tr("Open Raw Data File"), workingDir);
    fdlg.setAcceptMode(QFileDialog::AcceptOpen);
    fdlg.setNameFilters(filters);
    fdlg.setFileMode(QFileDialog::ExistingFiles);
    fdlg.setOption(QFileDialog::HideNameFilterDetails, false);

    if (!lastOpenGraphFilter.isEmpty()) {
        fdlg.selectNameFilter(lastOpenGraphFilter);
    }

    if (fdlg.exec() == QDialog::Accepted) {
        QStringList fn = fdlg.selectedFiles();
        lastOpenGraphFilter = fdlg.selectedNameFilter();
        if (settings->verboseLevel() > 2) qDebug() << QString("MainWindow::fileOpenGraph(): Last Open Graph Filter was: %1").arg(lastOpenGraphFilter);
        if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::fileOpenGraph(): Loading file: %1").arg(fn.join(", "));
        loadGraphFiles(fn, lastOpenGraphFilter);

        recentFilesGraph += fn;
        setupRecentFiles();
    }
}

/*
 * open a refinement project
 */
void MainWindow::fileOpenProject()
{
    QStringList filters;
    filters = importHandler->projectFilters();

    QFileDialog fdlg(this, tr("Open Refinement Project"), workingDir);
    fdlg.setAcceptMode(QFileDialog::AcceptOpen);
    fdlg.setNameFilters(filters);
    fdlg.setFileMode(QFileDialog::ExistingFiles);
    fdlg.setOption(QFileDialog::HideNameFilterDetails, false);

    if (!lastOpenProjectFilter.isEmpty()) {
        fdlg.selectNameFilter(lastOpenProjectFilter);
    }

    if (fdlg.exec()) {
        QStringList fn = fdlg.selectedFiles();
        lastOpenProjectFilter = fdlg.selectedNameFilter();
        if (settings->verboseLevel() > 2) qDebug() << QString("MainWindow::fileOpenProject(): Last Open Project Filter was: %1").arg(lastOpenProjectFilter);
        loadGraphFiles(fn, lastOpenProjectFilter);

        recentFilesGraph += fn;
        setupRecentFiles();
    }
}

/*
 * loads text files from a QStringList into new project widgets
 * Profex will attempt to load all the other files, too. So it doesn't really matter
 * which one was selected. If the other files don't exist, they are just skipped. But this
 * code is handled in ProjectWidget, here we just pass the filenames on and register
 * the project in the treeWidget (twProjects).
 */
QList<ProjectWidget *> MainWindow::loadTextFiles(const QStringList &fn)
{
    int defaultProjectType = settings->value("config/defaultProjectType", 1).toInt();
    QList<ProjectWidget *> projects;

    QProgressDialog *progDlg = new QProgressDialog(tr("Loading Files"), tr("Cancel"), 0, fn.size(), this);

    QTreeWidgetItem *newCurrent = nullptr;

    // loop through the stringlist of file names
    for (int i = 0; i < fn.size(); ++i) {

        // advance the progress dialog and check if the abort button was pressed
        progDlg->setValue(i);

        if (progDlg->wasCanceled()) {
            progDlg->close();
            break;
        }

        qApp->processEvents();

        // determine file name, base name, directory, extension etc.
        QFileInfo fi(fn.at(i));
        QString fext = fi.suffix().toLower();
        QString bn = fi.completeBaseName();
        QString dr = fi.absolutePath();

        // special treatment for *.str files, because they have different basenames, but
        // belong to the current project
        if ((fext == "str") && (widgetStack->count() > 1)) {
            ProjectWidget *pw = getCurrentProject();
            if (pw) {
                // only load to existing project if the project type is bgmn
                if (pw->type().toLower() == "bgmn") {
                    pw->openTextFileEditors(QStringList(fn.at(i)));
                    // file loaded, go to next one
                    continue;
                }
            }
        }

        bool projectExists = false;

        // check if a project with the same basename and directory already exists. if yes,
        // don't create a new one, update the existing one instead.
        for (int j = 0; j < twProjects->count(); ++j) {
            ProjectWidget *pw = getProjectFromTopLevelIndex(j);
            if (!pw) continue;

            if (pw->isIdenticalTo(bn, dr)) {
                pw->openTextFileEditors(QStringList(fn.at(i)));
                projectExists = true;
                break;
            }
        }

        // text was loaded into an existing project
        if (projectExists) {
            // all done, go to the next file
            continue;
        }

        // identify the type of project to be created
        int type = fExtProjectType.value(fext, defaultProjectType);

        // create the project widget
        ProjectWidget *pw = createProject(type, fn.at(i));
        if (pw) {
            pw->load(fn.at(i), QString(), QString());
            projects.append(pw);
            if (!newCurrent) newCurrent = pw->projectSelectItem();
        }
    }

    // twProjects->sortItems(0, Qt::AscendingOrder);

    // activate the first new project
    if (newCurrent) twProjects->setCurrentItem(newCurrent);

    progDlg->close();
    delete progDlg;
    return projects;
}

QList<ProjectWidget *> MainWindow::loadGraphFiles(const QStringList &fn, const QString &filter)
{
    QElapsedTimer timer;
    timer.start();

    QList<ProjectWidget *> projects;
    int defaultProjectType = settings->value("config/defaultProjectType", 1).toInt();

    QProgressDialog *progDlg = new QProgressDialog(tr("Loading Files..."), tr("Cancel"), 0, fn.size(), this);
    progDlg->setWindowTitle(tr("Open Projects"));
    progDlg->setAutoClose(false);

    // the default behaviour of the progress dialog doesn't seem to work reliably. Sometimes it is not shown
    // even though a large number of files is opened. So let's force it to show as soon as there are 3 or more
    // files to open.
    if (fn.size() > 2) progDlg->show();

    QTreeWidgetItem *newCurrent = nullptr;

    // get the uid of the file format. If filter is QString(), it will be determined
    // for each file separately below
    QString uid = importHandler->uidByFilter(filter);
    static QRegularExpression rxFilter("\\(\\*\\.\\*\\)$");
    QRegularExpressionMatch rmFilter = rxFilter.match(filter);
    bool autoDetectFormat = (rmFilter.hasMatch() || filter.isNull());

    // loop through the list of filenames and create new projects
    for (int i = 0; i < fn.size(); ++i) {
        if (!QFile::exists(fn.at(i))) {
            qDebug() << QString("MainWindow::loadGraphFiles: File doesn't exist: %1").arg(fn.at(i));
            continue;
        }

        // get this file's uid if no filter was provided
        if (autoDetectFormat) {
            uid = importHandler->uidByFileName(fn.at(i));
        }

        if (settings->verboseLevel() > 2) qDebug() << QString("MainWindow::loadGraphFiles: Loading file %1 with uid = %2").arg(fn.at(i), uid);

        if (!importHandler->isSupported(fn.at(i), uid)) {
            progDlg->close();
            QMessageBox::information(this, tr("Unknown file format"), tr("No import filter found for file:\n%1").arg(fn.at(i)));
            qDebug() << QString("MainWindow::loadGraphFiles: Unknown file format, aborting: %1").arg(fn.at(i));
            break;
        }

        progDlg->setValue(i);

        if (progDlg->wasCanceled()) {
            break;
        }

        qApp->processEvents();

        // extract the file extension and project type to be created (BGMN or FP)
        QFileInfo fi(fn.at(i));
        QString fext = fi.suffix().toLower();

        int type = fExtProjectType.value(fext, defaultProjectType);

        ProjectWidget *pw = createProject(type, fn.at(i));

        if (pw) {
            pw->load(QString(), fn.at(i), uid);
            statusWavelength(pw->wavelength());
            projects.append(pw);

            if (!newCurrent) {
                newCurrent = pw->projectSelectItem();
            }
        } else {
            qDebug() << QString("MainWindow::loadGraphFiles: Could not create project of type %1 for file extension %2").arg(type).arg(fext);
        }
    }

    if (newCurrent) {
        twProjects->setCurrentItem(newCurrent);
        setCurrentProject(newCurrent, nullptr);
    }

    progDlg->close();
    delete progDlg;

    if (settings->verboseLevel() > 2) qDebug() << QString("MainWindow::loadGraphFiles(): Needed %1 ms to open %2 files").arg(timer.elapsed()).arg(fn.size());

    return projects;
}

void MainWindow::fileInsertGraphFile()
{
    // set up the file dialog
    QStringList filters;
    filters = importHandler->filters();
    filters << tr("All Files (*.*)");

    QFileDialog fdlg(this, tr("Import Graph File"), workingDir);
    fdlg.setAcceptMode(QFileDialog::AcceptOpen);
    fdlg.setNameFilters(filters);
    fdlg.setFileMode(QFileDialog::ExistingFiles);
    fdlg.setOption(QFileDialog::HideNameFilterDetails, false);

    if (!lastOpenGraphFilter.isEmpty()) {
        fdlg.selectNameFilter(lastOpenGraphFilter);
    }

    // sent graph files to project widget
    if (fdlg.exec() == QDialog::Accepted) {
        QStringList fn = fdlg.selectedFiles();

        lastOpenGraphFilter = fdlg.selectedNameFilter();
        insertGraphFiles(fn, lastOpenGraphFilter);
    }
}

void MainWindow::insertGraphFiles(const QStringList &files, const QString &filter)
{
    QStringList fn = files;
    ProjectWidget *pw = getCurrentProject();
    if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::insertGraphFiles(): Inserting files %1").arg(files.join("; "));

    if (!pw) {
        // apparently no existing projectWidget available yet. So lets create a new one
        // from the first file name, and then proceed as planned
        QStringList firstfn(fn.takeFirst());
        loadGraphFiles(firstfn, filter);

        // try again to get a pointer to the new projectWidget
        pw = getCurrentProject();

        // still not ok? -> Exit
        if (!pw) {
            qDebug() << QString("MainWindow::insertGraphFiles(): Could not create project. Exiting.");
            return;
        } else {
            if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::insertGraphFiles(): New project was created.");
        }
    }

    QStringList err;

    for (int i = 0; i < fn.size(); ++i) {
        QString uid;
        if (filter.isEmpty()) uid = importHandler->uidByFileName(fn.at(i));
        else                  uid = importHandler->uidByFilter(filter);

        err.append(pw->insertGraph(QStringList(fn.at(i)), uid, true));
        recentFilesGraph.append(fn.at(i));
    }

    setupRecentFiles();

    if (err.size()) {
        qDebug() << QString("MainWindow::insertGraphFiles(): The following files could not be opened: %1").arg(err.join("; "));
        QMessageBox::information(this, tr("Unknown file format"),
                                 tr("The following files could not be opened:\n%1").arg(err.join("\n")));
    }
}

/*
 * creates a new projectWidget of type "type"
 */
ProjectWidget * MainWindow::createProject(int type, const QString &file)
{
    if ((type < 0) || (type > 1)) {
        qDebug() << QString("MainWindow::createProject(): Type %1 is invalid").arg(type);
        return nullptr;
    }

    QFileInfo fi(file);
    workingDir = fi.absolutePath();
    settings->setValue("config/workingdir", workingDir);

    // create the project
    ProjectWidget *pw = nullptr;

    switch (type) {
        case 0:  pw = new FpProjectWidget(this); break;
        case 1:  pw = new BgmnProjectWidget(this); break;
        default: pw = new BgmnProjectWidget(this); break;
    }

    twProjects->addTopLevelItem(pw->projectSelectItem());

    widgetStack->addWidget(pw);
    projectToolbarStack->addWidget(pw->getToolbarWidget());

    dwScans->addWidget(pw->getScanList());
    dwOutput->addWidget(pw->getRefOutput());
    dwChemistry->addWidget(pw->getChemOutput());
    dwConvergence->addWidget(pw->getConvDisplay());
    dwPeakIntegration->addWidget(pw->getPeakIntegrationWidget());
    dwPeakList->addWidget(pw->getPeakListWidget());
    dwSearchMatch->addWidget(pw->getSearchMatchWidget());
    dwResultsTree->addWidget(pw->getResultsTree());
    dwPeakFit->addWidget(pw->getPeakFitWidget());

    initProjectConnections(pw);
    updateNumberOfOpenProject();
    toggleProjectMenusEnabled(true, pw->type());

    return pw;
}

/*
   initializes connections between a projectWidget and this. Always call this function
   after creating a new projectWidget.
*/
void MainWindow::initProjectConnections(ProjectWidget *pw)
{
    connect(pw, SIGNAL(openNewProject(QStringList)), this, SLOT(loadTextFiles(QStringList)));
    connect(pw, SIGNAL(setCoordinates(double,double,double)), this, SLOT(coordinates(double,double,double)));
    connect(pw, SIGNAL(completed(ProjectWidget*)), this, SLOT(refinementCompleted(ProjectWidget*)));
    connect(pw, SIGNAL(cursorPosition(int,int)), this, SLOT(editorCursorChanged(int,int)));
    connect(pw, SIGNAL(updateWavelength(double)), this, SLOT(statusWavelength(double)));
    connect(pw, SIGNAL(currentTabChanged(ProjectWidget*,int)), this, SLOT(projectTabChanged(ProjectWidget*,int)));
    connect(pw, SIGNAL(referenceStructureFavoritesToggled(bool)), this, SLOT(refStrFavoritesToggled(bool)));
    connect(pw, SIGNAL(sigAllPeakIntegrals()), this, SLOT(applyPeakIntegralsToAll()));
    connect(pw, SIGNAL(openGraphFileInNewProject(QStringList,QString)), this, SLOT(loadGraphFiles(QStringList,QString)));
    connect(pw, SIGNAL(tabBarDoubleClicked(int)), this, SLOT(raiseTabAllProjects(int)));
    connect(pw, SIGNAL(sigAllCurveFits()), this, SLOT(applyCurveFits()));
    connect(pw, SIGNAL(sigUpdatePresetMenu()), this, SLOT(updatePresetMenu()));
    connect(pw, SIGNAL(parameterSelected(QStringList)), this, SLOT(contextHelp(QStringList)));
    connect(pw, SIGNAL(moduleHelpText(QString)), this, SLOT(contextHelpText(QString)));
    connect(pw, SIGNAL(applyPeakFiltersToAll()), this, SLOT(applyPeakListFiltersToAll()));
    connect(pw, &ProjectWidget::sigFileOpenedExclusively, this, &MainWindow::closeProjectStrFiles);
}

/*
   Close all selected projects.
*/
void MainWindow::fileCloseCurrentProject()
{
    if (!twProjects->count()) {
        return;
    }

    QList<QTreeWidgetItem *> selection = twProjects->selectedItems();
    if (selection.isEmpty()) selection.append(twProjects->currentItem());

    QVector<int> indices;
    for (int i = 0; i < selection.size(); ++i) {
        indices.append(twProjects->indexOfTopLevelItem(selection.at(i)));
    }

    std::sort(indices.begin(), indices.end());

    while (indices.size()) {
        closeProject(indices.takeLast(), true);
    }
}

/*
 * closes all open projects. Checks whether some are running
 */
void MainWindow::fileCloseAllProjects()
{
    if (hasRunningProjects()) {
        QMessageBox::information(this, tr("Close all projects"), tr("Please stop all running refinements first."));
        return;
    }

    qApp->setOverrideCursor(Qt::WaitCursor);

    twProjects->setCurrentItem(twProjects->topLevelItem(0));

    QProgressDialog *progDlg = new QProgressDialog(tr("Closing projects"), tr("Cancel"), 0, twProjects->count(), this);
    progDlg->setWindowTitle(tr("Closing all projects"));

    while (twProjects->count()) {
        closeProject(twProjects->count() - 1, false);

        progDlg->setValue(progDlg->value() + 1);
        qApp->processEvents();

        if (progDlg->wasCanceled()) break;
    }

    delete progDlg;
    qApp->restoreOverrideCursor();
}

void MainWindow::closeProject(int n, bool update)
{
    ProjectWidget *pw = getProjectFromTopLevelIndex(n);
    if (!pw) return; // something fishy!?! Exit safely.

    if (pw->isRunning()) {
        if (QMessageBox::question(this, "Closing Project",
                                  "Refinement is running.\nDo you want to close it anyway?",
                                  QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
            return;
        }

        pw->abort();
    }

    statusFileName->setText(QString());

    // if the currently selected item is going to be removed,
    // we must set the current item to the new position first
    if (update) {
        if (n == 0) {
            // the current Item was at pos 0, so lets select the one at pos 1,
            // but first make sure there is another project at pos 1.
            if (twProjects->count() > 1) {
                twProjects->setCurrentItem(twProjects->topLevelItem(1));
                setCurrentProject(twProjects->currentItem(), nullptr);
            }
        } else {
            // the current item was at a pos > 0, so lets select the previous one
            twProjects->setCurrentItem(twProjects->topLevelItem(n - 1));
            setCurrentProject(twProjects->currentItem(), nullptr);
        }
    }

    // remove all stack widgets of the closed project
    // TODO: handle unsafed data
    dwScans->removeWidget(pw->getScanList());
    dwOutput->removeWidget(pw->getRefOutput());
    dwChemistry->removeWidget(pw->getChemOutput());
    dwConvergence->removeWidget(pw->getConvDisplay());
    dwPeakIntegration->removeWidget(pw->getPeakIntegrationWidget());
    dwPeakList->removeWidget(pw->getPeakListWidget());
    dwSearchMatch->removeWidget(pw->getSearchMatchWidget());
    dwResultsTree->removeWidget(pw->getResultsTree());
    dwPeakFit->removeWidget(pw->getPeakFitWidget());

    projectToolbarStack->removeWidget(pw->getToolbarWidget());
    widgetStack->removeWidget(pw);
    delete pw;

    // now we can safely delete the previously selected toplevelitem
    delete twProjects->takeTopLevelItem(n);

    if (!twProjects->count()) {
        toggleProjectMenusEnabled(false, QString());
        clearToolsDialogs();

        statusLambda->setText(QString());
        statusAng->setText(QString());
        statusInt->setText(QString());
        statusD->setText(QString());
        statusCursor->setText(QString());
        statusFileName->setText(QString());
    }

    updateNumberOfOpenProject();
}

/*
   another project was selected in the tree widget. Raise the corresponding page
   in the widget stack and enable/disable menu actions
*/
void MainWindow::setCurrentProject(QTreeWidgetItem *cur, QTreeWidgetItem *prev)
{
    if (!cur)        return;
    if (cur == prev) return;

    setCurrentProject(twProjects->getProjectUid(cur));
}

void MainWindow::setCurrentProject(const QUuid &uid)
{
    if (uid.isNull()) return;
    ProjectWidget *pw = getProjectFromUid(uid);
    if (!pw) return;

    bool ok = true;

    // check if all widgets we want to raise are present
    if (widgetStack->indexOf(pw) < 0)                                    ok = false;
    if (projectToolbarStack->indexOf(pw->getToolbarWidget()) < 0)        ok = false;
    if (dwScans->indexOf(pw->getScanList()) < 0)                         ok = false;
    if (dwOutput->indexOf(pw->getRefOutput()) < 0)                       ok = false;
    if (dwChemistry->indexOf(pw->getChemOutput()) < 0)                   ok = false;
    if (dwConvergence->indexOf(pw->getConvDisplay()) < 0)                ok = false;
    if (dwPeakIntegration->indexOf(pw->getPeakIntegrationWidget()) < 0)  ok = false;
    if (dwPeakList->indexOf(pw->getPeakListWidget()) < 0)                ok = false;
    if (dwSearchMatch->indexOf(pw->getSearchMatchWidget()) < 0)          ok = false;
    if (dwResultsTree->indexOf(pw->getResultsTree()) < 0)                ok = false;
    if (dwPeakFit->indexOf(pw->getPeakFitWidget()) < 0)                  ok = false;

    if (!ok) return;

    widgetStack->setCurrentWidget(pw);
    projectToolbarStack->setCurrentWidget(pw->getToolbarWidget());

    dwScans->setCurrentWidget(pw->getScanList());
    dwOutput->setCurrentWidget(pw->getRefOutput());
    dwChemistry->setCurrentWidget(pw->getChemOutput());
    dwConvergence->setCurrentWidget(pw->getConvDisplay());
    dwPeakIntegration->setCurrentWidget(pw->getPeakIntegrationWidget());
    dwPeakList->setCurrentWidget(pw->getPeakListWidget());
    dwSearchMatch->setCurrentWidget(pw->getSearchMatchWidget());
    dwResultsTree->setCurrentWidget(pw->getResultsTree());
    dwPeakFit->setCurrentWidget(pw->getPeakFitWidget());

    statusWavelength(pw->wavelength());
    statusFileName->setText(QDir::toNativeSeparators(pw->currentFileName()));
    updateToolsDialogs();
    toggleTabDependentActions(pw);
}

/*
 * execute the refinement for the current projectWidget
 */
void MainWindow::runRefinement()
{
    ProjectWidget *pw = getCurrentProject();
    if (!pw) return;

    if (!pw->isRunning()) {
        sysTrayIcon->setIcon(QIcon(":/icons/profex5-active.png"));
        sysTrayIcon->setToolTip(tr("Running..."));
        pw->runRefinement();
        dwOutput->raise();
    }
}

/*
 *  start batch refinement
 */
void MainWindow::runBatchRefinement()
{
    if (hasScheduledProjects()) {
        appendToRunningBatch();
    } else {
        startNewBatch();
    }
}

/*
 * shows a project select dialog and starts a new batch refinement
 */
void MainWindow::startNewBatch()
{
    int nparallel = settings->value("config/batchParallelJobs", 1).toInt();

    QList<ProjectWidget*> plst = getProjectSelection(tr("Batch refinement"), false, true, 0);

    if (!plst.size()) return;

    for (int i = 0; i < plst.size(); ++i) {
        plst.at(i)->setStatus(global::RefinementStatus::SCHEDULED);
    }

    sysTrayIcon->setIcon(QIcon(":/icons/profex-active.png"));
    sysTrayIcon->setToolTip(QString(tr("Processing %1 projects").arg(plst.size())));

    sysTrayIcon->showMessage(tr("Profex"),
                             QString(tr("Running batch refinement: Processing %1 projects").arg(plst.size())),
                             QSystemTrayIcon::Information,
                             5000);

    if (ui->actionFollow_Active_Refinement->isChecked()) {
        twProjects->setCurrentItem(plst.at(0)->projectSelectItem());
        setCurrentProject(twProjects->currentItem(), nullptr);
    }

    for (int i = 0; i < nparallel; ++i) {
        if (i >= plst.size()) break;
        plst.at(i)->runRefinement();
    }

    dwOutput->raise();
}

/*
 * appends all selected projects to a running batch. Does not
 * start a new batch refinement, just changes the status to SCHEDULED
 */
void MainWindow::appendToRunningBatch()
{
    QList<ProjectWidget *> projects = getAllProjects();

    for (int i = 0; i < projects.count(); ++i) {
        // skip projects that are already scheduled for a batch or running
        if ((projects.at(i)->status() == global::RefinementStatus::RUNNING)
            || (projects.at(i)->status() == global::RefinementStatus::SCHEDULED)) {
            continue;
        }

        if (projects.at(i)->projectSelectItem()->isSelected()) {
            projects.at(i)->setStatus(global::RefinementStatus::SCHEDULED);
        }
    }
}

void MainWindow::runSearchMatch()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->runSearchMatch();
}

void MainWindow::runPeakDetection()
{
    ProjectWidget *pw = getCurrentProject();
    if (!pw) return;

    if (!pw->isRunning()) {
        sysTrayIcon->setIcon(QIcon(":/icons/profex-active.png"));
        sysTrayIcon->setToolTip(tr("Running..."));
        pw->runPeakDetection();
    }
}

void MainWindow::runBatchFitting()
{
    QList<ProjectWidget*> pendingProjects = getProjectSelection(tr("Run Curve Fitting"), false, true, 0);
    if (!pendingProjects.size()) return;

    for (int i = 0; i < pendingProjects.size(); ++i) {
        pendingProjects[i]->setStatus(global::RefinementStatus::FITSCHEDULED);
    }

    QThreadPool *globalPool = QThreadPool::globalInstance();
    int maxThreads = globalPool->maxThreadCount(); // Limit concurrency

    for (int i = 0; i < qMin(pendingProjects.size(), maxThreads); ++i) {
        connect(pendingProjects[i], &ProjectWidget::fitCompleted, this, &MainWindow::onFittingComplete);
        pendingProjects[i]->setStatus(global::RefinementStatus::FITRUNNING);
        pendingProjects[i]->startFit(globalPool);
    }
}

void MainWindow::onFittingComplete(ProjectWidget *)
{
    QList<ProjectWidget*> pendingProjects = getProjectsByStatus(global::RefinementStatus::FITSCHEDULED);

    // Start next project if pending
    if (!pendingProjects.isEmpty()) {
        QThreadPool *globalPool = QThreadPool::globalInstance();
        connect(pendingProjects[0], &ProjectWidget::fitCompleted, this, &MainWindow::onFittingComplete);
        pendingProjects[0]->setStatus(global::RefinementStatus::FITRUNNING);
        pendingProjects[0]->startFit(globalPool);
    }
}

void MainWindow::save()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->saveCurrent();
}

void MainWindow::saveAs()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->saveCurrentAs();
}

void MainWindow::saveAll()
{
    qApp->setOverrideCursor(Qt::WaitCursor);

    QList<ProjectWidget *> lst = getAllProjects();
    for (int i = 0; i < lst.size(); ++i) {
        lst.at(i)->saveAll();
    }

    qApp->restoreOverrideCursor();
}

void MainWindow::revertControlFile()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->revertControl();
}

/*
 * if a batch is running, it aborts / unschedules all projects that are part of the batch.
 * if no batch is running, it just aborts all running projects
 */
void MainWindow::abortAllRefinements()
{
    if (hasScheduledProjects() > 0) {
        if (QMessageBox::question(this,
                                  tr("Abort batch processing"),
                                  tr("Do you want to abort <b>all</b> running refinements?"))
            != QMessageBox::Yes) {
            return;
        }
    }

    QList<ProjectWidget *> lstRefScheduled = getProjectsByStatus(global::RefinementStatus::SCHEDULED);
    QList<ProjectWidget *> lstFitScheduled = getProjectsByStatus(global::RefinementStatus::FITSCHEDULED);
    QList<ProjectWidget *> lstRunning   = getProjectsByStatus(global::RefinementStatus::RUNNING);
    lstRunning.append(getProjectsByStatus(global::RefinementStatus::FITRUNNING));

    // first unschedule all scheduled projects
    for (int i = 0; i < lstRefScheduled.size(); ++i) {
        lstRefScheduled.at(i)->setStatus(global::RefinementStatus::IDLE);
    }

    for (int i = 0; i < lstFitScheduled.size(); ++i) {
        lstFitScheduled.at(i)->setStatus(global::RefinementStatus::IDLE);
    }

    // now abort the running ones
    for (int i = 0; i < lstRunning.size(); ++i) {
        lstRunning.at(i)->abort();
    }

    sysTrayIcon->setIcon(QIcon(":/icons/profex5.png"));
    sysTrayIcon->showMessage(tr("Profex"), tr("Refinement aborted"), QSystemTrayIcon::Information, 5000);
    sysTrayIcon->setToolTip(tr("Idle"));
}

void MainWindow::abortCurrentRefinement()
{
    ProjectWidget *pw = getCurrentProject();
    if (!pw) return;

    if (pw->isRunning()) {
        pw->abort();
    } else {
        if (pw->status() == global::RefinementStatus::SCHEDULED
            || pw->status() == global::RefinementStatus::FITSCHEDULED) {
            pw->setStatus(global::RefinementStatus::IDLE);
        }
    }
}

/*
 * this slot is called by all projects when they are done with the refinement. If we are in batch mode,
 * we call the next project, otherwise do nothing.
 */
void MainWindow::refinementCompleted(ProjectWidget *pw)
{
    // if another project is running, we just do nothing, as we don't want to start a second one in parallel
    if (hasRunningProjects()) return;

    QList<ProjectWidget *> pwlst = getProjectsByStatus(global::RefinementStatus::SCHEDULED);

    if (pwlst.isEmpty()) {
        qDebug() << QStringLiteral("MainWindow::refinementCompleted(): No scheduled projects to process.");

        sysTrayIcon->setIcon(QIcon(":/icons/profex5.png"));
        sysTrayIcon->setToolTip(tr("Refinement Complete"));
        sysTrayIcon->showMessage(tr("Profex"), tr("Refinement Complete"), QSystemTrayIcon::Information, 5000);

        return;
    }

    while (!pwlst.isEmpty()) {
        ProjectWidget *pwnext = pwlst.takeFirst();

        if (!pwnext->isRunning()) {
            qDebug() << QString("MainWindow::refinementCompleted(): Starting next scheduled project %1.").arg(pw->getGraphFile());
            sysTrayIcon->showMessage(tr("Profex"), QString(tr("%1 projects left to refine")).arg(pwlst.size()+1), QSystemTrayIcon::Information, 5000);
            sysTrayIcon->setToolTip(QString(tr("%1 projects left to refine")).arg(pwlst.size()));

            if (ui->actionFollow_Active_Refinement->isChecked()) {
                twProjects->setCurrentItem(pwnext->projectSelectItem());
                setCurrentProject(twProjects->currentItem(), nullptr);
            }

            pwnext->runRefinement();
            return;
        }
    }

    // no scheduled projects left
    sysTrayIcon->setIcon(QIcon(":/icons/profex5.png"));
    sysTrayIcon->setToolTip(tr("Refinement Complete"));
    sysTrayIcon->showMessage(tr("Profex"), tr("Refinement Complete"), QSystemTrayIcon::Information, 5000);
}

void MainWindow::coordinates(double x, double y, double d)
{
    statusAng->setText(QString(tr("2%1 = %2%3")).arg(global::theta).arg(x, 8, 'f', 3).arg(global::degree));
    statusInt->setText(QString(tr("I = %1 cts")).arg(y, 10, 'f', 3));
    statusD->setText(QString(tr("d = %1 %2")).arg(d, 6, 'f', 3).arg(global::angstrom));
}

void MainWindow::statusWavelength(double d)
{
    statusLambda->setText(QString(tr("%1 = %2 %3")).arg(global::lambda).arg(d, 0, 'f', 5).arg(global::angstrom));
}

void MainWindow::editorCursorChanged(int l, int c)
{
    statusCursor->setText(QString(tr("Line: %1, Column: %2")).arg(l).arg(c));
}

void MainWindow::addRemovePhase()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->addRemovePhase();
}

void MainWindow::exportResults()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Export local GOALs"), false, false);
    if (!pl.size()) return;

    QString eFile = QFileDialog::getSaveFileName(this, tr("Export results"), workingDir, tr("CSV File (*.CSV *.csv)"));
    if (eFile.isEmpty()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);

    QList<DataTable> data;

    for (int i = 0; i < pl.size(); ++i) {
        data.append(DataTable(pl.at(i)->getResultsCsv()));
    }

    // open the file for writing
    QFile f(eFile);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << QString("MainWindow::exportResults(): Could not write to file %1").arg(eFile);
        qApp->restoreOverrideCursor();
        return;
    }

    // write content to file
    QTextStream out(&f);
    out << buildGoalsCsvTable(data);
    f.close();

    qApp->restoreOverrideCursor();
}

void MainWindow::exportPeakList()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Export peak lists"), false, false, 0);
    if (!pl.size()) return;

    QString csvFilter("CSV File (*.csv *.CSV)");

    QString f = QFileDialog::getSaveFileName(this, tr("Save peak lists to text file"), workingDir, csvFilter);
    if (f.isEmpty()) return;

    QString out;

    for (int i = 0; i < pl.size(); ++i) {
        out.append(pl.at(i)->getPeakDataCsv(i == 0));
        out.append("\n");
    }

    BgmnFileIO::writeTextFile(f, out);
}

QString MainWindow::buildGoalsCsvTable(const QList<DataTable> &data)
{
    if (data.isEmpty()) return QString();

    QStringList globalHeader(data.first().header());

    // if other tables contain more columns, append the header cells to globalHeader
    for (int i = 1; i < data.size(); ++i) {
        QStringList dHeader = data.at(i).header();
        for (int j = 0; j < dHeader.size(); ++j) {
            if (!globalHeader.contains(dHeader.at(j))) globalHeader.append(dHeader.at(j));
        }
    }

    QString output(globalHeader.join(";"));
    output += "\n";

    // loop over all tables
    for (int t = 0; t < data.size(); ++t) {
        // loop over all lines in table t
        for (int r = 0; r < data.at(t).rows(); ++r) {
            // loop over all header cells c in globalHeaders, and query the
            // cell value for c in table t, row r
            QStringList globalLine;
            for (int c = 0; c < globalHeader.size(); ++c) {
                globalLine.append(data.at(t).value(globalHeader.at(c), r));
            }

            output += globalLine.join(";") + QStringLiteral("\n");
        }
    }

    return output;
}

void MainWindow::exportQuantities()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Export global GOALs"), false, false);
    if (!pl.size()) return;

    QString eFile = QFileDialog::getSaveFileName(this, tr("Global GOALs output file"), workingDir, tr("CSV File (*.CSV *.csv)"));
    if (eFile.isEmpty()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);

    QList<DataTable> data;

    for (int i = 0; i < pl.size(); ++i) {
        data.append(DataTable(pl.at(i)->getQuantitiesCsv()));
    }

    // open the file for writing
    QFile f(eFile);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        qDebug() << QString("MainWindow::exportQuantities(): Could not write to file %1").arg(eFile);
        qApp->restoreOverrideCursor();
        return;
    }

    // write content to file
    QTextStream out(&f);
    out << buildGoalsCsvTable(data);
    f.close();

    qApp->restoreOverrideCursor();
}

void MainWindow::exportChemistry()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Export chemical composition"), false, false);
    if (!pl.size()) return;

    QString eFile = QFileDialog::getSaveFileName(this, tr("Export to file"), workingDir, tr("CSV File (*.CSV *.csv)"));
    if (eFile.isEmpty()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);

    QStringList cont;
    for (int i = 0; i < pl.size(); ++i) {
        cont.append(pl.at(i)->getChemistry(i == 0 ? true : false));
    }

    if (!BgmnFileIO::writeTextFile(eFile, cont.join("\n"))) {
        qDebug() << QString("MainWindow::exportChemistry(): Could not write to file %1").arg(eFile);
    }

    qApp->restoreOverrideCursor();
}

void MainWindow::exportPeakIntegrals()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Export Peak Integrals"), false, true);
    if (!pl.size()) return;

    QString eFile = QFileDialog::getSaveFileName(this, tr("Peak integral table"), workingDir, tr("CSV File (*.CSV *.csv)"));
    if (eFile.isEmpty()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);

    QStringList cont;
    for (int i = 0; i < pl.size(); ++i) {
        cont.append(pl.at(i)->getPeakIntegralCsv());
    }

    if (!BgmnFileIO::writeTextFile(eFile, cont.join("\n"))) {
        qDebug() << QString("MainWindow::exportPeakIntegrals(): Could not write to file %1").arg(eFile);
    }

    qApp->restoreOverrideCursor();
}

void MainWindow::exportCurveFits()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Export Curve Fit Results"), false, true);
    if (!pl.size()) return;

    QString eFile = QFileDialog::getSaveFileName(this, tr("Curve fit table"), workingDir, tr("CSV File (*.CSV *.csv)"));
    if (eFile.isEmpty()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);

    QStringList cont;
    for (int i = 0; i < pl.size(); ++i) {
        cont.append(pl.at(i)->getCurveFitReport());
    }

    if (!BgmnFileIO::writeTextFile(eFile, cont.join("\n"))) {
        qDebug() << QString("MainWindow::exportCurveFits(): Could not write to file %1").arg(eFile);
    }

    qApp->restoreOverrideCursor();
}

void MainWindow::exportCifFiles()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Export CIF files"), false, false);
    if (pl.isEmpty()) return;

    CifExportDialog cdlg(this);

    if (cdlg.exec() == QDialog::Accepted) {
        QMap<QString, QVariant> auxData(cdlg.getAuxData());
        if (cdlg.getOutputMode() == SINGLE)  exportSingleCif(pl, auxData);
        if (cdlg.getOutputMode() == PROJECT) exportProjectCif(pl, auxData);
        if (cdlg.getOutputMode() == GLOBAL)  exportMergedCif(pl, auxData);
    }
}

void MainWindow::exportSingleCif(const QList<ProjectWidget *> &pl, const QMap<QString, QVariant> &auxData)
{
    qApp->setOverrideCursor(Qt::WaitCursor);

    for (int i = 0; i < pl.size(); ++i) {
        if (!pl.at(i)->isRunning()) {
            pl.at(i)->saveCifFiles(true, auxData);
        }
    }

    qApp->restoreOverrideCursor();
}

void MainWindow::exportProjectCif(const QList<ProjectWidget *> &pl, const QMap<QString, QVariant> &auxData)
{
    qApp->setOverrideCursor(Qt::WaitCursor);


    for (int i = 0; i < pl.size(); ++i) {
        QString fname(QDir::toNativeSeparators(pl.at(i)->workingDir() + "/" + pl.at(i)->baseName() + ".cif"));
        QList<phaseData> cifList;

        if (!pl.at(i)->isRunning()) {
            cifList.append(pl.at(i)->getCifData(auxData));
        }

        QString output;

        output += QStringLiteral("###############################################################################\n");
        output += QString(       "# Crystal structure data for project %1\n").arg(pl.at(i)->baseName());
        output += QString(       "# Created with Profex %1.%2.%3\n").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD);
        output += QString(       "# Export date: %1\n").arg(QDateTime::currentDateTime().toString("MMMM dd, yyyy - hh:mm"));
        output += QStringLiteral("###############################################################################\n\n");
        output += QStringLiteral("data_global\n\n");
        output += QStringLiteral("_publ_contact_author_name       ? # Name of author for correspondence\n");
        output += QStringLiteral("_publ_contact_author_address      # Address of author for correspondence\n");
        output += QStringLiteral(";?\n;\n\n");

        for (int j = 0; j < cifList.size(); ++j) {
            output.append(cifList.at(j).data.toString() + "\n\n");
        }

        output += QStringLiteral("\n# The following lines are used to test the character set of files sent by\n");
        output += QStringLiteral("# network email or other means. They are not part of the CIF data set.\n");
        output += QStringLiteral("# abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n");
        output += QString("# !@#$%^&*()_+{}:\"~<>?|\\-=[];'`,./ \n");

        if (!BgmnFileIO::writeTextFile(fname, output)) {
            qDebug() << QString("MainWindow::exportMergedCif(): Could not write to file %1").arg(fname);
            qApp->restoreOverrideCursor();
            return;
        }

        pl.at(i)->sendMessageToOutputConsole(QString("Exported CIF file:\n%1\n").arg(fname));
    }

    qApp->restoreOverrideCursor();
}

void MainWindow::exportMergedCif(const QList<ProjectWidget *> &pl, const QMap<QString, QVariant> &auxData)
{
    QString fname(QFileDialog::getSaveFileName(this, tr("Export CIF File"), workingDir, tr("CIF File (*.CIF *.cif)")));

    qApp->setOverrideCursor(Qt::WaitCursor);

    QList<phaseData> cifList;

    for (int i = 0; i < pl.size(); ++i) {
        if (!pl.at(i)->isRunning()) {
            cifList.append(pl.at(i)->getCifData(auxData));
        }
    }

    QString output;

    output += QStringLiteral("###############################################################################\n");
    output += QStringLiteral("# Crystal structure data\n");
    output += QString(       "# Created with Profex %1.%2.%3\n").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD);
    output += QString(       "# Export date: %1\n").arg(QDateTime::currentDateTime().toString("MMMM dd, yyyy - hh:mm"));
    output += QStringLiteral("###############################################################################\n\n");
    output += QStringLiteral("data_global\n\n");
    output += QStringLiteral("_publ_contact_author_name       ? # Name of author for correspondence\n");
    output += QStringLiteral("_publ_contact_author_address      # Address of author for correspondence\n");
    output += QStringLiteral(";?\n;\n\n");

    for (int i = 0; i < cifList.size(); ++i) {
        output.append(cifList.at(i).data.toString() + "\n\n");
    }

    output += QStringLiteral("\n# The following lines are used to test the character set of files sent by\n");
    output += QStringLiteral("# network email or other means. They are not part of the CIF data set.\n");
    output += QStringLiteral("# abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\n");
    output += QString("# !@#$%^&*()_+{}:\"~<>?|\\-=[];'`,./ \n");

    if (!BgmnFileIO::writeTextFile(fname, output)) {
        qDebug() << QString("MainWindow::exportMergedCif(): Could not write to file %1").arg(fname);
        qApp->restoreOverrideCursor();
        return;
    }

    qApp->restoreOverrideCursor();
}

void MainWindow::exportCastepCellFiles()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Export Castep CELL files"), false, false);

    for (int i = 0; i < pl.size(); ++i) {
        if (!pl.at(i)->isRunning()) {
            pl.at(i)->saveCastepCellFiles();
        }
    }
}

/*
 * exports information about all open projects to a CSV file
 */
void MainWindow::exportProjectInfo()
{
    QString file = QFileDialog::getSaveFileName(this, tr("Export project information"), workingDir, tr("CSV File (*.CSV *.csv)"));
    if (file.isEmpty()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);

    QMap<QString, QStringList> data;
    QString sep = ";";
    int n = twProjects->count();

    for (int i = 0; i < n; ++i) {
        QTreeWidgetItem *tit = twProjects->topLevelItem(i);
        data["Name"].append(tit->text(0));

        for (int j = 0; j < tit->childCount(); ++j) {
            QTreeWidgetItem *cit = tit->child(j);
            data[cit->text(0)].append(cit->text(1));
        }
    }

    QStringList keys = data.keys();
    QStringList out(keys.join(sep));

    for (int i = 0; i < n; ++i) {
        QStringList line;

        for (int j = 0; j < keys.size(); ++j) {
            line.append(i < data[keys.at(j)].size() ? data[keys.at(j)].at(i) : "");
        }

        out.append(line.join(sep));
    }


    if (!BgmnFileIO::writeTextFile(file, out.join("\n"))) {
        qDebug() << QString("MainWindow::exportProjectInfo(): Could not write to file %1").arg(file);
    }

    qApp->restoreOverrideCursor();
}

/*
 * scan batch conversion
 */
void MainWindow::fileScanBatchConversion()
{
    QStringList files;

    // if projects are loaded, show a project selection dialog and fill the
    // batch conversion dialog with all selected project scans
    if (countProjects(false, true) > 0) {
        QList<ProjectWidget *> pl = getProjectSelection(tr("Convert Scan Files"), false, true);
        if (!pl.size()) return;

        for (int i = 0; i < pl.size(); ++i) {
            files.append(pl.at(i)->getGraphFile());
        }
    } // else just show the empty batch dialog

    ExportGraphDialog *edlg = new ExportGraphDialog(this);
    edlg->setFiles(files);
    edlg->exec();
    delete edlg;
}

/*
 * imports a CIF or ICDD XML structure file
 */
void MainWindow::fileImportStructure()
{
    if (!stidlg) {
        stidlg = new StrucImportDialog(this);
        connect(stidlg, SIGNAL(emitRunIndexing()), this, SLOT(indexBgmnHkl()));
    }

    stidlg->show();
}

/*
 * copies the current control file to other projects. A dialog
 * will be shown to select the projects
 */
void MainWindow::applyControl()
{
    ProjectWidget *cpw = getCurrentProject();
    if (!cpw) return;

    // get the control file content of the current project
    QString cont = cpw->getControlFileContent();
    QFileInfoList sourceProjectFiles = cpw->getSharedInputProjectFilesList();

    // if nothing useful was returned, there is nothing to do.
    if (cont.isEmpty()) {
        QMessageBox::information(this, tr("No control file found"), tr("No control file found.\nCreate a control file first."));
        return;
    }

    QList<ProjectWidget *> pwl = getProjectSelection(tr("Apply Control File Content"), true, false);
    QProgressDialog pdlg(tr("Applying project files..."), tr("Abort"), 0, pwl.size(), this);
    pdlg.setWindowModality(Qt::WindowModal);

    // send the control file content to all selected projects
    qApp->setOverrideCursor(Qt::WaitCursor);

    bool forceOverwrite = false;
    for (int i = 0; i < pwl.size(); ++i) {
        pdlg.setValue(i);
        ProjectWidget *pw = pwl.at(i);

        // receive the control file content and adapt all file names
        pw->applyControlFileContent(cont);

        // gather shared files at the new destination
        forceOverwrite = pw->gatherSharedInputFiles(cpw->workingDir(), sourceProjectFiles, forceOverwrite);

        // if some files were present and the user decided not to overwrite them, bail out
        if (!forceOverwrite) break;
        if (pdlg.wasCanceled()) break;
    }

    pdlg.setValue(pwl.size());
    qApp->restoreOverrideCursor();
}

void MainWindow::applySequenceControl()
{
    ProjectWidget *cpw = getCurrentProject();
    if (!cpw) return;

    // get the control file content of the current project
    cpw->prepareForSequenceOrigin();
    QString cont = cpw->getControlFileContent();
    QString prevCont = cont;
    QFileInfoList sourceProjectFiles = cpw->getSharedInputProjectFilesList();

    // if nothing useful was returned, there is nothing to do.
    if (cont.isEmpty()) {
        QMessageBox::information(this, tr("No control file found"), tr("No control file found.\nCreate a control file first."));
        return;
    }

    QList<ProjectWidget *> pwl = getProjectSelection(tr("Create refinement sequence"), true, false);

    QProgressDialog pdlg(tr("Applying project files..."), tr("Abort"), 0, pwl.size(), this);
    pdlg.setWindowModality(Qt::WindowModal);

    // send the control file content to all selected projects
    qApp->setOverrideCursor(Qt::WaitCursor);

    bool forceOverwrite = false;
    QStringList errors;

    for (int i = 0; i < pwl.size(); ++i) {
        pdlg.setValue(i);
        ProjectWidget *pw = pwl.at(i);

        if (!pw->setSequenceInputFiles(cont, prevCont)) {
            errors.append(pw->baseName());
        } else {
            prevCont = pw->getControlFileContent();
        }

        // gather shared files at the new destination
        forceOverwrite = pw->gatherSharedInputFiles(cpw->workingDir(), sourceProjectFiles, forceOverwrite);

        // if some files were present and the user decided not to overwrite them, bail out
        if (!forceOverwrite) break;
        if (pdlg.wasCanceled()) break;
    }

    pdlg.setValue(pwl.size());
    qApp->restoreOverrideCursor();

    if (errors.size()) {
        QMessageBox::warning(this, tr("Errors occurred"), QString("The following projects created errors:\n\n%1\n\nFix the errors and regenerate the sequence.").arg(errors.join("\n")));
    }
}

void MainWindow::applyCurveFits()
{
    ProjectWidget *cpw = getCurrentProject();
    if (!cpw) return;

    QList<ProjectWidget *> pwl = getProjectSelection(tr("Apply Curve Fits"), true, false);
    if (!pwl.size()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);

    QDomElement el = cpw->getCurveFitData();

    for (int i = 0; i < pwl.size(); ++i) {
        ProjectWidget *pw = pwl.at(i);
        if (pw) pw->applyCurveFitData(el);
    }

    qApp->restoreOverrideCursor();
}

void MainWindow::print()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->print();
}

void MainWindow::printAllGraphs()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Print Graphs"), false, true);
    if (!pl.size()) return;

    QPrinter printer(QPrinterInfo::defaultPrinter(), QPrinter::HighResolution);

    if (!printer.isValid()) {
        qDebug() << QString("MainWindow::printAllGraphs(): Invalid printer object, exiting.");
        return;
    }

    printer.setPageOrientation(QPageLayout::Landscape);
    printer.setOutputFormat(QPrinter::NativeFormat);

    // this works on Linux et al, but not on OS X and Windows
#ifdef Q_OS_UNIX
    #ifndef Q_OS_OSX
        QFileInfo fi(pl.first()->getGraphFile());
        printer.setOutputFileName(QDir::fromNativeSeparators(fi.absolutePath() + "/" + fi.completeBaseName() + ".pdf"));
    #endif
#endif

    QPrintDialog printDialog(&printer, this);

    if (printDialog.exec() != QDialog::Accepted) {
        return;
    }

    QPainter painter;
    qApp->setOverrideCursor(Qt::WaitCursor);
    painter.begin(&printer);

    while (pl.size()) {
        pl.takeFirst()->printGraphDirectly(printer, painter);

        if (pl.size()) {
            printer.newPage();
            painter.setClipping(false);
            painter.resetTransform();
        }
    }

    painter.end();
    qApp->restoreOverrideCursor();
}

void MainWindow::renderAllSvg()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Save SVG Graph Files"), false, true);
    QStringList files;

    while (pl.size()) {
        files.append(pl.takeFirst()->renderSvgDirectly());
    }

    if (files.size()) {
        QMessageBox::information(this, tr("SVG Files Saved"), QString("SVG files saved:\n\n%1").arg(files.join("\n")));
    }
}

void MainWindow::helpAbout()
{
    HelpAboutDialog *hdlg = new HelpAboutDialog(this);
    hdlg->setVersion(version);
    hdlg->setLogDestination(logDest);
    hdlg->exec();
    delete hdlg;
}

/*
 * shows a help dialog with mouse actions on graphWindow
 */
void MainWindow::helpMouse()
{
    contextHelpText("graphWindow");
}

/*
 *  launches the BGMN website in a browser
 */
void MainWindow::helpBgmnVariables()
{
    QDesktopServices::openUrl(QUrl("http://www.bgmn.de/variables.html", QUrl::TolerantMode));
}

void MainWindow::spacegrpDat()
{
    // lazy initialization
    if (spdlg == nullptr) {
        spdlg = new SpacegroupDialog(this);
    }

    // only load the data from hd if it is necessary
    if (!spdlg->hasData()) {
        spdlg->setDirectory(settings->value("bgmnProject/bgmnExec", "").toString());
    }

    spdlg->show();
}

void MainWindow::afaparmDat()
{
    // lazy initialization
    if (asfdlg == Q_NULLPTR) {
        asfdlg = new AtomicScatteringFactorDialog(this);
    }

    asfdlg->show();
}

void MainWindow::editConfiguration()
{
    if (!instrdlg) {
        instrdlg = new EditInstrumentDialog(this);
    }

    instrdlg->resetDialog();
    instrdlg->show();
}

void MainWindow::editCurrentConfiguration()
{
    ProjectWidget *pw = getCurrentProject();
    if (!pw) return;

    QFileInfo fiProject(pw->getControlFileName());
    QFileInfo fiInstrument(pw->getInstrumentConfigFile(true));

    if (!fiInstrument.exists()) {
        QMessageBox::information(this, tr("Device file not found"),
                                 QString(tr("The device file was not found:\n\n%1"))
                                 .arg(fiInstrument.absoluteFilePath()));
        return;
    }

    if (!instrdlg) {
        instrdlg = new EditInstrumentDialog(this);
        connect(instrdlg, SIGNAL(helpText(QString)), chelp, SLOT(setText(QString)));
    }

    instrdlg->setWorkingDir(fiInstrument.absolutePath());
    instrdlg->openInstrumentSavFile(fiInstrument.absoluteFilePath(), fiProject.absoluteFilePath());
    instrdlg->show();
}

void MainWindow::showLegend(bool b)
{
    settings->setValue("graph/drawLegend", b);

    QList<ProjectWidget *> lst = getAllProjects();
    for (int i = 0; i < lst.size(); ++i) {
        lst.at(i)->setShowLegend(b);
    }
}

void MainWindow::contextHelp(QStringList list)
{
    if (list.size() > 1) {
        if (chelp) chelp->setKeyword(list.at(0), list.at(1));
    }
}

void MainWindow::contextHelpText(QString s)
{
    if (!chelp) return;

    if (!helpTextMgr) {
        helpTextMgr = new HelpTextManager;
    }

    chelp->setText(helpTextMgr->getHelpText(s));
    dwContextHelp->show();
}

/*
 *  Copy / Cut / Paste / Undo / Redo:
 *  no need to go through ProjectWidget. We send a keyPressEvent directly
 *  to the widget in focus. It will either preform its native action
 *  or ignore the event.
 *
 *  Important: This functionality breaks when the copy / cut etc. actions
 *  in the mainwindow form use shortcuts! Do not add shortcuts to these actions!
 */

void MainWindow::editCopy()
{
    QWidget *focusedWidget = QApplication::focusWidget();

    if (focusedWidget) {
        QKeyEvent *copyEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_C, Qt::ControlModifier);
        QApplication::sendEvent(focusedWidget, copyEvent);
    }
}

void MainWindow::editCut()
{
    QWidget *focusedWidget = QApplication::focusWidget();

    if (focusedWidget) {
        QKeyEvent *cutEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_X, Qt::ControlModifier);
        QApplication::sendEvent(focusedWidget, cutEvent);
    }
}

void MainWindow::editPaste()
{
    QWidget *focusedWidget = QApplication::focusWidget();

    if (focusedWidget) {
        QKeyEvent *pasteEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_V, Qt::ControlModifier);
        QApplication::sendEvent(focusedWidget, pasteEvent);
    }
}

void MainWindow::undo()
{
    QWidget *focusedWidget = QApplication::focusWidget();

    if (focusedWidget) {
        QKeyEvent *undoEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Z, Qt::ControlModifier);
        QApplication::sendEvent(focusedWidget, undoEvent);
    }
}

void MainWindow::redo()
{
    QWidget *focusedWidget = QApplication::focusWidget();

    if (focusedWidget) {
        QKeyEvent *redoEvent = new QKeyEvent(QEvent::KeyPress, Qt::Key_Z, Qt::ShiftModifier & Qt::ControlModifier);
        QApplication::sendEvent(focusedWidget, redoEvent);
    }
}

bool MainWindow::hasCurrentProject()
{
    if (!twProjects->currentItem()) return false;
    return true;
}

ProjectWidget * MainWindow::getProjectFromTopLevelIndex(int n)
{
    return getProjectFromUid(twProjects->getProjectUid(n));
}

ProjectWidget * MainWindow::getProjectFromUid(const QUuid &u)
{
    if (u.isNull()) return nullptr;

    for (int i = 1; i < widgetStack->count(); ++i) {
        if (widgetStack->widget(i)->inherits("ProjectWidget")) {
            ProjectWidget *pw = static_cast<ProjectWidget*>(widgetStack->widget(i));
            if (pw->uId() == u) return pw;
        }
    }

    return nullptr;
}

ProjectWidget * MainWindow::getCurrentProject()
{
    QTreeWidgetItem *it = twProjects->currentItem();
    if (!it) return nullptr;

    while (it->parent()) it = it->parent();

    if (it) return getProjectFromTopLevelIndex(twProjects->indexOfTopLevelItem(it));
    return nullptr;
}

/*
 * returns a list of all ProjectWidgets
 */
QList<ProjectWidget *> MainWindow::getAllProjects()
{
    QList<ProjectWidget *> lst;

    for (int i = 1; i < widgetStack->count(); ++i) {
        ProjectWidget *pw = static_cast<ProjectWidget*>(widgetStack->widget(i));
        if (pw) lst.append(pw);
    }

    return lst;
}

/*
 * calls the project widget's ::setZoomRange() function
 */
void MainWindow::setZoomRange()
{
    ProjectWidget *pw = getCurrentProject();
    if (!pw) return;

    QVector<double> r = pw->getZoomRange();
    ZoomRangeDialog *zdlg = new ZoomRangeDialog(this);

    zdlg->setIdealValues(r[4], r[5], r[6], r[7]);
    zdlg->setValues(r[0], r[1], r[2], r[3]);

    if (zdlg->exec() == QDialog::Accepted) {
        zdlg->getValues(r[0], r[1], r[2], r[3]);
    } else {
        return;
    }

    QList<ProjectWidget*> pwlist = getProjectSelection(tr("Apply Zoom Ranges"), false, false);

    for (int i = 0; i < pwlist.size(); ++i) {
        if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::setZoomRange(): Setting zoom range for project %1: %2 %3 %4 %5").arg(pw->getControlFileName()).arg(r[0]).arg(r[1]).arg(r[2]).arg(r[3]);
        pwlist.at(i)->setZoomRange(r[0], r[1], r[2], r[3]);
    }

    delete zdlg;
}

void MainWindow::resetZoomRange()
{
    QList<ProjectWidget*> pwlist = getProjectSelection(tr("Reset Zoom Ranges"), false, false);

    for (int i = 0; i < pwlist.size(); ++i) {
        pwlist.at(i)->resetZoomRange();
    }

}

/*
 * opens all STR files referenced in a BGMN control file
 */
void MainWindow::fileOpenProjectStrFiles()
{
    ProjectWidget *pw = getCurrentProject();

    if (pw) {
        QStringList lst = pw->openProjectStrFiles();
        closeProjectStrFiles(lst, pw);
    }
}

/*
 * closes all STR files referenced in a BGMN control file
 */
void MainWindow::fileCloseProjectStrFiles()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->closeProjectStrFiles();
}

/*
 * close the structure files in lst in all projects except in pw
 * if pw == nullptr, close them in all projects
 */
void MainWindow::closeProjectStrFiles(const QStringList &lst, ProjectWidget *pw)
{
    QList<ProjectWidget *> pwLst = getAllProjects();

    for (int p = 0; p < pwLst.size(); ++p) {
        if (pwLst.at(p) == pw) continue;
        pwLst.at(p)->closeProjectStrFiles(lst);
    }
}

/*
 * returns all projects with status global::RefinementStatus::STATUS
 */
QList<ProjectWidget *> MainWindow::getProjectsByStatus(global::RefinementStatus status)
{
    QList<ProjectWidget *> pwlist;

    for (int i = 0; i < twProjects->topLevelItemCount(); ++i) {
        ProjectWidget *pw = getProjectFromTopLevelIndex(i);
        if (!pw) continue;

        if (pw->status() == status) {
            pwlist.append(pw);
        }
    }

    return pwlist;
}

/*
 * shows a dialog of all projects with checkboxes. Returns a list with checked projects
 *
 * - skipCurrent == false: all projects will be shown, including the current one
 * - skipCurrent == true: the current project will not be shown in the list
 * - ignoreType == false: only projects of the same type as the current one (BGMN, FP) will be shown
 * - ignoreType == true:  all project types will be shown (BGMN, FP)
 *
 * - precheck == 0: default behaviour, precheck selected projects, or all if none are selected
 * - precheck == 1: precheck the current project
 */
QList<ProjectWidget *> MainWindow::getProjectSelection(const QString &title, bool skipCurrent, bool ignoreType, int precheck)
{
    QList<ProjectWidget *> selPw;

    ProjectWidget *cpw = getCurrentProject();
    if (!cpw) return selPw;

    QList<QTreeWidgetItem*> selectedItems = twProjects->selectedItems();
    bool checkAll = selectedItems.size() > 1 ? false : true;

    QList<ProjectWidget*> allProjects;
    QList<QUuid> selectedUids;
    QUuid currentUid;

    // compile a map of all projects to show in the selection dialog
    for (int i = 0; i < twProjects->count(); ++i) {
        ProjectSelectTreeItem * psIt = dynamic_cast<ProjectSelectTreeItem*>(twProjects->topLevelItem(i));
        if (!psIt) continue;

        ProjectWidget *pw = psIt->projectWidget();
        if (!pw) continue;

        if (pw == cpw) {
            if (skipCurrent) {
                // we don't want to show the current project
                continue;
            } else {
                currentUid = pw->uId();
            }
        }

        if (pw->type() != cpw->type()) {
            // it's not the same type of project, so skip it if requested
            if (!ignoreType) continue;
        }

        allProjects.append(pw);

        if (precheck == 0) {
            if (pw->projectSelectItem()->isSelected() || checkAll) {
                selectedUids.append(pw->uId());
            }
        }

        if (precheck == 1) {
            if (pw == cpw) selectedUids.append(pw->uId());
        }
    }

    // create the selection dialog and show it
    ProjectSelectDialog *cpdlg = new ProjectSelectDialog(this);
    cpdlg->setWindowTitle(title);
    cpdlg->setFiles(allProjects, selectedUids);
    cpdlg->setCurrentUid(currentUid);

    // send the control file content to all selected projects
    if (cpdlg->exec() == QDialog::Accepted) {
        // the returned stringlist holds the selected projects' UIDs
        QList<QUuid> f = cpdlg->getChecked();

        for (int i = 0; i < f.count(); ++i) {
            ProjectWidget *pw = getProjectFromUid(f.at(i));
            if (pw) selPw.append(pw);
        }
    }

    delete cpdlg;
    return selPw;
}

/*
 * counts the number of projects
 *
 * - skipCurrent == false: all projects will be counted, including the current one
 * - skipCurrent == true: the current project will not be counted
 * - ignoreType == false: only projects of the same type as the current one (BGMN, FP) will be counted
 * - ignoreType == true:  all project types will be counted (BGMN, FP)
 */
int MainWindow::countProjects(bool skipCurrent, bool ignoreType)
{
    int n = 0;

    // get a pointer to the current widget, so we can skip it if necessary and
    // determine it's type
    ProjectWidget *cpw = getCurrentProject();
    if (!cpw) return n;

    QUuid currentUid;

    // loop over all projects
    QList<ProjectWidget *> lst = getAllProjects();

    for (int i = 0; i < lst.count(); ++i) {
        if (lst.at(i) == cpw) {
            // we don't want to count the current project
            if (skipCurrent) continue;
            else currentUid = lst.at(i)->uId();
        }

        if (lst.at(i)->type() != cpw->type()) {
            // it's not the same type of project, so skip it if requested
            if (!ignoreType) continue;
        }

        ++n;
    }

    return n;
}

/*
 * returns the number of currently running projects
 */
int MainWindow::hasRunningProjects()
{
    int n = 0;

    QList<ProjectWidget *> lst = getAllProjects();

    for (int i = 0; i < lst.count(); ++i) {
        if (lst.at(i)->status() == global::RefinementStatus::RUNNING) ++n;
    }

    return n;
}


/*
 * returns the number of currently scheduled projects
 */
int MainWindow::hasScheduledProjects()
{
    return getProjectsByStatus(global::RefinementStatus::SCHEDULED).size();
}

/*
 * Populates the menus:
 *  File -> Recent Graph Files
 *  File -> Recent Text Files
 */
void MainWindow::setupRecentFiles()
{
    // deleting all actions manually. QMenu::clear() will remove but not delete them,
    // because they are not owned by the menu. This is imporant and done
    // deliberately, because we must be able to re-arrange the actions (move
    // a clicked action to the top) without the actions being automatically
    // deleted by the menu when removed from the menu.
    QList<QAction *> lstG = menuRecentGraphs->actions();
    QList<QAction *> lstT = menuRecentText->actions();

    menuRecentGraphs->clear();
    menuRecentText->clear();

    while (!lstG.isEmpty()) {
        QAction *act = lstG.takeFirst();
        if (act) delete act;
    }

    while (!lstT.isEmpty()) {
        QAction *act = lstT.takeFirst();
        if (act) delete act;
    }

    // loop from the end of the stringList 10 positions down towards the beginning
    int szG = recentFilesGraph.size() - 1;
    for (int i = szG; i >= qMax(szG - 10, 0); --i) {
        QAction *act = new QAction(recentFilesGraph.at(i), this);
        act->setData(recentFilesGraph.at(i));
        menuRecentGraphs->addAction(act);
    }

    // loop from the end of the stringList 10 positions down towards the beginning
    int szT = recentFilesText.size() - 1;
    for (int i = szT; i >= qMax(szT - 10, 0); --i) {
        QAction *act = new QAction(recentFilesText.at(i), this);
        act->setData(recentFilesText.at(i));
        menuRecentText->addAction(act);
    }
}

/*
 * clicking a recent graph file menu entry.
 * the clicked action must be moved to the top of the menu
 */
void MainWindow::openRecentGraph(QAction *a)
{
    QList<QAction *> lst = menuRecentGraphs->actions();

    // find the action a in the list of actions
    int idx = lst.indexOf(a);

    // if it was found, move it to the beginning of the list
    // and re-populate the menu
    if ((idx > 0) && (idx < lst.size())) {
        lst.insert(0, lst.takeAt(idx));

        // this will not delete the actions, because they are not owned by the menu
        menuRecentGraphs->clear();

        // actions are still in the list, so we can add them back to the menu in the
        // new order
        menuRecentGraphs->addActions(lst);
    }

    loadGraphFiles(QStringList(a->data().toString()), QString());
    qDebug() << QString("MainWindow::openRecentGraph(): Opening recent file %1").arg(a->data().toString());
}

/*
 * clicking a recent text file menu entry.
 * the clicked action must be moved to the top of the menu
 */
void MainWindow::openRecentText(QAction *a)
{
    QList<QAction *> lst = menuRecentText->actions();

    // find the action a in the list of actions
    int idx = lst.indexOf(a);

    // if it was found, move it to the beginning of the list
    // and re-populate the menu
    if ((idx > 0) && (idx < lst.size())) {
        lst.insert(0, lst.takeAt(idx));

        // this will not delete the actions, because they are not owned by the menu
        menuRecentText->clear();

        // actions are still in the list, so we can add them back to the menu in the
        // new order
        menuRecentText->addActions(lst);
    }

    loadTextFiles(QStringList(a->data().toString()));
    qDebug() << QString("MainWindow::openRecentText(): Opening recent file %1").arg(a->data().toString());
}

void MainWindow::scanYOffsetUp()
{
    bool firstOnTop = settings->value("graph/stackedScansFirstOnTop", true).toBool();
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->scanYOffsetUp(firstOnTop);
}

void MainWindow::scanYOffsetDown()
{
    bool firstOnTop = settings->value("graph/stackedScansFirstOnTop", true).toBool();
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->scanYOffsetDown(firstOnTop);
}

void MainWindow::scanXOffsetLeft()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->scanXOffsetLeft();
}

void MainWindow::scanXOffsetRight()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->scanXOffsetRight();
}

void MainWindow::scanOffsetReset()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->scanOffsetReset();
}

void MainWindow::removeScan()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->removeCurrentScan();
}

void MainWindow::createTemplate()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->createPreset();
}

void MainWindow::projectTabChanged(ProjectWidget *pw, int i)
{
    if (pw == getCurrentProject()) {
        statusFileName->setText(QDir::toNativeSeparators(pw->currentFileName()));
    }

    if (i) {
        if (hasScheduledProjects()) {
            ui->actionFollow_Active_Refinement->setChecked(false);
        }
    } else {
        statusCursor->setText(QString());
    }

    toggleTabDependentActions(pw);
}

/*
 * Saves a zip archive of the current project under a name created automatically
 */
void MainWindow::createZipArchive()
{
    ProjectWidget *pw = getCurrentProject();
    if (!pw) return;

    QDateTime dt = QDateTime::currentDateTime();
    QString zf = pw->workingDir() + "/" + pw->baseName() + "-" + dt.toString("yyyyMMdd-hhmmss");

    // add a number if the file name already exists
    if (QFile::exists(zf + ".zip")) {
        for (int i = 1; i < 999; ++i) {
            if (!QFile::exists(QString("%1-%2.zip").arg(zf).arg(i))) {
                zf = QString("%1-%2.zip").arg(zf).arg(i);
                break;
            }
        }
    } else {
        zf = zf + ".zip";
    }

    pw->createZipArchive(QDir::fromNativeSeparators(zf));
}

/*
 * Saves a zip archive of the current project under a user-given name
 */
void MainWindow::createZipArchiveAs()
{
    ProjectWidget *pw = getCurrentProject();
    if (!pw) return;

    QString zf = QFileDialog::getSaveFileName(this, tr("Save Compressed Archive"), pw->workingDir(), tr("Compressed Archive (*.zip *.ZIP)"));

    if (!zf.isEmpty()) {
        pw->createZipArchive(zf);
    }
}

void MainWindow::setInternalStandard()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->setInternalStandard();
}

void MainWindow::unsetInternalStandard()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->unsetInternalStandard();
}

void MainWindow::generateReport()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->generateReport();
}

void MainWindow::learnProfile()
{
    LearnProfileDialog *lpdlg = new LearnProfileDialog(this);
    lpdlg->exec();
    delete lpdlg;
}

void MainWindow::instrumentPeakProfile()
{
    if (!bgmninsprofdlg) {
        bgmninsprofdlg = new BgmnInstrumentProfileDialog(this);
    }

    bgmninsprofdlg->show();
}

void MainWindow::applyTextBlock(QAction *action)
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->applyTextBlock(action->data().toString());
}

void MainWindow::searchReplace()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->searchReplace();
}

void MainWindow::resetMarginColor()
{
    ProjectWidget *pw = getCurrentProject();
    if (pw) pw->resetMarginColor();
}

void MainWindow::updateNumberOfOpenProject()
{
    int n = twProjects->count();
    statusOpenProjects->setText(QString("%1 Project%2").arg(n).arg(n == 1 ? "" : "s"));
}

void MainWindow::diaStripImpurities()
{
    if (!purifyScansDlg) {
        purifyScansDlg = new PurifyScans(this);
    }

    ProjectWidget *pw = getCurrentProject();

    if (pw) {
        if (pw->type() == "BGMN") {
            purifyScansDlg->loadFile(pw->getGraphFile());
        }
    }

    purifyScansDlg->show();
}

void MainWindow::diaSimulateCompositions()
{
    if (!simScansDlg) {
        simScansDlg = new SimulateScans(this);
    }

    simScansDlg->show();
}

void MainWindow::diaFormatResults()
{
    if (!formatResultsDlg) {
        formatResultsDlg = new FormatResults(this);
    }

    formatResultsDlg->show();
}

void MainWindow::refStrFavoritesToggled(bool b)
{
    QList<ProjectWidget *> lst = getAllProjects();

    for (int i = 0; i < lst.count(); ++i) {
        lst.at(i)->toggleRefStrFavorites(b);
    }
}

void MainWindow::indexReferenceStructures()
{
    indexBgmnHkl();
}

void MainWindow::indexBgmnHkl()
{
    QFileInfo fiBgmn(settings->value("bgmnProject/bgmnExec", QString()).toString());

    if (!fiBgmn.exists()) {
        qDebug() << QString("MainWindow::indexBgmnHkl(): BGMN executable not found. Exiting.");
        return;
    }

    if (!indexProgressDialog) {
        indexProgressDialog = new IndexingProgressDialog(this);
        indexProgressDialog->setWindowTitle(tr("Updating reference structure database"));
        connect(refStrManager, SIGNAL(signalTotalFiles(int)), indexProgressDialog, SLOT(setMaximum(int)));
        connect(refStrManager, SIGNAL(signalCurrentFile(int)), indexProgressDialog, SLOT(setValue(int)));
        connect(refStrManager, SIGNAL(signalCurrentText(QString)), indexProgressDialog, SLOT(setLabelText(QString)));
        connect(indexProgressDialog, SIGNAL(canceled()), refStrManager, SLOT(cancel()));
        connect(indexProgressDialog, SIGNAL(skipped()), refStrManager, SLOT(skipCurrent()));
    }

    indexProgressDialog->show();
    refStrManager->update();
}

void MainWindow::indexBgmnHklComplete(const QStringList &errFiles)
{
    indexProgressDialog->hide();
    indexProgressDialog->reset();

    // send list of indexed file names to all open BGMN projects
    QList<ProjectWidget *> lst = getAllProjects();

    for (int i = 0; i < lst.count(); ++i) {
        if (lst.at(i)->type() == "BGMN") lst.at(i)->setReferenceStructureFileList();
    }

    if (errFiles.size() > 1) {
        BgmnIndexingErrorDialog *idxErrDlg = new BgmnIndexingErrorDialog(this);
        idxErrDlg->setErrorString(errFiles);
        idxErrDlg->exec();
        delete idxErrDlg;
    }
}

void MainWindow::indexFpHkl()
{
    /* todo */
}

/*
 * shows the absorption coefficient dialog
 */
void MainWindow::absorptionCoefficients()
{
    if (!abscoeffdlg) {
        abscoeffdlg = new AbsorptionCoefficientDialog(this);
    }

    ProjectWidget *pw = getCurrentProject();
    if (pw) abscoeffdlg->setProjectData(pw->getResultsFile(), pw->wavelength());

    abscoeffdlg->show();
}

/*
 * shows the bond length dialog
 */
void MainWindow::bondLengths()
{
    if (!bndLengthDlg) {
        bndLengthDlg = new BondLengthDialog(this);
    }

    bndLengthDlg->show();
    updateBondLengthDlg();
}

void MainWindow::updateBondLengthDlg()
{
    if (!bndLengthDlg) return;

    if (!bndLengthDlg->isVisible()) return;

    ProjectWidget *pw = getCurrentProject();
    if (pw) bndLengthDlg->setProject(pw);
    else    bndLengthDlg->clearProject();
}

void MainWindow::applyPeakIntegralsToAll()
{
    ProjectWidget *cpw = getCurrentProject();
    if (!cpw) return;

    QList<ProjectWidget *> pl = getProjectSelection(tr("Apply Peak Integral Ranges"), true, true);
    if (!pl.size()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);

    QDomElement el =cpw->getPeakIntegralRanges();

    for (int i = 0; i < pl.size(); ++i) {
        pl.at(i)->applyPeakIntegralRanges(el);
    }

    qApp->restoreOverrideCursor();
}

void MainWindow::applyPeakListFiltersToAll()
{
    ProjectWidget *cpw = getCurrentProject();
    if (!cpw) return;

    QList<ProjectWidget *> pl = getProjectSelection(tr("Apply Peak List Filters"), true, true);
    if (!pl.size()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);

    QDomElement el =cpw->getPeakListFilter();

    for (int i = 0; i < pl.size(); ++i) {
        pl.at(i)->setPeakListFilter(el);
    }

    qApp->restoreOverrideCursor();
}

void MainWindow::toggleProjectMenusEnabled(bool b, const QString &type)
{
    ui->menuProject->setEnabled(b);
    ui->menu_Results->setEnabled(b);
    ui->menu_View->setEnabled(b);
    menuTextBlocks->setEnabled(b);

    ui->projectToolBar->setEnabled(b);
    ui->refinementToolBar->setEnabled(b);
    referenceToolBar->setEnabled(b);

    // menu File
    ui->actionSave->setEnabled(b);
    ui->actionSaveAs->setEnabled(b);
    ui->actionCloseProject->setEnabled(b);
    ui->actionRemove_Scan->setEnabled(b);
    ui->actionPrint->setEnabled(b);
    ui->actionPrint_All_Graphs->setEnabled(b);
    ui->actionExport_all_Graphs_to_SVG->setEnabled(b);
    ui->actionSave_all_files->setEnabled(b);
    ui->actionClose_All_Projects->setEnabled(b);

    // menu Edit
    ui->actionUndo->setEnabled(b);
    ui->actionRedo->setEnabled(b);
    ui->actionCut->setEnabled(b);
    ui->actionCopy->setEnabled(b);
    ui->actionPaste->setEnabled(b);
    ui->actionApply_Control->setEnabled(b);
    ui->actionRevertControlFile->setEnabled(b && (type == "FP"));
    ui->actionSearch_and_Replace->setEnabled(b);
    ui->actionFind_in_Files->setEnabled(b);
    ui->actionOpen_Structure_File->setEnabled(b);
    ui->actionCreate_Refinement_Sequence->setEnabled(b);

    // menu Instrument
    ui->actionEdit_Current_FPA_Configuration->setEnabled(b);

    // menu Tools
    ui->actionScan_Math->setEnabled(b);
    ui->actionAdd_Base_Line->setEnabled(b);
    ui->actionSmooth_Scan->setEnabled(b);
    ui->actionConvert_Divergence_Slit->setEnabled(b);
    // ui->menu_Process_DIA_Files->setEnabled(b);

    // menu Locations
    ui->actionProjectOpenFileManager->setEnabled(b);

    // menu Run
    ui->actionAbortAllRefinements->setEnabled(b);
    ui->actionAbortCurrentRefinement->setEnabled(b);
    ui->actionFollow_Active_Refinement->setEnabled(b);
    ui->actionRunRefinement->setEnabled(b);
    ui->actionRun_Batch_Refinement->setEnabled(b);
    ui->actionRun_Peak_Detection->setEnabled(b);
    ui->actionRun_Search_Match->setEnabled(b);
}

void MainWindow::toggleTabDependentActions(ProjectWidget *pw)
{
    if (!pw) return;
    bool isTxtTab = pw->currentIndex() > 0;

    if (menuTextBlocks)  menuTextBlocks->setEnabled(isTxtTab);
    if (textBlockButton) textBlockButton->setEnabled(isTxtTab);

    pw->getToolbarWidget()->setEnabled(!isTxtTab);
    ui->actionRevertControlFile->setEnabled(isTxtTab && (pw->type() == "FP"));
    ui->actionUndo->setEnabled(isTxtTab);
    ui->actionRedo->setEnabled(isTxtTab);
    ui->actionSet_Zoom_Range->setEnabled(!isTxtTab);
    ui->actionLegend->setEnabled(!isTxtTab);
    ui->actionHkl_Indices->setEnabled(!isTxtTab);
    ui->actionInsert_Graph_File->setEnabled(!isTxtTab);
    ui->actionIncrease_Vertical_Displacement->setEnabled(!isTxtTab);
    ui->actionDecrease_Vertical_Displacement->setEnabled(!isTxtTab);
    ui->actionReset_Vertical_Displacement->setEnabled(!isTxtTab);
    ui->actionRemove_Scan->setEnabled(!isTxtTab);
    ui->actionDisplace_Left->setEnabled(!isTxtTab);
    ui->actionDisplace_Right->setEnabled(!isTxtTab);
    ui->actionTextBlock->setEnabled(isTxtTab);
    ui->actionSearch_and_Replace->setEnabled(isTxtTab);
    ui->actionReset_Margin_Color->setEnabled(!isTxtTab);
    ui->actionScan_Math->setEnabled(!isTxtTab);
    ui->actionCut->setEnabled(isTxtTab);
    ui->actionPaste->setEnabled(isTxtTab);
    ui->actionAdd_Base_Line->setEnabled(!isTxtTab);
    ui->actionReset_Zoom_Range->setEnabled(!isTxtTab);
    ui->actionSmooth_Scan->setEnabled(!isTxtTab);
    ui->actionRun_Peak_Detection->setEnabled(!isTxtTab);
    ui->actionExport_Curve_Fits->setEnabled(!isTxtTab);
}

void MainWindow::fileOpenProjectArchive()
{
    QFileInfo archive(QFileDialog::getOpenFileName(this, tr("Import Project Archive"), workingDir, tr("ZIP Archives (*.zip *.ZIP)")));
    qDebug() << QString("MainWindow::fileOpenProjectArchive(): Opening archive %1").arg(archive.absoluteFilePath());

    if (!archive.exists()) {
        qDebug() << QString("MainWindow::fileOpenProjectArchive(): File doesn't exist. Exiting.");
        return;
    }

    QString stdtmp = settings->getTempLocation();
    QDir tempdir(stdtmp + "/" + archive.completeBaseName());
    QDir finaldir(archive.absolutePath() + "/" + archive.completeBaseName());

    // check if the dirs already exists. If yes, add a increment counter to the dir path
    for (int i = 1; i < 10000; ++i) {
        if (tempdir.exists()) tempdir.setPath(stdtmp + "/" + archive.completeBaseName() + QString("-%1").arg(i));
        else break;
    }

    for (int i = 1; i < 10000; ++i) {
        if (finaldir.exists()) finaldir.setPath(archive.absolutePath() + "/" + archive.completeBaseName() + QString("-%1").arg(i));
        else break;
    }

    if (!finaldir.exists()) finaldir.mkpath(".");
    if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::fileOpenProjectArchive(): Extracting files to %1").arg(tempdir.path());

    // extract the archive to the temporary directory
    QStringList cfiles = JlCompress::getFileList(archive.absoluteFilePath());
    QStringList efiles = JlCompress::extractFiles(archive.absoluteFilePath(), cfiles, tempdir.path());
    QStringList ffiles;
    QStringList graphExtensions(importHandler->extensions());

    QStringList convExtensions;
    convExtensions << "sav" << "str" << "xy" << "val" << "ger" << "lam" << "dia" << "lst" << "par";

    // copy to final location, using functions converting line ends between Windows and Unix
    for (int i = 0; i < efiles.size(); ++i) {
        QFileInfo fi(efiles.at(i));
        QFileInfo fo(finaldir.absolutePath() + "/" + fi.fileName());

        if (convExtensions.contains(fi.suffix().toLower())) {
            if (settings->verboseLevel() > 1) {
                qDebug() << QString("MainWindow::fileOpenProjectArchive(): Copying file %1 to %2, converting line endings if needed.")
                            .arg(fi.absoluteFilePath(), fo.absoluteFilePath());
            }
            BgmnFileIO::copyFile(fi.absoluteFilePath(), fo.absoluteFilePath());
        } else {
            if (settings->verboseLevel() > 1) {
                qDebug() << QString("MainWindow::fileOpenProjectArchive(): Copying file %1 to %2, not converting line endings.")
                            .arg(fi.absoluteFilePath(), fo.absoluteFilePath());
            }
            QFile::copy(fi.absoluteFilePath(), fo.absoluteFilePath());
        }

        ffiles.append(fo.absoluteFilePath());
    }

    if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::fileOpenProjectArchive(): Removing temporary directory %1").arg(tempdir.path());
    tempdir.removeRecursively();

    QList<ProjectWidget *> pwList;
    QStringList uniqueGraphFiles;
    QStringList uniqueTextFiles;
    QStringList generalGraphFiles;
    QStringList generalTextFiles;
    QString projectFile;

    // we try to find a file with an extension that is clearly assigned to a certain project type (dia, sav, pcr, prf, ...).
    // this guarantees that the correct project type is created. If no such file is found, we open one recognized as a graph file
    // and create the default project type. If no graph file is found, we open any of the remaining files as text in a default project.
    for (int i = 0; i < ffiles.size(); ++i) {
        QFileInfo ffile(ffiles.at(i));
        QString fileSuffix = ffile.suffix().toLower();
        QString filePath   = ffile.absoluteFilePath();

        if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::fileOpenProjectArchive(): Probing file %1").arg(filePath);

        if (fileSuffix == "pfp") {
            projectFile = filePath;
            continue;
        }

        if (fExtProjectType.contains(fileSuffix)) {
            if (graphExtensions.contains(fileSuffix)) {
                uniqueGraphFiles.append(filePath);
            } else {
                uniqueTextFiles.append(filePath);
            }
        } else {
            if (graphExtensions.contains(fileSuffix)) {
                generalGraphFiles.append(filePath);
            } else {
                generalTextFiles.append(filePath);
            }
        }
    }

    // priorities for creating a project:
    // 1. graph file with extension assigned to a project type
    // 2. text file with extension assigned to a project type
    // 3. graph file opened as default project
    // 4. text file opened as default project
    if (uniqueGraphFiles.size()) {
        if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::fileOpenProjectArchive(): Loading as graph file %1").arg(uniqueGraphFiles.constFirst());
        pwList = loadGraphFiles(QStringList(uniqueGraphFiles.constFirst()), QString());
    } else if (uniqueTextFiles.size()) {
        if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::fileOpenProjectArchive(): Loading as text file %1").arg(uniqueTextFiles.constFirst());
        pwList = loadTextFiles(QStringList(uniqueTextFiles.constFirst()));
    } else if (generalGraphFiles.size()) {
        if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::fileOpenProjectArchive(): Loading as graph file %1").arg(generalGraphFiles.constFirst());
        pwList = loadGraphFiles(QStringList(generalGraphFiles.constFirst()), QString());
    } else if (generalTextFiles.size()) {
        if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::fileOpenProjectArchive(): Loading as text file %1").arg(generalTextFiles.constFirst());
        pwList = loadTextFiles(QStringList(generalTextFiles.constFirst()));
    }

    if (pwList.size() && !projectFile.isEmpty()) {
        pwList[0]->applyXmlSettings(projectFile);
    }
}

void MainWindow::projectOpenFileManager()
{
    ProjectWidget *pw = getCurrentProject();

    if (pw) {
        QUrl url = QUrl::fromLocalFile(pw->workingDir());

        if (QDesktopServices::openUrl(url)) {
            if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::projectOpenFileManager(): Opening %1").arg(url.toDisplayString());
        } else {
            qDebug() << QString("MainWindow::projectOpenFileManager(): Opening %1 failed").arg(url.toDisplayString());
        }
    }
}

void MainWindow::clearToolsDialogs()
{
    if (smdlg)      smdlg->clearProject();
    if (blDialog)   blDialog->clearProject();
    if (sSmoothDlg) sSmoothDlg->clearProject();
    if (dsConvDlg)  dsConvDlg->clearProject();
    if (bndLengthDlg) bndLengthDlg->clearProject();
}

void MainWindow::updatePresetMenu()
{
    if (presetMenuMgr) {
        presetMenuMgr->setPresetRepos(settings->value("bgmnProject/presetDirectory", QStringList()).toStringList());
        presetMenuMgr->update();
    }
}

void MainWindow::updateToolsDialogs()
{
    updateScanMathDlg();
    updateBaseLineDlg();
    updateSmoothScanDlg();
    updateCalcDspacingDlg();
    updateBondLengthDlg();
}

void MainWindow::scanMath()
{
    if (!hasCurrentProject()) return;

    if (!smdlg) {
        smdlg = new ScanMathDialog(this);
    }

    smdlg->show();
    updateScanMathDlg();
}

void MainWindow::updateScanMathDlg()
{
    if (!smdlg) return;
    if (!smdlg->isVisible()) return;

    ProjectWidget *pw = getCurrentProject();
    if (pw) smdlg->setProject(pw);
    else    smdlg->clearProject();
}

void MainWindow::addBaseLine()
{
    if (!hasCurrentProject()) return;

    if (!blDialog) {
        blDialog = new BaseLineDialog(this);
    }

    blDialog->show();
    updateBaseLineDlg();
}

void MainWindow::updateBaseLineDlg()
{
    if (!blDialog) return;
    blDialog->clearTemporary(); // in the old project

    if (!blDialog->isVisible()) return;

    ProjectWidget *pw = getCurrentProject();
    if (pw) blDialog->setProject(pw);
    else    blDialog->clearProject();
}

void MainWindow::smoothScan()
{
    if (!hasCurrentProject()) return;

    if (!sSmoothDlg) {
        sSmoothDlg = new ScanSmoothDialog(this);
    }

    sSmoothDlg->show();
    updateSmoothScanDlg();
}

void MainWindow::updateSmoothScanDlg()
{
    if (!sSmoothDlg) return;
    sSmoothDlg->clearTemporary(); // in the old project
    if (!sSmoothDlg->isVisible()) return;

    ProjectWidget *pw = getCurrentProject();
    if (pw) sSmoothDlg->setProject(pw);
    else    sSmoothDlg->clearProject();
}

void MainWindow::divSlitConvertScan()
{
    if (!hasCurrentProject()) return;

    if (!dsConvDlg) {
        dsConvDlg = new DivSlitConvertDialog(this);
    }

    dsConvDlg->show();
    updateDivSlitConvDlg();
}

void MainWindow::updateDivSlitConvDlg()
{
    if (!dsConvDlg) return;
    dsConvDlg->clearTemporary(); // in the old project
    if (!dsConvDlg->isVisible()) return;

    ProjectWidget *pw = getCurrentProject();
    if (pw) dsConvDlg->setProject(pw);
    else    dsConvDlg->clearProject();
}

void MainWindow::applyPreset(QString s)
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Apply Preset"), false, false, 1);
    if (!pl.size()) return;

    QStringList errors;
    bool overwrite = false;

    QMessageBox msgBox;
    msgBox.setWindowTitle(tr("Overwrite project"));
    msgBox.setText(tr("The following project already has a control file."));
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::YesToAll | QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel);
    msgBox.setDefaultButton(QMessageBox::No);

    QProgressDialog *prgDlg = new QProgressDialog(this);
    prgDlg->setMaximum(pl.size());
    prgDlg->setWindowTitle(tr("Applying Presets"));
    prgDlg->show();
    int prg = 0;

    for (int i = 0; i < pl.size(); ++i) {
        prgDlg->setLabelText(pl.at(i)->baseName());
        prgDlg->setValue(prg);
        if (prgDlg->wasCanceled()) break;

        int n = pl.at(i)->applyPreset(s, overwrite);

        QFileInfo gfi(pl.at(i)->getGraphFile());

        if (n < 0) { // a serious error occurred
            qDebug() << QString("MainWindow::applyPreset(): An error occurred when applying preset to project %1").arg(gfi.fileName());
            errors << gfi.fileName();
        }

        if (n == 0) { // project already has control file
            msgBox.setInformativeText(QString("Do you want to overwrite it?\n\n%1").arg(gfi.completeBaseName()));
            qApp->restoreOverrideCursor();
            prgDlg->hide();
            int ovr = msgBox.exec();
            qApp->setOverrideCursor(Qt::WaitCursor);
            prgDlg->show();

            if (ovr == QMessageBox::YesToAll) {
                if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::applyPreset(): Overwrite project %1: User selection = yes to all").arg(gfi.fileName());
                overwrite = true;
                pl.at(i)->applyPreset(s, overwrite);
            }

            if (ovr == QMessageBox::Yes) {
                if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::applyPreset(): Overwrite project %1: User selection = yes").arg(gfi.fileName());
                pl.at(i)->applyPreset(s, true);
            }

            if (ovr == QMessageBox::No) {
                if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::applyPreset(): Overwrite project %1: User selection = no").arg(gfi.fileName());
            }

            if (ovr == QMessageBox::Cancel) {
                if (settings->verboseLevel() > 1) qDebug() << QString("MainWindow::applyPreset(): Overwrite project %1: User selection = cancel").arg(gfi.fileName());
                break;
            }
        }

        ++prg;
    }

    prgDlg->hide();
    delete prgDlg;

    qApp->restoreOverrideCursor();

    if (errors.size()) {
        QMessageBox::information(this, tr("Applying presets"),
                                 QString(tr("Applying the preset to the following projects failed.\n"
                                            "Please fix manually.\n\n%1")).arg(errors.join("\n")));
    }
}

void MainWindow::saveBatchScript()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Save Batch Refinement Script"), false, false);
    if (!pl.size()) return;

#ifdef Q_OS_WIN
    QString env("WIN");
#else
    QString env("UNIX");
#endif

    ProjectWidget *pw = getCurrentProject();
    QFileInfo exec(pw->type() == "BGMN" ? QDir::toNativeSeparators(settings->value("bgmnProject/bgmnExec", "").toString())
                                        : QDir::toNativeSeparators(settings->value("fpProject/fpExec", "").toString()));

    QString fileOut;
    QString strOut;

    if (env == "UNIX") {
        fileOut = QFileDialog::getSaveFileName(this, tr("Save Batch File"), workingDir, tr("Bash script (*.sh *.SH);;All files (*.*"));
        if (fileOut.isEmpty()) return;

        strOut += QString("#! /bin/bash\n\n");
        strOut += QString("export EFLECH=%1\nexport PATH=$PATH:%1\n").arg(QDir::fromNativeSeparators(exec.absolutePath()));
        strOut += QString("EXEC=%1\n\n").arg(QDir::fromNativeSeparators(exec.absoluteFilePath()));

        for (int i = 0; i < pl.size(); ++i) {
            pl.at(i)->saveAll();
            pl.at(i)->convertRawDataFile(QStringList());
            QFileInfo fiCtrFile(pl.at(i)->getControlFileName());
            strOut += QString("echo refining project %1 of %2\n").arg(i+1).arg(pl.size());
            strOut += QString("cd %1\n").arg(QDir::fromNativeSeparators(fiCtrFile.absolutePath()));
            strOut += QString("$EXEC %1\n\n").arg(fiCtrFile.fileName());
        }

        strOut += QString("echo All done.\n");
    } else if (env == "WIN") {
        fileOut = QFileDialog::getSaveFileName(this, tr("Save Batch File"), workingDir, tr("CMD script (*.cmd *.CMD);;All files (*.*"));
        if (fileOut.isEmpty()) return;

        strOut += QString("@ECHO OFF\n\n");
        strOut += QString("setlocal\n");
        strOut += QString("SET PATH=%PATH%;\"%1\"\n").arg(QDir::fromNativeSeparators(exec.absolutePath()));
        strOut += QString("SET EFLECH=%1\n").arg(QDir::fromNativeSeparators(exec.absolutePath()));
        strOut += QString("SET EXEC=\"%1\"\n\n").arg(QDir::fromNativeSeparators(exec.absoluteFilePath()));

        for (int i = 0; i < pl.size(); ++i) {
            pl.at(i)->saveAll();
            pl.at(i)->convertRawDataFile(QStringList());
            QFileInfo fiCtrFile(pl.at(i)->getControlFileName());
            strOut += QString("ECHO refining project %1 of %2\n").arg(i+1).arg(pl.size());
            strOut += QString("cd /d \"%1\"\n").arg(QDir::toNativeSeparators(fiCtrFile.absolutePath()));
            strOut += QString("%EXEC% \"%1\"\n\n").arg(fiCtrFile.fileName());
        }

        strOut += QString("ECHO All done.\n");
        strOut += QString("PAUSE\n");
    }

    BgmnFileIO::writeTextFile(fileOut, strOut);
}

void MainWindow::editProjectStrFiles()
{
    ProjectWidget *pw = getCurrentProject();
    if (!pw) return;
    pw->editProjectStrFiles();
}

void MainWindow::amorphousPeak()
{
    ProjectWidget *pw = getCurrentProject();
    if (!pw) return;
    if (pw->type() == "BGMN") pw->addAmorphousPeak();
}

void MainWindow::createSinglePeakRefinement()
{
    ProjectWidget *pw = getCurrentProject();
    if (!pw) return;
    pw->createSinglePeakRefinement();
}

void MainWindow::editFindInFiles()
{
    if (!searchInFilesDlg) {
        searchInFilesDlg = new SearchInFilesDialog(this);
        connect(searchInFilesDlg, SIGNAL(runSearch()), this, SLOT(runFindInFiles()));
        connect(searchInFilesDlg, SIGNAL(projectFileSelected(QUuid,QString,int)), this, SLOT(raiseProjectFile(QUuid,QString,int)));
    }

    searchInFilesDlg->show();
}

void MainWindow::runFindInFiles()
{
    if (!searchInFilesDlg) return;
    QList<ProjectWidget*> pwList;

    if (searchInFilesDlg->getProjectSelection() == "cur") {
        pwList.append(getCurrentProject());
    } else {
        pwList.append(getAllProjects());
    }

    QString expr = searchInFilesDlg->getSearchExpression();

    for (int i = 0; i < pwList.size(); ++i) {
        QMap<QString, QMap<int, QStringList> > results = pwList.at(i)->searchInOpenFiles(searchInFilesDlg->getFileTypeSelection(), expr);
        searchInFilesDlg->appendResults(pwList.at(i)->baseName(), pwList.at(i)->uId(), results);
    }
}

void MainWindow::raiseProjectFile(QUuid uid, QString fn, int ln)
{
    ProjectWidget *pw = getProjectFromUid(uid);
    if (!pw) return;

    twProjects->setCurrentItem(pw->projectSelectItem());
    pw->raiseTab(fn, ln);
}

void MainWindow::exportToExcel()
{
    QList<ProjectWidget *> pl = getProjectSelection(tr("Export Results to Excel"));
    if (!pl.size()) return;

    for (int i = 0; i < pl.size(); ++i) {
        pl.at(i)->exportToExcel();
    }
}

void MainWindow::editExcelExport()
{
    ProjectWidget *pw = getCurrentProject();
    if (!pw) return;

    pw->editExcelExport();
}

void MainWindow::elementScatteringData()
{
    if (!elScatDataDlg) {
        elScatDataDlg = new ElementDataDialog(this);
    }

    elScatDataDlg->show();
}

void MainWindow::overrideProjectWavelength()
{
    double d = 0.154056;

    WavelengthSelectDialog *wlsDlg = new WavelengthSelectDialog(this);

    if (wlsDlg->exec() != QDialog::Accepted) {
        delete wlsDlg;
        return;
    }

    d = wlsDlg->wavelength();
    Scan::WavelengthMode wlm = wlsDlg->wavelengthMode();
    delete wlsDlg;

    QList<ProjectWidget *> pl = getProjectSelection(tr("Set Wavelength"), false, true, 1);
    if (!pl.size()) return;

    for (int i = 0; i < pl.size(); ++i) {
        pl.at(i)->overrideProjectWavelength(10.0 * d, wlm);
    }

    ProjectWidget *pw = getCurrentProject();
    if (pw) statusWavelength(pw->wavelength());
}

void MainWindow::toolsBrowseReferenceStructures()
{
    if (!refstrDlg) {
        refstrDlg = new BrowseReferenceStructuresDialog(this);
        connect(refstrDlg, SIGNAL(favoritesChanged()), this, SLOT(updateReferenceStructureList()));
    }

    refstrDlg->show();
}

void MainWindow::updateReferenceStructureList()
{
    QList<ProjectWidget *> pl = getAllProjects();
    for (int i = 0; i < pl.size(); ++i) {
        pl[i]->setReferenceStructureFileList();
    }
}

void MainWindow::electronDensityMap()
{
    if (!profexEdProcess) profexEdProcess = new QProcess();

    QString exe = getModuleExe("profexed");
    QStringList args;
    ProjectWidget *pw = getCurrentProject();
    if (pw) args << pw->workingDir();

    qDebug() << QString("MainWindow::electronDensityMap(): Starting %1 %2").arg(exe, args.join(" "));
    if (QFile::exists(exe)) profexEdProcess->startDetached(exe, args);
}

void MainWindow::traceScanFigures()
{
    if (!profexStProcess) profexStProcess = new QProcess();

    QString exe = getModuleExe("profexst");
    QStringList args;
    ProjectWidget *pw = getCurrentProject();
    if (pw) args << pw->workingDir();

    qDebug() << QString("MainWindow::traceScanFigures(): Starting %1 %2").arg(exe, args.join(" "));
    if (QFile::exists(exe)) profexStProcess->startDetached(exe, args);
}

void MainWindow::waterfallPlot()
{
    if (!profexWpProcess) profexWpProcess = new QProcess();

    QString exe = getModuleExe("profexwp");
    QStringList args;
    ProjectWidget *pw = getCurrentProject();
    if (pw) args << pw->workingDir();

    qDebug() << QString("MainWindow::waterfallPlot(): Starting %1 %2").arg(exe, args.join(" "));
    if (QFile::exists(exe)) profexWpProcess->startDetached(exe, args);
}

void MainWindow::synchrotronConfiguration()
{
    if (!profexScProcess) profexScProcess = new QProcess();

    QString exe = getModuleExe("profexsc");
    QStringList args;
    ProjectWidget *pw = getCurrentProject();
    if (pw) args << pw->workingDir();

    qDebug() << QString("MainWindow::synchrotronConfiguration(): Starting %1 %2").arg(exe, args.join(" "));
    if (QFile::exists(exe)) profexScProcess->startDetached(exe, args);
}

QString MainWindow::getModuleExe(const QString &e)
{
    QString d = qApp->applicationDirPath() + QDir::separator();
    if (settings->verboseLevel() > 2) qDebug() << QString("MainWindow::getModuleExe(): Application dir path = %1").arg(d);

#ifdef Q_OS_WIN
    return d + e + ".exe";
#else
#ifdef Q_OS_MACOS
    if (e == "profexed") {
        if (QFile::exists(d + "../../../ProfexEd.app")) {
            return d + "../../../ProfexEd.app/Contents/MacOS/" + e;
        } else if (QFile::exists(d + "../../../profexed.app")) {
            return d + "../../../profexed.app/Contents/MacOS/" + e;
        }
    } else if (e == "profexst") {
        if (QFile::exists(d + "../../../ProfexSt.app")) {
            return d + "../../../ProfexSt.app/Contents/MacOS/" + e;
        } else if (QFile::exists(d + "../../../profexst.app")) {
            return d + "../../../profexst.app/Contents/MacOS/" + e;
        }
    } else if (e == "profexwp") {
        if (QFile::exists(d + "../../../ProfexWp.app")) {
            return d + "../../../ProfexWp.app/Contents/MacOS/" + e;
        } else if (QFile::exists(d + "../../../profexwp.app")) {
            return d + "../../../profexwp.app/Contents/MacOS/" + e;
        }
    } else if (e == "profexsc") {
        if (QFile::exists(d + "../../../ProfexSc.app")) {
            return d + "../../../ProfexSc.app/Contents/MacOS/" + e;
        } else if (QFile::exists(d + "../../../profexsc.app")) {
            return d + "../../../profexsc.app/Contents/MacOS/" + e;
        }
    }
#else
    return d + e;
#endif
#endif

    return QString();
}

void MainWindow::openCurrentStructureFile()
{
    ProjectWidget *pw = getCurrentProject();

    if (pw) {
        QStringList files = pw->openStructureFileUnderCursor();
        closeProjectStrFiles(files, pw);
    }
}

void MainWindow::simulateTubeTails()
{
    if (!ttSimDlg) {
        ttSimDlg = new TubeTailSimulatorDialog(this);
    }

    ttSimDlg->show();
}

void MainWindow::createOqProject()
{
    if (!oqProDlg) {
        oqProDlg = new OqProjectDialog(this);
        connect(oqProDlg, SIGNAL(sigOpenProject(QStringList,QString)), this, SLOT(loadGraphFiles(QStringList,QString)));
    }

    oqProDlg->show();
}

void MainWindow::checkForUpdates()
{
    UpdateManager umgr(qApp->applicationDirPath(), this);

    if (umgr.hasUpdates()) {
        bool upd = QMessageBox::question(this,
                                         tr("Check for updates"),
                                         tr("A new version is available. Do you want to run the updater?"),
                                         QMessageBox::Yes,
                                         QMessageBox::No) == QMessageBox::Yes;
        if (upd) {
            umgr.runUpdate();
            close();
        }
    } else {
        QMessageBox::information(this, tr("Check for updates"), tr("No updates available."));
    }
}

void MainWindow::calculateDspacing()
{
    if (!dSpaceDlg) {
        dSpaceDlg = new CalcDspacingDialog(this);
    }

    dSpaceDlg->show();
    updateCalcDspacingDlg();
}

void MainWindow::updateCalcDspacingDlg()
{
    if (!dSpaceDlg) return;

    if (!dSpaceDlg->isVisible()) return;

    ProjectWidget *pw = getCurrentProject();
    if (pw) dSpaceDlg->setProject(pw);
    else    dSpaceDlg->clearProject();
}

/* EOF */




