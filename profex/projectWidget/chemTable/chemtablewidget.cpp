/***************************************************************************
                          chemtablewidget.cpp  -  description
                             -------------------
    begin                : Mon Jun 30 14:16:07 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#include "ui_chemtablewidgetform.h"
#include "chemtablewidget.h"
#include "../libXrdIO/functions.h"
#include <QDebug>
#include <QHeaderView>
#include <QMimeData>
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QLocale>
#include <QActionGroup>

ChemTableWidget::ChemTableWidget(GraphDataController *c, QWidget *parent) :
    AbstractGraphView(c, parent),
    ui(new Ui::ChemTableWidgetForm)
{
    ui->setupUi(this);
    isShown = false;

    ui->tableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->tableWidget->setSelectionMode(QAbstractItemView::ContiguousSelection);
    ui->tableWidget->setSelectionBehavior(QAbstractItemView::SelectItems);

    this->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->tableWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    auto *actCopy = addAction(tr("Copy table"), global::Functions::keyCopy(), this, &ChemTableWidget::copy);
    actCopy->setShortcutContext(Qt::WidgetWithChildrenShortcut);

    contextMenu = new QMenu(this);
    contextMenu->addAction(actCopy);
    contextMenu->addSeparator();

    QActionGroup *group = new QActionGroup(this);
    group->setExclusive(true);  // only one checked at a time

    actSetElement = new QAction(tr("Elements by weight-%"), this);
    actSetAtomic  = new QAction(tr("Elements by atom-%"), this);
    actSetOxide   = new QAction(tr("Oxides by weight-%"), this);

    // Make them checkable
    actSetElement->setCheckable(true);
    actSetAtomic->setCheckable(true);
    actSetOxide->setCheckable(true);

    // Add to the group (so only one can be checked)
    group->addAction(actSetElement);
    group->addAction(actSetAtomic);
    group->addAction(actSetOxide);

    // Add actions to the menu
    contextMenu->addAction(actSetElement);
    contextMenu->addAction(actSetAtomic);
    contextMenu->addAction(actSetOxide);

    // Connect to handle selection changes
    connect(group, &QActionGroup::triggered,
            this, &ChemTableWidget::toggleMode);
    connect(this,  &QWidget::customContextMenuRequested,
            this,  &ChemTableWidget::showContextMenu);
    connect(ui->tableWidget, &QTableWidget::customContextMenuRequested,
            this,  &ChemTableWidget::showContextMenu);
}

ChemTableWidget::~ChemTableWidget()
{
    if (contextMenu) delete contextMenu;
    if (ui) delete ui;
}

void ChemTableWidget::initSettings()
{
    tableData.initSettings();
    mode = static_cast<ChemistryMode>(settings->value("chemistry/mode", 0).toInt());

    QSignalBlocker block1(actSetElement);
    QSignalBlocker block2(actSetAtomic);
    QSignalBlocker block3(actSetOxide);

    if (mode == ChemistryMode::ELEMENT) {
        actSetElement->setChecked(true);
        actSetAtomic->setChecked(false);
        actSetOxide->setChecked(false);
    } else if (mode == ChemistryMode::ATOMIC) {
        actSetElement->setChecked(false);
        actSetAtomic->setChecked(true);
        actSetOxide->setChecked(false);
    } else if (mode == ChemistryMode::OXIDE) {
        actSetElement->setChecked(false);
        actSetAtomic->setChecked(false);
        actSetOxide->setChecked(true);
    }

    if (!lstFileName.isEmpty() && !savFileName.isEmpty()) {
        setData(savFileName, lstFileName, false);
    }
}

bool ChemTableWidget::hasData()
{
    return ((ui->tableWidget->rowCount() > 0) && (ui->tableWidget->columnCount() > 0)) ? true : false;
}

void ChemTableWidget::updateView()
{
    /* We do not react to view updates. Call ::setData() manually
     * if new data is available */
}

void ChemTableWidget::setShown(bool b)
{
    isShown = b;

    if (isShown && !lstFileName.isEmpty() && !savFileName.isEmpty()) {
        setData(savFileName, lstFileName, false);
    }
}

void ChemTableWidget::setData(const QString &savFile, const QString &lstFile, bool force)
{
    tableData.setData(savFile, lstFile, isShown || force);
    lstFileName = tableData.getLstFileName();
    savFileName = tableData.getSavFileName();
    sampleName = tableData.getSampleName();
    updateTable();
}

void ChemTableWidget::setData(const BgmnSavParser &sp, const BgmnLstParser &lp, bool force)
{
    tableData.setData(sp, lp, isShown || force);
    lstFileName = tableData.getLstFileName();
    savFileName = tableData.getSavFileName();
    sampleName = tableData.getSampleName();
    updateTable();
}

bool ChemTableWidget::reload()
{
    return tableData.reload();
}

void ChemTableWidget::updateTable()
{
    if (!tableData.hasData(mode)) return;

    QStringList vHeader, hHeader;
    QList<QStringList> tableContent = tableData.getTableData(mode, vHeader, hHeader);

    QTableWidget *table = ui->tableWidget;

    // freeze the UI
    table->setUpdatesEnabled(false);
    table->setSortingEnabled(false);
    QSignalBlocker b1(table);
    QSignalBlocker b2(table->horizontalHeader());
    QSignalBlocker b3(table->verticalHeader());

    // update the table structure
    table->clearContents();
    table->setColumnCount(hHeader.size());
    table->setRowCount(vHeader.size());
    table->setHorizontalHeaderLabels(hHeader);
    table->setVerticalHeaderLabels(vHeader);

    QColor bgCol = QColor(209, 233, 248);
    bgCol = settings->isDarkMode() ? global::Functions::colorToDarkMode(bgCol) : bgCol;

    // fill the cells
    for (int r = 0; r < tableContent.size(); ++r) {
        const QStringList &line = tableContent[r];

        for (int c = 0; c < line.size(); ++c) {
            QTableWidgetItem *item = new QTableWidgetItem(tableContent.at(r).at(c));
            item->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
            if (r == tableContent.size() - 1) item->setBackground(bgCol);
            table->setItem(r, c, item);
        }
    }

    // thaw the UI
    QFontMetrics fm(table->font());
    int minWidth = fm.horizontalAdvance("Pr6O11 (wt-%)");

    table->setUpdatesEnabled(true);
    table->horizontalHeader()->setMinimumSectionSize(minWidth);
    table->resizeColumnsToContents();
    table->viewport()->update();
}

void ChemTableWidget::clearAll()
{
    ui->tableWidget->clear();
    ui->tableWidget->setColumnCount(0);
    ui->tableWidget->setRowCount(0);
}

/*
 * Returns the table in csv ";" list format. One parameter per line.
 * The header can be switched on or off, so the output of several
 * projects can be merged without header lines interrupting the lines
 */
QStringList ChemTableWidget::getCsvList(bool header, const QString &sampleId)
{
    return tableData.getCsvList(mode, header, sampleId);
}


/*
 * Returns the table as csv (;) table sorted as shown on screen.
 */
QString ChemTableWidget::getCsvTable()
{
    return tableData.getCsvTable(mode);
}

/*
 * Returns the table in Html table format as shown on screen.
 */
QString ChemTableWidget::getHtmlTable()
{
    return tableData.getHtmlTable(mode);
}

void ChemTableWidget::showContextMenu(const QPoint &pos)
{
    QWidget *src = qobject_cast<QWidget*>(sender());
    if (!src) src = this;

    // make sure the menu is placed correctly, also when called over the table
    QPoint globalPos = (src == ui->tableWidget || src == ui->tableWidget->viewport())
                           ? ui->tableWidget->viewport()->mapToGlobal(pos)
                           : src->mapToGlobal(pos);
    contextMenu->exec(globalPos);
}

void ChemTableWidget::copy()
{
    QMimeData *mime = new QMimeData();
    mime->setText(getCsvTable());
    qApp->clipboard()->setMimeData(mime);
}

void ChemTableWidget::toggleMode(QAction *act)
{
    ChemistryMode nMode;

    if (act == actSetElement)     nMode = ELEMENT;
    else if (act == actSetAtomic) nMode = ATOMIC;
    else if (act == actSetOxide)  nMode = OXIDE;
    else return;

    settings->setValue("chemistry/mode", nMode);
    initSettings();
}

