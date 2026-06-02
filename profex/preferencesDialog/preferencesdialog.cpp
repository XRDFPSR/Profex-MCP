/***************************************************************************
                          preferencesdialog.cpp  -  description
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

#include "preferencesdialog.h"
#include "../libXrdIO/structs.h"
#include "ui_preferencesdialog.h"

#include "prefpagegeneral.h"
#include "prefpagetexteditor.h"
#include "prefpagegraphs.h"
#include "prefpagegraphaxes.h"
#include "prefpagegraphfonts.h"
#include "prefpagegraphcursors.h"
#include "prefpagescanstyle.h"
#include "prefpagegraphprinting.h"
#include "prefpagebgmnconfig.h"
#include "prefpagebgmndirectories.h"
#include "prefpageeflechconfig.h"
#include "prefpagelimits.h"
#include "prefpagesummarytables.h"
#include "prefpagefullprofconfig.h"
#include "prefpagereferencelines.h"
#include "prefpagechemicalcomp.h"
#include "prefpagetextblocks.h"
#include "prefpagesearchmatch.h"
#include "prefpagebgmnreport.h"
#include "prefpagecod.h"

PreferencesDialog::PreferencesDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PreferencesDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    doProfile = true;

    setupPageList();
    initSettings();

    ui->treeWidgetPages->setCurrentItem(ui->treeWidgetPages->topLevelItem(0));
    ui->stackedWidgetPages->setCurrentIndex(0);
}

PreferencesDialog::~PreferencesDialog()
{
    delete ui;
}

void PreferencesDialog::setupPageList()
{
    int minWidth = 0;
    int minHeight = 0;

    if (doProfile) {
        qDebug() << QString("PreferencesDialog::setupPageList(): Profiling");
        qDebug() << QString("   creating the pages");
        qDebug() << QString("   page                    ms");
        qDebug() << QString("   -------------------------");
    }

    QElapsedTimer pageCreationTimer;
    QElapsedTimer pageSetupTimer;
    QElapsedTimer pageTotalTimer;

    if (doProfile) {
        pageCreationTimer.start();
        pageTotalTimer.start();
        pageProfileTimer.start();
    }

    ui->stackedWidgetPages->addWidget(new PrefPageGeneral(this));
    QTreeWidgetItem *itemGeneral = new QTreeWidgetItem(ui->treeWidgetPages, QStringList("General"));
    itemGeneral->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    ui->treeWidgetPages->addTopLevelItem(itemGeneral);
    if (doProfile) qDebug() << QString("   01. General               %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageTextEditor(this));
    QTreeWidgetItem *itemEditor = new QTreeWidgetItem(ui->treeWidgetPages, QStringList("Text Editors"));
    itemEditor->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    ui->treeWidgetPages->addTopLevelItem(itemEditor);
    if (doProfile) qDebug() << QString("   02. Text Editors          %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new QWidget(this));
    QTreeWidgetItem *itemGraphs = new QTreeWidgetItem(ui->treeWidgetPages, QStringList("Graphs"));
    itemGraphs->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    ui->treeWidgetPages->addTopLevelItem(itemGraphs);
    if (doProfile) qDebug() << QString("   03. Graphs                %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageGraphs(this));
    QTreeWidgetItem *itemGraphsAppearance = new QTreeWidgetItem(itemGraphs, QStringList("Appearance"));
    itemGraphsAppearance->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemGraphs->addChild(itemGraphsAppearance);
    if (doProfile) qDebug() << QString("   04. Appearance            %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageGraphAxes(this));
    QTreeWidgetItem *itemGraphsAxes = new QTreeWidgetItem(itemGraphs, QStringList("Axes and Lines"));
    itemGraphsAxes->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemGraphs->addChild(itemGraphsAxes);
    if (doProfile) qDebug() << QString("   05. Axes and Lines        %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageGraphFonts(this));
    QTreeWidgetItem *itemGraphsFonts = new QTreeWidgetItem(itemGraphs, QStringList("Fonts"));
    itemGraphsFonts->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemGraphs->addChild(itemGraphsFonts);
    if (doProfile) qDebug() << QString("   06. Fonts                 %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageGraphCursors(this));
    QTreeWidgetItem *itemGraphsCursors = new QTreeWidgetItem(itemGraphs, QStringList("Cursors"));
    itemGraphsCursors->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemGraphs->addChild(itemGraphsCursors);
    if (doProfile) qDebug() << QString("   07. Cursors               %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageScanStyle(this));
    QTreeWidgetItem *itemGraphsStyles = new QTreeWidgetItem(itemGraphs, QStringList("Scan Styles"));
    itemGraphsStyles->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemGraphs->addChild(itemGraphsStyles);
    if (doProfile) qDebug() << QString("   08. Scan Styles           %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageGraphPrinting(this));
    QTreeWidgetItem *itemGraphsPrinting = new QTreeWidgetItem(itemGraphs, QStringList("Print and Export"));
    itemGraphsPrinting->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemGraphs->addChild(itemGraphsPrinting);
    if (doProfile) qDebug() << QString("   09. Print and Export      %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new QWidget(this));
    QTreeWidgetItem *itemBgmn = new QTreeWidgetItem(ui->treeWidgetPages, QStringList("BGMN"));
    itemBgmn->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    ui->treeWidgetPages->addTopLevelItem(itemBgmn);
    if (doProfile) qDebug() << QString("   10. BGMN                  %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageBgmnConfig(this));
    QTreeWidgetItem *itemBgmnBackendConfig = new QTreeWidgetItem(itemBgmn, QStringList("Backend Configuration"));
    itemBgmnBackendConfig->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemBgmn->addChild(itemBgmnBackendConfig);
    if (doProfile) qDebug() << QString("   11. Backend Configuration %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageBgmnDirectories(this));
    QTreeWidgetItem *itemBgmnDir = new QTreeWidgetItem(itemBgmn, QStringList("Repositories"));
    itemBgmnDir->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemBgmn->addChild(itemBgmnDir);
    if (doProfile) qDebug() << QString("   12. Repositories          %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageEflechConfig(this));
    QTreeWidgetItem *itemEflechConfig = new QTreeWidgetItem(itemBgmn, QStringList("Peak Detection"));
    itemEflechConfig->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemBgmn->addChild(itemEflechConfig);
    if (doProfile) qDebug() << QString("   13. Peak Detection        %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageSearchMatch(this));
    QTreeWidgetItem *itemBgmnSearchMatch = new QTreeWidgetItem(itemBgmn, QStringList("Search-Match"));
    itemBgmnSearchMatch->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemBgmn->addChild(itemBgmnSearchMatch);
    if (doProfile) qDebug() << QString("   14. Search-Match          %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageReferenceLines(this));
    QTreeWidgetItem *itemRefStr = new QTreeWidgetItem(itemBgmn, QStringList("Reference Structures"));
    itemRefStr->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemBgmn->addChild(itemRefStr);
    if (doProfile) qDebug() << QString("   15. Reference Structures  %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageLimits(this));
    QTreeWidgetItem *itemBgmnCif = new QTreeWidgetItem(itemBgmn, QStringList("Refinement Limits"));
    itemBgmnCif->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemBgmn->addChild(itemBgmnCif);
    if (doProfile) qDebug() << QString("   17. Refinement Limits     %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageSummaryTables(this));
    QTreeWidgetItem *itemBgmnTab = new QTreeWidgetItem(itemBgmn, QStringList("Summary Tables"));
    itemBgmnTab->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemBgmn->addChild(itemBgmnTab);
    if (doProfile) qDebug() << QString("   18. Summary Tables        %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageBgmnReport(this));
    QTreeWidgetItem *itemBgmnBgmnReport = new QTreeWidgetItem(itemBgmn, QStringList("Refinement Report"));
    itemBgmnBgmnReport->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    itemBgmn->addChild(itemBgmnBgmnReport);
    if (doProfile) qDebug() << QString("   19. Refinement Report     %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageFullprofConfig(this));
    QTreeWidgetItem *itemFullprof = new QTreeWidgetItem(ui->treeWidgetPages, QStringList("Fullprof.2k"));
    itemFullprof->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    ui->treeWidgetPages->addTopLevelItem(itemFullprof);
    if (doProfile) qDebug() << QString("   20. Fullprof.2k           %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageChemicalComp(this));
    QTreeWidgetItem *itemChem = new QTreeWidgetItem(ui->treeWidgetPages, QStringList("Chemical Composition"));
    itemChem->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    ui->treeWidgetPages->addTopLevelItem(itemChem);
    if (doProfile) qDebug() << QString("   21. Chemical Composition  %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageTextBlocks(this));
    QTreeWidgetItem *itemTextBlocks = new QTreeWidgetItem(ui->treeWidgetPages, QStringList("Text Blocks"));
    itemTextBlocks->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    ui->treeWidgetPages->addTopLevelItem(itemTextBlocks);
    if (doProfile) qDebug() << QString("   22. Text Blocks           %1").arg(pageProfileTimer.elapsed());

    pageProfileTimer.restart();
    ui->stackedWidgetPages->addWidget(new PrefPageCod(this));
    QTreeWidgetItem *itemCod = new QTreeWidgetItem(ui->treeWidgetPages, QStringList("COD Database"));
    itemCod->setData(0, Qt::UserRole, ui->stackedWidgetPages->count() - 1);
    ui->treeWidgetPages->addTopLevelItem(itemCod);
    if (doProfile) qDebug() << QString("   23. COD Database          %1").arg(pageProfileTimer.elapsed());

    for (int i = 0; i < ui->treeWidgetPages->topLevelItemCount(); ++i) {
        ui->treeWidgetPages->topLevelItem(i)->setExpanded(true);
    }

    if (doProfile) {
        qDebug() << QString("----------------------------");
        qDebug() << QString("   total creation time: %1").arg(pageCreationTimer.elapsed());
        qDebug() << QString("============================");
        qDebug() << QString("   setting up the pages");
        qDebug() << QString("   page  ms");
        qDebug() << QString("   -------------------------");
        pageSetupTimer.start();
    }

    for (int i = 0; i < ui->stackedWidgetPages->count(); ++i) {
        pageProfileTimer.restart();
        PrefPageTemplate* page = dynamic_cast<PrefPageTemplate*>(ui->stackedWidgetPages->widget(i));
        if (page) page->initUi();

        minWidth  = qMax(minWidth,  ui->stackedWidgetPages->widget(i)->minimumWidth());
        minHeight = qMax(minHeight, ui->stackedWidgetPages->widget(i)->minimumHeight());

        if (doProfile) {
            qDebug() << QString("   %1  %2").arg(i+1).arg(pageProfileTimer.elapsed());
        }
    }

    if (doProfile) {
        qDebug() << QString("----------------------------");
        qDebug() << QString("   total setup time: %1").arg(pageSetupTimer.elapsed());
        qDebug() << QString("============================");
        qDebug() << QString("   total overall time: %1").arg(pageTotalTimer.elapsed());
        qDebug() << QString("============================");
    }

    connect(ui->treeWidgetPages, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)),
            this, SLOT(pageChanged(QTreeWidgetItem*,QTreeWidgetItem*)));

    setMinimumWidth(minWidth + ui->treeWidgetPages->minimumWidth());
    setMinimumHeight(minHeight + ui->treeWidgetPages->minimumHeight());
}

void PreferencesDialog::raisePage(const QString &s)
{
    QList<QTreeWidgetItem*> lst = ui->treeWidgetPages->findItems(s, Qt::MatchFixedString | Qt::MatchRecursive, 0);

    if (lst.size()) {
        int i = lst.first()->data(0, Qt::UserRole).toInt();
        ui->treeWidgetPages->setCurrentItem(lst.first());
        ui->stackedWidgetPages->setCurrentIndex(i);
    }
}

void PreferencesDialog::accept()
{
    saveSettings();
    savePageSettings();
    QDialog::accept();
}

void PreferencesDialog::initSettings()
{
    ui->splitterV->restoreState(settings->value("preferences/splitterV", QByteArray()).toByteArray());
    QRect rec = settings->value("preferences/geometry", QRect()).toRect();
    if (!rec.isNull()) resize(rec.width(), rec.height());
}

void PreferencesDialog::saveSettings()
{
    settings->setValue("preferences/splitterV", ui->splitterV->saveState());
    settings->setValue("preferences/geometry", geometry());
}

void PreferencesDialog::changeEvent(QEvent *e)
{
    QDialog::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void PreferencesDialog::pageChanged(QTreeWidgetItem *it, int)
{
    ui->stackedWidgetPages->setCurrentIndex(it->data(0, Qt::UserRole).toInt());
}

void PreferencesDialog::pageChanged(QTreeWidgetItem *it, QTreeWidgetItem *)
{
    ui->stackedWidgetPages->setCurrentIndex(it->data(0, Qt::UserRole).toInt());
}

void PreferencesDialog::savePageSettings()
{
    for (int i = 0; i < ui->stackedWidgetPages->count(); ++i) {
        if (PrefPageTemplate *page = dynamic_cast<PrefPageTemplate*>(ui->stackedWidgetPages->widget(i))) {
            page->saveSettings();
        }
    }
}

void PreferencesDialog::applyPreferences()
{
    savePageSettings();
    emit preferencesApplied();
}

void PreferencesDialog::updatePreferences()
{
    for (int i = 0; i < ui->stackedWidgetPages->count(); ++i) {
        if (PrefPageTemplate *page = dynamic_cast<PrefPageTemplate*>(ui->stackedWidgetPages->widget(i))) {
            page->initSettings();
        }
    }
}
