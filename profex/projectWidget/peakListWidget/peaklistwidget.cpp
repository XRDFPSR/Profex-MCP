/***************************************************************************
                          projectsearchmatchwidget.cpp  -  description
                             -------------------
    begin                : Tue Jan 22 21:00:00 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#include "peaklistwidget.h"
#include "../graphWidget/graphdatacontroller.h"
#include "../../../libXrdIO/bgmnfileio.h"
#include "../../../libXrdIO/functions.h"
#include "ui_peaklistwidget.h"

#include <QtMath>
#include <QFileDialog>
#include <QGroupBox>
#include <QItemSelectionModel>
#include <QMenu>
#include <QSplitter>
#include <QClipboard>
#include <QKeyEvent>
#include <utility> // for std::as_const

PeakListWidget::PeakListWidget(GraphDataController *c, QWidget *parent) :
    AbstractGraphView(c, parent),
    ui(new Ui::PeakListWidget)
{
    ui->setupUi(this);
    refStrManager = BgmnRefStructureManager::getInstance();
    saveSettingsRequested = false;

    prevNumberOfScans = -1;
    ui->comboBoxHklData->setEditable(true);
    ui->comboBoxHklData->setInsertPolicy(QComboBox::InsertAtBottom);
    ui->comboBoxHklData->setCompleter(nullptr);

    hklHeaderLabels << QString("Phase")
                    << QString("h")
                    << QString("k")
                    << QString("l")
                    << QString("Angle (%1%2%3)").arg(global::degree).arg(2).arg(global::theta)
                    << QString("d (nm)")
                    << QString("Intensity (deg*cts)")
                    << QString("Rel. intensity (%)")
                    << QString("Texture")
                    << QString("B1 (1/nm)")
                    << QString("B2 (1/nm)");

    columns["phase"]     = 0;
    columns["h"]         = 1;
    columns["k"]         = 2;
    columns["l"]         = 3;
    columns["2theta"]    = 4;
    columns["dnm"]       = 5;
    columns["intensity"] = 6;
    columns["intensrel"] = 7;
    columns["texture"]   = 8;
    columns["b1"]        = 9;
    columns["b2"]        = 10;

    peakDataModel = new QStandardItemModel(this);
    peakDataModel->setHorizontalHeaderLabels(hklHeaderLabels);
    peakDataModel->setSortRole(Qt::UserRole+1);

    peakDataProxy = new HklSortFilterProxyModel(this);
    peakDataProxy->setSourceModel(peakDataModel);
    peakDataProxy->setSortRole(Qt::UserRole+1);
    peakDataProxy->setColumnIndices(columns);
    ui->peakListFilterForm->setProxyModel(peakDataProxy);
    ui->peakListFilterForm->setComboBox(ui->comboBoxHklData);

    toolsMenu = new QMenu(this);
    QAction *actApplyFilters   = new QAction(QIcon::fromTheme("profex-document-clone"), tr("Apply filters to other projects"));
    QAction *actSaveList       = new QAction(QIcon::fromTheme("profex-save-as"), tr("Save peak data to file"));
    QAction *actRemoveCurrent  = new QAction(QIcon::fromTheme("profex-remove"), tr("Remove selected peak"));
    QAction *actUpdateData     = new QAction(QIcon::fromTheme("profex-refresh"), tr("Reload data from disk"));
    QAction *actClearSelection = new QAction(QIcon::fromTheme("profex-select-none"), tr("Clear selection"));

    connect(actApplyFilters,   SIGNAL(triggered(bool)), this, SIGNAL(applyFiltersToAll()));
    connect(actSaveList,       SIGNAL(triggered(bool)), this, SLOT(saveHklList()));
    connect(actRemoveCurrent,  SIGNAL(triggered(bool)), this, SLOT(removeCurrentHkl()));
    connect(actUpdateData,     SIGNAL(triggered(bool)), this, SLOT(reloadData()));
    connect(actClearSelection, SIGNAL(triggered(bool)), this, SLOT(clearHklSelection()));

    toolsMenu->addAction(actApplyFilters);
    toolsMenu->addAction(actSaveList);
    toolsMenu->addAction(actRemoveCurrent);
    toolsMenu->addAction(actUpdateData);
    toolsMenu->addAction(actClearSelection);

    ui->toolButtonToolsMenu->setIcon(QIcon::fromTheme("profex-preferences"));
    ui->toolButtonToolsMenu->setMenu(toolsMenu);

    ui->toolButtonFilter->setIcon(QIcon::fromTheme("profex-filter"));
    ui->toolButtonContextHelp->setIcon(QIcon::fromTheme("profex-help"));

    contextMenuHeader = new QMenu(this);

    for (int i = 0; i < hklHeaderLabels.size(); ++i) {
        QAction *act = new QAction(hklHeaderLabels.at(i));
        act->setCheckable(true);
        act->setData(i);
        contextMenuHeader->addAction(act);
    }

    contextMenuTable = new QMenu(this);

    QAction *actCopyText = addAction(QIcon::fromTheme("profex-document-copy"), tr("Copy"));
    actCopyText->setShortcut(global::Functions::keyCopy());
    actCopyText->setShortcutContext(Qt::WidgetWithChildrenShortcut);
    connect(actCopyText, &QAction::triggered, this, &PeakListWidget::copyText);

    contextMenuTable->addAction(actCopyText);
    contextMenuTable->addAction(actRemoveCurrent);
    contextMenuTable->addAction(actClearSelection);

    ui->tableViewHklData->setModel(peakDataProxy);
    ui->tableViewHklData->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableViewHklData->horizontalHeader()->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableViewHklData->horizontalHeader()->setSortIndicatorShown(false);

    initSettings();

    connect(contextMenuHeader, SIGNAL(triggered(QAction*)), this, SLOT(toggleColumnHidden(QAction*)));
    connect(ui->splitter, SIGNAL(splitterMoved(int,int)), this, SLOT(splitterResized(int,int)));
    connect(ui->tableViewHklData->horizontalHeader(), SIGNAL(sectionResized(int,int,int)), this, SLOT(headerResized()));
    connect(ui->tableViewHklData->horizontalHeader(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(openCustomHeaderContextMenu(QPoint)));
    connect(ui->tableViewHklData, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(openCustomTableContextMenu(QPoint)));
    connect(ui->tableViewHklData->selectionModel(), SIGNAL(selectionChanged(QItemSelection,QItemSelection)),
            this, SLOT(selectHkl(QItemSelection,QItemSelection)));
}

PeakListWidget::~PeakListWidget()
{
    if (peakDataModel) delete peakDataModel;
    if (peakDataProxy) delete peakDataProxy;
    delete ui;
}

void PeakListWidget::initSettings()
{
    ui->tableViewHklData->horizontalHeader()->restoreState(settings->value("peakListWidget/hklHeader", QByteArray()).toByteArray());
    ui->splitter->restoreState(settings->value("peakListWidget/splitter", QByteArray()).toByteArray());

    QList<QVariant> hiddenCols;
    hiddenCols.append(QVariant(columns.value("texture"))); // hide the texture column by default
    hiddenCols.append(QVariant(columns.value("b1"))); // hide the b1 column by default
    hiddenCols.append(QVariant(columns.value("b2"))); // hide the k2 column by default

    hiddenCols = settings->value("peakListWidget/hiddenColumns", hiddenCols).toList();

    for (int i = 0; i < ui->tableViewHklData->horizontalHeader()->count(); ++i) {
        bool b = hiddenCols.contains(QVariant(i));
        ui->tableViewHklData->setColumnHidden(i, b);
        contextMenuHeader->actions().at(i)->setChecked(!b);
    }
}

void PeakListWidget::openCustomHeaderContextMenu(const QPoint &p)
{
    contextMenuHeader->popup(ui->tableViewHklData->viewport()->mapToGlobal(p));
}

void PeakListWidget::openCustomTableContextMenu(const QPoint &p)
{
    contextMenuTable->popup(ui->tableViewHklData->viewport()->mapToGlobal(p));
}

void PeakListWidget::copyText()
{
    auto *view  = ui->tableViewHklData;
    auto *model = view->model();

    QModelIndexList sel = view->selectionModel()->selectedIndexes();
    if (sel.isEmpty()) return;

    // Collect selected rows and columns
    QSet<int> rows, cols;
    for (const QModelIndex &idx : std::as_const(sel)) {
        rows.insert(idx.row());
        cols.insert(idx.column());
    }

    QList<int> rowList = rows.values();
    QList<int> colList = cols.values();
    std::sort(rowList.begin(), rowList.end());
    std::sort(colList.begin(), colList.end());

    // Column index -> position in output
    QHash<int,int> colPos;
    for (int i = 0; i < colList.size(); ++i)
        colPos[colList[i]] = i;

    QString sep = settings->value("config/clipboardFieldSeparator", ";").toString();
    QString out;

    // Header line
    QStringList header(colList.size());
    for (int i = 0; i < colList.size(); ++i) {
        int c = colList[i];
        header[i] = "\"" + model->headerData(c, Qt::Horizontal, Qt::DisplayRole).toString() + "\"";
    }
    out += header.join(sep) + "\n";

    // Data lines
    QMap<int, QMap<int, QVariant>> data;
    for (const QModelIndex &idx : std::as_const(sel)) {
        data[idx.row()][idx.column()] = idx.data(Qt::EditRole);
    }

    for (int r : std::as_const(rowList)) {
        QStringList line(colList.size(), QString());
        const auto &colsForRow = data.value(r);

        for (auto it = colsForRow.cbegin(); it != colsForRow.cend(); ++it) {
            int c = it.key();
            int pos = colPos.value(c, -1);
            if (pos < 0) continue;

            QString value = it.value().toString();

            // Wrap text from column 0 in double quotes
            if (c == 0 && !value.isEmpty()) {
                value.replace('"', "\"\""); // escape embedded quotes
                value = "\"" + value + "\"";
            }

            line[pos] = value;
        }
        out += line.join(sep) + "\n";
    }

    QApplication::clipboard()->setText(out);
}


void PeakListWidget::keyPressEvent(QKeyEvent *e)
{
    if (e->key() == Qt::Key_Delete) {
        removeCurrentHkl();
    } else {
        AbstractGraphView::keyPressEvent(e);
    }
}

void PeakListWidget::updateView()
{
    if (!isShown) return;
    updateData();
}

void PeakListWidget::updateData(bool force)
{
    if (!isShown && !force) return;

    qApp->setOverrideCursor(Qt::WaitCursor);
    // blocking signals to prevent reset of the column width
    bool oldStateHeader = ui->tableViewHklData->horizontalHeader()->blockSignals(true);
    bool oldStateCombo = ui->comboBoxHklData->blockSignals(true);

    clearAll();
    ui->comboBoxHklData->clear();
    ui->comboBoxHklData->addItem("All phases", ".*");

    double _absIntMax = -1.0;
    double _waveLength  = scanControl->getWaveLength(settings->defaultWavelength());
    double _twoThetaMin = scanControl->getXmin();
    double _twoThetaMax = scanControl->getXmax();
    double _dMax = global::Functions::twoThetaToD(_twoThetaMin, 0.1*_waveLength);
    double _dMin = global::Functions::twoThetaToD(_twoThetaMax, 0.1*_waveLength);

    // loop over scans to find the strongest peak
    for (int i = 0; i < scanControl->count(); ++i) {
        _absIntMax = qMax(_absIntMax, scanControl->at(i)->getHklMaxIntensity());
    }

    if (_absIntMax < 0.0) _absIntMax = 1.0;

    for (int i = 0; i < scanControl->count(); ++i) {
        const Scan *scan = scanControl->getScan(i);
        if (scan->hasHklData()) {
            QString sName = scanControl->scanName(scan);
            ui->comboBoxHklData->addItem(sName, sName);
            appendDataToModel(scan, _absIntMax, _waveLength);
        }
    }

    // we sort the source model, becaus if we sort the proxyfilter model, the vertical header
    // numbers are out of order
    peakDataModel->sort(columns.value("2theta", 4), Qt::AscendingOrder);
    peakDataProxy->sort(-1); // uses the sort order of the source model
    ui->peakListFilterForm->setIntensityLimits(0.0, _absIntMax, 0.0, 100.0);
    ui->peakListFilterForm->setRangeLimits(_twoThetaMin, _twoThetaMax, _dMin, _dMax);

    scanControl->setAllHklDisplayStatus(1, false);

    ui->comboBoxHklData->blockSignals(oldStateCombo);
    ui->tableViewHklData->horizontalHeader()->blockSignals(oldStateHeader);
    ui->peakListFilterForm->filterParametersChanged();
    qApp->restoreOverrideCursor();
}

void PeakListWidget::clearAll()
{
    int r = peakDataModel->rowCount();
    peakDataModel->removeRows(0, r);
}

/*
 * appends scan's hkl data to an existing model
 */
void PeakListWidget::appendDataToModel(const Scan *scan, double maxI, double wl)
{
    if (!scan) return;

    int _hmin = ui->peakListFilterForm->getRangeHmin();
    int _hmax = ui->peakListFilterForm->getRangeHmax();
    int _kmin = ui->peakListFilterForm->getRangeKmin();
    int _kmax = ui->peakListFilterForm->getRangeKmax();
    int _lmin = ui->peakListFilterForm->getRangeLmin();
    int _lmax = ui->peakListFilterForm->getRangeLmax();
    double _texmin = ui->peakListFilterForm->getRangeTexMin();
    double _texmax = ui->peakListFilterForm->getRangeTexMax();
    double _b1min  = ui->peakListFilterForm->getRangeB1Min();
    double _b1max  = ui->peakListFilterForm->getRangeB1Max();
    double _b2min  = ui->peakListFilterForm->getRangeB2Min();
    double _b2max  = ui->peakListFilterForm->getRangeB2Max();

    QString sName = scanControl->scanName(scan);

    for (int i = 0; i < scan->hklCount(); ++i) {
        const Hkl *hkl = scan->getHkl(i);
        if (!hkl) continue;

        double hklIntens = hkl->intensity();
        double hklIntensRel = 100.0 * hklIntens / maxI;
        double b1 = hkl->B1();
        double b2 = hkl->B2();
        double pos = hkl->position();
        double d  = scan->getHklXunit() == "dnm" ? pos : global::Functions::twoThetaToD(pos, wl) / 10.0;
        double tt = scan->getHklXunit() == "dnm" ? global::Functions::dToTwoTheta(10.0 * pos, wl) : pos;
        double tex = hkl->texture();
        QList<int> hklList = hklToInt(hkl->hkl());

        _hmin = qMin(_hmin, hklList.at(0));
        _hmax = qMax(_hmax, hklList.at(0));
        _kmin = qMin(_kmin, hklList.at(1));
        _kmax = qMax(_kmax, hklList.at(1));
        _lmin = qMin(_lmin, hklList.at(2));
        _lmax = qMax(_lmax, hklList.at(2));
        _texmin = qMin(_texmin, tex);
        _texmax = qMax(_texmax, tex);
        _b1min = qMin(_b1min, b1);
        _b1max = qMax(_b1max, b1);
        _b2min = qMin(_b2min, b2);
        _b2max = qMax(_b2max, b2);

        QStandardItem *itPhase = new QStandardItem(sName);
        QStandardItem *itH     = new QStandardItem(QString("%1").arg(hklList.at(0)));
        QStandardItem *itK     = new QStandardItem(QString("%1").arg(hklList.at(1)));
        QStandardItem *itL     = new QStandardItem(QString("%1").arg(hklList.at(2)));
        QStandardItem *iTT     = new QStandardItem(QString("%1").arg(tt,           0, 'f', 4));
        QStandardItem *iD      = new QStandardItem(QString("%1").arg(d,            0, 'f', 4));
        QStandardItem *iInt    = new QStandardItem(QString("%1").arg(hklIntens,    0, 'f', 4));
        QStandardItem *iIntR   = new QStandardItem(QString("%1").arg(hklIntensRel, 0, 'f', 2));
        QStandardItem *iTex    = new QStandardItem(QString("%1").arg(tex,          0, 'f', 4));
        QStandardItem *iB1     = new QStandardItem(QString("%1").arg(b1,           0, 'f', 7));
        QStandardItem *iB2     = new QStandardItem(QString("%1").arg(b2,           0, 'f', 9));

        // sort role data
        itPhase->setData(sName,        Qt::UserRole+1);
        itH->setData(hklList.at(0),    Qt::UserRole+1);
        itK->setData(hklList.at(1),    Qt::UserRole+1);
        itL->setData(hklList.at(2),    Qt::UserRole+1);
        iTT->setData(tt,               Qt::UserRole+1);
        iD->setData(d,                 Qt::UserRole+1);
        iInt->setData(hklIntens,       Qt::UserRole+1);
        iIntR->setData(hklIntensRel,   Qt::UserRole+1);
        iTex->setData(tex,             Qt::UserRole+1);
        iB1->setData(b1,               Qt::UserRole+1);
        iB2->setData(b2,               Qt::UserRole+1);

        // user role data
        itPhase->setData(QVariant(scan->uid()), Qt::UserRole);
        iTT->setData(QVariant(hkl->uid()), Qt::UserRole);

        // disable editing
        itPhase->setEditable(false);
        itH->setEditable(false);
        itK->setEditable(false);
        itL->setEditable(false);
        iTT->setEditable(false);
        iD->setEditable(false);
        iInt->setEditable(false);
        iIntR->setEditable(false);
        iTex->setEditable(false);
        iB1->setEditable(false);
        iB2->setEditable(false);

        itH->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itK->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        itL->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        iTT->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        iD->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        iInt->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        iIntR->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        iTex->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        iB1->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        iB2->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

        QList<QStandardItem *> row {nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, nullptr};
        row[columns.value("phase")]     = itPhase;
        row[columns.value("h")]         = itH;
        row[columns.value("k")]         = itK;
        row[columns.value("l")]         = itL;
        row[columns.value("2theta")]    = iTT;
        row[columns.value("dnm")]       = iD;
        row[columns.value("intensity")] = iInt;
        row[columns.value("intensrel")] = iIntR;
        row[columns.value("texture")]   = iTex;
        row[columns.value("b1")]        = iB1;
        row[columns.value("b2")]        = iB2;

        peakDataModel->appendRow(row);
    }

    ui->peakListFilterForm->setHklLimits(_hmin, _hmax, _kmin, _kmax, _lmin, _lmax);
    ui->peakListFilterForm->setProfileLimits(_texmin, _texmax, _b1min, _b1max, _b2min, _b2max);
}

void PeakListWidget::hasPeakSelection(int &total, int &selected) const
{
    total = peakDataProxy->rowCount();
    selected = ui->tableViewHklData->selectionModel()->selectedRows().size();
}

void PeakListWidget::saveHklList()
{
    QString fDia = scanControl->fileName();

    QString csvFilter("CSV File (*.csv *.CSV)");
    QString qualxDFilter("d values (*.dif *.DIF)");
    QString qualxTFilter("2theta values (*.dif *.DIF)");

    QStringList filters;
    filters << csvFilter << qualxDFilter << qualxTFilter;

    QString selectedFilter;

    QString f = QFileDialog::getSaveFileName(this, tr("Save hkl list to CSV"), fDia, filters.join(";;"), &selectedFilter);
    if (f.isEmpty()) return;

    if (selectedFilter == csvFilter)    saveToCsv(f);
    if (selectedFilter == qualxDFilter) saveToQualxD(f);
    if (selectedFilter == qualxTFilter) saveToQualxT(f);
}

QList<Hkl> PeakListWidget::getDataAll(const global::PositionUnit &posUnit) const
{
    QModelIndexList l;

    for (int r = 0; r < peakDataModel->rowCount(); ++r) {
        l.append(peakDataModel->index(r, 0));
    }

    return getHklFromModel(peakDataModel, l, posUnit);
}

QList<Hkl> PeakListWidget::getDataFiltered(const global::PositionUnit &posUnit) const
{
    QModelIndexList l;

    for (int r = 0; r < peakDataProxy->rowCount(); ++r) {
        l.append(peakDataProxy->index(r, 0));
    }

    return getHklFromModel(peakDataProxy, l, posUnit);
}

QList<Hkl> PeakListWidget::getDataSelected(const global::PositionUnit &posUnit) const
{
    QModelIndexList l = ui->tableViewHklData->selectionModel()->selectedRows(0);
    return getHklFromModel(peakDataProxy, l, posUnit);
}

QList<Hkl> PeakListWidget::getHklFromModel(const QAbstractItemModel *modl,
                                           const QModelIndexList &idxList,
                                           const global::PositionUnit &posUnit) const
{
    QList<Hkl> lst;

    for (int i = 0; i < idxList.size(); ++i) {
        int r = idxList.at(i).row();
        QModelIndex idxPhase  = modl->index(r, columns.value("phase"));
        QModelIndex idxH      = modl->index(r, columns.value("h"));
        QModelIndex idxK      = modl->index(r, columns.value("k"));
        QModelIndex idxL      = modl->index(r, columns.value("l"));
        QModelIndex idx2T     = modl->index(r, columns.value("2theta"));
        QModelIndex idxDnm    = modl->index(r, columns.value("dnm"));
        // QModelIndex idxIntens = modl->index(r, columns.value("intensity"));
        QModelIndex idxIntRel = modl->index(r, columns.value("intensrel"));
        QModelIndex idxTex    = modl->index(r, columns.value("texture"));
        QModelIndex idxB1     = modl->index(r, columns.value("b1"));
        QModelIndex idxB2     = modl->index(r, columns.value("b2"));

        QString dataPhase = modl->data(idxPhase,  Qt::EditRole).toString();
        int dataH         = modl->data(idxH,      Qt::EditRole).toInt();
        int dataK         = modl->data(idxK,      Qt::EditRole).toInt();
        int dataL         = modl->data(idxL,      Qt::EditRole).toInt();
        double data2T     = modl->data(idx2T,     Qt::EditRole).toDouble();
        double dataDnm    = modl->data(idxDnm,    Qt::EditRole).toDouble();
        // double dataIntens = modl->data(idxIntens, Qt::EditRole).toDouble();
        double dataIntRel = modl->data(idxIntRel, Qt::EditRole).toDouble();
        double dataTex    = modl->data(idxTex,    Qt::EditRole).toDouble();
        double dataB1     = modl->data(idxB1,     Qt::EditRole).toDouble();
        double dataB2     = modl->data(idxB2,     Qt::EditRole).toDouble();

        double pos = 0.0;

        switch (posUnit) {
        case global::PositionUnit::TWOTHETA:
            pos = data2T;
            break;
        case global::PositionUnit::DNM:
            pos = dataDnm;
            break;
        case global::PositionUnit::DANGSTROM:
            pos = 10.0 * dataDnm;
            break;
        default:
            pos = data2T;
        }

        Hkl hkl(pos, QString("%1%2%3").arg(dataH).arg(dataK).arg(dataL), 0, dataPhase, QColor(), dataIntRel, dataTex, 1, dataB1, dataB2, 1.0);
        lst.append(hkl);
    }

    return lst;
}

QString PeakListWidget::getCsvDataAll(bool header) const
{
    QList<int> idxString(QList<int>() << columns.value("phase"));
    QList<int> idxInt(QList<int>() << columns.value("h") << columns.value("k") << columns.value("l"));

    QString tsep("\"" + settings->value("config/asciiFieldSeparator", " ").toString() + "\"");
    QString sep(settings->value("config/asciiFieldSeparator", " ").toString());
    QStringList out;
    if (header) out.append("\"File" + tsep + hklHeaderLabels.join(tsep) + "\"");

    // loop over the data model, not the proxy model, to capture all data
    for (int r = 0; r < peakDataModel->rowCount(); ++r) {
        QStringList line("\"" + scanControl->fileName() + "\"");
        for (int c = 0; c < peakDataModel->columnCount(); ++c) {
            QModelIndex idx = peakDataModel->index(r, c);

            if (idxString.contains(c))   line.append(QString("\"%1\"").arg(peakDataModel->data(idx, Qt::EditRole).toString()));
            else if (idxInt.contains(c)) line.append(QString("%1").arg(peakDataModel->data(idx, Qt::EditRole).toInt()));
            else                         line.append(QString("%1").arg(peakDataModel->data(idx, Qt::EditRole).toDouble(), 0, 'f', 9));
        }

        out.append(line.join(sep));
    }

    return out.join("\n");
}

QString PeakListWidget::getCsvDataFiltered(bool header) const
{
    QList<int> idxString(QList<int>() << columns.value("phase"));
    QList<int> idxInt(QList<int>() << columns.value("h") << columns.value("k") << columns.value("l"));

    QString tsep("\"" + settings->value("config/asciiFieldSeparator", " ").toString() + "\"");
    QString sep(settings->value("config/asciiFieldSeparator", " ").toString());
    QStringList out;
    if (header) out.append("\"File" + tsep + hklHeaderLabels.join(tsep) + "\"");

    // loop over the data model, not the proxy model, to capture all data
    for (int r = 0; r < peakDataProxy->rowCount(); ++r) {
        QStringList line("\"" + scanControl->fileName() + "\"");
        for (int c = 0; c < peakDataProxy->columnCount(); ++c) {
            QModelIndex idx = peakDataProxy->index(r, c);

            if (idxString.contains(c))   line.append(QString("\"%1\"").arg(peakDataProxy->data(idx, Qt::EditRole).toString()));
            else if (idxInt.contains(c)) line.append(QString("%1").arg(peakDataProxy->data(idx, Qt::EditRole).toInt()));
            else                         line.append(QString("%1").arg(peakDataProxy->data(idx, Qt::EditRole).toDouble(), 0, 'f', 9));
        }

        out.append(line.join(sep));
    }

    return out.join("\n");
}

QString PeakListWidget::getQualxDdata() const
{
    QStringList out;

    for (int r = 0; r < peakDataProxy->rowCount(); ++r) {
        QStringList line;
        QModelIndex idxD = peakDataProxy->index(r, columns.value("dnm"));
        QModelIndex idxI = peakDataProxy->index(r, columns.value("intensity"));
        line.append(QString("%1").arg(10.0 * peakDataProxy->data(idxD, Qt::EditRole).toDouble(), 0, 'f', 6));
        line.append(QString("%1").arg(peakDataProxy->data(idxI, Qt::EditRole).toDouble(), 0, 'f', 6));

        out.append(line.join(" "));
    }

    return out.join("\n");
}

QString PeakListWidget::getQualxTdata() const
{
    QStringList out;

    for (int r = 0; r < peakDataProxy->rowCount(); ++r) {
        QStringList line;
        QModelIndex idxT = peakDataProxy->index(r, columns.value("2theta"));
        QModelIndex idxI = peakDataProxy->index(r, columns.value("intensity"));
        line.append(QString("%1").arg(peakDataProxy->data(idxT, Qt::EditRole).toDouble(), 0, 'f', 6));
        line.append(QString("%1").arg(peakDataProxy->data(idxI, Qt::EditRole).toDouble(), 0, 'f', 6));

        out.append(line.join(" "));
    }

    return out.join("\n");
}

void PeakListWidget::saveToCsv(const QString &f)
{
    BgmnFileIO::writeTextFile(f, getCsvDataAll(true));
}

void PeakListWidget::saveToQualxD(const QString &f)
{

    BgmnFileIO::writeTextFile(f, getQualxDdata());
}

void PeakListWidget::saveToQualxT(const QString &f)
{
    BgmnFileIO::writeTextFile(f, getQualxTdata());
}

void PeakListWidget::clearHklSelection()
{
    ui->tableViewHklData->clearSelection();
    ui->tableViewHklData->setCurrentIndex(QModelIndex());

    for (int i = 0; i < scanControl->count(); ++i) {
        scanControl->getScan(i)->setAllHklDisplayStatus(2, 1);
    }

    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, this);
}

void PeakListWidget::resultSelected(QTreeWidgetItem *it)
{
    if (!it) return;
    emit resultsSelectionChanged(it->data(0, Qt::UserRole).toString());
}

/*
 * if a change to the data model was caused by this widget, we must initiate
 * an update of all views (e.g. after deleting peaks or reloading the peak data from disk)
 */
void PeakListWidget::reloadData()
{
    updateData();
    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, this);
}

void PeakListWidget::selectHkl(const QItemSelection &current, const QItemSelection &previous)
{
    Q_UNUSED (current);
    // do not use "current" to highlight the selected hkl indices, because it only contains
    // newly selected ones, which will cause inconsistent behaviour if an existing selection
    // is changed.
    QModelIndexList prevIdx = previous.indexes();
    QModelIndexList selcIdx = ui->tableViewHklData->selectionModel()->selectedIndexes();

    QSet<int> prevRows;
    QSet<int> currRows;

    int colScanUid = columns.value("phase");
    int colHklUid  = columns.value("2theta");

    // unselect deselected hkl indices by setting their status to "1"
    for (int i = 0; i < prevIdx.size(); ++i) {
        // only process each row once
        if (prevRows.contains(prevIdx.at(i).row())) continue;
        prevRows.insert(prevIdx.at(i).row());

        QUuid scanUid = peakDataProxy->data(prevIdx.at(i).siblingAtColumn(colScanUid), Qt::UserRole).toUuid();
        QUuid hklUid  = peakDataProxy->data(prevIdx.at(i).siblingAtColumn(colHklUid),  Qt::UserRole).toUuid();

        Scan *scan = scanControl->getScan(scanUid);

        if (scan) {
            Hkl *hkl = scan->getHkl(hklUid);
            if (hkl) {
                if (hkl->status() == 2) hkl->setStatus(1);
            }
        }
    }

    // select selected hkl indices by setting their status to "2"
    for (int i = 0; i < selcIdx.size(); ++i) {
        // only process each row once
        if (currRows.contains(selcIdx.at(i).row())) continue;
        currRows.insert(selcIdx.at(i).row());

        QUuid scanUid = peakDataProxy->data(selcIdx.at(i).siblingAtColumn(colScanUid), Qt::UserRole).toUuid();
        QUuid hklUid  = peakDataProxy->data(selcIdx.at(i).siblingAtColumn(colHklUid),  Qt::UserRole).toUuid();

        Scan *scan = scanControl->getScan(scanUid);

        if (scan) {
            Hkl *hkl = scan->getHkl(hklUid);
            if (hkl) hkl->setStatus(2);
        }
    }

    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, this);
}

void PeakListWidget::removeCurrentHkl()
{
    bool oldState = ui->tableViewHklData->blockSignals(true);

    QItemSelectionModel *selected = ui->tableViewHklData->selectionModel();
    QModelIndexList indexList = selected->selectedIndexes();

    int cPhase = columns.value("phase");
    int c2theta = columns.value("2theta");

    for (int i = indexList.count() - 1; i >= 0; i--) {
        const QStandardItem *iPhase = peakDataModel->item(indexList.at(i).row(), cPhase);
        const QStandardItem *iHkl   = peakDataModel->item(indexList.at(i).row(), c2theta);

        QUuid uidScan = iPhase ? iPhase->data(Qt::UserRole).toUuid() : QUuid();
        QUuid uidHkl  = iHkl   ? iHkl->data(Qt::UserRole).toUuid()   : QUuid();

        Scan *scan = scanControl->getScan(uidScan);

        if (scan) {
            Hkl *hkl = scan->getHkl(uidHkl);
            if (hkl) hkl->setStatus(0);
        }

        peakDataModel->removeRow(indexList.at(i).row());
    }

    scanControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY, this);
    ui->tableViewHklData->blockSignals(oldState);
}

/*
 * always returns a list of size 3
 */
QList<int> PeakListWidget::hklToInt(const QString &strHkl)
{
    static QRegularExpression rxSplit("\\s+");
    QStringList l = strHkl.split(rxSplit);
    QList<int> intHkl;

    for (int i = 0; i < 3; ++i) {
        if (i >= l.size()) intHkl.append(0);
        else               intHkl.append(l.at(i).toInt());
    }

    return intHkl;
}

int PeakListWidget::indexOfHkl(const Scan *scan, const QUuid &uid)
{
    if (!scan) return -1;

    for (int i = 0; i < scan->pDataHkl().size(); ++i) {
        if (uid == scan->pDataHkl().at(i).uid()) return i;
    }

    return -1;
}

void PeakListWidget::toggleColumnHidden(QAction *a)
{
    int c = a->data().toInt();
    ui->tableViewHklData->setColumnHidden(c, !a->isChecked());

    QList<QVariant> hiddenCols;
    for (int i = 0; i < ui->tableViewHklData->horizontalHeader()->count(); ++i) {
        if (ui->tableViewHklData->isColumnHidden(i)) hiddenCols.append(QVariant(i));
    }

    // storing an empty QList<QVariant> in the preferences seems to create an invalid settings entry
    if (hiddenCols.isEmpty()) {
        settings->setValue("peakListWidget/hiddenColumns", "");
    } else {
        settings->setValue("peakListWidget/hiddenColumns", hiddenCols);
    }
}

void PeakListWidget::headerResized()
{
    settings->setValue("peakListWidget/hklHeader", ui->tableViewHklData->horizontalHeader()->saveState());
}

void PeakListWidget::splitterResized(int, int)
{
    settings->setValue("peakListWidget/splitter", ui->splitter->saveState());
}

void PeakListWidget::toggleFilterWidget()
{
    if (ui->splitter->sizes().at(0) == 0) {
        if (previousSplitterSizes.size() < 2) {
            ui->splitter->setSizes(QList<int>() << height() / 4 << 3 * height() / 4);
        } else {
            ui->splitter->setSizes(previousSplitterSizes);
        }
    } else {
        previousSplitterSizes = ui->splitter->sizes();
        ui->splitter->setSizes(QList<int>() << 0 << height());
    }

    settings->setValue("peakListWidget/splitter", ui->splitter->saveState());
}

void PeakListWidget::showHelp()
{
    emit helpText("peakListWidget");
}

QDomElement PeakListWidget::getFilterParameters() const
{
    return ui->peakListFilterForm->getFilterParameters();
}

void PeakListWidget::setFilterParameters(const QDomElement &e)
{
    ui->peakListFilterForm->setFilterParameters(e);

    if (ui->splitter->sizes().at(0) == 0) {
        if (previousSplitterSizes.size() < 2) {
            ui->splitter->setSizes(QList<int>() << height() / 4 << 3 * height() / 4);
        } else {
            ui->splitter->setSizes(previousSplitterSizes);
        }
    }
}

void PeakListWidget::getPreset(QDomDocument &doc)
{
    doc.firstChild().appendChild(ui->peakListFilterForm->getFilterParameters());
}

void PeakListWidget::applyPreset(const QDomElement &e)
{
    setFilterParameters(e);
}
