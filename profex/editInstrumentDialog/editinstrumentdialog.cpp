/***************************************************************************
                          editinstrumentdialog.cpp  -  description
                             -------------------
    begin                : Wed Feb 30 11:00:00 CEST 2013
    copyright            : (C) 2013 by Nicola Doebelin
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

#include "editinstrumentdialog.h"
#include "editinstrumentparsingerrordialog.h"
#include "ui_editinstrumentdialog.h"
#include "../libXrdIO/bgmnfileio.h"
#include "projectWidget/syntaxHighlighter/bgmnhighlighter.h"
#include "projectWidget/bgmnbackendconfig.h"
#include "../libXrdIO/parser/bgmninstrumentsavparser.h"
#include "../libXrdIO/parser/bgmnsavparser.h"
#include "opticsconfigpageblank.h"
#include "opticsconfigpagegonio.h"
#include "opticsconfigpagefocus.h"
#include "opticsconfigpagepcol.h"
#include "opticsconfigpagescol.h"
#include "opticsconfigpagesslit.h"
#include "opticsconfigpagehslit.h"
#include "opticsconfigpagevslit.h"
#include "opticsconfigpagesample.h"
#include "opticsconfigpagedetector.h"
#include "opticsconfigpagemonochromator.h"
#include "opticsconfigpagetslit.h"
#include "opticsconfigpagerslit.h"
#include "opticsconfigpagedslit.h"
#include "opticsconfigpageairscat.h"

#include <QString>
#include <QtCore>
#include <QtGui>
#include <QtWidgets>
#include <QSplitter>

EditInstrumentDialog::EditInstrumentDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EditInstrumentDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    rayTracer = new InstrumentRayTracerMT;
    rayTracer->setProgressBar(ui->progressBar);
    rayTracer->setOutputEditor(ui->textEditOutput);

    for (int i = 0; i < ui->stackedWidgetEditor->count(); ++i) {
        ui->stackedWidgetEditor->widget(i)->layout()->setContentsMargins(0, 0, 0, 0);
    }

    bool dm = settings->isDarkMode();
    scene = new InstrumentScene(dm);
    if (dm) scene->setBackgroundBrush(QBrush(QGuiApplication::palette().color(QPalette::Base)));
    ui->graphicsViewInstrument->setScene(scene);

    ui->tabWidgetParams->widget(0)->layout()->setContentsMargins(0, 0, 0, 0);
    ui->tabWidgetParams->widget(1)->layout()->setContentsMargins(0, 0, 0, 0);

    ui->toolButtonRevertFile->setEnabled(false);

    connect(scene, SIGNAL(itemSelected(QString)), this, SLOT(instrumentItemSelected(QString)));
    connect(ui->textEditFile, SIGNAL(textChanged()), this, SLOT(textDocumentChanged()));
    connect(ui->textEditFile, SIGNAL(wordUnderCursor(QStringList)), this, SLOT(wordUnderCursorChanged(QStringList)));
    connect(ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(configItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
    connect(rayTracer, SIGNAL(processComplete()), this, SLOT(calculationComplete()));
    connect(ui->treeWidget, SIGNAL(itemChanged(QTreeWidgetItem*,int)), this, SLOT(toggleItemInstalled(QTreeWidgetItem*,int)));

    QStringList devDirs = settings->value("bgmnProject/deviceDatabase", QStringList()).toStringList();

    QMenu *fileOpenMenu = new QMenu;
    QMenu *fileSaveAsMenu = new QMenu;

    loadCurrentAction = fileOpenMenu->addAction(tr("Current directory"), this, SLOT(loadFile()));
    saveCurrentAction = fileSaveAsMenu->addAction(tr("Current directory"), this, SLOT(fileSaveAs()));

    for (int i = 0; i < devDirs.size(); ++i) {
        QAction *actLoad = fileOpenMenu->addAction(QString(tr("Repository: %1")).arg(devDirs.at(i)), this, SLOT(loadFile()));
        QAction *actSave = fileSaveAsMenu->addAction(QString(tr("Repository: %1")).arg(devDirs.at(i)), this, SLOT(fileSaveAs()));
        actLoad->setData(devDirs.at(i));
        actSave->setData(devDirs.at(i));
    }

    ui->toolButtonLoadFile->setPopupMode(QToolButton::InstantPopup);
    ui->toolButtonSaveAs->setPopupMode(QToolButton::InstantPopup);
    ui->toolButtonLoadFile->setMenu(fileOpenMenu);
    ui->toolButtonSaveAs->setMenu(fileSaveAsMenu);

    ui->textEditParameterHelp->document()->setDefaultStyleSheet(BgmnFileIO::readTextFile("://resources/contextHelp.css"));

    initConfigPages();
    initSettings();

    instrSavFile = QString();
    workingDir = QString();
}

EditInstrumentDialog::~EditInstrumentDialog()
{
    if (rayTracer) delete rayTracer;
    if (scene)     delete scene;
    if (ui)        delete ui;
}

void EditInstrumentDialog::showEvent(QShowEvent *e)
{
    e->accept();
}

void EditInstrumentDialog::closeEvent(QCloseEvent *e)
{
    saveSettings();
    e->accept();
}

void EditInstrumentDialog::initConfigPages()
{
    configPageMap.insert("BLANK",         new OpticsConfigPageBlank("BLANK"));
    configPageMap.insert("GONIO",         new OpticsConfigPageGonio("GONIO"));
    configPageMap.insert("FOCUS",         new OpticsConfigPageFocus("FOCUS"));
    configPageMap.insert("PCOLL",         new OpticsConfigPagePcol("PCOLL"));
    configPageMap.insert("SCOLL",         new OpticsConfigPageScol("SCOLL"));
    configPageMap.insert("SSLIT",         new OpticsConfigPageSslit("SSLIT"));
    configPageMap.insert("VSLIT",         new OpticsConfigPageVslit("VSLIT"));
    configPageMap.insert("HSLIT",         new OpticsConfigPageHslit("HSLIT"));
    configPageMap.insert("TSLIT",         new OpticsConfigPageTslit("TSLIT"));
    configPageMap.insert("RSLIT",         new OpticsConfigPageRslit("RSLIT"));
    configPageMap.insert("DSLIT",         new OpticsConfigPageDslit("DSLIT"));
    configPageMap.insert("AIRSCAT",       new OpticsConfigPageAirscat("AIRSCAT"));
    configPageMap.insert("SAMPLE",        new OpticsConfigPageSample("SAMPLE"));
    configPageMap.insert("DETECTOR",      new OpticsConfigPageDetector("DETECTOR"));
    configPageMap.insert("MONOCHROMATOR", new OpticsConfigPageMonochromator("MONOCHROMATOR"));

    QTreeWidgetItem *itmGonio =     new QTreeWidgetItem(QStringList("Goniometer"));
    QTreeWidgetItem *itmPrimBeam =  new QTreeWidgetItem(QStringList("Primary Beam"));
    QTreeWidgetItem *itmTube =      new QTreeWidgetItem(QStringList("X-ray tube"));
    QTreeWidgetItem *itmTSlit =     new QTreeWidgetItem(QStringList("Axial slit"));
    QTreeWidgetItem *itmHSlit =     new QTreeWidgetItem(QStringList("Divergence slit"));
    QTreeWidgetItem *itmPColl =     new QTreeWidgetItem(QStringList("Soller slit"));
    QTreeWidgetItem *itmVSlit =     new QTreeWidgetItem(QStringList("Axial beam mask"));
    QTreeWidgetItem *itmRSlit =     new QTreeWidgetItem(QStringList("Pinhole aperture"));

    QTreeWidgetItem *itmSampStage = new QTreeWidgetItem(QStringList("Sample Stage"));
    QTreeWidgetItem *itmSample =    new QTreeWidgetItem(QStringList("Sample"));
    QTreeWidgetItem *itmAirScat =   new QTreeWidgetItem(QStringList("Beam knife"));

    QTreeWidgetItem *itmSecBeam =   new QTreeWidgetItem(QStringList("Secondary Beam"));
    QTreeWidgetItem *itmSSlit =     new QTreeWidgetItem(QStringList("Anti-scatter slit"));
    QTreeWidgetItem *itmSColl =     new QTreeWidgetItem(QStringList("Soller slit"));
    QTreeWidgetItem *itmDSlit =     new QTreeWidgetItem(QStringList("Detector slit"));
    QTreeWidgetItem *itmMono =      new QTreeWidgetItem(QStringList("Monochromator"));
    QTreeWidgetItem *itmDet =       new QTreeWidgetItem(QStringList("Detector"));

    itmGonio->setData(    0, Qt::UserRole, "GONIO");
    itmPrimBeam->setData( 0, Qt::UserRole, "BLANK");
    itmSecBeam->setData(  0, Qt::UserRole, "BLANK");
    itmSampStage->setData(0, Qt::UserRole, "BLANK");
    itmTube->setData(     0, Qt::UserRole, "FOCUS");
    itmPColl->setData(    0, Qt::UserRole, "PCOLL");
    itmSColl->setData(    0, Qt::UserRole, "SCOLL");
    itmSSlit->setData(    0, Qt::UserRole, "SSLIT");
    itmVSlit->setData(    0, Qt::UserRole, "VSLIT");
    itmHSlit->setData(    0, Qt::UserRole, "HSLIT");
    itmTSlit->setData(    0, Qt::UserRole, "TSLIT");
    itmRSlit->setData(    0, Qt::UserRole, "RSLIT");
    itmAirScat->setData(  0, Qt::UserRole, "AIRSCAT");
    itmSample->setData(   0, Qt::UserRole, "SAMPLE");
    itmDSlit->setData(    0, Qt::UserRole, "DSLIT");
    itmMono->setData(     0, Qt::UserRole, "MONOCHROMATOR");
    itmDet->setData(      0, Qt::UserRole, "DETECTOR");

    itmGonio->setFlags(    Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    itmPrimBeam->setFlags( Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    itmSecBeam->setFlags(  Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    itmSampStage->setFlags(Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    itmTube->setFlags(     Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    itmPColl->setFlags(    Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    itmSColl->setFlags(    Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    itmSSlit->setFlags(    Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    itmVSlit->setFlags(    Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    itmHSlit->setFlags(    Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    itmTSlit->setFlags(    Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    itmRSlit->setFlags(    Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    itmAirScat->setFlags(  Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    itmSample->setFlags(   Qt::ItemIsSelectable | Qt::ItemIsEnabled);
    itmDSlit->setFlags(    Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    itmMono->setFlags(     Qt::ItemIsSelectable | Qt::ItemIsEnabled | Qt::ItemIsUserCheckable);
    itmDet->setFlags(      Qt::ItemIsSelectable | Qt::ItemIsEnabled);

    itmPColl->setCheckState(  0, Qt::Unchecked);
    itmSColl->setCheckState(  0, Qt::Unchecked);
    itmSSlit->setCheckState(  0, Qt::Unchecked);
    itmVSlit->setCheckState(  0, Qt::Unchecked);
    itmHSlit->setCheckState(  0, Qt::Unchecked);
    itmTSlit->setCheckState(  0, Qt::Unchecked);
    itmRSlit->setCheckState(  0, Qt::Unchecked);
    itmAirScat->setCheckState(0, Qt::Unchecked);
    itmDSlit->setCheckState(  0, Qt::Unchecked);
    itmMono->setCheckState(   0, Qt::Unchecked);

    treeWidgetMap.insert(itmGonio->data(0,   Qt::UserRole).toString(),   itmGonio);
    treeWidgetMap.insert(itmTube->data(0,    Qt::UserRole).toString(),    itmTube);
    treeWidgetMap.insert(itmPColl->data(0,   Qt::UserRole).toString(),   itmPColl);
    treeWidgetMap.insert(itmSColl->data(0,   Qt::UserRole).toString(),   itmSColl);
    treeWidgetMap.insert(itmSSlit->data(0,   Qt::UserRole).toString(),   itmSSlit);
    treeWidgetMap.insert(itmVSlit->data(0,   Qt::UserRole).toString(),   itmVSlit);
    treeWidgetMap.insert(itmHSlit->data(0,   Qt::UserRole).toString(),   itmHSlit);
    treeWidgetMap.insert(itmTSlit->data(0,   Qt::UserRole).toString(),   itmTSlit);
    treeWidgetMap.insert(itmRSlit->data(0,   Qt::UserRole).toString(),   itmRSlit);
    treeWidgetMap.insert(itmAirScat->data(0, Qt::UserRole).toString(), itmAirScat);
    treeWidgetMap.insert(itmSample->data(0,  Qt::UserRole).toString(),  itmSample);
    treeWidgetMap.insert(itmDSlit->data(0,   Qt::UserRole).toString(),   itmDSlit);
    treeWidgetMap.insert(itmMono->data(0,    Qt::UserRole).toString(),    itmMono);
    treeWidgetMap.insert(itmDet->data(0,     Qt::UserRole).toString(),     itmDet);

    itmPrimBeam->addChild(itmTube);
    itmPrimBeam->addChild(itmTSlit);
    itmPrimBeam->addChild(itmHSlit);
    itmPrimBeam->addChild(itmPColl);
    itmPrimBeam->addChild(itmVSlit);
    itmPrimBeam->addChild(itmRSlit);

    itmSampStage->addChild(itmSample);
    itmSampStage->addChild(itmAirScat);

    itmSecBeam->addChild(itmSSlit);
    itmSecBeam->addChild(itmSColl);
    itmSecBeam->addChild(itmDSlit);
    itmSecBeam->addChild(itmMono);
    itmSecBeam->addChild(itmDet);

    itmGonio->addChild(itmPrimBeam);
    itmGonio->addChild(itmSampStage);
    itmGonio->addChild(itmSecBeam);

    ui->treeWidget->addTopLevelItem(itmGonio);

    itmGonio->setExpanded(true);
    itmPrimBeam->setExpanded(true);
    itmSampStage->setExpanded(true);
    itmSecBeam->setExpanded(true);

    QMapIterator<QString, AbstractOpticsConfigPage*> it(configPageMap);

    while (it.hasNext()) {
        it.next();
        ui->stackedWidgetParameter->addWidget(it.value());
        configParamHelpText[it.value()->configTag()] = it.value()->helpText();
        connect(it.value(), SIGNAL(updateLayout(QString,QVariant)), scene, SLOT(updateSceneElement(QString,QVariant)));
    }

    if (configPageMap.contains("GONIO")) {
        ui->stackedWidgetParameter->setCurrentWidget(configPageMap.value("GONIO"));
        ui->textEditParameterHelp->setText(configParamHelpText.value("GONIO"));
    }
}

void EditInstrumentDialog::initSettings()
{
    QRect r = settings->value("instrumentDialog/geometry", QRect()).toRect();
    if (!r.isEmpty()) resize(r.width(), r.height());

    ui->splitter->setStretchFactor(0, 10);
    ui->splitter->setStretchFactor(1, 1);

    ui->splitterInstrument->setStretchFactor(0, 1);
    ui->splitterInstrument->setStretchFactor(1, 10);

    ui->splitterParameters->setStretchFactor(0, 10);
    ui->splitterParameters->setStretchFactor(1, 1);

    ui->splitterFileHelp->setStretchFactor(0, 10);
    ui->splitterFileHelp->setStretchFactor(1, 1);

    ui->splitter->restoreState(settings->value("instrumentDialog/splitter", QByteArray()).toByteArray());
    ui->splitterInstrument->restoreState(settings->value("instrumentDialog/splitterInstrument", QByteArray()).toByteArray());
    ui->splitterParameters->restoreState(settings->value("instrumentDialog/splitterParameters", QByteArray()).toByteArray());
    ui->splitterFileHelp->restoreState(settings->value("instrumentDialog/splitterHelp", QByteArray()).toByteArray());

    ui->progressBar->setEnabled(false);

    // set up text editors
    BgmnHighlighter *bhlSav = new BgmnHighlighter(settings->getSyntaxHighlightingMode(), this);
    bhlSav->setDocument(ui->textEditFile->document());

    QFont edFont;
    edFont.fromString(settings->value("config/editorFont", font().toString()).toString());
    ui->textEditFile->setFont(edFont);
    ui->plainTextEditFileContextHelp->setFont(edFont);
    ui->textEditOrigFile->setFont(edFont);
    ui->textEditOutput->setFont(edFont);

    ui->toolButtonToggleGraphicalEditor->setChecked(true);
    ui->stackedWidgetEditor->setCurrentIndex(0);
    ui->plainTextEditFileContextHelp->parseHelpFiles();
}

void EditInstrumentDialog::saveSettings()
{
    QMapIterator<QString, AbstractOpticsConfigPage *> iter(configPageMap);

    while (iter.hasNext()) {
        iter.next();
        iter.value()->saveSettings();
    }

    settings->setValue("instrumentDialog/geometry", geometry());
    settings->setValue("instrumentDialog/splitter", ui->splitter->saveState());
    settings->setValue("instrumentDialog/splitterInstrument", ui->splitterInstrument->saveState());
    settings->setValue("instrumentDialog/splitterParameters", ui->splitterParameters->saveState());
    settings->setValue("instrumentDialog/splitterHelp", ui->splitterFileHelp->saveState());
}

void EditInstrumentDialog::loadFile()
{
    QString dir;

    QAction *action = qobject_cast<QAction *>(sender());
    if (action) dir = action->data().toString();
    if (dir.isEmpty()) dir = workingDir;

    QString f = QFileDialog::getOpenFileName(this,
                                             tr("Load instrument configuration"),
                                             dir,
                                             tr("Instrument configuration (*.sav *.SAV)"));

    if (f.isEmpty()) return;

    QFileInfo fi(f);
    workingDir = fi.absolutePath();
    resetDialog();
    openInstrumentSavFile(f, QString());
    loadCurrentAction->setText(QString(tr("Current directory: %1")).arg(workingDir));
    saveCurrentAction->setText(QString(tr("Current directory: %1")).arg(workingDir));
}

void EditInstrumentDialog::setSavTemplateFile(const QString &s)
{
    QFileInfo isv(s);
    QString tpl = QDir::fromNativeSeparators(isv.absolutePath() + QDir::separator() + isv.completeBaseName());

    if (QFile::exists(tpl + ".tpl")) {
        templManager.loadTemplateFile(tpl + ".tpl");
    } else if (QFile::exists(tpl + ".TPL")) {
        templManager.loadTemplateFile(tpl + ".TPL");
    } else if (!projectSavFile.isEmpty()) {
        templManager.loadTemplateFile(projectSavFile);
    } else{
        templManager.generateDefaultTemplate();
    }

    updateTemplateGui();
}

void EditInstrumentDialog::textDocumentChanged()
{
}

void EditInstrumentDialog::setUiStateRunning(RunState r)
{
    bool b = r == RunState::running;
    ui->progressBar->setEnabled(b);
    ui->stackedWidgetEditor->setEnabled(!b);
    ui->topButtonBox->setEnabled(!b);
    ui->toolButtonLoadFile->setEnabled(!b);
    ui->pushButtonClose->setEnabled(!b);

    if (b) {
        ui->toolButtonRun->setIcon(QIcon::fromTheme("profex-run-abort"));
        ui->textEditOutput->clear();
        ui->progressBar->reset();
        ui->progressBar->setValue(0);
        ui->textEditOutput->appendPlainText(QString(tr("Computation started. Please be patient...")));
        qDebug() << QString("EditInstrumentDialog::setUiStateRunning(): Profile calculation started");
    } else {
        ui->toolButtonRun->setIcon(QIcon::fromTheme("profex-run"));
        ui->progressBar->reset();

        if (r == RunState::complete) {
            ui->textEditOutput->appendPlainText(QString(tr("Calculation completed")));
            qDebug() << QString("EditInstrumentDialog::setUiStateRunning(): Profile calculation completed");
        } else {
            ui->textEditOutput->appendPlainText(QString(tr("Calculation aborted")));
            qDebug() << QString("EditInstrumentDialog::setUiStateRunning(): Profile calculation was aborted");
        }
    }
}

void EditInstrumentDialog::toggleItemInstalled(QTreeWidgetItem *itm, int)
{
    QString tag = itm->data(0, Qt::UserRole).toString();
    scene->itemActiveStatusChanged(tag, itm->checkState(0) == Qt::Checked);

    if (configPageMap.contains(tag)) {
        configPageMap.value(tag)->setInstalled(itm->checkState(0) == Qt::Checked);
    }
}

void EditInstrumentDialog::toggleOrigFileEditor(bool b)
{
    if (!b) return;
    raiseOriginalFileEditor();
}

void EditInstrumentDialog::toggleControlFileEditor(bool b)
{
    if (!b) return;

    if (ui->stackedWidgetEditor->currentIndex() == 0) {
        ui->textEditFile->setPlainText(updateControlFile());
    }

    raiseControlFileEditor();
}

void EditInstrumentDialog::toggleGraphEditor(bool b)
{
    if (!b) return;
    bool doToggle = true;

    if (ui->stackedWidgetEditor->currentIndex() == 1) {
        QMap<QString, QString> parsingErrors = parseControlFile();

        if (!parsingErrors.isEmpty()) {
            EditInstrumentParsingErrorDialog *errDlg = new EditInstrumentParsingErrorDialog(1, this);
            errDlg->setErrorMap(parsingErrors);

            if (errDlg->exec() == QDialog::Rejected) {
                doToggle = false;
            }

            delete errDlg;
        }
    }

    if (doToggle) raiseGraphicalEditor();
    else          raiseControlFileEditor();
}

void EditInstrumentDialog::fileSave()
{
    save(instrSavFile);
    saveTemplate(instrSavFile);
}

void EditInstrumentDialog::openInstrumentSavFile(const QString &s, const QString &p)
{
    QFileInfo fis(s);
    QFileInfo fip(p);
    instrSavFile = fis.absoluteFilePath();
    workingDir = fis.absolutePath();
    projectSavFile = fip.absoluteFilePath();
    setSavTemplateFile(instrSavFile);

    origInstrSavFileContent = BgmnFileIO::readTextFile(instrSavFile);
    ui->textEditFile->setPlainText(origInstrSavFileContent);
    ui->textEditOrigFile->setPlainText(origInstrSavFileContent);

    ui->textEditFile->moveCursor(QTextCursor::Start);
    setWindowTitle(QString(tr("Instrument Configuration - %1")).arg(instrSavFile));
    loadCurrentAction->setText(QString(tr("Current directory: %1")).arg(fis.absolutePath()));
    saveCurrentAction->setText(QString(tr("Current directory: %1")).arg(fis.absolutePath()));

    QMap<QString, QString> parsingErrors = parseControlFile();
    bool raiseGraph = true;

    if (!parsingErrors.isEmpty()) {
        EditInstrumentParsingErrorDialog *errDlg = new EditInstrumentParsingErrorDialog(0, this);
        errDlg->setErrorMap(parsingErrors);
        errDlg->exec();
        raiseGraph = false;
        delete errDlg;
    }

    if (raiseGraph) raiseGraphicalEditor();
    else            raiseControlFileEditor();
}

void EditInstrumentDialog::fileReset()
{
    if (QMessageBox::question(this,
                              tr("Reset control file"),
                              tr("Reset the control file to the original version?\n"
                                 "All changes will be discarded."))
            == QMessageBox::No)
        return;

    ui->textEditFile->setPlainText(origInstrSavFileContent);
    raiseControlFileEditor();
}

void EditInstrumentDialog::runCalculation()
{
    if (rayTracer->isRunning()) {
        rayTracer->abortProcess();
        setUiStateRunning(RunState::aborted);
        return;
    }

    if (instrSavFile.isEmpty()) {
        fileSaveAs();
        if (instrSavFile.isEmpty()) return;
    }

    setUiStateRunning(RunState::running);

    if (ui->stackedWidgetEditor->currentIndex() == 0) {
        ui->textEditFile->setPlainText(updateControlFile());
    }

    if (!checkOutputNames()) fixOutputNames();

    save(instrSavFile);
    saveTemplate(instrSavFile);

    rayTracer->setConfigFile(ui->textEditFile->toPlainText(), instrSavFile);
    rayTracer->runProcess();
}

void EditInstrumentDialog::calculationComplete()
{
    setUiStateRunning(RunState::complete);
}

bool EditInstrumentDialog::checkOutputNames()
{
    BgmnInstrumentSavParser isParser(ui->textEditFile->toPlainText(), instrSavFile);
    return isParser.checkOutputNames(instrSavFile);
}

void EditInstrumentDialog::fixOutputNames()
{
    BgmnInstrumentSavParser isParser(ui->textEditFile->toPlainText(), instrSavFile);
    isParser.fixOutputNames(instrSavFile);
    ui->textEditFile->setPlainText(isParser.getText());
}

void EditInstrumentDialog::fileSaveAs()
{
    QString dir;

    QAction *action = qobject_cast<QAction *>(sender());
    if (action) dir = action->data().toString();
    if (dir.isEmpty()) dir = workingDir;

    QString f = QFileDialog::getSaveFileName(this,
                                             tr("Save File as"),
                                             dir,
                                             tr("Control File (*.SAV *.sav)"));

    if (f.isEmpty()) return;

    QFileInfo fi(f);
    workingDir = fi.absolutePath();

    instrSavFile = f;
    setWindowTitle(QString(tr("Instrument Configuration - %1")).arg(QDir::toNativeSeparators(instrSavFile)));
    save(instrSavFile);
    saveTemplate(instrSavFile);
    loadCurrentAction->setText(QString(tr("Current directory: %1")).arg(workingDir));
    saveCurrentAction->setText(QString(tr("Current directory: %1")).arg(workingDir));
}

void EditInstrumentDialog::save(const QString &s)
{
    if (instrSavFile.isEmpty()) {
        fileSaveAs();
    } else {
        if (ui->stackedWidgetEditor->currentIndex() == 0) {
            updateControlFile();
        }

        qDebug() << QString("EditInstrumentDialog::save(): Saving instrument file to %1").arg(s);
        BgmnFileIO::writeTextFile(s, ui->textEditFile->toPlainText());
    }
}

void EditInstrumentDialog::saveTemplate(const QString &s)
{
    // s and instrSavFile contain the file name of the instr control file, not of the template.
    // we use them to identify the correct source and destination paths for the template, background, and tubetail files
    QFileInfo fiDestDir(s);
    bool ok;

    QString polStr = getParameterFromPage("MONOCHROMATOR", "POL", ok);
    if (ok) templManager.setPolarization(true, polStr);
    else    templManager.setPolarization(false, QString());

    OpticsConfigPageFocus *tubePage = dynamic_cast<OpticsConfigPageFocus*>(configPageMap.value("FOCUS", nullptr));
    OpticsConfigPageGonio *gonioPage = dynamic_cast<OpticsConfigPageGonio*>(configPageMap.value("GONIO", nullptr));

    if (tubePage) {
        if (tubePage->hasLambda()) {
            templManager.setLambda(tubePage->getLambda());
        } else {
            templManager.setSynchrotron(tubePage->getSynchrotron());
        }
    }

    if (gonioPage) {
        QFileInfo fiBkgr(gonioPage->getUNT());

        // also send empty file name to remove the UNT line
        templManager.setBackground(fiBkgr.fileName());

        if (fiBkgr.absolutePath() != fiDestDir.absolutePath()) {
            BgmnFileIO::copyFile(fiBkgr.absoluteFilePath(), fiDestDir.absolutePath() + QDir::separator() + fiBkgr.fileName());
        }

        int ru = gonioPage->getRU();
        if (ru >= 0) templManager.setRU(ru);
    }

    /*
    QFileInfo fiSourceDir(instrSavFile);
    QFileInfo fiTubeTails(getParameterFromPage("FOCUS", "TubeTails", ok));

    if (ok) {
        if (fiTubeTails.isRelative()) {
            fiTubeTails = QFileInfo(fiSourceDir.absolutePath() + QDir::separator() + fiTubeTails.fileName());
        }

        if (fiTubeTails.exists()) {
            if (fiTubeTails.absolutePath() != fiDestDir.absolutePath()) {
                BgmnFileIO::copyFile(fiTubeTails.absoluteFilePath(), fiDestDir.absolutePath() + QDir::separator() + fiTubeTails.fileName());
            }
        }
    }
    */

    templManager.saveTemplate(QString("%1/%2.tpl").arg(fiDestDir.absolutePath()).arg(fiDestDir.completeBaseName()));
    qDebug() << QString("EditInstrumentDialog::saveTemplate(): Writing template to file %1").arg(templManager.fileName());
}

/*
 * the control file or the template control file is parsed to determine the wavelength / lambda file
 */
void EditInstrumentDialog::updateTemplateGui()
{
    OpticsConfigPageFocus *tubePage  = dynamic_cast<OpticsConfigPageFocus*>(configPageMap.value("FOCUS", nullptr));
    OpticsConfigPageGonio *gonioPage = dynamic_cast<OpticsConfigPageGonio*>(configPageMap.value("GONIO", nullptr));

    if (tubePage) {
        double syn = templManager.getSynchrotron();
        QString lam = templManager.getLambda();

        if (syn >= 0.0) tubePage->setSynchrotron(syn);
        else            tubePage->setLambda(lam);
    }

    if (gonioPage) {
        gonioPage->setUNT(templManager.getBackground());
        gonioPage->setRU(templManager.getRU());
    }
}

/*
 * updates the graphicsscene parameters based on the control file
 */
QMap<QString, QString> EditInstrumentDialog::parseControlFile()
{
    BgmnInstrumentSavParser isParser(ui->textEditFile->toPlainText(), instrSavFile);
    header = isParser.getHeader();

    QFileInfo fi(instrSavFile);
    QMap<QString, QString> params = isParser.getParameters();
    params.insert("VERZERR", QString("%1.ger").arg(fi.completeBaseName()));
    params.insert("GEQ",     QString("%1.geq").arg(fi.completeBaseName()));

    QMap<QString, QString> paramErrors;
    QMapIterator<QString, AbstractOpticsConfigPage*> itCPage(configPageMap);

    while (itCPage.hasNext()) {
        itCPage.next();
        paramErrors.insert(itCPage.value()->setParameters(params));
    }

    QMap<QString, bool> checkStateMap;

    // GONIO permanently installed
    // FOCUS permanently installed
    // SAMPLE permanently installed
    // DETECTOR permanently installed
    checkStateMap["PCOLL"] = params.contains("PColl");
    checkStateMap["SCOLL"] = params.contains("SColl");
    checkStateMap["SSLIT"] = params.contains("SSlitR");
    checkStateMap["VSLIT"] = params.contains("VSlitH");
    checkStateMap["HSLIT"] = params.contains("HSlitR");
    checkStateMap["TSLIT"] = params.contains("TSlitR");
    checkStateMap["RSLIT"] = params.contains("RoundSlitR");
    checkStateMap["AIRSCAT"] = params.contains("AirScat");
    checkStateMap["DSLIT"] = params.contains("DetW");
    checkStateMap["MONOCHROMATOR"] = params.contains("MonR");

    bool oldState = ui->treeWidget->blockSignals(true);

    QMapIterator<QString, bool> itCheckState(checkStateMap);

    while (itCheckState.hasNext()) {
        itCheckState.next();
        treeWidgetMap.value(itCheckState.key())->setCheckState(0, itCheckState.value() ? Qt::Checked : Qt::Unchecked);

        if (configPageMap.contains(itCheckState.key())) {
            configPageMap.value(itCheckState.key())->setInstalled(itCheckState.value());
        }

        scene->itemActiveStatusChanged(itCheckState.key(), itCheckState.value());
    }

    scene->updateSceneElement("SAMPLE", params.value("GEOMETRY"));
    ui->treeWidget->blockSignals(oldState);
    return paramErrors;
}

/*
 * updates the control file based on the graphicsScene
 */
QString EditInstrumentDialog::updateControlFile()
{
    QMap<QString, QString> params;
    QMapIterator<QString, AbstractOpticsConfigPage*> itCPage(configPageMap);

    while (itCPage.hasNext()) {
        itCPage.next();

        QMap<QString, QString> currentParams = itCPage.value()->getParameters();
        QMapIterator<QString, QString> itCParams(currentParams);

        while (itCParams.hasNext()) {
            itCParams.next();
            params.insert(itCParams.key(), itCParams.value());
        }
    }

    BgmnInstrumentSavParser isParser(params, header, instrSavFile);
    return isParser.getText();
}

QString EditInstrumentDialog::getParameterFromPage(const QString &page, const QString &param, bool &ok)
{
    if (configPageMap.contains(page)) {
        AbstractOpticsConfigPage *oPage = dynamic_cast<AbstractOpticsConfigPage*>(configPageMap.value(page));

        if (oPage) {
            QMap<QString, QString> p = oPage->getParameters();

            if (p.contains(param)) {
                ok = true;
                return p.value(param);
            }
        }
    }

    ok = false;
    return QString();
}

void EditInstrumentDialog::instrumentItemSelected(QString mod)
{
    QString tag = configPageMap.contains(mod) ? mod : "GONIO";

    bool oldState = ui->treeWidget->blockSignals(true);

    scene->setItemSelected(tag);
    ui->treeWidget->setCurrentItem(treeWidgetMap.value(tag));

    if (configPageMap.contains(tag)) {
        ui->stackedWidgetParameter->setCurrentWidget(configPageMap.value(tag));
    }

    ui->textEditParameterHelp->setText(configParamHelpText.value(tag));

    ui->treeWidget->blockSignals(oldState);
}

void EditInstrumentDialog::configItemChanged(QTreeWidgetItem *it, QTreeWidgetItem *)
{
    instrumentItemSelected(it->data(0, Qt::UserRole).toString());
}

void EditInstrumentDialog::resetDialog()
{
    instrSavFile = QString();
    origInstrSavFileContent = QString();
    templManager.loadTemplateFile(QString());
    header = QString();
    ui->textEditFile->clear();
    ui->textEditOrigFile->clear();
    ui->textEditOutput->clear();

    QMapIterator<QString, QTreeWidgetItem *> it(treeWidgetMap);

    while (it.hasNext()) {
        it.next();

        if (!it.value()->flags().testFlag(Qt::ItemIsUserCheckable)) {
            continue;
        }

        if (it.value()->checkState(0) == Qt::Checked) {
            it.value()->setCheckState(0, Qt::Unchecked);
        }

        toggleItemInstalled(it.value(), 0);
    }
}

void EditInstrumentDialog::blockToggleButtonSignals(bool b)
{
    ui->toolButtonToggleGraphicalEditor->blockSignals(b);
    ui->toolButtonToggleControlFileEditor->blockSignals(b);
    ui->toolButtonToggleOrigFileEditor->blockSignals(b);
    ui->toolButtonRevertFile->blockSignals(b);
    ui->stackedWidgetEditor->blockSignals(b);
}

void EditInstrumentDialog::raiseGraphicalEditor()
{
    blockToggleButtonSignals(true);
    ui->toolButtonToggleGraphicalEditor->setChecked(true);
    ui->toolButtonToggleControlFileEditor->setChecked(false);
    ui->toolButtonToggleOrigFileEditor->setChecked(false);
    ui->toolButtonRevertFile->setEnabled(false);

    ui->stackedWidgetEditor->setCurrentIndex(0);
    blockToggleButtonSignals(false);
}

void EditInstrumentDialog::raiseControlFileEditor()
{
    blockToggleButtonSignals(true);
    ui->toolButtonToggleGraphicalEditor->setChecked(false);
    ui->toolButtonToggleControlFileEditor->setChecked(true);
    ui->toolButtonToggleOrigFileEditor->setChecked(false);
    ui->toolButtonRevertFile->setEnabled(false);

    ui->stackedWidgetEditor->setCurrentIndex(1);
    blockToggleButtonSignals(false);
}

void EditInstrumentDialog::raiseOriginalFileEditor()
{
    blockToggleButtonSignals(true);
    ui->toolButtonToggleGraphicalEditor->setChecked(false);
    ui->toolButtonToggleControlFileEditor->setChecked(false);
    ui->toolButtonToggleOrigFileEditor->setChecked(true);
    ui->toolButtonRevertFile->setEnabled(true);

    ui->stackedWidgetEditor->setCurrentIndex(2);
    blockToggleButtonSignals(false);
}

void EditInstrumentDialog::wordUnderCursorChanged(QStringList list)
{
    if (list.size() > 1) {
        ui->plainTextEditFileContextHelp->setKeyword(list.at(0), list.at(1));
    }
}

void EditInstrumentDialog::showHelp()
{
    static QString s(tr("<h1>Edit Instrument Configuration</h1>"
                        "<p>This module allows to edit the fundamental parameters of instrument configurations, to start the computation "
                        "of peak profiles, and to create templates for refinement control files.</p>"
                        "<h2>Graphical Editor or Text Editor?</h2>"
                        "<p>The text file, in which the instrument configuration is described, is represented in a graphical editor. However, "
                        "since the text file supports scripting features which cannot be represented by the grapical editor, it is also possible "
                        "to edit the instrument control file directly in a text editor.</p>"
                        "<p>When the graphical editor detects a setting it cannot represent graphically, a warning is issued and the instrument "
                        "configuration is opened in the text editor by default. This protects the incompatible setting from being overwritten "
                        "by the graphical editor.</p>"
                        "<p>The user can toggle between the graphical editor and the text editor using the buttons in the top-right corner of this module.</p>"
                        "<h2>Getting Started</h2>"
                        "<h3>Editing the Configuration</h3>"
                        "<p>Select the modules which are installed on the instrument from the tree structure. To edit the module parameters, click on the "
                        "module in the schematic representation of the instrument and edit the parameter values in the \"Parameters\" page. For each module, "
                        "a description shown in the \"Description\" page provides additional information.</p>"
                        "<p>Clicking on the empty background in the schematic instrument representation raises the \"Goniometer\" configuration page.</p>"
                        "<h2>Saving the Configuration</h2>"
                        "<p>Using the \"Save As...\" button, the configuration can be saved under a new name. The button proposes several default "
                        "locations:"
                        "<ul><li>Current project directory: If the new configuration is saved here, it will be immediately available in the current "
                        "project. Set the value of VERZERR in the refinement control file to the new name once the profile computation is complete. "
                        "To make the new configuration available to all new projects, it must be copied from the current project directory to one of "
                        "the device repositories.</li>"
                        "<li>Device repositories: If the new configuration is saved here, it will be available for new projects immediately once the "
                        "profile computation is complete. "
                        "To use it in an existing project (including the current project), the instrument configuration files must be copied "
                        "manually to the project directory and the VERZERR variable in the refinement control file must be updated accordingly.</li></ul></p>"
                        "<h3>Control File Template</h3>"
                        "<p>The editor will create a Profex-specific template file for refinement projects (*.tpl). Select the correct radiation "
                        "settings for the instrument, and specify whether or not to use a constant number of background coefficients or a "
                        "measured background scan. The template file will be created automatically when the profile calculation is started.</p>"
                        "<h3>Profile Calculation</h3>"
                        "<p>Start the profile calculation by pressing the \"&gt;\" button in the botton-left corner of the dialog. "
                        "The computation may take several minutes to complete. After completion, all configuration files have been created and saved "
                        "and the dialog can be closed safely.</p>"
                     ));
    emit helpText(s);
}
