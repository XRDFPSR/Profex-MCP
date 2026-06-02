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
#include "ui_mainwindow.h"
#include "helpaboutdialog.h"
#include "../../libXrdIO/import/importhandler.h"
#include "filenamefilterdialog.h"

#include <QGuiApplication>
#include <QMessageBox>
#include <QMainWindow>
#include <QFileDialog>
#include <QInputDialog>
#include <QDir>
#include <QFile>
#include <QVector>
#include <QDateTime>
#include <QMenu>

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    settings = SettingsManager::getInstance();
    settings->initialize();

    QString iconTheme = settings->value("Gui/iconTheme", QString()).toString();

    if (iconTheme.isEmpty()) {
        if (settings->isDarkMode()) iconTheme = QString("profex-dark");
        else                        iconTheme = QString("profex-light-colored");
    }

    QIcon::setThemeName(iconTheme);

    ui->setupUi(this);
    setWindowTitle(QString("Profex Waterfall Plots"));

    monDirDlg = nullptr;
    _workingDir = QString();

    ui->treeWidgetScans->setColumnCount(2);
    ui->treeWidgetScans->setHeaderLabels(QStringList() << tr("Scan") << tr("Label"));
    ui->treeWidgetScans->setContextMenuPolicy(Qt::CustomContextMenu);

    statusCoordinates = new QLabel(statusBar());
    statusCoordinates->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    statusCoordinates->setMinimumWidth(statusCoordinates->fontMetrics().horizontalAdvance("MMMMMMMMMM.MMM 000.00 MMM"));
    statusBar()->addPermanentWidget(statusCoordinates, 0);

    statusMonitorDir = new QLabel(statusBar());
    statusMonitorDir->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    statusBar()->insertPermanentWidget(0, statusMonitorDir, 1);

    statusMonitorStatus = new QLabel(statusBar());
    statusMonitorStatus->setFrameStyle(QFrame::StyledPanel | QFrame::Sunken);
    statusMonitorStatus->setMinimumWidth(statusMonitorStatus->fontMetrics().horizontalAdvance(tr("Monitoring")));
    statusBar()->insertPermanentWidget(1, statusMonitorStatus);

    restoreState(settings->value("Gui/state", QByteArray()).toByteArray(), 0);
    restoreGeometry(settings->value("Gui/geometry", QByteArray()).toByteArray());

    ui->plotWidget->setScanData(&_scanHeap);
    ui->plotWidget->setUidData(&_uidList);

    ui->comboBoxColorTemp->addItem(tr("Warm"));
    ui->comboBoxColorTemp->addItem(tr("Cold"));

    ui->spinBoxYrangeMin->setValue(0);
    ui->spinBoxYrangeMax->setValue(1);

    // enable drag-n-drop in the treewidget for manual item sorting
    ui->treeWidgetScans->setDragEnabled(true);
    ui->treeWidgetScans->setAcceptDrops(true);
    ui->treeWidgetScans->setDropIndicatorShown(true);
    ui->treeWidgetScans->setDragDropMode(QAbstractItemView::InternalMove);

    // enable drag-n-drop in the mainwindow for adding scan files
    this->setAcceptDrops(true);

    // custom activation of sorting, necessary to call a function after sorting the items
    ui->treeWidgetScans->setSortingEnabled(false);
    ui->treeWidgetScans->header()->setSortIndicatorShown(true);
    ui->treeWidgetScans->header()->setSectionsClickable(true);
    connect(ui->treeWidgetScans->header(), SIGNAL(sectionClicked(int)), this, SLOT(sortFiles(int)));

    LutGenerator lutGen(256, false);
    lutList = lutGen.getLuts(true, false, false, false, false);

    for (int i = 0; i < lutList.size(); ++i) {
        ui->comboBoxColorScale->addItem(lutList.keys().at(i));
    }

    twContextMenu = new QMenu(this);
    twContextMenu->addAction(QIcon::fromTheme("profex-select-none"), tr("Clear selection"), this, SLOT(clearSelection()));
    twContextMenu->addSeparator();
    twContextMenu->addAction(QIcon::fromTheme("profex-document-close"), tr("Remove selected scans"), this, SLOT(removeSelected()));
    twContextMenu->addSeparator();
    twContextMenu->addAction(tr("Check all"), this, SLOT(treeWidgetCheckAll()));
    twContextMenu->addAction(tr("Uncheck all"), this, SLOT(treeWidgetUncheckAll()));
    twContextMenu->addSeparator();
    twContextMenu->addAction(tr("Check selected"), this, SLOT(treeWidgetCheckSelected()));
    twContextMenu->addAction(tr("Uncheck selected"), this, SLOT(treeWidgetUncheckSelected()));

    initSettings();

    connect(ui->plotWidget, SIGNAL(sigMouseCoordinates(QString)), this, SLOT(mouseCoordinates(QString)));
    connect(ui->plotWidget, SIGNAL(sigXrangeWasZoomed()), this, SLOT(updateXRangeSpinboxes()));
    connect(ui->comboBoxColorScale, SIGNAL(currentIndexChanged(int)), this, SLOT(lutChanged()));
    connect(ui->comboBoxColorTemp, SIGNAL(currentIndexChanged(int)), this, SLOT(colorTempChanged()));
    connect(&fsWatcher, SIGNAL(directoryChanged(const QString &)), this, SLOT(dirContentChanged(const QString &)));

    connect(ui->treeWidgetScans, SIGNAL(itemChanged(QTreeWidgetItem *, int)), this, SLOT(updateScene()));
    connect(ui->treeWidgetScans, SIGNAL(itemSelectionChanged()), this, SLOT(itemSelectionChanged()));
    connect(ui->treeWidgetScans, SIGNAL(customContextMenuRequested(QPoint)), this, SLOT(showContextMenu(QPoint)));
    connect(ui->treeWidgetScans->model(), SIGNAL(rowsInserted(const QModelIndex &, int, int)), this, SLOT(updateScene()));
}

MainWindow::~MainWindow()
{
    delete ui;
    settings->destroy();
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
    if (ui->actionMonitor->isChecked()) {
        monitorDir(false);
    }

    saveSettings();
    settings->sync();
    e->accept();
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
    loadFileList(l);
}

void MainWindow::initSettings()
{
    if (settings->format() == QSettings::NativeFormat)  qDebug() << QString("MainWindow::initSettings(): Settings format Native");
    if (settings->format() == QSettings::IniFormat)     qDebug() << QString("MainWindow::initSettings(): Settings format INI");
    if (settings->format() == QSettings::InvalidFormat) qDebug() << QString("MainWindow::initSettings(): Settings format Invalid");
    qDebug() << QString("MainWindow::initSettings(): Reading settings from %1").arg(settings->fileName());

    restoreGeometry(settings->value("Gui/geometry", QByteArray()).toByteArray());

    QByteArray splArr = settings->value("Gui/splitterV", QByteArray()).toByteArray();

    if (splArr.isEmpty()) {
        QList<int> l = QList<int>() << width() / 4 << 3 * width() / 4;
        ui->splitterV->setSizes(l);
    } else {
        ui->splitterV->restoreState(splArr);
    }


    ui->comboBoxColorScale->setCurrentIndex(settings->value("Plot/lut", 0).toInt());
    ui->comboBoxColorTemp->setCurrentIndex(settings->value("Plot/colorTemp", 0).toInt());

    ui->actionHorizontal_grid->setChecked(settings->value("Plot/drawGridH", false).toBool());
    ui->actionVertical_grid->setChecked(settings->value("Plot/drawGridV", false).toBool());
    ui->actionSmooth_interpolation->setChecked(settings->value("Plot/smoothInterpolation", false).toBool());
    ui->actionDraw_scan_names->setChecked(settings->value("Plot/drawScanNames", true).toBool());

    ui->treeWidgetScans->header()->restoreState(settings->value("Gui/treeHeader", QByteArray()).toByteArray());
    _labelFilterRegexp = settings->value("Gui/labelRegEx", QString()).toString();
    _labelFilterPrefix = settings->value("Gui/labelPrefix", QString()).toString();
    _labelFilterSuffix = settings->value("Gui/labelSuffix", QString()).toString();
    ui->sliderGamma->setValue(settings->value("Plot/gamma", 10).toUInt());
    ui->actionHighlight_selected_scans->setChecked(settings->value("Gui/highlighSelected", false).toBool());

    if (_workingDir.isEmpty()) _workingDir = settings->value("Gui/workingdir", QDir::homePath()).toString();

    _monitorFileType = settings->value("Gui/monitorFileType", 0).toInt();
    _monitorFilter = settings->value("Gui/monitorFilter", "xy").toStringList();
    _monitorDir = settings->value("Gui/monitorDir", _workingDir).toString();
    statusMonitorDir->setText(QString("Monitoring: %1 (inactive)").arg(_monitorDir));

    ui->plotWidget->setDrawGridHorizontal(settings->value("Plot/drawGridH", false).toBool());
    ui->plotWidget->setDrawGridVertical(settings->value("Plot/drawGridV", false).toBool());
    ui->plotWidget->setSmoothInterpolation(settings->value("Plot/smoothInterpolation", false).toBool());
    ui->plotWidget->setShowScanLabels(settings->value("Plot/drawScanNames", true).toBool());

    ui->lutScaleWidget->setLut(lutList.value(ui->comboBoxColorScale->currentText()));
    ui->lutScaleWidget->setColorTemp(ui->comboBoxColorTemp->currentIndex());
    gammaChanged(ui->sliderGamma->value());
}

void MainWindow::saveSettings()
{
    settings->setValue("Gui/geometry", saveGeometry());
    settings->setValue("Gui/splitterV", ui->splitterV->saveState());
    settings->setValue("Gui/treeHeader", ui->treeWidgetScans->header()->saveState());
    settings->setValue("Plot/lut", ui->comboBoxColorScale->currentIndex());
    settings->setValue("Plot/colorTemp", ui->comboBoxColorTemp->currentIndex());
    settings->setValue("Plot/smoothInterpolation", ui->actionSmooth_interpolation->isChecked());
    settings->setValue("Plot/drawGridH", ui->actionHorizontal_grid->isChecked());
    settings->setValue("Plot/drawGridV", ui->actionVertical_grid->isChecked());
    settings->setValue("Plot/drawScanNames", ui->actionDraw_scan_names->isChecked());

    settings->setValue("Gui/monitorFileType", _monitorFileType);
    settings->setValue("Gui/labelRegEx", _labelFilterRegexp);
    settings->setValue("Gui/labelPrefix", _labelFilterPrefix);
    settings->setValue("Gui/labelSuffix", _labelFilterSuffix);
    settings->setValue("Gui/highlighSelected", ui->actionHighlight_selected_scans->isChecked());
    settings->setValue("Plot/gamma", ui->sliderGamma->value());
    settings->setValue("Gui/workingdir", _workingDir);

    settings->setValue("Gui/monitorFileType", _monitorFileType);
    settings->setValue("Gui/monitorFilter", _monitorFilter);
    settings->setValue("Gui/monitorDir", _monitorDir);
}

void MainWindow::sortFiles(int c)
{
    Qt::SortOrder order = ui->treeWidgetScans->header()->sortIndicatorOrder();
    ui->treeWidgetScans->sortItems(c, order);
    updateScene();
}

void MainWindow::addFile()
{
    ImportHandler importHandler;
    QStringList filters;
    filters = importHandler.filters();
    filters << tr("All Files (*.*)");

    QFileDialog fdlg(this, tr("Add Scan File"), _workingDir);
    fdlg.setAcceptMode(QFileDialog::AcceptOpen);
    fdlg.setNameFilters(filters);
    fdlg.setFileMode(QFileDialog::ExistingFiles);
    fdlg.setOption(QFileDialog::HideNameFilterDetails, false);

    if (curFilter.isEmpty()) curFilter = settings->value("Gui/lastOpenGraphFilter", "").toString();
    if (!curFilter.isEmpty()) fdlg.selectNameFilter(curFilter);

    if (fdlg.exec()) {
        QStringList fn = fdlg.selectedFiles();
        curFilter = fdlg.selectedNameFilter();
        settings->setValue("Gui/lastOpenGraphFilter", curFilter);

        if (fn.size()) {
            QFileInfo fi(fn.first());
            _workingDir = fi.absolutePath();
        }

        for (int i = 0; i < fn.size(); ++i) {
            appendFileScans(fn.at(i), curFilter);
        }

        updateLabels();
        updateScene();
    }
}

void MainWindow::loadFileList(const QStringList &l)
{
    for (int i = 0; i < l.size(); ++i) {
        appendFileScans(l.at(i));
    }

    updateLabels();
    updateScene();
}

void MainWindow::removeSelected()
{
    for (int i = ui->treeWidgetScans->topLevelItemCount() - 1; i >= 0; --i) {
        if (ui->treeWidgetScans->topLevelItem(i)->isSelected()) {
            QTreeWidgetItem *it = ui->treeWidgetScans->takeTopLevelItem(i);
            if (it) delete it;
        }
    }

    updateScene();
}

void MainWindow::removeUnchecked()
{
    for (int i = ui->treeWidgetScans->topLevelItemCount() - 1; i >= 0; --i) {
        if (ui->treeWidgetScans->topLevelItem(i)->checkState(0) == Qt::Unchecked) {
            QTreeWidgetItem *it = ui->treeWidgetScans->takeTopLevelItem(i);
            if (it) delete it;
        }
    }

    updateScene();
}

void MainWindow::clearScans()
{
    if (QMessageBox::question(this,
                              tr("Clear all scans"),
                              tr("Do you want to remove all scans from the plot?"))
            == QMessageBox::No) {
        return;
    }

    bool oldState = ui->treeWidgetScans->blockSignals(true);
    ui->treeWidgetScans->clear();
    ui->treeWidgetScans->blockSignals(oldState);

    _scanHeap.clear();
    _fileNames.clear();
    _uidList.clear();
    ui->plotWidget->clearScans();
}

void MainWindow::saveAs()
{
    QString f = QFileDialog::getSaveFileName(this, tr("Save graph"), _workingDir,
                                             QString("Pixel image (*.png *.PNG);;"
                                                     "Scalable vector graphics (*.svg *.SVG);;"
                                                     "Surfer 6 text grid (*.grd *.GRD)"));

    if (!f.isEmpty()) {
        int rasterW = settings->value("Plot/rasterResolutionWidth", 1536).toInt();
        int rasterH = settings->value("Plot/rasterResolutionHeight", 1024).toInt();
        ui->plotWidget->save(f, rasterW, rasterH);
    }
}

void MainWindow::appendFileScans(const QString &file, const QString &filter)
{
    QFileInfo fi(file);
    if (!fi.exists()) return;

    QVector<Scan> sh;

    ImportHandler iHandler;
    QString uid = filter.isNull() ? iHandler.uidByFileName(fi.absoluteFilePath()) : iHandler.uidByFilter(filter);
    iHandler.load(fi.absoluteFilePath(), uid, sh, true);

    for (int i = 0; i < sh.size(); ++i) {
        QUuid uid = QUuid::createUuid();

        _scanHeap.insert(uid, sh.at(i));
        _fileNames.insert(uid, fi.absoluteFilePath());

        QStringList lbls;
        lbls.append(sh.at(i).name());
        lbls.append(getLabel(sh.at(i).name()));

        QTreeWidgetItem *it = new QTreeWidgetItem(lbls);
        it->setData(0, Qt::UserRole, uid);
        it->setCheckState(0, Qt::Checked);
        it->setFlags(it->flags() | Qt::ItemIsUserCheckable );
        it->setFlags(it->flags() & ~Qt::ItemIsDropEnabled);

        ui->treeWidgetScans->addTopLevelItem(it);
    }
}

void MainWindow::updateScene()
{
    updateCheckedItemList();

    ui->plotWidget->blockUpdate(true);
    ui->plotWidget->setGamma(0.1 * double(ui->sliderGamma->value()));
    ui->plotWidget->setLut(lutList.value(ui->comboBoxColorScale->currentText()));
    ui->plotWidget->setColorTemp(ui->comboBoxColorTemp->currentIndex());
    ui->plotWidget->blockUpdate(false);
    ui->plotWidget->updateScanData();
    ui->lutScaleWidget->updateAll();
    updateXRangeSpinboxes();
    updateYRangeSpinboxes();
}

void MainWindow::updateCheckedItemList()
{
    _uidList.clear();

    for (int i = 0; i < ui->treeWidgetScans->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetScans->topLevelItem(i);

        if (it->checkState(0) == Qt::Unchecked) continue;

        QUuid uid = it->data(0, Qt::UserRole).toUuid();
        if (_scanHeap.contains(uid)) _uidList.append(uid);
    }
}

void MainWindow::lutChanged()
{
    ui->lutScaleWidget->setLut(lutList.value(ui->comboBoxColorScale->currentText()));
    ui->plotWidget->setLut(lutList.value(ui->comboBoxColorScale->currentText()));
}

void MainWindow::colorTempChanged()
{
    ui->lutScaleWidget->setColorTemp(ui->comboBoxColorTemp->currentIndex());
    ui->plotWidget->setColorTemp(ui->comboBoxColorTemp->currentIndex());
}

void MainWindow::gammaChanged(int n)
{
    double g = 0.1 * double(n);
    ui->plotWidget->setGamma(g);
    ui->lutScaleWidget->setGamma(g);
}

void MainWindow::updateXRangeSpinboxes()
{
    bool oldStateMin = ui->doubleSpinBoxXrangeMin->blockSignals(true);
    bool oldStateMax = ui->doubleSpinBoxXrangeMax->blockSignals(true);

    double min, lmin;
    double max, lmax;

    ui->plotWidget->getXrange(min, max);
    ui->plotWidget->getXlimits(lmin, lmax);

    ui->doubleSpinBoxXrangeMin->setValue(min);
    ui->doubleSpinBoxXrangeMax->setValue(max);

    ui->doubleSpinBoxXrangeMin->setMinimum(lmin);
    ui->doubleSpinBoxXrangeMin->setMaximum(max);
    ui->doubleSpinBoxXrangeMax->setMinimum(min);
    ui->doubleSpinBoxXrangeMax->setMaximum(lmax);

    ui->doubleSpinBoxXrangeMin->blockSignals(oldStateMin);
    ui->doubleSpinBoxXrangeMax->blockSignals(oldStateMax);
}

void MainWindow::updateYRangeSpinboxes()
{
    bool oldStateMin = ui->spinBoxYrangeMin->blockSignals(true);
    bool oldStateMax = ui->spinBoxYrangeMax->blockSignals(true);

    double min, lmin;
    double max, lmax;

    ui->plotWidget->getYrange(min, max);
    ui->plotWidget->getYlimits(lmin, lmax);

    ui->lutScaleWidget->setRange(min, max);

    ui->spinBoxYrangeMin->setValue(int(min));
    ui->spinBoxYrangeMax->setValue(int(max + 1.0));

    ui->spinBoxYrangeMin->setMinimum(0);
    ui->spinBoxYrangeMin->setMaximum(int(max));
    ui->spinBoxYrangeMax->setMinimum(int(min));
    ui->spinBoxYrangeMax->setMaximum(int(lmax + 1.0));

    ui->spinBoxYrangeMin->blockSignals(oldStateMin);
    ui->spinBoxYrangeMax->blockSignals(oldStateMax);
}

void MainWindow::xRangeChanged()
{
    ui->plotWidget->setXrange(ui->doubleSpinBoxXrangeMin->value(), ui->doubleSpinBoxXrangeMax->value());

    ui->doubleSpinBoxXrangeMin->setMaximum(ui->doubleSpinBoxXrangeMax->value() - 0.1);
    ui->doubleSpinBoxXrangeMax->setMinimum(ui->doubleSpinBoxXrangeMin->value() + 0.1);
}

void MainWindow::yRangeChanged()
{
    ui->lutScaleWidget->setRange(double(ui->spinBoxYrangeMin->value()), double(ui->spinBoxYrangeMax->value()));
    ui->plotWidget->setYrange(double(ui->spinBoxYrangeMin->value()), double(ui->spinBoxYrangeMax->value()));

    ui->spinBoxYrangeMin->setMaximum(ui->spinBoxYrangeMax->value() - 1);
    ui->spinBoxYrangeMax->setMinimum(ui->spinBoxYrangeMin->value() + 1);
}

void MainWindow::mouseCoordinates(QString s)
{
    statusCoordinates->setText(s);
}

void MainWindow::resetX()
{
    ui->plotWidget->resetXrange();
    updateXRangeSpinboxes();
}

void MainWindow::resetY()
{
    ui->plotWidget->resetYrange();
    updateYRangeSpinboxes();
}

void MainWindow::resetGamma()
{
    ui->sliderGamma->setValue(10);
}

void MainWindow::selectDir()
{
    if (!monDirDlg) {
        monDirDlg = new DirectoryMonitorSetupDialog(this);

        ImportHandler iHandler;
        QMap<QString, GenericImport *> fileTypes = iHandler.importers();

        QStringList fTypes;
        QList<QStringList> fExts;
        QMapIterator<QString, GenericImport *> iter(fileTypes);

        while (iter.hasNext()) {
            iter.next();
            fTypes.append(iter.value()->filter());
            fExts.append(iter.value()->extensions());
        }

        monDirDlg->setFileFormats(fTypes, fExts);
    }

    monDirDlg->setDir(_monitorDir);
    monDirDlg->setCurrentFormat(_monitorFileType);

    if (monDirDlg->exec() == QDialog::Accepted) {
        _monitorDir = monDirDlg->getDir();
        _monitorFilter = monDirDlg->getCurrentFilter();
        _monitorFileType = monDirDlg->getFormat();
    }

    statusMonitorDir->setText(QString("Monitoring: %1 (inactive)").arg(_monitorDir));
}

void MainWindow::monitorDir(bool b)
{
    if (b) {
        QDir dir(_monitorDir);

        if (dir.exists()) {
            fsWatcher.addPath(_monitorDir);
            statusMonitorDir->setText(QString("Monitoring: %1").arg(_monitorDir));
            dirContentChanged(_monitorDir);
            qDebug() << QString("WaterfallPlotDialog::monitorDir(): FileSystemWatcher started at %1").arg(_monitorDir);
        } else {
            qDebug() << QString("WaterfallPlotDialog::monitorDir(): Cannot start FileSystemWatcher at %1").arg(_monitorDir);
            qDebug() << QString("                                   Directory does not exist");
        }
    } else {
        fsWatcher.removePaths(fsWatcher.directories());
        statusMonitorDir->setText(QString("Monitoring: %1 (inactive)").arg(_monitorDir));
        qDebug() << QString("WaterfallPlotDialog::monitorDir(): FileSystemWatcher stopped");
    }
}

void MainWindow::dirContentChanged(const QString &s)
{
    bool doUpdate = false;
    QDir dir(s);
    QStringList filters;

    for (int i = 0; i < _monitorFilter.size(); ++i) {
        filters.append("*." + _monitorFilter.at(i));
    }

    QStringList files = dir.entryList(filters, QDir::Files | QDir::Readable);

    for (int i = 0; i < files.size(); ++i) {
        QFileInfo fi(s + QDir::separator() + files.at(i));

        if (!_fileNames.values().contains(fi.absoluteFilePath())) {
            appendFileScans(fi.absoluteFilePath(), QString());
            doUpdate = true;
        }
    }

    removeNonexistentFiles();

    if (doUpdate) updateScene();
}

void MainWindow::removeNonexistentFiles()
{
    for (int i = ui->treeWidgetScans->topLevelItemCount() - 1; i >= 0; --i) {
        QUuid uid = ui->treeWidgetScans->topLevelItem(i)->data(0, Qt::UserRole).toUuid();

        if (!QFile::exists(_fileNames.value(uid))) {
            QTreeWidgetItem *it = ui->treeWidgetScans->takeTopLevelItem(i);
            if (it) delete it;
        }
    }

    updateScene();
}

void MainWindow::updateLabels()
{
    bool oldState = ui->treeWidgetScans->blockSignals(true);

    for (int i = 0; i < ui->treeWidgetScans->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetScans->topLevelItem(i);

        if (!it) continue;
        QUuid uid = it->data(0, Qt::UserRole).toUuid();
        QString l = getLabel(it->text(0));

        it->setText(1, l);
        _scanHeap[uid].setAuxInfo("waterfallPlotLabel", l);
    }

    ui->treeWidgetScans->blockSignals(oldState);
    updateScene();
}

QString MainWindow::getLabel(const QString &s)
{
    if (_labelFilterRegexp.isEmpty()) return _labelFilterPrefix + s + _labelFilterSuffix;

    QRegularExpression rx(_labelFilterRegexp);
    QRegularExpressionMatch rm = rx.match(s);
    QString l = s;

    if (rm.hasMatch()) {
        if (rx.captureCount() <= 0) l = rm.captured(0);
        else                        l = rm.captured(1);
    }

    return _labelFilterPrefix + l + _labelFilterSuffix;
}

void MainWindow::itemSelectionChanged()
{
    QList<QUuid> idx;

    if (ui->actionHighlight_selected_scans->isChecked()) {
        for (int i= 0; i < ui->treeWidgetScans->topLevelItemCount(); ++i) {
            QTreeWidgetItem *it = ui->treeWidgetScans->topLevelItem(i);
            if (!it->isSelected()) continue;

            idx.append(it->data(0, Qt::UserRole).toUuid());
        }

        ui->plotWidget->highlightScans(idx);
    } else {
        if (ui->plotWidget->hasHighlights()) {
            ui->plotWidget->highlightScans(idx);
        }
    }
}

void MainWindow::itemsSorted()
{
    updateScene();
}

void MainWindow::showContextMenu(const QPoint &p)
{
    twContextMenu->popup(ui->treeWidgetScans->viewport()->mapToGlobal(p));
}

void MainWindow::treeWidgetCheckSelected()
{
    bool oldState = ui->treeWidgetScans->blockSignals(true);

    for (int i = 0; i < ui->treeWidgetScans->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetScans->topLevelItem(i);
        if (it->isSelected()) it->setCheckState(0, Qt::Checked);
    }

    ui->treeWidgetScans->blockSignals(oldState);
    updateScene();
}

void MainWindow::treeWidgetUncheckSelected()
{
    bool oldState = ui->treeWidgetScans->blockSignals(true);

    for (int i = 0; i < ui->treeWidgetScans->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetScans->topLevelItem(i);
        if (it->isSelected()) it->setCheckState(0, Qt::Unchecked);
    }

    ui->treeWidgetScans->blockSignals(oldState);
    updateScene();
}

void MainWindow::treeWidgetCheckAll()
{
    bool oldState = ui->treeWidgetScans->blockSignals(true);

    for (int i = 0; i < ui->treeWidgetScans->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetScans->topLevelItem(i);
        it->setCheckState(0, Qt::Checked);
    }

    ui->treeWidgetScans->blockSignals(oldState);
    updateScene();
}

void MainWindow::treeWidgetUncheckAll()
{
    bool oldState = ui->treeWidgetScans->blockSignals(true);

    for (int i = 0; i < ui->treeWidgetScans->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetScans->topLevelItem(i);
        it->setCheckState(0, Qt::Unchecked);
    }

    ui->treeWidgetScans->blockSignals(oldState);
    updateScene();
}

void MainWindow::scanHighlightingToggled(bool)
{
    itemSelectionChanged();
}

void MainWindow::toggleDrawScanNames(bool b)
{
    ui->plotWidget->setShowScanLabels(b);
}

void MainWindow::setLabelFilter()
{
    FileNameFilterDialog *fnFilterDlg = new FileNameFilterDialog(this);

    QStringList fnames;

    for (int i = 0; i < ui->treeWidgetScans->topLevelItemCount(); ++i) {
        fnames.append(ui->treeWidgetScans->topLevelItem(i)->text(0));
    }

    fnFilterDlg->setRegExp(_labelFilterRegexp);
    fnFilterDlg->setPrefix(_labelFilterPrefix);
    fnFilterDlg->setSuffix(_labelFilterSuffix);
    fnFilterDlg->setFileNames(fnames);

    if (fnFilterDlg->exec() == QDialog::Accepted) {
        _labelFilterRegexp = fnFilterDlg->getRegExp();
        _labelFilterPrefix = fnFilterDlg->getPrefix();
        _labelFilterSuffix = fnFilterDlg->getSuffix();
        updateLabels();
    }

    delete fnFilterDlg;
}

void MainWindow::clearSelection()
{
    ui->treeWidgetScans->clearSelection();
}

void MainWindow::helpAbout()
{
    HelpAboutDialog *hdlg = new HelpAboutDialog(this);
    hdlg->setVersion(QString("%1.%2.%3").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD));
    hdlg->setLogDestination(logDest);
    hdlg->exec();
    delete hdlg;
}

/* EOF */




