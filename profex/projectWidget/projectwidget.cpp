/***************************************************************************
                          projectwidget.cpp  -  description
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

#include "projectwidget.h"
#include "../libXrdIO/export/exporthandler.h"
#include "../libXrdIO/functions.h"
#include "searchreplacealldialog.h"
#include "math.h"

#include <QPrinter>
#include <QPrintDialog>
#include <QUuid>
#include <QHeaderView>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QToolButton>
#include <QThread>
#include <QProgressDialog>
#include <QDoubleSpinBox>
#include <QDebug>
#include <QTime>

ProjectWidget::ProjectWidget(QWidget *parent) :
        QTabWidget(parent)
{
    id = QUuid::createUuid();
    refStatus = global::RefinementStatus::IDLE;
    settings = SettingsManager::getInstance();

    graphModel = new GraphDataModel();
    graphControl = new GraphDataController(graphModel, this);
    graphView = new GraphWindow(graphControl, this);
    graphControl->addView(graphView);

    // signal mapper, used to identify the QTextEdit emitting a textChanged() signal
    textChangedSignalMapper = new QSignalMapper(this);
    connect(textChangedSignalMapper, SIGNAL(mappedObject(QObject*)), this, SLOT(textChanged(QObject*)));

    // create the treeWidget to toggle visibility of scans
    scanList = new ScanListWidget(graphControl, this);
    scanList->setAccessibleName(tr("Scan list"));
    graphControl->addView(scanList);

    refOutput = new QPlainTextEdit(this);
    refOutput->setAccessibleName(tr("Refinement protocol"));
    refOutput->setLineWrapMode(QPlainTextEdit::NoWrap);
    refOutput->setReadOnly(true);

    chemOutput = new ChemTableWidget(graphControl, this);
    chemOutput->setAccessibleName(tr("Chemical composition"));
    graphControl->addView(chemOutput);

    convDisplay = new ConvergenceDisplay(this);
    convDisplay->setAccessibleName(tr("Convergence graph"));

    peakIntegrationWidget = new PeakIntegrationWidget(graphControl, graphView, this);
    peakIntegrationWidget->setAccessibleName(tr("Peak integrals"));
    graphControl->addView(peakIntegrationWidget);

    peakListWidget = new PeakListWidget(graphControl, this);
    peakListWidget->setAccessibleName(tr("Peak list"));
    graphControl->addView(peakListWidget);

    searchMatchWidget = new SearchMatchWidget(graphControl, this);
    searchMatchWidget->setAccessibleName(tr("Search-match phases"));
    searchMatchWidget->setWidgets(refOutput);
    graphControl->addView(searchMatchWidget);

    peakFitWidget = new PeakFitWidget(graphControl, graphView, this);
    peakFitWidget->setAccessibleName(tr("Peak fitting"));
    graphControl->addView(peakFitWidget);

    // must be placed in the toolbar by the parent
    toolbarWidget = new QWidget(this);

    comboBoxRefStrucRepos = new QComboBox(this);
    comboBoxRefStrucRepos->setAccessibleName(tr("Structure repository"));
    comboBoxRefStrucRepos->setToolTip(tr("Structure repository"));
    comboBoxRefStrucRepos->setEditable(true);
    comboBoxRefStrucRepos->setInsertPolicy(QComboBox::NoInsert);
    comboBoxRefStrucRepos->lineEdit()->setPlaceholderText(tr("<Repository>"));
    QFontMetrics fmRepStr(comboBoxRefStrucRepos->font());
    comboBoxRefStrucRepos->setMinimumWidth(fmRepStr.horizontalAdvance("<Repository>"));
    // QCompleter *repoCompleter = comboBoxRefStrucRepos->completer();
    // repoCompleter->setCompletionMode(QCompleter::PopupCompletion); // not sure if popup or inline (default) completion is better

    // combo box holding the reference structures
    comboBoxRefStructures = new QComboBox(this);
    comboBoxRefStructures->setAccessibleName(tr("Reference structure"));
    comboBoxRefStructures->setToolTip(tr("Reference Structure"));
    comboBoxRefStructures->setEditable(true);
    comboBoxRefStructures->setInsertPolicy(QComboBox::NoInsert);
    comboBoxRefStructures->lineEdit()->setPlaceholderText(tr("<Reference Structures>"));
    QFontMetrics fmRefStr(comboBoxRefStructures->font());
    comboBoxRefStructures->setMinimumWidth(fmRefStr.horizontalAdvance("<Reference Structures>"));
    // QCompleter *strCompleter = comboBoxRefStructures->completer();
    // strCompleter->setCompletionMode(QCompleter::PopupCompletion); // not sure if popup or inline (default) completion is better

    // create the button to reset the reference structure
    refStrResetButton = new QToolButton(this);
    refStrResetButton->setAccessibleName(tr("Reset reference structure"));
    refStrResetButton->setText(tr("Reset"));
    refStrResetButton->setAutoRaise(true);
    refStrResetButton->setIcon(QIcon::fromTheme("profex-delete"));
    refStrResetButton->setToolTip(tr("Reset Reference Structure"));

    // create the button to index new reference structures
    refStrFavoritesButton = new QToolButton(this);
    refStrFavoritesButton->setAccessibleName(tr("Favorites"));
    refStrFavoritesButton->setText(tr("Favorites"));
    refStrFavoritesButton->setAutoRaise(true);
    refStrFavoritesButton->setIcon(QIcon::fromTheme("profex-favorites"));
    refStrFavoritesButton->setToolTip(tr("Show favorites only"));
    refStrFavoritesButton->setCheckable(true);

    // create the button to append the current reference hkl lines to the graph
    refStrAppendHklButton = new QToolButton(this);
    refStrAppendHklButton->setAccessibleName(tr("Append hkl lines"));
    refStrAppendHklButton->setText(tr("Append HKL"));
    refStrAppendHklButton->setAutoRaise(true);
    refStrAppendHklButton->setIcon(QIcon::fromTheme("profex-add-hkl"));
    refStrAppendHklButton->setToolTip(tr("Permanently append hkl lines to the plot"));
    refStrAppendHklButton->setEnabled(false);

    QToolButton *refStrHklBaseButton = new QToolButton(this);
    refStrHklBaseButton->setAccessibleName(tr("Base for reference lines"));
    refStrHklBaseButton->setText(tr("Reference line base"));
    refStrHklBaseButton->setToolTip(tr("Set the base of hkl reference lines"));
    refStrHklBaseButton->setPopupMode(QToolButton::InstantPopup);
    refStrHklBaseButton->setIcon(QIcon::fromTheme("profex-hkl-base"));
    menuHklBase = new QMenu(this);
    actHklBaseZero = menuHklBase->addAction("Zero line");
    actHklBaseBkgr = menuHklBase->addAction("Background curve");
    actHklBaseDiff = menuHklBase->addAction("Difference curve");
    actHklBaseZero->setCheckable(true);
    actHklBaseBkgr->setCheckable(true);
    actHklBaseDiff->setCheckable(true);
    refStrHklBaseButton->setMenu(menuHklBase);

    refHdispValue = new QDoubleSpinBox(this);
    refHdispValue->setAccessibleName(tr("Sample height displacement"));
    refHdispValue->setMaximum(5.0);
    refHdispValue->setMinimum(-5.0);
    refHdispValue->setSingleStep(0.0001);
    refHdispValue->setValue(0.0);
    refHdispValue->setDecimals(6);
    refHdispValue->setToolTip(QString(tr("Refined value of EPS2.\n"
                                         "Sample height displacement %1 is calculated as:\n\n"
                                         "%1 = EPS2 * R\n\n"
                                         "where R is the goniometer radius.\n"
                                         "The unit of %1 is the same in which R is specified.")).arg(global::Delta));

    refHdispZeroButton = new QToolButton(this);
    refHdispZeroButton->setAccessibleName(tr("Reset height displacement"));
    refHdispZeroButton->setText(tr("Reset"));
    refHdispZeroButton->setAutoRaise(true);
    refHdispZeroButton->setIcon(QIcon::fromTheme("profex-sample-height-zero"));
    refHdispZeroButton->setToolTip(tr("Set angular corrections to zero"));

    refHdispResetButton = new QToolButton(this);
    refHdispResetButton->setAccessibleName(tr("Refined height displacement"));
    refHdispResetButton->setText(tr("Refined"));
    refHdispResetButton->setAutoRaise(true);
    refHdispResetButton->setIcon(QIcon::fromTheme("profex-sample-height-refined"));
    refHdispResetButton->setToolTip(tr("Set angular corrections to refined values"));

    // this frame is used as a vertical separator line in the toolbar widget. It is implemented
    // the same way QtDesigner implements vertical lines.
    QFrame *vsep = new QFrame(toolbarWidget);
    vsep->setFrameStyle(QFrame::VLine | QFrame::Sunken);

    QHBoxLayout *tbLayout = new QHBoxLayout(toolbarWidget);
    tbLayout->setContentsMargins(0, 0, 0, 0);
    tbLayout->addWidget(refStrFavoritesButton);
    tbLayout->addWidget(comboBoxRefStrucRepos);
    tbLayout->addWidget(comboBoxRefStructures);
    tbLayout->addWidget(refStrResetButton);
    tbLayout->addWidget(refStrAppendHklButton);
    tbLayout->addWidget(refStrHklBaseButton);
    tbLayout->addWidget(vsep);
    tbLayout->addWidget(refHdispValue);
    tbLayout->addWidget(refHdispResetButton);
    tbLayout->addWidget(refHdispZeroButton);
    tbLayout->addStretch();

    addTab(graphView, tr("Graph"));
    setTabsClosable(true);

    // remove the close button from the first tab (graph)
    QWidget *closeButton = tabBar()->tabButton(0, QTabBar::RightSide);
    if (closeButton) tabBar()->tabButton(0, QTabBar::RightSide)->deleteLater();
    tabBar()->setTabButton(0, QTabBar::RightSide, nullptr);

    controlFile = "";
    refinedGraphFile = "";
    projectBasename = "";
    projectDir = "";
    loadedGraphFile = "";
    sampleID = "";

    autoShowOutput = true;
    graphLiveUpdate = true;

    lastEps1 = 0.0;
    lastEps2 = 0.0;
    lastEps3 = 0.0;

    // detect number of cpu cores
    nCpus = 4;
    int nc = QThread::idealThreadCount();
    if (nc > 0) {
        nCpus = nc;
    }

    pSelectItem = new ProjectSelectTreeItem(id, this);

    connect(this, SIGNAL(currentChanged(int)), this, SLOT(tabChanged(int)));
    connect(this, SIGNAL(tabCloseRequested(int)), this, SLOT(closeTab(int)));

    connect(graphView, SIGNAL(sigCoordinates(double,double,double)), this, SIGNAL(setCoordinates(double,double,double)));
    connect(graphView, SIGNAL(sigDoubleClickA(double,double,double)), this, SLOT(referencePosClicked(double,double,double)));
    connect(graphView, SIGNAL(sigDoubleClickB(double,double,double)), this, SLOT(dumpCoordinates(double,double,double)));
    connect(graphView, SIGNAL(sigRangeDoubleClicked(int,int)), this, SLOT(rangeDoubleClicked(int,int)));

    connect(comboBoxRefStrucRepos, SIGNAL(currentIndexChanged(int)), this, SLOT(setReferenceStructureFileList()));
    connect(comboBoxRefStructures, SIGNAL(currentIndexChanged(int)), this, SLOT(referenceStructureChanged(int)));
    connect(refStrResetButton, SIGNAL(clicked()), this, SLOT(resetRefStructure()));
    connect(refStrFavoritesButton, SIGNAL(toggled(bool)), this, SLOT(refStrFavoritesButtonToggled(bool)));
    connect(refStrAppendHklButton, SIGNAL(clicked()), this, SLOT(refStrAppendHklToGraph()));
    connect(menuHklBase, SIGNAL(triggered(QAction*)), this, SLOT(setHklBaseMode(QAction*)));

    connect(refHdispValue, SIGNAL(valueChanged(double)), this, SLOT(sampleHeightDisplacementChanged(double)));
    connect(refHdispResetButton, SIGNAL(clicked()), this, SLOT(resetSampleHeightDisplacement()));
    connect(refHdispZeroButton, SIGNAL(clicked()), this, SLOT(zeroSampleHeightDisplacement()));

    connect(peakIntegrationWidget, SIGNAL(sigApplyToAll()), this, SIGNAL(sigAllPeakIntegrals()));

    connect(peakListWidget, SIGNAL(resultsSelectionChanged(QString)), this, SLOT(selectReferenceHklFile(QString)));
    connect(peakListWidget, SIGNAL(applyFiltersToAll()), this, SIGNAL(applyPeakFiltersToAll()));

    connect(peakFitWidget, SIGNAL(clearProtocol()), refOutput, SLOT(clear()));
    connect(peakFitWidget, SIGNAL(protocol(QString)), refOutput, SLOT(appendPlainText(QString)));
    connect(peakFitWidget, SIGNAL(applyToAll()), this, SIGNAL(sigAllCurveFits()));
    connect(peakFitWidget, SIGNAL(statusChanged(global::RefinementStatus)), this, SLOT(setStatus(global::RefinementStatus)));
    connect(peakFitWidget, &PeakFitWidget::fitCompleted, this, &ProjectWidget::onFitCompleted);

    connect(peakIntegrationWidget, SIGNAL(helpText(QString)), this, SIGNAL(moduleHelpText(QString)));
    connect(searchMatchWidget,     SIGNAL(helpText(QString)), this, SIGNAL(moduleHelpText(QString)));
    connect(peakFitWidget,         SIGNAL(helpText(QString)), this, SIGNAL(moduleHelpText(QString)));
    connect(scanList,              SIGNAL(helpText(QString)), this, SIGNAL(moduleHelpText(QString)));
    connect(peakListWidget,        SIGNAL(helpText(QString)), this, SIGNAL(moduleHelpText(QString)));

    QKeySequence seqResetTextZoom = QKeySequence("Ctrl+0");
#ifdef Q_OS_MAC
    seqResetTextZoom = QKeySequence("Meta+0");
#endif
    QShortcut *scTZ = new QShortcut(seqResetTextZoom, this);
    connect(scTZ, SIGNAL(activated()), this, SLOT(resetTextZoom()));

    // do not call initGlobalSettings here.
    // derived classes BgmnProject and FpProject need to
    // call their initSettings() first, and will
    // call ProjectWidget::initGlobalSettings() afterwards.
}

ProjectWidget::~ProjectWidget()
{
    // do not delete QObjects owned by this, will be done automatically
    if (graphModel) delete graphModel;
}

/*
 *  initialize global settings used by both types of projects
 */
void ProjectWidget::initGlobalSettings()
{
    edFont.fromString(settings->value("config/editorFont", font().toString()).toString());

    if (editors.size()) {
        QList<ControlFileEdit*> lst = editors.values();
        for (int i = 0; i < lst.size(); ++i) {
            lst[i]->setFont(edFont);
        }
    }

    refOutput->setFont(edFont);

    autoShowOutput = settings->value("config/autoShowOutput", false).toBool();
    stopOnConvergence = settings->value("config/stopOnConvergence", true).toBool();
    graphLiveUpdate = settings->value("config/graphLiveUpdate", true).toBool();
    switchToPage = settings->value("config/switchToPage", "0").toInt();

    int hklRefLineMode = settings->value("graph/hklOnBackground", 1).toInt();
    bool os = menuHklBase->blockSignals(true);
    actHklBaseZero->setChecked(hklRefLineMode == 0);
    actHklBaseBkgr->setChecked(hklRefLineMode == 1);
    actHklBaseDiff->setChecked(hklRefLineMode == 2);
    menuHklBase->blockSignals(os);

    // init the graph's settings
    graphView->initSettings();
    graphView->forceUpdate();

    ResultsTreeWidget *rtw = getResultsTree();
    if (rtw) rtw->initSettings();
    chemOutput->initSettings();
    scanList->initSettings();
    scanList->updateView();

    bool oldState = refStrFavoritesButton->blockSignals(true);
    refStrFavoritesButton->setChecked(settings->value("config/refStrFavoritesButton", false).toBool());
    refStrFavoritesButton->blockSignals(oldState);

    comboBoxRefStrucRepos->setEnabled(!refStrFavoritesButton->isChecked());

    Qt::ToolButtonStyle toolStyle = static_cast<Qt::ToolButtonStyle>(settings->value("window/toolButtonLayout", 4).toInt());
    refStrResetButton->setToolButtonStyle(toolStyle);
    refStrFavoritesButton->setToolButtonStyle(toolStyle);
    refStrAppendHklButton->setToolButtonStyle(toolStyle);
    refHdispResetButton->setToolButtonStyle(toolStyle);
    refHdispZeroButton->setToolButtonStyle(toolStyle);

    emit updateWavelength(graphView->getWaveLength());
}

/*
 *  updates the status flag and status text of this project
 */
void ProjectWidget::setStatus(global::RefinementStatus s)
{
    refStatus = s;
    pSelectItem->setStatus(refStatus);
    graphControl->setViewStatus(refStatus);
}

/*
 *  this function reads the text content of file fn, and displays it in editor e
 */
bool ProjectWidget::readFile(const QString &fn, ControlFileEdit *e)
{
    e->setPlainText(BgmnFileIO::readTextFile(fn));
    toggleTabNameSaving(e, false);
    e->setTextChanged(false);

    return !e->toPlainText().isEmpty();
}

/*
 * saves a text file under the same name
 */
bool ProjectWidget::saveCurrent()
{
    // get a pointer to the current tab's QTextEdit
    ControlFileEdit *e = getCurrentEditor();

    if (!e) return false;

    QString s = editors.key(e);

    if (s.isEmpty()) {
        return saveCurrentAs();
    }

    return writeFile(s, e);
}

/*
 * saves a text file under a new name
 */
bool ProjectWidget::saveCurrentAs()
{
    if (currentWidget()->inherits("GraphWindow")) {
        qDebug() << QString("ProjectWidget::saveCurrentAs(): Saving graph file under a new name.");
        return saveGraphFileAs();
    }

    // get a pointer to the current tab's QTextEdit
    ControlFileEdit *e = getCurrentEditor();

    // could not get the pointer
    if (!e) {
        qDebug() << QString("ProjectWidget::saveCurrentAs(): Could not get a pointer to the text edit");
        return false;
    }

    QFileInfo oldFi(editors.key(e));
    QFileInfo newFi(QFileDialog::getSaveFileName(this, tr("Save File As"), projectDir, tr("All Files (*.*)")));

    if (newFi.fileName().isEmpty()) {
        qDebug() << QString("ProjectWidget::saveCurrentAs(): File dialog was aborted, exiting.");
        return false;
    }

    // add the file extension if none was added (default: oldFi.suffix(), fallback: .txt)
    if (newFi.suffix().isEmpty()) {
        newFi = QFileInfo(newFi.absoluteFilePath() + QString(".%1").arg(oldFi.suffix().isEmpty() ? "txt" : oldFi.suffix()));
    }

    bool b = writeFile(newFi.absoluteFilePath(), e);

    if (b) {
        qDebug() << QString("ProjectWidget::saveCurrentAs(): Writing file %1 successful. Changing tab file name.").arg(newFi.absoluteFilePath());

        setTabText(currentIndex(), newFi.fileName());

        qDebug() << QString("ProjectWidget::saveCurrentAs(): Checking if project base name must be changed:");
        qDebug() << QString("    old file path:       %1").arg(oldFi.absolutePath());
        qDebug() << QString("    old file base name:  %1").arg(oldFi.completeBaseName());
        qDebug() << QString("    old projectDir:      %1").arg(projectDir);
        qDebug() << QString("    old projectBasename: %1").arg(projectBasename);
        qDebug() << QString("    new file path:       %1").arg(newFi.absolutePath());
        qDebug() << QString("    new file base name:  %1").arg(newFi.completeBaseName());

        // check if the old file name was a projectBaseName in projectDir
        // if yes, change the variables to the new file name
        // if no (e.g. because the file was a structure file, and not a control file), don't do anything
        if ((oldFi.absolutePath() == projectDir) && (oldFi.completeBaseName() == projectBasename)) {
            projectDir = newFi.absolutePath();
            projectBasename = newFi.completeBaseName();

            pSelectItem->setProjectName(projectBasename);
            pSelectItem->setProjectFileName(newFi.fileName());
            pSelectItem->setProjectFilePath(projectDir);

            qDebug() << QString("ProjectWidget::saveCurrentAs(): Changing projectDir to %1 and projectBasename to %2.").arg(projectDir, projectBasename);

            QString key = editors.key(e, QString());

            if (!key.isNull()) {
                ControlFileEdit *pe = editors.take(key);
                editors[newFi.absoluteFilePath()] = pe;
            } else {
                qDebug() << QString("ProjectWidget::saveCurrentAs(): Could not get key for textEditor.");
            }
        } else {
            qDebug() << QString("ProjectWidget::saveCurrentAs(): Not a control file, no need to change project variables");
        }
    } else {
        qDebug() << QString("ProjectWidget::saveCurrentAs(): Writing file failed.");
    }

    return b;
}

void ProjectWidget::resetTextZoom()
{
    ControlFileEdit *e = getCurrentEditor();
    if (!e) return;
    e->resetZoom();
}

/*
 * saves all open text files
 */
void ProjectWidget::saveAll()
{
    QStringList keys = static_cast<QStringList>(editors.keys());

    for (int i = 0; i < keys.size(); ++i) {
        saveEditorFile(keys.at(i));
    }
}

/*
 * provide an absolute file name. If an editor exists with this file
 * it will be saved to disk.
 */
void ProjectWidget::saveEditorFile(const QString &filename)
{
    if (!editors.contains(filename)) {
        qDebug() << QString(tr("ProjectWidget::saveEditorFile(): No editor for file %1 found. Skipping it.").arg(filename));
        return;
    }

    ControlFileEdit *e = editors[filename];

    if (e->getTextChanged()) {
        qDebug() << QString(tr("ProjectWidget::saveEditorFile(): File %1 was changed. Saving it.").arg(filename));
        writeFile(filename, e);
    } else {
        qDebug() << QString(tr("ProjectWidget::saveEditorFile(): File %1 was not changed. Skipping.").arg(filename));
    }
}

/*
 * writes the text from a QTextEdit to a file of filename fn
 */
bool ProjectWidget::writeFile(const QString &fn, ControlFileEdit *e)
{
    if (!e) {
        qDebug() << QString("ProjectWidget::writeFile(): Could not get a pointer to the text edit %1").arg(fn);
        return false;
    }

    BgmnFileIO::writeTextFile(fn, e->toPlainText());
    toggleTabNameSaving(e, false);
    e->setTextChanged(false);
    return true;
}

/*
 * saves the graph file in another format, either another scan format, or as a
 * rendered pixel or svg image
 */
bool ProjectWidget::saveGraphFileAs()
{
    QMap<QString, QVariant> flags;
    flags["fieldSeparator"] = settings->value("config/asciiFieldSeparator", QVariant(QString(" ")));
    flags["fixBgmnZero"]    = QVariant(false);

    // export handler in interactive mode
    ExportHandler exHandler;

    QMap<QString, QString> formats = exHandler.uidsByFilter();
    formats["Pixel Image (*.png *.PNG)"] = "RENDER_PNG";
    formats["Scalable Vector Graphics (*.svg *.SVG)"] = "RENDER_SVG";
    QStringList keys = static_cast<QStringList>(formats.keys());

    QString selFilter = settings->value("graph/saveAsFilter", QString()).toString();
    QString f = QDir::fromNativeSeparators(QString("%1/%2").arg(projectDir, projectBasename));
    f = QFileDialog::getSaveFileName(this, tr("Save File As"), f, keys.join(";;"), &selFilter);

    if (f.isEmpty()) {
        return false;
    }

    QFileInfo fout(f);
    if (fout.suffix().isEmpty()) {
        fout = QFileInfo(fout.absoluteFilePath() + QString(".%1").arg(exHandler.extensionByFilter(selFilter)));
    }

    QString uid = formats.value(selFilter);
    settings->setValue("graph/saveAsFilter", selFilter);

    if (uid == "RENDER_PNG") {
        qDebug() << QString("ProjectWidget::saveGraphFileAs(): Saving PNG file as %1").arg(fout.absoluteFilePath());
        int rasterW = settings->value("graph/rasterResolutionWidth", 1536).toInt();
        int rasterH = settings->value("graph/rasterResolutionHeight", 1024).toInt();
        graphView->renderBitmap(fout.absoluteFilePath(), rasterW, rasterH);
        return true;
    }

    if (uid == "RENDER_SVG") {
        graphView->saveSvg(fout.absoluteFilePath());
        qDebug() << QString("ProjectWidget::saveGraphFileAs(): Saving SVG file as %1").arg(fout.absoluteFilePath());
        return true;
    }

    // export to data file
    if (graphControl->saveScanFile(fout.absoluteFilePath(), uid, -1, flags)) {
        qDebug() << QString("ProjectWidget::saveGraphFileAs(): Scan file saved to %1 using format id = %2").arg(fout.absoluteFilePath(), uid);
        return true;
    }

    // no scan saved: report error
    qDebug() << QString("ProjectWidget::saveGraphFileAs(): No scan saved to file %1 using format id = %2)").arg(fout.absoluteFilePath(), uid);
    return false;
}

QString ProjectWidget::renderSvgDirectly()
{
    QFileInfo fi(graphModel->fileInfo());
    QFileInfo fo(QDir::toNativeSeparators(fi.absolutePath() + "/" + fi.completeBaseName() + ".svg"));
    graphView->saveSvg(fo.absoluteFilePath());
    qDebug() << QString("ProjectWidget::renderSvgDirectly(): SVG file saved as %1").arg(fo.absoluteFilePath());
    return fo.absoluteFilePath();
}

/*
 * emit a signal when the current tab is changed (use to enable/disable actions)
 */
void ProjectWidget::tabChanged(int i)
{
    if (i > 0) {
        emit savingEnabled(true);
    } else {
        emit savingEnabled(false);
    }

    // emit a signal anywas, in case other guis want to be synchronized
    emit currentTabChanged(this, i);
}

/*
 * raises a certain tab, used to synchronize project guis
 */
void ProjectWidget::raiseTab(int i)
{
    if (currentIndex() == i) return;
    if ((i >= 0) && (i < count())) setCurrentIndex(i);
}

/*
 * overloaded function. Tries to locate a tab with the name "s".
 * note that s can have a trailing * if the file is unsaved. We must ignore it.
 *
 * if none is found, tries to locate a tab with the same file extension.
 * if none is found, does nothing.
 * if multiple files with the same extension are found, raises the first one.
 *
 * all comparisons are case-insensitive
 *
 * l is the current line to be set. If -1, l is ignored.
 */
void ProjectWidget::raiseTab(const QString &s, int l)
{
    if (s.isEmpty()) return;
    QString rname(s.left(s.indexOf("*")));
    QStringList fnames;
    QStringList fextensions;

    for (int i = 0; i < count(); ++i) {
        QFileInfo ft(tabText(i).left(tabText(i).indexOf("*")));
        fnames << ft.fileName().toLower();
        fextensions << ft.suffix().toLower();
    }

    QFileInfo fi(rname);
    int n = fnames.indexOf(fi.fileName().toLower());
    if (n < 0) n = fextensions.indexOf(fi.suffix().toLower());
    if ((n < 0) || (n >= count())) return;

    setCurrentIndex(n);

    if (l >= 0) {
        ControlFileEdit *e = getCurrentEditor();
        if (e) e->setCurrentLine(l);
    }
}

/*
 * This slot is called when process handler has aborted the process
 */
void ProjectWidget::processAborted()
{
    pSelectItem->setStatus(global::RefinementStatus::ABORTED);
}

/*
 *  reverts the content of the control editor to the text that was saved
 *  just before the last refinement call
 */
void ProjectWidget::revertControl()
{
    if (!previousControlContent.size()) {
        return;
    }

    QPlainTextEdit *e = fileEditor(controlFile, false);

    if (e) {
        e->setPlainText(previousControlContent.takeLast());
    }
}

void ProjectWidget::selectReferenceHklFile(const QString &f)
{
    QFileInfo fi(f);
    int idx = comboBoxRefStructures->findText(fi.completeBaseName());
    comboBoxRefStructures->setCurrentIndex(idx);
}

/*
 * Inserts a graph file into the existing graphWindow without clearing
 * the existing scans. All names will still be taken from the first
 * scan, and no other files will be loaded automatically.
 * The first scan which is already present will always be the "master"
 * file.
 */
QStringList ProjectWidget::insertGraph(const QStringList &l, const QString &uid, bool switchTab)
{
    QStringList errors;

    for (int i = 0; i < l.size(); ++i) {
        if (graphControl->addScanFile(l.at(i), uid, false, true) > 0) {
            qDebug() << QString("ProjectWidget::insertGraph(): File %1 added").arg(l.at(i));
        } else {
            qDebug() << QString("ProjectWidget::insertGraph(): Could not add file %1").arg(l.at(i));
            errors.append(l.at(i));
        }
    }

    if (switchTab) {
        setCurrentIndex(0);
    }

    return errors;
}

void ProjectWidget::openTextFileEditors(const QStringList &txt)
{
    if (!txt.size()) return;

    for (int i = 0; i < txt.size(); ++i) {
        if (!txt.at(i).isEmpty()) loadTextFile(txt.at(i));
    }

    ControlFileEdit *e = fileEditor(txt.last(), false);
    if (e) raiseTab(editorsTabIndex(e));
}

/*
 * loads a graph file in the graph widget
 */
void ProjectWidget::loadGraph(const QString &s, const QString &uid)
{
    QFileInfo fi(s);
    setTabText(0, fi.fileName());

    // NOTE: Graphs are the ones that determine the project name. Text files can have other
    // names (e.g. STR files) that should not change the project basename.
    projectBasename = fi.completeBaseName();
    projectDir = QDir::fromNativeSeparators(fi.absolutePath());
    loadedGraphFile = fi.fileName();
    controlFile = getControlFileName();

    // store the current sample id, in case we load a DIA or PRF file, we must write it
    // back after loading the file because it is not read from the file.
    sampleID = getSampleID();

    pSelectItem->setProjectName(projectBasename);
    pSelectItem->setProjectFileName(fi.fileName());
    pSelectItem->setProjectFilePath(projectDir);

    // load the file, provide the sampleID. if the file contains a sampleID, the provided
    // one will be ignored
    int n = graphControl->loadScanFile(s, uid, false, sampleID, false);

    // read back the sample in case a different one was read from the graph file.
    if (n > 0) {
        sampleID = graphControl->getSampleId();
    }

    pSelectItem->setProjectSampleId(sampleID);

    qDebug() << QString("ProjectWidget::loadGraph(): Using wavelength %1").arg(graphControl->getWaveLength(settings->defaultWavelength()));
    emit updateWavelength(graphControl->getWaveLength(settings->defaultWavelength()));
}

/*
 * loads a text file. If a text editor with the same document title already
 * exists, it replaces its text. otherwise a new tab and editor is created
 */
bool ProjectWidget::loadTextFile(const QString &s)
{
    QFileInfo fi(s);
    if (!fi.exists()) return false;

    // we need projectBasename and projectDir for the auto-loading functions. so if they are empty (e.g.
    // if a new project was created from a text file), use this information even though it will be
    // overwritten when loading a graph file. but without a temporary projectBasename or Dir, auto-
    // loading would not work at all.
    if ((projectBasename == "") || (projectDir == "")) {
        if (textFormats.contains(fi.suffix().toLower())) {
            projectBasename = fi.completeBaseName();
            projectDir = fi.absolutePath(); // always uses "/" separator, which is correct

            pSelectItem->setProjectName(projectBasename);
            pSelectItem->setProjectFileName(fi.fileName());
            pSelectItem->setProjectFilePath(projectDir);
        }
    }

    bool isControlFile = false;

    // if the loaded file was a control file, set the variable name to the file's name
    if (controlFileExtension.simplified().toLower() == fi.suffix().simplified().toLower()) {
        controlFile = fi.absoluteFilePath();
        isControlFile = true;
        qDebug() << QString("ProjectWidget::loadTextFile(): setting control file to %1").arg(controlFile);
    }

    bool b = false;

    // check if there is an editor with the same document title
    ControlFileEdit *e = fileEditor(fi.absoluteFilePath(), true);

    if (e) {
        // replace its text with the new file
        b = readFile(fi.absoluteFilePath(), e);

        if (b && isControlFile) {
            previousControlContent.append(e->toPlainText());
        }

        // set the tab title, just in case something changed
        int n = editorsTabIndex(e);
        setTabText(n, fi.fileName());
    }

    return b;
}

/*
 * returns the index of the tab containing a certain texteditor
 */
int ProjectWidget::editorsTabIndex(const ControlFileEdit *e)
{
    for (int i = 0; i < count(); ++i) {
        if (widget(i) == e) {
            return i;
        }
    }

    return -1;
}

/*
 * returns true if the project basename and directory is identical to the provided strings.
 * this is used by the caller, e.g. to check whether a new project should be created or the
 * files of this project should be updated.
 */
bool ProjectWidget::isIdenticalTo(const QString &basename, const QString &dir)
{
    if ((basename == projectBasename) && (dir == projectDir)) {
        return true;
    }

    return false;
}

/*
 * automatically load other text files belonging to a project (except the file "caller")
 */
void ProjectWidget::autoLoadTextFiles(const QString &caller)
{
    QStringList tFiles = autoDetermineTextFiles(caller);

    for (int i = 0; i < tFiles.size(); ++i) {
        loadTextFile(tFiles.at(i));
    }
}

QStringList ProjectWidget::autoDetermineTextFiles(const QString &caller)
{
    QFileInfo fiCaller(caller);

    // searching file names is case-insensitive
    QSet<QString> fileMatcher;
    QStringList matchedFiles;

    for (int i = 0; i < textFormats.size(); ++i) {
        QString absFilePath(projectDir + "/" + projectBasename + "." + textFormats.at(i));
        QFileInfo fiText(absFilePath);

        if (fiText == fiCaller) {
            continue;
        }

        if (fileMatcher.contains(absFilePath.toLower())) {
            continue;
        }

        fileMatcher << absFilePath.toLower();
        matchedFiles << absFilePath;
    }

    return matchedFiles;
}

QString ProjectWidget::autoDetermineGraphFile()
{
    QString fileBase = projectDir + "/" + projectBasename;

    for (int i = 0; i < graphFormats.size(); ++i) {
        QString f = fileBase + "." + graphFormats.at(i);

        if (QFileInfo::exists(f)) {
            return f;
        }
    }

    return QString();
}

/*
 * returns the name of the current graph file
 */
QString ProjectWidget::getGraphFile()
{
    return graphControl->fileInfo().absoluteFilePath();
}

/*
 * close a tab (ignore closing of the graph tab)
 */
void ProjectWidget::closeTab(int index)
{
    if (index < 1) {
        qDebug() << QString("ProjectWidget::closeTab(): Tab %1 cannot be closed").arg(index);
        return;
    }

    if (widget(index)->inherits("QPlainTextEdit")) {
        ControlFileEdit *e = static_cast<ControlFileEdit*>(widget(index));
        QString f = editors.key(e);
        e = editors.take(f);

        if (highlighters.contains(e)) {
            QSyntaxHighlighter *hl = static_cast<QSyntaxHighlighter*>(highlighters.take(e));
            delete hl;
        }

        delete e;
    }
}

/*
 * close a tab (s is an absolute file name)
 */
void ProjectWidget::closeTab(const QString &s)
{
    ControlFileEdit *e = editors.take(s);
    if (!e) return;

    if (highlighters.contains(e)) {
        QSyntaxHighlighter *hl = static_cast<QSyntaxHighlighter*>(highlighters.take(e));
        delete hl;
    }

    delete e;
}

/*
 * returns the qtextedit for a file of name 'f'
 *
 * c = true: create a new one of none exists
 * c = false: return nullpointer if none exists
 */
ControlFileEdit * ProjectWidget::fileEditor(const QString &f, bool c)
{
    if (editors.contains(f)) return editors.value(f);
    if (!c)                  return nullptr;

    QFileInfo fi(f);
    ControlFileEdit *e = new ControlFileEdit(this, textFileType(f));
    e->setFileName(fi.absoluteFilePath());
    e->setBaseName(fi.baseName());
    textChangedSignalMapper->setMapping(e, e);
    connect(e, SIGNAL(cursorPositionChanged()), this, SLOT(cursorPositionChanged()));
    connect(e, SIGNAL(textChanged()), textChangedSignalMapper, SLOT(map()));
    connect(e, SIGNAL(openText(QString)), this, SLOT(loadTextFileFromContextMenu(QString)));
    connect(e, SIGNAL(openGraph(QString)), this, SLOT(loadGraphFileFromContextMenu(QString)));
    connect(e, SIGNAL(wordUnderCursor(QStringList)), this, SIGNAL(parameterSelected(QStringList)));
    connect(e, SIGNAL(helpText(QString)), this, SIGNAL(moduleHelpText(QString)));

    editors.insert(f, e);
    addTab(e, fi.fileName());
    e->setFont(edFont);
    setHighlighting(e);
    return e;
}

ControlFileEdit::ControlFileEditFileType ProjectWidget::textFileType(const QString &s)
{
    QFileInfo fi(s);
    if (fi.suffix().toLower() == "sav") return ControlFileEdit::ControlFileEditFileType::BGMN_SAV;
    if (fi.suffix().toLower() == "lst") return ControlFileEdit::ControlFileEditFileType::BGMN_LST;
    if (fi.suffix().toLower() == "str") return ControlFileEdit::ControlFileEditFileType::BGMN_STR;
    return ControlFileEdit::ControlFileEditFileType::OTHER;
}

QString ProjectWidget::currentFileName() const
{
    if (currentWidget()->inherits("ControlFileEdit")) {
        return editors.key(static_cast<ControlFileEdit *>(currentWidget()), QString());
    }

    return graphControl->fileName();
}

/*
 * writes the string s to the texteditor of the control file. If no text editor
 * exists, a new one is created first.
 *
 * This function will not change the content of s at all.
 */
void ProjectWidget::setControlFileContent(const QString &s)
{
    QPlainTextEdit *e = fileEditor(controlFile, true);
    if (e) e->setPlainText(s);
}

/*
 * returns the content of the control file text editor
 */
QString ProjectWidget::getControlFileContent()
{
    QPlainTextEdit *e = fileEditor(controlFile, false);
    if (e) return e->toPlainText();
    return QString();
}

/*
 * writes the string s to the texteditor or the control file. in addition
 * to ::setControlFileContent(), this function changes all output filenames
 * to the project's basename.
 */
void ProjectWidget::applyControlFileContent(const QString &s)
{
    QString str = adjustControlOutputFiles(s, true);
    setControlFileContent(str);
}

void ProjectWidget::print()
{
    if (currentIndex() == 0) {
        printGraph();
    } else{
        printText();
    }
}

void ProjectWidget::printText()
{
    ControlFileEdit *e = getCurrentEditor();

    if (!e) return;

    QPrinter printer(QPrinterInfo::defaultPrinter(), QPrinter::HighResolution);

#ifdef Q_OS_UNIX
    QString s = editors.key(e) + ".pdf";
    printer.setOutputFileName(s);
#endif

    QPrintDialog printDialog(&printer, this);

    if (printDialog.exec() != QDialog::Accepted) {
        return;
    }

    e->print(&printer);
}

void ProjectWidget::printGraph()
{
    QPrinter printer(QPrinterInfo::defaultPrinter(), QPrinter::HighResolution);

    if (!printer.isValid()) {
        qDebug() << QString("ProjectWidget::printGraph(): Invalid printer object, exiting.");
        return;
    }


    // this works on Linux et al, but not on OS X and Windows
#ifdef Q_OS_UNIX
    #ifndef Q_OS_OSX
        QFileInfo fi(getGraphFile());
        printer.setOutputFileName(QDir::fromNativeSeparators(fi.absolutePath() + "/" + fi.completeBaseName() + ".pdf"));
    #endif
#endif

    QPrintDialog printDialog(&printer, this);

    if (printDialog.exec() != QDialog::Accepted) {
        return;
    }

    QPainter painter;
    painter.begin(&printer);
    graphView->print(printer, painter);
    painter.end();
}

void ProjectWidget::printGraphDirectly(QPrinter &printer, QPainter &painter)
{
    graphView->print(printer, painter);
}

void ProjectWidget::cursorPositionChanged()
{
    if (!QObject::sender()->inherits("ControlFileEdit")) {
        return;
    }

    ControlFileEdit *e = dynamic_cast<ControlFileEdit*>(QObject::sender());

    if (e) {
        QTextCursor cursor = e->textCursor();
        emit cursorPosition(cursor.blockNumber() + 1, cursor.positionInBlock() + 1);
    }
}

void ProjectWidget::setShowLegend(bool b)
{
    graphView->slotSetDrawLegend(b);
}

/*
 * is called by a QTextEdit::textChanged() signal
 */
void ProjectWidget::textChanged(QObject *w)
{
    // make sure the signal was sent by a QTextEdit
    if (w->inherits("ControlFileEdit")) {
        ControlFileEdit *cte = static_cast<ControlFileEdit*>(w);
        if (cte) {
            toggleTabNameSaving(cte, true);
            cte->setTextChanged(true);
        }
    }
}

/*
 * appends / removes a * to the tab name if the text was changed
 * and needs to be saved
 */
void ProjectWidget::toggleTabNameSaving(ControlFileEdit *e, bool b)
{
    int n = editorsTabIndex(e);
    static QRegularExpression rx("^([^\\*]*)\\*?$");
    QRegularExpressionMatch rm = rx.match(tabText((n)));

    if (rm.hasMatch()) {
        if (b) {
            setTabText(n, QString("%1*").arg(rm.captured(1)));
        } else {
            setTabText(n, QString("%1").arg(rm.captured(1)));
        }
    }
}

/*
 * the combo box containing reference structures was changed.
 * update the graphs
 */
void ProjectWidget::referenceStructureChanged(int i)
{
    displayReferenceStructure(comboBoxRefStructures->itemData(i).toString());

    if (i) {
        refStrAppendHklButton->setEnabled(true);
    }
}

/*
 * resets the combo box to the first item, which is empty and
 * removes all reference lines from the graph
 */
void ProjectWidget::resetRefStructure()
{
    if (comboBoxRefStructures->count()) {
        comboBoxRefStructures->setCurrentIndex(0);
    }

    refStrAppendHklButton->setEnabled(false);
}

/*
 * return the graph's wavelength
 */
double ProjectWidget::wavelength()
{
    return graphView->getWaveLength();
}

/*
 * return the control file name. Make sure to capture extensions in captial and small letters
 */
QString ProjectWidget::getControlFileName()
{
    QDir pdir(projectDir);

    // scan the project dir for an existing control file with either small or capital extension
    QStringList filter;
    filter << projectBasename + "." + controlFileExtension.toLower();
    filter << projectBasename + "." + controlFileExtension.toUpper();

    QFileInfoList flist = pdir.entryInfoList(filter, QDir::Files);

    // if at least one was found, return it. If both were found, return whatever is first in the list.
    if (flist.size()) {
        return flist.first().absoluteFilePath();
    }

    // if no existing one was found, construct a new file name.
    return QDir::fromNativeSeparators(projectDir + "/" + projectBasename + "." + controlFileExtension);
}

/*
 * Returns the following values of the current graph:
 *
 * x1: current lower angle
 * x2: current upper angle
 * y1: current lower intensity
 * y2: current upper intensity
 * mx1: minimum angle
 * mx2: maximum angle
 * my1: minimum intensity
 * my2: maximum intensity
 */
QVector<double> ProjectWidget::getZoomRange()
{
    double x1 = 0.0;
    double x2 = 0.0;
    double y1 = 0.0;
    double y2 = 0.0;

    double mx1 = 0.0;
    double mx2 = 0.0;
    double my1 = 0.0;
    double my2 = 0.0;

    graphView->getZoomRange(x1, x2, y1, y2);
    graphView->getMaxRange(mx1, mx2, my1, my2);

    qDebug() << QString("ProjectWidget::getZoomRange(): Graph returned current values: %1 %2 %3 %4").arg(x1).arg(x2).arg(y1).arg(y2);
    qDebug() << QString("ProjectWidget::getZoomRange(): Graph returned maximum values: %1 %2 %3 %4").arg(mx1).arg(mx2).arg(my1).arg(my2);

    if (graphView->getYScaling() == YSCALESQRT) {
        y1 *= y1;
        y2 *= y2;
        my1 *= my1;
        my2 *= my2;
    }

    if (graphView->getYScaling() == YSCALELOG10) {
        y1 = pow(10.0, y1);
        y2 = pow(10.0, y2);
        my1 = pow(10.0, my1);
        my2 = pow(10.0, my2);
    }

    qDebug() << QString("ProjectWidget::getZoomRange(): Current values adjusted for y-scale: %1 %2 %3 %4").arg(x1).arg(x2).arg(y1).arg(y2);
    qDebug() << QString("ProjectWidget::getZoomRange(): Maximum values adjusted for y-scale: %1 %2 %3 %4").arg(mx1).arg(mx2).arg(my1).arg(my2);

    QVector<double> vals(8, 0.0);
    vals[0] = x1;
    vals[1] = x2;
    vals[2] = y1;
    vals[3] = y2;
    vals[4] = mx1;
    vals[5] = mx2;
    vals[6] = my1;
    vals[7] = my2;
    return vals;
}

void ProjectWidget::setZoomRange(double x1, double x2, double y1, double y2)
{
    double zy1 = y1;
    double zy2 = y2;

    if (graphView->getYScaling() == YSCALESQRT) {
        zy1 = sqrt(y1);
        zy2 = sqrt(y2);
    }

    if (graphView->getYScaling() == YSCALELOG10) {
        zy1 = y1 < 1.0 ? y1 : log10(y1);
        zy2 = y2 < 1.0 ? y2 : log10(y2);
    }

    graphView->setZoomRange(x1, x2, zy1, zy2, false);
}

void ProjectWidget::resetZoomRange()
{
    graphView->resetZoom();
    graphView->forceUpdate();
}

void ProjectWidget::dumpCoordinates(double x, double y, double d)
{
    refOutput->appendPlainText(QString("2theta=%1 I=%2 d=%3").arg(x, 0, 'f', 6).arg(y, 0, 'f', 6).arg(d, 0, 'f', 6));

    Scan *actScan = graphControl->firstActiveScan();

    if (!actScan) return;

    double dnm = 0.1 * d;
    double tt  = global::Functions::dToTwoTheta(d, graphView->getWaveLength());
    double intens = actScan->intensity(actScan->indexOfAngle(tt, 0));
    if (intens < 0.0) intens = y;

    double pos = tt;
    if (actScan->getHklXunit() == "dnm") pos = dnm;

    actScan->pDataHkl().append(Hkl(pos, QString(), 0, QString(), QColor(), intens));
    graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);
}

void ProjectWidget::scanYOffsetUp(bool firstOnTop)
{
    graphControl->yOffsetUp(firstOnTop);
    graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
}

void ProjectWidget::scanYOffsetDown(bool firstOnTop)
{
    graphControl->yOffsetDown(firstOnTop);
    graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
}

void ProjectWidget::scanXOffsetLeft()
{
    graphControl->xOffsetLeft();
    graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
}

void ProjectWidget::scanXOffsetRight()
{
    graphControl->xOffsetRight();
    graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
}

void ProjectWidget::scanOffsetReset()
{
    graphControl->resetOffset();
    graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
}

void ProjectWidget::removeCurrentScan()
{
    QVector<Scan*> aScans = graphControl->activeScans();

    for (int i = 0; i < aScans.size(); ++i) {
        if (aScans.at(i)) graphControl->removeScan(aScans.at(i)->uid(), true);
    }
}

/*
 * this slot loads a text file. It is called from the mouse context menu
 * of a ControlFileEditor.
 */
void ProjectWidget::loadTextFileFromContextMenu(const QString &s)
{
    QFileInfo fi(QDir(projectDir), s);

    if (!fi.exists()) {
        qDebug() << QString("ProjectWidget::loadTextFileFromContextMenu(): file %1 does not exist").arg(fi.absoluteFilePath());
        refOutput->appendPlainText(QString("File %1 does not exist").arg(fi.absoluteFilePath()));
        return;
    }

    openTextFileEditors(QStringList(fi.absoluteFilePath()));
    emit sigFileOpenedExclusively(QStringList(fi.absoluteFilePath()), this);
}

/*
 * this slot loads a text file. It is called from the mouse context menu
 * of a ControlFileEditor.
 */
void ProjectWidget::loadGraphFileFromContextMenu(const QString &s)
{
    QFileInfo fi(QDir(projectDir), s);

    if (!fi.exists()) {
        qDebug() << QString("ProjectWidget::loadTextFileFromContextMenu(): file %1 does not exist").arg(fi.absoluteFilePath());
        refOutput->appendPlainText(QString("File %1 does not exist").arg(fi.absoluteFilePath()));
        return;
    }

    emit openGraphFileInNewProject(QStringList(fi.absoluteFilePath()), QString());
}

void ProjectWidget::sampleDisplacementsChanged(double e1, double e2, double e3)
{
    lastEps1 = e1;
    lastEps2 = e2;
    lastEps3 = e3;

    bool oldstate = refHdispValue->blockSignals(true);
    refHdispValue->setValue(e2);
    refHdispValue->blockSignals(oldstate);

    graphControl->setAngularCorrections(e1, e2, e3);
    graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);

    if (searchMatchWidget)     searchMatchWidget->setEps2(e2);
    if (peakIntegrationWidget) peakIntegrationWidget->setSampleDisplacements(e1, e2, e3);
}

void ProjectWidget::sampleHeightDisplacementChanged(double d)
{
    graphControl->setAngularCorrections(lastEps1, d, lastEps3);
    graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);

    if (searchMatchWidget)     searchMatchWidget->setEps2(d);
    if (peakIntegrationWidget) peakIntegrationWidget->setSampleDisplacements(lastEps1, d, lastEps3);
}

void ProjectWidget::zeroSampleHeightDisplacement()
{
    bool oldstate = refHdispValue->blockSignals(true);
    refHdispValue->setValue(0.0);
    refHdispValue->blockSignals(oldstate);

    graphControl->setAngularCorrections(0.0, 0.0, 0.0);
    graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
}

void ProjectWidget::resetSampleHeightDisplacement()
{
    sampleDisplacementsChanged(lastEps1, lastEps2, lastEps3);
}

ControlFileEdit* ProjectWidget::getCurrentEditor()
{
    if (!currentWidget()->inherits("ControlFileEdit")) {
        return nullptr;
    }

    return dynamic_cast<ControlFileEdit*>(currentWidget());
}

void ProjectWidget::searchReplace()
{
    ControlFileEdit *e = getCurrentEditor();
    if (!e) return;

    SearchReplaceAllDialog *srdlg = new SearchReplaceAllDialog(this);

    if (srdlg->exec() == QDialog::Accepted) {
        int n = e->replaceAll(srdlg->findString(),
                              srdlg->isRegExp(),
                              srdlg->replaceString(),
                              srdlg->caseSensitive(),
                              srdlg->wholeWords());
        QMessageBox::information(this, tr("Search and Replace"), QString(tr("%1 occurrences replaced.").arg(n)));
    }

    delete srdlg;
}

void ProjectWidget::resetMarginColor()
{
    setStatus(global::RefinementStatus::IDLE);
}

void ProjectWidget::saveCifFiles(bool complete, const QMap<QString, QVariant> &auxDataGlobal)
{
    /* subclass if needed */
    Q_UNUSED(complete);
    Q_UNUSED(auxDataGlobal);
}

QList<phaseData> ProjectWidget::getCifData(const QMap<QString, QVariant> &auxDataGlobal)
{
    /* subclass if needed */
    Q_UNUSED(auxDataGlobal);
    return QList<phaseData>();
}

QString ProjectWidget::getPeakDataCsv(bool header) const
{
    if (!peakListWidget) return QString();
    peakListWidget->updateData(true);
    return peakListWidget->getCsvDataAll(header);
}

void ProjectWidget::refStrFavoritesButtonToggled(bool b)
{
    // if the button was toggled, we only send a signal to the parent.
    // the parent will forward the signal to all open projects, including
    // the sender
    settings->setValue("config/refStrFavoritesButton", b);
    emit referenceStructureFavoritesToggled(b);
}

bool ProjectWidget::isRefStrFavorites()
{
    return refStrFavoritesButton->isChecked();
}

QDomElement ProjectWidget::getPeakIntegralRanges()
{
    if (!peakIntegrationWidget) return QDomElement();

    QDomDocument doc("ProfexPreset");
    QDomElement root = doc.createElement("preset");
    doc.appendChild(root);

    peakIntegrationWidget->getPreset(doc);

    QDomNodeList l = doc.elementsByTagName("peakIntegrals");
    if (l.size()) return l.at(0).toElement();
    return QDomElement();
}

void ProjectWidget::applyPeakIntegralRanges(const QDomElement &e)
{
    peakIntegrationWidget->applyPreset(e);
}

QString ProjectWidget::getPeakIntegralCsv()
{
    return peakIntegrationWidget->getDataCsv();
}

QString ProjectWidget::getCurveFitReport()
{
    return peakFitWidget->getReport(loadedGraphFile);
}

void ProjectWidget::refStrAppendHklToGraph()
{
    QString struc = comboBoxRefStructures->currentData().toString();
    QString sname = comboBoxRefStructures->currentText();
    qDebug() << QString("ProjectWidget::refStrAppendHklToGraph(): getting hkl lines for %1").arg(struc);

    QVector<Hkl> vec = getReferenceHklLines(struc);

    if (!vec.size()) {
        qDebug() << QString("ProjectWidget::refStrAppendHklToGraph(): hkl data is empty, exiting.");
        return;
    }

    resetRefStructure();

    qDebug() << QString("ProjectWidget::refStrAppendHklToGraph(): found %1 hkl lines").arg(vec.size());
    Scan scan(sname + " (hkl)");
    scan.setHklData(vec);
    scan.setWaveLength(graphView->getWaveLength());
    scan.setTypes(Scan::HKL | Scan::SYNTHETIC);
    scan.setSourceFileName(struc);
    graphControl->appendHklScan(scan, true);
}

QStringList ProjectWidget::getOpenFileNames(const QString &ext)
{
    QStringList openFiles;
    QMapIterator<QString, ControlFileEdit*> it(editors);

    while (it.hasNext()) {
        it.next();

        QFileInfo fi(it.value()->getFileName());

        if (ext.isEmpty()) {
            openFiles.append(fi.absoluteFilePath());
        } else {
            if (fi.suffix().toLower() == ext.toLower()) {
                openFiles.append(fi.absoluteFilePath());
            }
        }
    }

    return openFiles;
}

QDomElement ProjectWidget::getCurveFitData()
{
    if (!peakFitWidget) return QDomElement();

    QDomDocument doc("ProfexPreset");
    QDomElement root = doc.createElement("preset");
    doc.appendChild(root);

    peakFitWidget->getPreset(doc);

    QDomNodeList l = doc.elementsByTagName("curveFit");
    if (l.size()) return l.at(0).toElement();
    return QDomElement();
}

void ProjectWidget::applyCurveFitData(const QDomElement &el)
{
    if (!peakFitWidget) return;
    peakFitWidget->applyPreset(el);
}

QMap<QString, QMap<int, QStringList> > ProjectWidget::searchInOpenFiles(const QString &type, const QString &pattern)
{
    QMap<QString, QMap<int, QStringList> > results;

    if (type == "cur") {
        ControlFileEdit *e = getCurrentEditor();
        if (e) {
            results[e->getFileName()] = e->findAll(pattern);
        }

        return results;
    }

    QStringList ext;
    if (type == "sav") ext << "sav" << "pcr";
    if (type == "lst") ext << "lst" << "out";
    if (type == "str") ext << "str" << "pcr";

    QMapIterator<QString, ControlFileEdit*> it(editors);

    while (it.hasNext()) {
        it.next();
        QFileInfo fi(it.value()->getFileName());

        if (type == "all" || ext.contains(fi.suffix().toLower())) {
            QMap<int, QStringList> lines = findInEditorText(it.value(), pattern);

            if (lines.size()) {
                results[fi.absoluteFilePath()] = lines;
            }
        }
    }

    return results;
}

QMap<int, QStringList> ProjectWidget::findInEditorText(const ControlFileEdit *e, const QString &pattern)
{
    QMap<int, QStringList> matches;
    if (!e) return matches;

    QRegularExpression rx(pattern);
    QStringList content = e->toPlainText().split("\n");

    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch rm = rx.match(content.at(i));
        if (rm.hasMatch()) matches[i+1] = QStringList() << rm.captured(1) << content.at(i);
    }

    return matches;
}

QMap<int, QStringList> ProjectWidget::findInFileText(const QString &file, const QString &pattern)
{
    QMap<int, QStringList> matches;
    if (!QFile::exists(file)) return matches;

    QRegularExpression rx(pattern);
    QStringList content = BgmnFileIO::readTextFileLines(file);

    for (int i = 0; i < content.size(); ++i) {
        QRegularExpressionMatch rm = rx.match(content.at(i));
        if (rm.hasMatch()) matches[i+1] = QStringList() << rm.captured(1) << content.at(i);
    }

    return matches;
}

void ProjectWidget::overrideProjectWavelength(double d, const Scan::WavelengthMode &w)
{
    graphView->overrideWaveLength(d, w);
    graphControl->setOverrideWavelength(d, w);
}

void ProjectWidget::setHklBaseMode(QAction *act)
{
    int n = 0;
    if      (act == actHklBaseBkgr) n = 1;
    else if (act == actHklBaseDiff) n = 2;

    bool os = menuHklBase->blockSignals(true);
    actHklBaseZero->setChecked(n == 0);
    actHklBaseBkgr->setChecked(n == 1);
    actHklBaseDiff->setChecked(n == 2);
    menuHklBase->blockSignals(os);

    graphView->setOverrideHklBaseLine(n);
}

void ProjectWidget::setPeakListFilter(const QDomElement &el)
{
    if (peakListWidget) peakListWidget->setFilterParameters(el);
}

QDomElement ProjectWidget::getPeakListFilter() const
{
    if (peakListWidget) return peakListWidget->getFilterParameters();
    return QDomElement();
}

void ProjectWidget::startFit(QThreadPool *pool)
{
    if (!peakFitWidget) return;

    peakFitWidget->startFit(pool);
}

void ProjectWidget::onFitCompleted(global::RefinementStatus stat)
{
    setStatus(stat);
    emit fitCompleted(this);
}

void ProjectWidget::rangeDoubleClicked(int g, int n)
{
    if (g == 1) {
        if (!peakFitWidget) return;
        peakFitWidget->selectRegion(n);
    }
}
