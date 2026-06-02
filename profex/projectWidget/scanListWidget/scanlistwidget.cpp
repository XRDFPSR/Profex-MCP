/***************************************************************************
                          scanlistwidget.cpp  -  description
                             -------------------
    begin                : Thu May 12 15:27:00 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#include "scanlistwidget.h"
#include "scanlistwidgetdelegate.h"
#include "../graphWidget/graphdatacontroller.h"
#include "../libXrdIO/export/asciihklexport.h"
#include "../libXrdIO/export/exporthandler.h"
#include "../libXrdIO/functions.h"
#include "../libXrdIO/scanops.h"
#include <QString>
#include <QStringList>
#include <QHeaderView>
#include <QClipboard>
#include <QGuiApplication>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QObject>
#include <QKeyEvent>
#include <QDebug>
#include <QMenu>
#include <QColorDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QShortcut>

ScanListWidget::ScanListWidget(GraphDataController *c, QWidget *parent)
    : AbstractGraphView(c, parent)
{
    vModes.insert(global::ViewUpdateMode::DISPLAY);

    double limMin = std::numeric_limits<double>::lowest();
    double limMax = std::numeric_limits<double>::max();
    treeWidget = new QTreeWidget(this);
    treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    treeWidget->setSelectionMode(QAbstractItemView::ExtendedSelection);
    treeWidget->setItemDelegateForColumn(2, new ScanListWidgetDelegate(2, -999.9, 999.9, 0.1, treeWidget)); // scale factor
    treeWidget->setItemDelegateForColumn(3, new ScanListWidgetDelegate(2, limMin, limMax, 1.0, treeWidget)); // vertical offset
    treeWidget->setItemDelegateForColumn(4, new ScanListWidgetDelegate(4, limMin, limMax, 0.1, treeWidget)); // horizontal offset

    // enable drag-n-drop in the treewidget for manual item sorting
    treeWidget->setDragEnabled(true);
    treeWidget->setAcceptDrops(true);
    treeWidget->setDropIndicatorShown(true);
    treeWidget->setDragDropMode(QAbstractItemView::InternalMove);

    treeWidget->setSortingEnabled(false);
    treeWidget->header()->setSortIndicatorShown(true);
    treeWidget->header()->setSectionsClickable(true);

    QStringList slHeaders;
    slHeaders << QString() << tr("Scan") << tr("Scaling") << tr("Vertical Offset") << tr("Horizontal Offset");

    treeWidget->setColumnCount(slHeaders.size());
    treeWidget->setRootIsDecorated(false);
    treeWidget->setSortingEnabled(false);
    treeWidget->setHeaderLabels(slHeaders);
    treeWidget->header()->setSectionResizeMode(QHeaderView::Interactive);
    treeWidget->header()->setContextMenuPolicy(Qt::CustomContextMenu);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(treeWidget);
    this->setLayout(layout);

    actExportScan       = new QAction(QIcon::fromTheme("profex-save"), tr("Export scan..."));
    actRemoveScan       = new QAction(QIcon::fromTheme("profex-delete"), tr("Remove selected scans"));
    actCopyScan         = addAction(QIcon::fromTheme("profex-document-copy"), tr("Copy scan data"));
    actCopyScanX        = addAction(QIcon::fromTheme("profex-document-copy"), tr("Copy scan angle data"));
    actCopyScanY        = addAction(QIcon::fromTheme("profex-document-copy"), tr("Copy scan intensity data"));
    actCopyHkl          = new QAction(QIcon::fromTheme("profex-document-copy"), tr("Copy HKL data"));
    actExportScanOffset = new QAction(QIcon::fromTheme("profex-save"), tr("Export scan with offsets..."));
    actCopyScanOffset   = new QAction(QIcon::fromTheme("profex-document-copy"), tr("Copy scan data with offsets"));
    actHidePhases       = new QAction(tr("Hide all phases"));
    actShowPhases       = new QAction(tr("Show all phases"));
    actAddNoise         = new QAction(tr("Add synthetic noise"));
    actChangeColor      = new QAction(tr("Change scan color..."));
    actChangeStyle      = new QAction(tr("Change scan style..."));
    actContextHelp      = new QAction(QIcon::fromTheme("profex-help"), tr("Show context help"));

    actCopyScan->setShortcut(global::Functions::keyCopy());
    actCopyScanX->setShortcut(global::Functions::keyAltCopy());
    actCopyScanY->setShortcut(global::Functions::keyShiftCopy());

    actCopyScan->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    actCopyScanX->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    actCopyScanY->setShortcutContext(Qt::WidgetWithChildrenShortcut);

    connect(actCopyScan,  &QAction::triggered, this, &ScanListWidget::copyScans);
    connect(actCopyScanX, &QAction::triggered, this, &ScanListWidget::copyScansX);
    connect(actCopyScanY, &QAction::triggered, this, &ScanListWidget::copyScansY);

    slContextMenu = new QMenu(this);

    slContextMenu->addAction(actExportScan);
    slContextMenu->addAction(actRemoveScan);
    slContextMenu->addAction(actCopyScan);
    slContextMenu->addAction(actCopyScanX);
    slContextMenu->addAction(actCopyScanY);
    slContextMenu->addAction(actCopyHkl);
    slContextMenu->addSeparator();
    slContextMenu->addAction(actExportScanOffset);
    slContextMenu->addAction(actCopyScanOffset);
    slContextMenu->addSeparator();
    slContextMenu->addAction(actHidePhases);
    slContextMenu->addAction(actShowPhases);
    slContextMenu->addSeparator();
    slContextMenu->addAction(actAddNoise);
    slContextMenu->addSeparator();
    slContextMenu->addAction(actChangeColor);
    slContextMenu->addAction(actChangeStyle);
    slContextMenu->addSeparator();
    slContextMenu->addAction(actContextHelp);


    slHeaderMenu = new QMenu(this);

    for (int i = 2; i < treeWidget->header()->count(); ++i) {
        QAction *act = new QAction(treeWidget->headerItem()->text(i), slHeaderMenu);
        act->setCheckable(true);
        act->setData(i);
        slHeaderMenu->addAction(act);
    }

    redrawBlocked = false;

    initSettings();
    initConnections();
}

ScanListWidget::~ScanListWidget()
{
}

void ScanListWidget::initSettings()
{
    headerStateRestored = treeWidget->header()->restoreState(settings->value("scanListWidget/scanListState", QByteArray()).toByteArray());

    QList<QVariant> h = settings->value("scanListWidget/columnVisibility", QList<QVariant>()).toList();

    for (int i = 0; i < treeWidget->header()->count() - 2; ++i) {
        bool b = i >= h.size() ? true : h.at(i).toBool();
        treeWidget->setColumnHidden(i + 2, !b);
        slHeaderMenu->actions().at(i)->setChecked(b);
    }
}

void ScanListWidget::treeWidgetHeaderChanged()
{
    settings->setValue("scanListWidget/scanListState", treeWidget->header()->saveState());
}

void ScanListWidget::initConnections()
{
    connect(treeWidget->header(), &QHeaderView::sectionClicked,             this, &ScanListWidget::sortScans);
    connect(treeWidget->header(), &QHeaderView::sectionResized,             this, &ScanListWidget::treeWidgetHeaderChanged);
    connect(treeWidget->header(), &QHeaderView::customContextMenuRequested, this, &ScanListWidget::headerContextMenuRequested);
    connect(treeWidget,           &QTreeWidget::customContextMenuRequested, this, &ScanListWidget::showContextMenu);
    connect(treeWidget,           &QTreeWidget::itemChanged,                this, &ScanListWidget::scanParameterChanged);
    connect(treeWidget,           &QTreeWidget::itemSelectionChanged,       this, &ScanListWidget::activeScanChanged);
    connect(treeWidget->model(),  &QAbstractItemModel::rowsInserted,        this, &ScanListWidget::updateScanOrder);
    connect(slHeaderMenu,         &QMenu::triggered,                        this, &ScanListWidget::toggleColumnVisibility);

    connect(actExportScan,       &QAction::triggered, this, &ScanListWidget::exportScans);
    connect(actRemoveScan,       &QAction::triggered, this, &ScanListWidget::removeScan);
    connect(actCopyScan,         &QAction::triggered, this, &ScanListWidget::copyScans);
    connect(actCopyScanX,        &QAction::triggered, this, &ScanListWidget::copyScansX);
    connect(actCopyScanY,        &QAction::triggered, this, &ScanListWidget::copyScansY);
    connect(actCopyHkl,          &QAction::triggered, this, &ScanListWidget::copyHklData);
    connect(actExportScanOffset, &QAction::triggered, this, &ScanListWidget::exportSingleScanWithAngularCorrections);
    connect(actCopyScanOffset,   &QAction::triggered, this, &ScanListWidget::copyScansWithAngularCorrections);
    connect(actHidePhases,       &QAction::triggered, this, &ScanListWidget::hideAllPhases);
    connect(actShowPhases,       &QAction::triggered, this, &ScanListWidget::showAllPhases);
    connect(actAddNoise,         &QAction::triggered, this, &ScanListWidget::addNoise);
    connect(actChangeColor,      &QAction::triggered, this, &ScanListWidget::changeScanColor);
    connect(actChangeStyle,      &QAction::triggered, this, &ScanListWidget::changeScanStyle);
    connect(actContextHelp,      &QAction::triggered, this, &ScanListWidget::showHelp);
}

void ScanListWidget::adjustHeaderSize()
{
    if (!headerStateRestored) {
        treeWidget->header()->resizeSections(QHeaderView::ResizeToContents);
    }
}

int ScanListWidget::currentIndex()
{
    return treeWidget->indexOfTopLevelItem(treeWidget->currentItem());
}

void ScanListWidget::updateView()
{
    bool oldState = blockRedraw(true);
    QMap<int, bool> checkStates;
    QMap<int, bool> selectStates;
    storeTreeItemStates(checkStates, selectStates);
    adjustNumberOfTreeItems();
    generateTreeItems(checkStates, selectStates);
    adjustHeaderSize();
    blockRedraw(oldState);
}

/*
 * using the uid to store the checkstate and activestate of scans doesn't work because the
 * uid changes when reloading the file. Therefore we use the position in the tree widget to
 * restore the states
 */
void ScanListWidget::storeTreeItemStates(QMap<int, bool> &checkStates, QMap<int, bool> &selectStates)
{
    checkStates.clear();
    selectStates.clear();

    for (int i = 0; i < scanControl->count(); ++i) {
        checkStates[i]  = scanControl->at(i)->isVisible();
        selectStates[i] = scanControl->at(i)->isActive();
    }
}

void ScanListWidget::generateTreeItems(const QMap<int, bool> &checkStates, const QMap<int, bool> &selectStates)
{
    bool drawPhasePatterns = (settings->value("graph/phaseVisibilityPattern", true).toBool()
                              || settings->value("graph/phaseVisibilityHkl", true).toBool());

    bool oldState = treeWidget->blockSignals(true);

    for (int i = 0; i < scanControl->count(); ++i) {
        ScanListWidgetItem *it = topLevelScanListItem(i);
        if (!it) continue;

        Scan *scan = scanControl->getScan(i);
        if (!scan) continue;

        bool isPhase = scan->scanTypes().testFlag(Scan::PHASE);
        if (isPhase && !drawPhasePatterns) continue;

        bool isVisible   = checkStates.value(i, true);
        bool isActive    = selectStates.value(i, false);
        bool isTemporary = scan->scanTypes().testFlag(Scan::TEMPORARY);

        scan->setVisible(isVisible);
        scan->setActive(isActive);

        it->setItemStatus(isVisible, isActive, isTemporary);
        it->setUid(scan->uid());
        it->setName(scanControl->scanName(scan));
        it->setScaleFactor(scan->scaleFactor());
        it->setXoffset(scan->xOffset());
        it->setYoffset(scan->yOffset());
        it->setColor(scan->color(), settings->isDarkMode());

        updateScan(it, scan);
    }

    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, this);
    treeWidget->blockSignals(oldState);
}

void ScanListWidget::adjustNumberOfTreeItems()
{
    if (scanControl->count() == treeWidget->topLevelItemCount()) return;

    while (scanControl->count() > treeWidget->topLevelItemCount()) {
        treeWidget->addTopLevelItem(new ScanListWidgetItem(QStringList()));
    }

    while (scanControl->count() < treeWidget->topLevelItemCount()) {
        delete treeWidget->takeTopLevelItem(treeWidget->topLevelItemCount() - 1);
    }
}

void ScanListWidget::showContextMenu(const QPoint &p)
{
    QList<QTreeWidgetItem*> sel = treeWidget->selectedItems();
    bool b = sel.size() > 0;

    actExportScan->setEnabled(b);
    actRemoveScan->setEnabled(b);
    actCopyScan->setEnabled(b);
    actCopyHkl->setEnabled(b);
    actExportScanOffset->setEnabled(b);
    actCopyScanOffset->setEnabled(b);
    // actHidePhases;
    // actShowPhases;
    actAddNoise->setEnabled(b);
    actChangeColor->setEnabled(b);
    actChangeStyle->setEnabled(b);

    slContextMenu->popup(treeWidget->viewport()->mapToGlobal(p));
}

void ScanListWidget::exportScans()
{
    exportDoDisk(false);
}

void ScanListWidget::exportSingleScanWithAngularCorrections()
{
    exportDoDisk(true);
}

void ScanListWidget::exportDoDisk(bool angCorr)
{

    QList<QTreeWidgetItem*> sel = treeWidget->selectedItems();
    if (!sel.size()) return;

    QUuid firstUid = sel.first()->data(0, Qt::UserRole).toUuid();
    Scan *firstScan = scanControl->getScan(firstUid);
    if (!firstScan) return;

    QString path;
    QString basename;
    QString extension;
    QString filterUid;
    QString bnScan = sel.size() == 1 ? scanControl->scanName(firstScan) : "";

    if (!getExportFileBaseName(bnScan, path, basename, extension, filterUid)) {
        qDebug() << QString("ScanListWidget::exportScans(): "
                            "Could not export scan %1/%2 with extension %3 and filterUid %4")
                        .arg(path, basename, extension, filterUid);
        return;
    }

    QMap<QString, QVariant> flags;
    flags["fieldSeparator"] = settings->value("config/asciiFieldSeparator", QVariant(QString(" ")));
    flags["fixBgmnZero"]    = QVariant(false);

    double eps1 = scanControl->getAngularCorrEPS1();
    double eps2 = scanControl->getAngularCorrEPS2();
    double eps3 = scanControl->getAngularCorrEPS3();

    for (int i = 0; i < sel.size(); ++i) {
        QUuid uid = sel.at(i)->data(0, Qt::UserRole).toUuid();
        Scan *scan = scanControl->getScan(uid);
        if (!scan) continue;

        // reset the offsets, otherwise the
        // exporter will apply them
        Scan corScan = scan->clone();

        if (angCorr) {
            // offsets will be applied by the exporter
            for (int i = 0; i < corScan.pDataAngle().size(); ++i) {
                double tt = corScan.pDataAngle().at(i);
                corScan.pDataAngle()[i] = tt - global::Functions::angularCorrection(tt, eps1, eps2, eps3);
            }
        } else {
            corScan.setXoffset(0.0);
            corScan.setYoffset(0.0);
            corScan.setScaleFactor(1.0);
        }

        QString fname = basename;

        // replace the tag with the scan name. If the tag is not found, the user is responsible for
        // giving a unique file name.
        fname.replace("<scan name>", scan->name()); // not using scanControl->scanName, because it might contain parentheses.

        QFileInfo fi(path + "/" + fname + "." + extension);
        qDebug() << QString("ScanListWidget::exportToDisk(): Exporting scan to %1").arg(fi.absoluteFilePath());

        ExportHandler exHandler;
        exHandler.save(filterUid, fi.absoluteFilePath(), corScan, flags);
    }
}

bool ScanListWidget::getExportFileBaseName(const QString &scanName, QString &path, QString &basename, QString &extension, QString &filterUid)
{
    ExportHandler exHandler;
    // contains <Filter display text> <unique filter UID>
    QMap<QString, QString> formats = exHandler.uidsByFilter();
    QStringList keys = static_cast<QStringList>(formats.keys());
    QString bnScan = scanName.isEmpty() ? "<scan name>" : scanName;

    QString selFilter = settings->value("graph/saveAsFilter", QString()).toString();

    QString f = QDir::fromNativeSeparators(QString("%1/%2.%3")
                                           .arg(scanControl->fileInfo().absolutePath(),
                                                scanControl->fileInfo().baseName() + "-" + bnScan,
                                                exHandler.extensionByFilter(selFilter)));

    f = QFileDialog::getSaveFileName(this, tr("Export Scan"), f, keys.join(";;"), &selFilter);

    if (f.isEmpty()) return false;

    QFileInfo fout(f);

    if (fout.suffix().isEmpty()) {
        fout = QFileInfo(fout.absoluteFilePath() + QString(".") + exHandler.extensionByFilter(selFilter));
    }

    path = fout.absolutePath();
    basename = fout.baseName();
    extension = fout.suffix();
    filterUid = formats.value(selFilter);

    settings->setValue("graph/saveAsFilter", selFilter);
    return true;
}

void ScanListWidget::removeScan()
{
    if (!treeWidget->selectedItems().size()) return;

    for (int i = treeWidget->topLevelItemCount() - 1; i >= 0; --i) {
        if (!treeWidget->topLevelItem(i)->isSelected()) continue;

        QTreeWidgetItem *it = treeWidget->takeTopLevelItem(i);
        if (it) delete it;
        scanControl->removeScan(i, false);
    }

    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);
}

void ScanListWidget::copyScans()
{
    if (!scanControl->count()) return;

    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();
    QList<const Scan *> scans;

    if (!items.size()) {
        scans.append(scanControl->first());
    } else {
        for (int i = 0; i < items.size(); ++i) {
            const Scan *scan = scanControl->at(treeWidget->indexOfTopLevelItem(items.at(i)));
            if (scan) scans.append(scan);
        }
    }

    if (scans.size()) {
        QString sep = settings->value("config/clipboardFieldSeparator", ";").toString();
        QString out = ScanOps::scansToAsciiMatrix(scans, sep, 6, false);
        QClipboard *clipBoard = qApp->clipboard();
        clipBoard->setText(out);
    }
}


void ScanListWidget::copyScansX()
{
    if (!scanControl->count()) return;

    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();
    QList<const Scan *> scans;

    if (!items.size()) {
        scans.append(scanControl->first());
    } else {
        for (int i = 0; i < items.size(); ++i) {
            const Scan *scan = scanControl->at(treeWidget->indexOfTopLevelItem(items.at(i)));
            if (scan) scans.append(scan);
        }
    }

    if (scans.size()) {
        QString sep = settings->value("config/clipboardFieldSeparator", ";").toString();
        QString out = ScanOps::scansXToAsciiMatrix(scans, sep, 6, false);
        QClipboard *clipBoard = qApp->clipboard();
        clipBoard->setText(out);
    }
}


void ScanListWidget::copyScansY()
{
    if (!scanControl->count()) return;

    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();
    QList<const Scan *> scans;

    if (!items.size()) {
        scans.append(scanControl->first());
    } else {
        for (int i = 0; i < items.size(); ++i) {
            const Scan *scan = scanControl->at(treeWidget->indexOfTopLevelItem(items.at(i)));
            if (scan) scans.append(scan);
        }
    }

    if (scans.size()) {
        QString sep = settings->value("config/clipboardFieldSeparator", ";").toString();
        QString out = ScanOps::scansYToAsciiMatrix(scans, sep, 6, false);
        QClipboard *clipBoard = qApp->clipboard();
        clipBoard->setText(out);
    }
}

void ScanListWidget::copyScansWithAngularCorrections()
{
    if (!scanControl->count()) return;

    double eps1 = scanControl->getAngularCorrEPS1();
    double eps2 = scanControl->getAngularCorrEPS2();
    double eps3 = scanControl->getAngularCorrEPS3();

    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();
    QList<const Scan *> scans;

    if (!items.size()) {
        scans.append(scanControl->first());
    } else {
        for (int i = 0; i < items.size(); ++i) {
            const Scan *scan = scanControl->at(treeWidget->indexOfTopLevelItem(items.at(i)));
            if (scan) scans.append(scan);
        }
    }

    if (scans.size()) {
        QString sep = settings->value("config/clipboardFieldSeparator", ";").toString();
        QString out = ScanOps::scansToAsciiMatrix(scans, sep, 6, true, eps1, eps2, eps3);
        QClipboard *clipBoard = qApp->clipboard();
        clipBoard->setText(out);
    }
}

void ScanListWidget::copyHklData()
{
    if (!scanControl->count()) return;

    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();
    if (items.isEmpty()) items.append(treeWidget->currentItem());

    QString out;
    AsciiHklExport hklExport;
    bool header = true;
    int nHkl = 1;

    for (int i = 0; i < items.size(); ++i) {
        int n = treeWidget->indexOfTopLevelItem(items.at(i));

        QString sep = settings->value("config/clipboardFieldSeparator", ";").toString();
        Scan *scan = scanControl->getScan(n);

        if (scan) {
            if (scan->hasHklData()) {
                out += hklExport.getData(*scan, sep, header, nHkl);
                header = false; // only for the first phase
                ++nHkl;
            }
        }
    }

    QClipboard *clipBoard = qApp->clipboard();
    clipBoard->setText(out);
}

void ScanListWidget::activeScanChanged()
{
    bool oldState = treeWidget->blockSignals(true);

    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        ScanListWidgetItem *it = topLevelScanListItem(i);
        Scan *scan = scanControl->getScan(i);

        if (!it || !scan) continue;

        bool isTemporary = scan->scanTypes().testFlag(Scan::TEMPORARY);
        bool isVisible   = it->isVisible();
        bool isActive    = it->isSelected();

        it->setItemStatus(isVisible, isActive, isTemporary);
        scan->setActive(isActive);
    }

    treeWidget->blockSignals(oldState);
    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, this);
    emit currentScanChanged(treeWidget->indexOfTopLevelItem(treeWidget->currentItem()));
}

void ScanListWidget::addNoise()
{
    if (!scanControl->count()) return;

    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();

    for (int i = 0; i < items.size(); ++i) {
        int n = treeWidget->indexOfTopLevelItem(items.at(i));

        Scan *scan = scanControl->getScan(n);
        if (scan) scan->addNoise();
    }

    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, this);
}

void ScanListWidget::scanParameterChanged(QTreeWidgetItem *it, int)
{
    Scan *scan = scanControl->getScan(treeWidget->indexOfTopLevelItem(it));
    if (scan) updateScan(it, scan);
}

void ScanListWidget::updateScan(QTreeWidgetItem *it, Scan *scan)
{
    if (redrawBlocked || !it || !scan) return;

    QString sName = scanControl->scanName(scan->uid());

    changeScanVisibility(scan, it->checkState(0));
    if (it->text(1) != sName) scan->setOverrideName(it->text(1));
    changeScanScaling(scan, it->text(2));
    changeScanYOffset(scan, it->text(3));
    changeScanXOffset(scan, it->text(4));

    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, this);
}

void ScanListWidget::changeScanVisibility(Scan *scan, int visible)
{
    if (visible == Qt::Unchecked) {
        scan->setVisible(false);
    } else {
        scan->setVisible(true);
    }
}

void ScanListWidget::changeScanScaling(Scan *scan, const QString &s)
{
    bool ok;
    double d = s.toDouble(&ok);

    if (ok) scan->setScaleFactor(d);
}

void ScanListWidget::changeScanYOffset(Scan *scan, const QString &s)
{
    bool ok;
    double d = s.toDouble(&ok);

    if (ok) scan->setYoffset(d);
}

void ScanListWidget::changeScanXOffset(Scan *scan, const QString &s)
{
    bool ok;
    double d = s.toDouble(&ok);

    if (ok) scan->setXoffset(d);
}

void ScanListWidget::sortScans(int c)
{
    Qt::SortOrder order = treeWidget->header()->sortIndicatorOrder();
    treeWidget->sortItems(c, order);
    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);
}

void ScanListWidget::updateScanOrder()
{
    if (redrawBlocked) return;

    QVector<QUuid> v;

    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        v.append(treeWidget->topLevelItem(i)->data(0, Qt::UserRole).toUuid());
    }

    scanControl->sortScanOrder(v, true, nullptr);
}

ScanListWidgetItem * ScanListWidget::topLevelScanListItem(int i) const
{
    return dynamic_cast<ScanListWidgetItem*>(treeWidget->topLevelItem(i));
}

bool ScanListWidget::blockRedraw(bool b)
{
    bool bPrev = redrawBlocked;
    redrawBlocked = b;
    return bPrev;
}

void ScanListWidget::headerContextMenuRequested(QPoint p)
{
    slHeaderMenu->popup(treeWidget->header()->viewport()->mapToGlobal(p));
}

void ScanListWidget::toggleColumnVisibility(QAction *a)
{
    treeWidget->setColumnHidden(a->data().toInt(), !a->isChecked());

    QList<QVariant> columnVisibilities;

    for (int i = 2; i < treeWidget->header()->count(); ++i) {
        columnVisibilities.append(QVariant(!treeWidget->isColumnHidden(i)));
    }

    settings->setValue("scanListWidget/columnVisibility", columnVisibilities);
}

void ScanListWidget::hideAllPhases()
{
    scanControl->setPhaseVisibility(false);
    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, nullptr);
}

void ScanListWidget::showAllPhases()
{
    scanControl->setPhaseVisibility(true);
    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, nullptr);
}

void ScanListWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_0 && e->modifiers() & Qt::ControlModifier) {
        scanControl->togglePhaseVisibility();
        scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, nullptr);
        e->accept();
        return;
    } else if (e->key() == Qt::Key_Delete) {
        if (scanControl->count() == 1) {
            // ask for the last scan, because an empty plot can look confusing
            if (QMessageBox::question(this, tr("Remove scan"),
                                      tr("Do you really want to remove the scan from the graph?"))
                != QMessageBox::Yes) {
                return;
            }
        }
        removeScan();
        e->accept();
        return;
    }

    AbstractGraphView::keyPressEvent(e);
}

void ScanListWidget::changeScanColor()
{
    Scan *cscan = scanControl->getScan(currentIndex());

    QColor col = cscan ? cscan->color() : Qt::blue;
    col = QColorDialog::getColor(col, this, tr("Select scan color"));
    if (!col.isValid()) return;

    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();
    if (items.isEmpty()) items.append(treeWidget->currentItem());

    for (int i = 0; i < items.size(); ++i) {
        int n = treeWidget->indexOfTopLevelItem(items.at(i));

        Scan *scan = scanControl->getScan(n);
        if (scan) scan->setColor(col);
    }

    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, nullptr);
}

void ScanListWidget::changeScanStyle()
{
    Scan *cscan = scanControl->getScan(currentIndex());
    if (!cscan) return;

    QStringList pstyleNames = settings->getScanStyleNames();

    bool ok;
    QString name = QInputDialog::getItem(this,
                                         tr("Select scan style"),
                                         tr("Style"),
                                         pstyleNames,
                                         cscan->pointSymbol(),
                                         false,
                                         &ok);

    if (!ok) return;
    int ps = pstyleNames.indexOf(name);

    QList<QTreeWidgetItem*> items = treeWidget->selectedItems();
    if (items.isEmpty()) items.append(treeWidget->currentItem());

    for (int i = 0; i < items.size(); ++i) {
        int n = treeWidget->indexOfTopLevelItem(items.at(i));

        Scan *scan = scanControl->getScan(n);
        if (scan) scan->setPointSymbol(ps);
    }

    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, this);
}

int ScanListWidget::countItems() const
{
    return treeWidget->topLevelItemCount();
}

QString ScanListWidget::getScanName(int i) const
{
    ScanListWidgetItem *it = dynamic_cast<ScanListWidgetItem*>(treeWidget->topLevelItem(i));
    if (it) return it->getName();
    return QString();
}

QStringList ScanListWidget::getScanNames() const
{
    QStringList l;

    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        ScanListWidgetItem *it = dynamic_cast<ScanListWidgetItem*>(treeWidget->topLevelItem(i));
        if (it) l.append(it->getName());
    }

    return l;
}

QStringList ScanListWidget::getActiveScanNames() const
{
    QStringList l;

    for (int i = 0; i < treeWidget->topLevelItemCount(); ++i) {
        ScanListWidgetItem *it = dynamic_cast<ScanListWidgetItem*>(treeWidget->topLevelItem(i));
        if (!it) continue;
        if (it->isActive()) l.append(it->getName());
    }

    return l;
}

void ScanListWidget::showHelp()
{
    emit helpText("scanList");
}
