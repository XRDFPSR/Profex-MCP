/***************************************************************************
                          bgmnappenddialog.cpp  -  description
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

#include "bgmnaddremovedialog.h"
#include "gentemplatedialog.h"
#include "../libXrdIO/parser/bgmnsavparser.h"
#include "../libXrdIO/parser/bgmnstrparser.h"
#include "../libXrdIO/bgmnfileio.h"
#include "bgmnbackendconfig.h"

#include "ui_addremovephasedialog.h"

#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QMessageBox>
#include <QMap>
#include <QDirIterator>
#include <QTime>
#include <QList>
#include <QTimer>
#include <QPushButton>

BgmnAddRemoveDialog::BgmnAddRemoveDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddRemovePhaseDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    refStrManager = BgmnRefStructureManager::getInstance();

    expandState = true;
    wasShown = false;

    initGui();
    initSettings();
}

BgmnAddRemoveDialog::~BgmnAddRemoveDialog()
{
    delete ui;
}

void BgmnAddRemoveDialog::initGui()
{
    // these checkboxes are required for BGMN projects (but not for Fullprof projects).
    // make sure they are visible
    ui->checkBoxOverwrite->show();
    ui->checkBoxDeleteFiles->show();
    ui->toolButtonFavorites->show();
    ui->labelFilter->show();
    ui->lineEditFilter->show();
    ui->toolButtonClearFilter->show();
    ui->toolButtonFilterOptions->show();
    ui->labelFavWarning->clear();
    ui->labelMessage->clear();

    QStringList header;
    header << "File Name" << "Phase" << "Comment";
    ui->treeWidgetStructures->setColumnCount(header.size());
    ui->treeWidgetStructures->setHeaderLabels(header);
    ui->treeWidgetStructures->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    ui->treeWidgetRemovePhases->setHeaderLabel("File Name");

    filterOptionsMenu = new QMenu(this);

    actionFilterRegExp        = new QAction("Use regular expression", this);
    actionFilterCaseSensitive = new QAction("Case sensitive", this);
    actionFilterFile          = new QAction("Filter file name", this);
    actionFilterPhase         = new QAction("Filter phase name", this);
    actionFilterComment       = new QAction("Filter comment", this);

    actionFilterRegExp->setCheckable(true);
    actionFilterCaseSensitive->setCheckable(true);
    actionFilterFile->setCheckable(true);
    actionFilterPhase->setCheckable(true);
    actionFilterComment->setCheckable(true);

    connect(actionFilterRegExp, SIGNAL(toggled(bool)), this, SLOT(updateFilter()));
    connect(actionFilterCaseSensitive, SIGNAL(toggled(bool)), this, SLOT(updateFilter()));
    connect(actionFilterFile, SIGNAL(toggled(bool)), this, SLOT(updateFilter()));
    connect(actionFilterPhase, SIGNAL(toggled(bool)), this, SLOT(updateFilter()));
    connect(actionFilterComment, SIGNAL(toggled(bool)), this, SLOT(updateFilter()));

    filterOptionsMenu->addAction(actionFilterCaseSensitive);
    filterOptionsMenu->addAction(actionFilterRegExp);
    filterOptionsMenu->addAction(actionFilterFile);
    filterOptionsMenu->addAction(actionFilterPhase);
    filterOptionsMenu->addAction(actionFilterComment);

    ui->toolButtonFilterOptions->setMenu(filterOptionsMenu);

    ui->comboBoxDevices->lineEdit()->setPlaceholderText(QString("<select instrument>"));

    contextMenuHeader = new QMenu(this);

    for (int i = 1; i < ui->treeWidgetStructures->header()->count(); ++i) {
        QAction *act = new QAction(ui->treeWidgetStructures->headerItem()->text(i), contextMenuHeader);
        act->setCheckable(true);
        act->setData(i);
        contextMenuHeader->addAction(act);
    }

    connect(contextMenuHeader, SIGNAL(triggered(QAction*)), this, SLOT(toggleColumnHidden(QAction*)));
    connect(ui->treeWidgetStructures->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuRequested(QPoint)));
}

/*
 * reimplemented ::exec() slot, to register whether the dialog was shown or not.
 */
int BgmnAddRemoveDialog::exec()
{
    wasShown = true;

    ui->tabWidget->setCurrentIndex(0);
    ui->lineEditFilter->clear();

    applyFilter(QString());

    ui->checkBoxDeleteFiles->setChecked(false);
    ui->checkBoxOverwrite->setChecked(false);

    return QDialog::exec();
}

void BgmnAddRemoveDialog::accept()
{
    saveSettings();
    return QDialog::accept();
}

void BgmnAddRemoveDialog::customMenuRequested(QPoint p)
{
    contextMenuHeader->popup(ui->treeWidgetStructures->header()->viewport()->mapToGlobal(p));
}

void BgmnAddRemoveDialog::initSettings()
{
    devExt << "geq" << "GEQ";
    strExt << "str" << "STR";

    QRect geo = settings->value("bgmnProject/appendDialog/geometry", QVariant(QRect())).toRect();
    if (!geo.isEmpty()) resize(geo.width(), geo.height());

    sortCol = settings->value("bgmnProject/appendDialog/sortColumn", ui->treeWidgetStructures->sortColumn()).toInt();
    sortOrder = Qt::SortOrder(settings->value("bgmnProject/appendDialog/sortOrder", 0).toInt());
    ui->treeWidgetStructures->sortItems(sortCol, sortOrder);

    QByteArray colWidthAdd = settings->value("bgmnProject/appendDialog/addHeader", QByteArray()).toByteArray();
    QByteArray colWidthRem = settings->value("bgmnProject/appendDialog/removeHeader", QByteArray()).toByteArray();

    if (colWidthAdd.isEmpty()) {
        QFontMetrics fm(ui->treeWidgetStructures->font());
        int w = fm.size(Qt::TextSingleLine, "Hydroxylapatite.str").width();

        ui->treeWidgetStructures->header()->resizeSection(0, 3*w);
        ui->treeWidgetStructures->header()->resizeSection(1, 2*w);
        ui->treeWidgetStructures->header()->resizeSection(2, w);
    } else {
        ui->treeWidgetStructures->header()->restoreState(colWidthAdd);
    }

    if (colWidthRem.isEmpty()) {
        int w = ui->treeWidgetRemovePhases->viewport()->width();
        ui->treeWidgetRemovePhases->header()->resizeSection(0, w);
    } else {
        ui->treeWidgetRemovePhases->header()->restoreState(colWidthRem);
    }

    bool oldState = ui->toolButtonFavorites->blockSignals(true);
    bool favs = settings->value("bgmnProject/appendDialog/filterFavorites", false).toBool();
    ui->labelFavWarning->setText(favs ? QString(tr("Showing favorite phases only")) : QString());
    ui->toolButtonFavorites->setChecked(favs);
    ui->toolButtonFavorites->blockSignals(oldState);

    actionFilterCaseSensitive->setChecked(settings->value("bgmnProject/appendDialog/filterCaseSensitive", false).toBool());
    actionFilterRegExp->setChecked(settings->value("bgmnProject/appendDialog/filterRegExp", false).toBool());
    actionFilterFile->setChecked(settings->value("bgmnProject/appendDialog/filterFile", true).toBool());
    actionFilterPhase->setChecked(settings->value("bgmnProject/appendDialog/filterPhase", true).toBool());
    actionFilterComment->setChecked(settings->value("bgmnProject/appendDialog/filterComment", true).toBool());

    QList<QVariant> hiddenCols;
    hiddenCols.append(QVariant(2)); // hide the comment column by default
    hiddenCols = settings->value("bgmnProject/appendDialog/hiddenColumns", hiddenCols).toList();

    int n = qMin(ui->treeWidgetStructures->header()->count(), contextMenuHeader->actions().count() + 1);

    for (int i = 1; i < n; ++i) {
        bool b = hiddenCols.contains(QVariant(i));
        ui->treeWidgetStructures->setColumnHidden(i, b);
        contextMenuHeader->actions().at(i - 1)->setChecked(!b);
    }
}

/*
 * saves the current widget state to the settings->
 */
void BgmnAddRemoveDialog::saveSettings()
{
    // if the dialog was never shown, we will not write the settings to disk
    if (!wasShown) return;

    settings->setValue("bgmnProject/appendDialog/geometry", geometry());
    settings->setValue("bgmnProject/appendDialog/sortColumn", ui->treeWidgetStructures->sortColumn());
    settings->setValue("bgmnProject/appendDialog/sortOrder", int(ui->treeWidgetStructures->header()->sortIndicatorOrder()));
    settings->setValue("bgmnProject/defaultInstrument", ui->comboBoxDevices->currentText());
    settings->setValue("bgmnProject/appendDialog/filterFavorites", ui->toolButtonFavorites->isChecked());

    settings->setValue("bgmnProject/appendDialog/filterCaseSensitive", actionFilterCaseSensitive->isChecked());
    settings->setValue("bgmnProject/appendDialog/filterRegExp", actionFilterRegExp->isChecked());
    settings->setValue("bgmnProject/appendDialog/filterFile", actionFilterFile->isChecked());
    settings->setValue("bgmnProject/appendDialog/filterPhase", actionFilterPhase->isChecked());
    settings->setValue("bgmnProject/appendDialog/filterComment", actionFilterComment->isChecked());

    settings->setValue("bgmnProject/appendDialog/addHeader", ui->treeWidgetStructures->header()->saveState());
    settings->setValue("bgmnProject/appendDialog/removeHeader", ui->treeWidgetRemovePhases->header()->saveState());

    QList<QVariant> hiddenCols;
    for (int i = 1; i < ui->treeWidgetStructures->header()->count(); ++i) {
        if (ui->treeWidgetStructures->isColumnHidden(i)) hiddenCols.append(QVariant(i));
    }

    settings->setValue("bgmnProject/appendDialog/hiddenColumns", hiddenCols);
}

/*
 * Updates the dialog according to the project status
 */
void BgmnAddRemoveDialog::updateDialog(const QString &device, const QStringList &preselectStr, const QStringList &projStr)
{
    readDirectories();
    setSelectedPhases(preselectStr);
    setDeleteStrFiles(projStr);

    if (!device.isEmpty()) {
        int n = ui->comboBoxDevices->findText(device);
        ui->comboBoxDevices->setCurrentIndex(n < 0 ? 0 : n);
    }

    ui->checkBoxGenerateControlFile->setChecked(device.isEmpty());
}

/*
 * Scans the directories for str and device files.
 *
 * This function must be called by the parent. Scanning large numbers of files stored on a network
 * share can take quite a while. Calling this function from the parent object allows to perform the
 * scan when the delay doesn't feel too inappropriate.
 */
void BgmnAddRemoveDialog::readDirectories()
{
    // clear old entries in the tree and list widgets
    ui->treeWidgetStructures->clear();
    ui->treeWidgetRemovePhases->clear();

    // get the current sorting behaviour
    sortCol = ui->treeWidgetStructures->sortColumn();
    sortOrder = ui->treeWidgetStructures->header()->sortIndicatorOrder();

    // re-populate the tree and list widgets
    setStrDir();
    setDevDir();
}

/*
 * returns true if a default control file shall be created, else returns false
 */
bool BgmnAddRemoveDialog::createDefaultControlFile()
{
    return ui->checkBoxGenerateControlFile->isChecked();
}

/*
 * returns true if existing str files shall be overwritten, else returns false
 */
bool BgmnAddRemoveDialog::overwriteFiles()
{
    return ui->checkBoxOverwrite->isChecked();
}

/*
 * returns the selected device file
 */
QString BgmnAddRemoveDialog::deviceFile()
{
    return ui->comboBoxDevices->currentData(Qt::UserRole).toString();
}

QStringList BgmnAddRemoveDialog::getCheckedAddFiles()
{
    QList<QTreeWidgetItem *> items = allStructureItems();
    QStringList files;

    for (int i = 0; i < items.size(); ++i) {
        if (items.at(i)->checkState(0) == Qt::Checked) {
            QString fn(items.at(i)->data(0, Qt::UserRole).toString());

            if (!fn.isEmpty()) {
                files.append(fn);
            }
        }
    }

    return files;
}

/*
 * reads all structure files from refStrManager and populates the listWidget
 */
void BgmnAddRemoveDialog::setStrDir()
{
    ui->treeWidgetStructures->clear();

    bool favs = ui->toolButtonFavorites->isChecked();

    if (favs) {
        if (!refStrManager->countFavPhases()) {
            favs = false;
            bool oldState = ui->toolButtonFavorites->blockSignals(true);
            ui->toolButtonFavorites->setChecked(false);
            ui->toolButtonFavorites->blockSignals(oldState);
            ui->labelFavWarning->setText(tr("No favorites set"));
            QTimer::singleShot(5000, this, SLOT(clearFavsLabel()));
        } else {
            ui->labelFavWarning->setText(tr("Showing favorite phases only"));
        }
    } else {
        ui->labelFavWarning->clear();
    }

    QStringList repos = settings->value("bgmnProject/structureDatabaseIndexed", QStringList()).toStringList();
    repos.append(settings->value("bgmnProject/structureDatabaseNonIndexed", QStringList()).toStringList());

    refStrManager->createTree(ui->treeWidgetStructures, repos, favs, Qt::NoItemFlags | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);

    // sort the items
    ui->treeWidgetStructures->sortItems(sortCol, sortOrder);

    // fit the columns to the contents if the size is 0
    for (int i = 0; i < ui->treeWidgetStructures->columnCount(); ++i) {
        if (ui->treeWidgetStructures->columnWidth(i) == 0) {
            ui->treeWidgetStructures->resizeColumnToContents(i);
        }
    }
}

/*
 * scan the device directory for geq files
 */
void BgmnAddRemoveDialog::setDevDir()
{
    ui->comboBoxDevices->clear();
    ui->comboBoxDevices->addItem(QString(), QString());

    BgmnBackendConfig bkgCfg;
    QMultiMap<QString,QString> fileMap = bkgCfg.getAllDevFiles();

    // iterate over the dev file map
    QMultiMap<QString,QString>::const_iterator i = fileMap.constBegin();

    while (i != fileMap.constEnd()) {
        ui->comboBoxDevices->addItem(i.key(), i.value());
        ++i;
    }

    int n = ui->comboBoxDevices->findText(settings->value("bgmnProject/defaultInstrument", QString()).toString());
    ui->comboBoxDevices->setCurrentIndex(n < 0 ? 0 : n);
}

void BgmnAddRemoveDialog::setDeviceFile(bool enable, const QString &s)
{
    QFileInfo fi(s);
    int n = ui->comboBoxDevices->findText(fi.baseName());
    ui->comboBoxDevices->setCurrentIndex(n < 0 ? 0 : n);

    ui->checkBoxGenerateControlFile->setChecked(enable);
    ui->comboBoxDevices->setEnabled(enable);
}

/*
 * expands or collapses the tree widget items
 */
void BgmnAddRemoveDialog::toggleExpand()
{
    expandState = !expandState;

    QTreeWidgetItemIterator it(ui->treeWidgetStructures);

    while (*it) {
        if ((*it)->type() == TYPE_DIRECTORY) {
            (*it)->setExpanded(expandState);
        }
        ++it;
    }

    if (ui->treeWidgetStructures->topLevelItemCount() == 1) {
        ui->treeWidgetStructures->topLevelItem(0)->setExpanded(true);
    }
}

/*
 * pre-selects some phases, e.g. because they are activated in the
 * reference structure combo box
 */
void BgmnAddRemoveDialog::setSelectedPhases(const QStringList &l)
{
    if (!l.size()) {
        return;
    }

    qDebug() << QString("BgmnAddRemoveDialog::setSelectedPhases(): Pre-selecting phases: %1").arg(l.join(", "));

    QTreeWidgetItem *scrollTo = nullptr;
    QList<QTreeWidgetItem*> expandItems;

    QTreeWidgetItemIterator itCheck(ui->treeWidgetStructures);

    while (*itCheck) {
        if ((*itCheck)->type() == TYPE_DIRECTORY) (*itCheck)->setExpanded(false);

        if (l.contains((*itCheck)->data(0, Qt::UserRole).toString())) {
            (*itCheck)->setCheckState(0, Qt::Checked);
            if ((*itCheck)->parent()) expandItems.append((*itCheck)->parent());
            if (!scrollTo) scrollTo = *itCheck;
        }

        ++itCheck;
    }

    for (int i = 0; i < expandItems.size(); ++i) {
        QTreeWidgetItem *it = expandItems.at(i);
        while (it) {
            it->setExpanded(true);
            it = it->parent();
        }
    }

    if (scrollTo) {
        ui->treeWidgetStructures->scrollToItem(scrollTo, QAbstractItemView::PositionAtCenter);
    }
}

/*
 * populates the treewidget with str files found in the control file.
 * these are the files that can be removed from the refinement.
 */
void BgmnAddRemoveDialog::setDeleteStrFiles(const QStringList &lst)
{
    ui->treeWidgetRemovePhases->clear();

    for (int i = 0; i < lst.size(); ++i) {
        QFileInfo fi(lst.at(i));

        // create the phase item
        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetRemovePhases, TYPE_FILE);
        it->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        it->setCheckState(0, Qt::Unchecked);
        it->setData(0, Qt::UserRole, fi.absoluteFilePath());
        it->setText(0, fi.fileName());
    }
}

QStringList BgmnAddRemoveDialog::getCheckedDeleteFiles()
{
    QStringList lst;
    QTreeWidgetItemIterator it(ui->treeWidgetRemovePhases);

    while (*it) {
        if ((*it)->checkState(0) == Qt::Checked) {
            lst.append((*it)->text(0));
        }
        ++it;
    }

    return lst;
}

bool BgmnAddRemoveDialog::deleteStrFiles()
{
    return ui->checkBoxDeleteFiles->isChecked();
}

void BgmnAddRemoveDialog::toggleFavorites(bool)
{
    qApp->setOverrideCursor(Qt::WaitCursor);

    setStrDir();
    updateFilter();

    qApp->restoreOverrideCursor();
}

/*
 * returns a list of pointers to all file items in the structure treewidget.
 */
QList<QTreeWidgetItem *> BgmnAddRemoveDialog::allStructureItems()
{
    QList<QTreeWidgetItem *> items;
    QTreeWidgetItemIterator it(ui->treeWidgetStructures);

    while (*it) {
        if ((*it)->type() == TYPE_FILE) {
            items.append(*it);
        }
        ++it;
    }

    return items;
}

/*
 * shows / hides items according to the filter string. does not add / remove any items
 */
void BgmnAddRemoveDialog::applyFilter(QString s)
{
    if (s.simplified().isEmpty()) {
        unhideAllItems();
        return;
    }

    QRegularExpression rx;
    QRegularExpressionMatch rm;

    QRegularExpression::PatternOptions options = rx.patternOptions();

    if (!actionFilterCaseSensitive->isChecked()) {
        rx.setPatternOptions(options | QRegularExpression::CaseInsensitiveOption);
    }

    if (actionFilterRegExp->isChecked()) {
        // if regexp option is checked, we will interpret the string exactly as entered
        rx.setPattern(s);
    } else {
        // if the filter string doesn't contain wildcards, we wrap it in
        // wildcards, so the search string is also matched in the middle
        // of words. This feels more natural from the user perspective.
        QString filter = s.simplified();

        if (!filter.contains(QString("*"))) {
            filter = QString("*%1*").arg(filter);
        }

        filter.replace(QString("*"), QString(".*"));
        filter.replace(QString("?"), QString("."));
        rx.setPattern(QString("^%1$").arg(filter));
    }

    if (!rx.isValid()) {
        qDebug() << QString("BgmnAddRemoveDialog::applyFilter: Invalid regular expression (%1)").arg(rx.errorString());
        return;
    }

    QString txt;

    QList<QTreeWidgetItem *> items = allStructureItems();

    for (int i = 0; i < items.size(); ++i) {
        int matches = 0;

        if (actionFilterFile->isChecked()) {
            txt = itemText(items.at(i), 0);
            rm = rx.match(txt);
            if (rm.hasMatch()) matches++;
        }

        if (actionFilterPhase->isChecked()) {
            txt = itemText(items.at(i), 1);
            rm = rx.match(txt);
            if (rm.hasMatch()) matches++;
        }

        if (actionFilterComment->isChecked()) {
            txt = itemText(items.at(i), 2);
            rm = rx.match(txt);
            if (rm.hasMatch()) matches++;
        }

        items.at(i)->setHidden(matches == 0 ? true : false);
    }
}

void BgmnAddRemoveDialog::updateFilter()
{
    applyFilter(ui->lineEditFilter->text());
}

void BgmnAddRemoveDialog::unhideAllItems()
{
    QList<QTreeWidgetItem *> items = allStructureItems();

    for (int i = 0; i < items.size(); ++i) {
        if (items.at(i)->isHidden()) items.at(i)->setHidden(false);
    }
}

/*
 * returns the column text in column n of item it. If it.columnCount() is
 * < n, returns an empty string. not sure if QTreeWidgetItem::text(int) does the
 * same, but better safe than sorry.
 */
QString BgmnAddRemoveDialog::itemText(const QTreeWidgetItem *it, int n)
{
    int c = it->columnCount();
    if ((n >= c) || (n < 0)) return QString();
    return it->text(n);
}

void BgmnAddRemoveDialog::toggleColumnHidden(QAction *a)
{
    int c = a->data().toInt();
    ui->treeWidgetStructures->setColumnHidden(c, !a->isChecked());
}

void BgmnAddRemoveDialog::clearFavsLabel()
{
    ui->labelFavWarning->clear();
}

void BgmnAddRemoveDialog::generateDefaultControlFileToggled(bool b)
{
    ui->comboBoxDevices->setEnabled(b);
    instrumentConfigChanged(ui->comboBoxDevices->currentIndex());
}

void BgmnAddRemoveDialog::instrumentConfigChanged(int n)
{
    if (ui->checkBoxGenerateControlFile->isChecked()) {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(n > 0);

        if (n == 0) ui->labelMessage->setText(tr("Select an instrument configuration"));
        else        ui->labelMessage->clear();
    } else {
        ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
        ui->labelMessage->clear();
    }
}
