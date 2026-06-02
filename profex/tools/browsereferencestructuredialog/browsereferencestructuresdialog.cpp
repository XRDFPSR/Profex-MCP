/***************************************************************************
                          browsereferencestructuresdialog.cpp  -  description
                             -------------------
    begin                : Tue 16 21:00:00 CEST 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#include "browsereferencestructuresdialog.h"
#include "ui_browsereferencestructuresdialog.h"
#include "../../../libXrdIO/functions.h"
#include "projectWidget/syntaxHighlighter/bgmnhighlighter.h"
#include "../../../libXrdIO/bgmnfileio.h"
#include <QFileDialog>

BrowseReferenceStructuresDialog::BrowseReferenceStructuresDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::BrowseReferenceStructuresDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();

    initUi();
    initSettings();
}

BrowseReferenceStructuresDialog::~BrowseReferenceStructuresDialog()
{
    delete ui;
}

void BrowseReferenceStructuresDialog::showEvent(QShowEvent *e)
{
    itemsExpanded = true;
    setStrDir();
    e->accept();
}

void BrowseReferenceStructuresDialog::closeEvent(QCloseEvent *e)
{
    saveSettings();
    emit favoritesChanged();
    e->accept();
}

void BrowseReferenceStructuresDialog::initUi()
{
    refStrManager = BgmnRefStructureManager::getInstance();

    ui->lineEditCurrentFile->clear();
    ui->comboBoxWavelength->showKa2(true);
    ui->comboBoxWavelength->showKb(true);
    ui->comboBoxWavelength->initData();

    ui->plotWidget->setBackground(QBrush(QGuiApplication::palette().color(QPalette::Base)));

    QPen penAxis(settings->isDarkMode() ? global::Functions::colorToDarkMode(QColor(Qt::black)) : Qt::black);
    QPen penGrid(settings->isDarkMode() ? global::Functions::colorToDarkMode(QColor(Qt::lightGray)) : Qt::lightGray, 0, Qt::DotLine);
    ui->plotWidget->xAxis->setBasePen(penAxis);
    ui->plotWidget->xAxis->setTickPen(penAxis);
    ui->plotWidget->xAxis->setSubTickPen(penAxis);
    ui->plotWidget->xAxis->setLabelColor(penAxis.color());
    ui->plotWidget->xAxis->setTickLabelColor(penAxis.color());
    ui->plotWidget->xAxis->grid()->setPen(penGrid);
    ui->plotWidget->xAxis->grid()->setSubGridPen(penGrid);
    ui->plotWidget->xAxis->grid()->setVisible(false);
    ui->plotWidget->xAxis->grid()->setSubGridVisible(false);

    ui->plotWidget->yAxis->setBasePen(penAxis);
    ui->plotWidget->yAxis->setTickPen(penAxis);
    ui->plotWidget->yAxis->setSubTickPen(penAxis);
    ui->plotWidget->yAxis->setLabelColor(penAxis.color());
    ui->plotWidget->yAxis->setTickLabelColor(penAxis.color());
    ui->plotWidget->yAxis->grid()->setPen(penGrid);
    ui->plotWidget->yAxis->grid()->setSubGridPen(penGrid);
    ui->plotWidget->yAxis->grid()->setVisible(false);
    ui->plotWidget->yAxis->grid()->setSubGridVisible(false);

    QString xa = "Diffraction Angle [" + QString(global::degree) + "2" + QString(global::theta) + "]";
    ui->plotWidget->xAxis->setLabel(xa);
    ui->plotWidget->yAxis->setLabel("Intensity [%]");

    QStringList tabHeader;
    tabHeader << "h" << "k" << "l" << QString("2%1 [°]").arg(global::theta) << "d [nm]" << "Intensity [%]";
    ui->tableWidgetHkl->setColumnCount(tabHeader.size());
    ui->tableWidgetHkl->setHorizontalHeaderLabels(tabHeader);

    QStringList strHeader;
    strHeader << "File Name" << "Phase" << "Comment";
    ui->treeWidget->setColumnCount(strHeader.size());
    ui->treeWidget->setHeaderLabels(strHeader);

    ui->treeWidget->header()->setContextMenuPolicy(Qt::CustomContextMenu);
    contextMenuHeader = new QMenu(this);

    for (int i = 1; i < ui->treeWidget->header()->count(); ++i) {
        QAction *act = new QAction(ui->treeWidget->headerItem()->text(i), contextMenuHeader);
        act->setCheckable(true);
        act->setData(i);
        contextMenuHeader->addAction(act);
    }

    connect(contextMenuHeader, SIGNAL(triggered(QAction*)), this, SLOT(toggleColumnHidden(QAction*)));
    connect(ui->treeWidget->header(), SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(customMenuRequested(QPoint)));
    connect(ui->treeWidget, SIGNAL(currentItemChanged(QTreeWidgetItem*,QTreeWidgetItem*)), this, SLOT(currentChanged(QTreeWidgetItem*,QTreeWidgetItem*)));
}

void BrowseReferenceStructuresDialog::initSettings()
{
    restoreGeometry(settings->value("refStrBrowser/geometry", QByteArray()).toByteArray());
    QByteArray wdt = settings->value("refStrBrowser/colWidth", QByteArray()).toByteArray();

    if (!wdt.isEmpty()) {
        ui->treeWidget->header()->restoreState(wdt);
    } else {
        QFontMetrics fm(ui->treeWidget->font());
        int w = fm.size(Qt::TextSingleLine, "Hydroxylapatite.str").width();
        ui->treeWidget->header()->resizeSection(0, 2*w);
        ui->treeWidget->header()->resizeSection(1, w);
        ui->treeWidget->header()->resizeSection(2, w/2);
    }

    sortCol = settings->value("refStrBrowser/sortColumn", ui->treeWidget->sortColumn()).toInt();
    sortOrder = (Qt::SortOrder)settings->value("refStrBrowser/sortOrder", 0).toInt();
    ui->treeWidget->sortItems(sortCol, sortOrder);
    ui->splitterVertical->restoreState(settings->value("refStrBrowser/splitterV", QByteArray()).toByteArray());
    ui->splitterHorizontal->restoreState(settings->value("refStrBrowser/splitterH", QByteArray()).toByteArray());
    ui->comboBoxWavelength->setCurrentIndex(settings->value("refStrBrowser/currentWl", 12).toInt());

    QByteArray tabHeader = settings->value("refStrBrowser/tableHeader", QByteArray()).toByteArray();
    ui->tableWidgetHkl->horizontalHeader()->restoreState(tabHeader);

    QList<QVariant> hiddenCols;
    hiddenCols.append(QVariant(2)); // hide the comment column by default
    hiddenCols = settings->value("refStrBrowser/hiddenColumns", hiddenCols).toList();

    int n = qMin(ui->treeWidget->header()->count(), contextMenuHeader->actions().count() + 1);

    for (int i = 1; i < n; ++i) {
        bool b = hiddenCols.contains(QVariant(i));
        ui->treeWidget->setColumnHidden(i, b);
        contextMenuHeader->actions().at(i - 1)->setChecked(!b);
    }

    double wl = ui->comboBoxWavelength->currentData(Qt::UserRole).toDouble();
    double utt = global::Functions::dToTwoTheta(settings->value("config/hklUpperRangeD", wl).toDouble(), wl);
    ui->plotWidget->setXLimits(0.0, utt);
    ui->plotWidget->xAxis->setRange(0.0, utt);
    ui->plotWidget->yAxis->setRange(0.0, 100.0);

    BgmnHighlighter *bhl = new BgmnHighlighter(settings->getSyntaxHighlightingMode(), this);
    bhl->setDocument(ui->plainTextEdit->document());

    QFont edFont;
    edFont.fromString(settings->value("config/editorFont", font().toString()).toString());
    ui->plainTextEdit->setFont(edFont);
}

void BrowseReferenceStructuresDialog::saveSettings()
{
    saveFavorites();
    settings->setValue("refStrBrowser/geometry", saveGeometry());
    settings->setValue("refStrBrowser/sortColumn", ui->treeWidget->sortColumn());
    settings->setValue("refStrBrowser/sortOrder", (int)ui->treeWidget->header()->sortIndicatorOrder());
    settings->setValue("refStrBrowser/colWidth", ui->treeWidget->header()->saveState());
    settings->setValue("refStrBrowser/splitterV", ui->splitterVertical->saveState());
    settings->setValue("refStrBrowser/splitterH", ui->splitterHorizontal->saveState());
    settings->setValue("refStrBrowser/tableHeader", ui->tableWidgetHkl->horizontalHeader()->saveState());
    settings->setValue("refStrBrowser/currentWl", ui->comboBoxWavelength->currentIndex());

    QList<QVariant> hiddenCols;
    for (int i = 1; i < ui->treeWidget->header()->count(); ++i) {
        if (ui->treeWidget->isColumnHidden(i)) hiddenCols.append(QVariant(i));
    }

    settings->setValue("refStrBrowser/hiddenColumns", hiddenCols);
}

void BrowseReferenceStructuresDialog::customMenuRequested(QPoint p)
{
    contextMenuHeader->popup(ui->treeWidget->header()->viewport()->mapToGlobal(p));
}

void BrowseReferenceStructuresDialog::setStrDir()
{
    if (!refStrManager) return;
    QStringList repos = settings->value("bgmnProject/structureDatabaseIndexed", QStringList()).toStringList();
    repos.append(settings->value("bgmnProject/structureDatabaseNonIndexed", QStringList()).toStringList());

    refStrManager->createTree(ui->treeWidget, repos, false, Qt::ItemFlags(Qt::ItemIsAutoTristate | Qt::ItemIsUserCheckable | Qt::ItemIsEnabled));
    setFavorites();

    ui->treeWidget->sortItems(0, Qt::AscendingOrder);

    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        ui->treeWidget->topLevelItem(i)->setExpanded(true);
    }

    for (int i = 0; i < ui->treeWidget->columnCount(); ++i) {
        if (ui->treeWidget->columnWidth(i) == 0) {
            ui->treeWidget->resizeColumnToContents(i);
        }
    }
}

void BrowseReferenceStructuresDialog::currentChanged(QTreeWidgetItem *cur, QTreeWidgetItem *)
{
    clearDataWidgets();

    if (!cur) {
        ui->plotWidget->replot();
        return;
    }

    currentFile = cur->data(0, Qt::UserRole).toString();
    updateDataWidgets();
}

void BrowseReferenceStructuresDialog::clearDataWidgets()
{
    ui->lineEditCurrentFile->clear();

    ui->tableWidgetHkl->clearContents();
    ui->tableWidgetHkl->setRowCount(0);

    ui->plainTextEdit->clear();

    ui->plotWidget->clearGraphs();
    ui->plotWidget->clearPlottables();
}

void BrowseReferenceStructuresDialog::updateDataWidgets()
{
    if (refStrManager) {
        QVector<Hkl> vec = refStrManager->getReferenceLines(currentFile);
        double maxI = plotHkl(vec);
        updateTable(vec, maxI);
    }

    updateStrEditor();
    ui->lineEditCurrentFile->setText(currentFile);
}

double BrowseReferenceStructuresDialog::plotHkl(const QVector<Hkl> &vec)
{
    double maxI = 0.0;
    QVector<double> xdata(hklToTwoTheta(vec, ui->comboBoxWavelength->currentData(Qt::UserRole).toDouble()));
    QVector<double> ydata(hklToIntensity(vec, maxI));

    QCPBars *hklBars = new QCPBars(ui->plotWidget->xAxis, ui->plotWidget->yAxis);
    hklBars->setData(xdata, ydata);
    hklBars->setWidthType(QCPBars::wtAbsolute);
    hklBars->setPen(QPen(settings->isDarkMode() ? global::Functions::colorToDarkMode(QColor(Qt::blue)) : Qt::blue, 0.0));

    ui->plotWidget->replot();
    return maxI;
}

void BrowseReferenceStructuresDialog::updateTable(const QVector<Hkl> &vec, double maxI)
{
    double my = qFuzzyIsNull(maxI) ? 100.0 : maxI;
    double wl = ui->comboBoxWavelength->currentData(Qt::UserRole).toDouble();
    static QRegularExpression rxSplit("\\s+");

    for (int i = 0; i < vec.size(); ++i) {
        double d = vec.at(i).position();
        double tt = global::Functions::dToTwoTheta(d, wl);
        double intens = 100 * vec.at(i).intensity() / my;
        QStringList hkl = vec.at(i).hkl().split(rxSplit);

        int r = ui->tableWidgetHkl->rowCount();
        ui->tableWidgetHkl->setRowCount(r + 1);
        ui->tableWidgetHkl->setItem(r, 0, new QTableWidgetItem(hkl.size() > 0 ? hkl.at(0) : QString()));
        ui->tableWidgetHkl->setItem(r, 1, new QTableWidgetItem(hkl.size() > 1 ? hkl.at(1) : QString()));
        ui->tableWidgetHkl->setItem(r, 2, new QTableWidgetItem(hkl.size() > 2 ? hkl.at(2) : QString()));
        ui->tableWidgetHkl->setItem(r, 3, new QTableWidgetItem(QString("%1").arg(tt, 0, 'f', 5)));
        ui->tableWidgetHkl->setItem(r, 4, new QTableWidgetItem(QString("%1").arg(d, 0, 'f', 6)));
        ui->tableWidgetHkl->setItem(r, 5, new QTableWidgetItem(QString("%1").arg(intens, 0, 'f', 2)));
    }
}

void BrowseReferenceStructuresDialog::updateStrEditor()
{
    QString s(BgmnFileIO::readTextFile(currentFile));
    ui->plainTextEdit->setPlainText(s);
}

void BrowseReferenceStructuresDialog::toggleColumnHidden(QAction *a)
{
    int c = a->data().toInt();
    ui->treeWidget->setColumnHidden(c, !a->isChecked());
}

QVector<double> BrowseReferenceStructuresDialog::hklToTwoTheta(const QVector<Hkl> &vec, double wl)
{
    QVector<double> ttvec;

    for (int i = 0; i < vec.size(); ++i) {
        ttvec.append(global::Functions::dToTwoTheta(vec.at(i).position(), wl));
    }

    return ttvec;
}

QVector<double> BrowseReferenceStructuresDialog::hklToIntensity(const QVector<Hkl> &vec, double &maxI)
{
    QVector<double> ivec;
    double imax = 0.0;

    for (int i = 0; i < vec.size(); ++i) {
        imax = qMax(imax, vec.at(i).intensity());
    }

    for (int i = 0; i < vec.size(); ++i) {
        ivec.append(100.0 * vec.at(i).intensity() / imax);
    }

    maxI = imax;
    return ivec;
}

/*
 * shows / hides items according to the filter string. does not add / remove any items
 */
void BrowseReferenceStructuresDialog::applyFilter(const QString &s)
{
    if (s.simplified().isEmpty()) {
        unhideAllItems();
        return;
    }

    QRegularExpression rx;
    QRegularExpressionMatch rm;

    QRegularExpression::PatternOptions options = rx.patternOptions();

    rx.setPatternOptions(options | QRegularExpression::CaseInsensitiveOption);
    QString filter = s.simplified();

    if (!filter.contains(QString("*"))) {
        filter = QString("*%1*").arg(filter);
    }

    filter.replace(QString("*"), QString(".*"));
    filter.replace(QString("?"), QString("."));
    rx.setPattern(QString("^%1$").arg(filter));

    if (!rx.isValid()) {
        qDebug() << QString("BgmnAddRemoveDialog::applyFilter: Invalid regular expression (%1)").arg(rx.errorString());
        return;
    }

    QTreeWidgetItemIterator it(ui->treeWidget);

    while (*it) {
        if ((*it)->type() != TYPE_FILE) {
            ++it;
            continue;
        }

        int matches = 0;

        for (int i = 0; i < 3; ++i) {
            if (rx.match((*it)->text(i)).hasMatch()) {
                ++matches;
                break;
            }
        }

        (*it)->setHidden(matches == 0);
        ++it;
    }
}

void BrowseReferenceStructuresDialog::updateFilter()
{
    applyFilter(ui->lineEditFilter->text());
}

void BrowseReferenceStructuresDialog::unhideAllItems()
{
    QTreeWidgetItemIterator it(ui->treeWidget);

    while (*it) {
        if ((*it)->type() != TYPE_FILE) {
            ++it;
            continue;
        }

        if ((*it)->isHidden()) (*it)->setHidden(false);
        ++it;
    }
}

/*
 * returns the column text in column n of item it. If it.columnCount() is
 * < n, returns an empty string. not sure if QTreeWidgetItem::text(int) does the
 * same, but better safe than sorry.
 */
QString BrowseReferenceStructuresDialog::itemText(const QTreeWidgetItem *it, int n)
{
    int c = it->columnCount();
    if ((n >= c) || (n < 0)) return QString();
    return it->text(n);
}

void BrowseReferenceStructuresDialog::wavelengthChanged(int)
{
    clearDataWidgets();
    updateDataWidgets();
}

void BrowseReferenceStructuresDialog::setFavorites()
{
    if (!refStrManager) return;
    QSet<QString> favs = refStrManager->getFavFileNames();
    QTreeWidgetItemIterator itCheck(ui->treeWidget);

    while (*itCheck) {
        if (favs.contains((*itCheck)->data(0, Qt::UserRole).toString())) {
            (*itCheck)->setCheckState(0, Qt::Checked);
        }
        ++itCheck;
    }
}

void BrowseReferenceStructuresDialog::saveFavorites()
{
    if (!refStrManager) return;
    QStringList favs;
    QTreeWidgetItemIterator it(ui->treeWidget);

    while (*it) {
        if ((*it)->type() == TYPE_FILE) {
            if ((*it)->checkState(0) == Qt::Checked) {
                favs.append((*it)->data(0, Qt::UserRole).toString());
            }
        }
        ++it;
    }

    refStrManager->setFavorites(favs);
}

void BrowseReferenceStructuresDialog::savePeakData()
{
    if (!ui->tableWidgetHkl->rowCount()) return;
    QString dir = settings->value("config/workingdir", QDir::homePath()).toString();

    QString outStr(ui->lineEditCurrentFile->text() + "\n");
    outStr += QString("Wavelength;%1;nm\n").arg(ui->comboBoxWavelength->currentData(Qt::UserRole).toDouble());

    for (int r = -1; r < ui->tableWidgetHkl->rowCount(); ++r) {
        QStringList line;

        for (int c = 0; c < ui->tableWidgetHkl->columnCount(); ++c) {
            QTableWidgetItem *it = r < 0 ? ui->tableWidgetHkl->horizontalHeaderItem(c) :
                                           ui->tableWidgetHkl->item(r, c);
            if (it) line.append(it->text());
        }

        outStr.append(line.join(";") + "\n");
    }

    dir = QFileDialog::getSaveFileName(this, tr("Save peak data"), dir, QString("CSV File (*.csv *.CSV"));
    qDebug() << QString("BrowseReferenceStructuresDialog::savePeakData(): Saving peak data to %1").arg(dir);
    BgmnFileIO::writeTextFile(dir, outStr);
}

void BrowseReferenceStructuresDialog::saveStickPattern()
{
    QString dir = settings->value("config/workingdir", QDir::homePath()).toString();
    dir = QFileDialog::getSaveFileName(this, tr("Save stick pattern"), dir, QString("PDF File (*.pdf *.PDF"));
    ui->plotWidget->savePdf(dir);
}

void BrowseReferenceStructuresDialog::toggleExpand()
{
    itemsExpanded = !itemsExpanded;
    QTreeWidgetItemIterator it(ui->treeWidget);

    while (*it) {
        if ((*it)->type() == TYPE_DIRECTORY) {
            if (itemsExpanded) {
                (*it)->setExpanded(itemsExpanded);
            } else {
                // do not collapse toplevelitems
                if ((*it)->parent()) (*it)->setExpanded(itemsExpanded);
            }
        }

        ++it;
    }
}
