/***************************************************************************
                          baselinedialog.cpp  -  description
                             -------------------
    begin                : Tue Jan 23 22:00:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#include <QComboBox>
#include <QSpinBox>
#include <QShowEvent>
#include <QCloseEvent>
#include <QTableWidget>
#include <QDebug>
#include "../libXrdIO/structs.h"
#include "baselinedialog.h"
#include "ui_baselinedialog.h"

BaseLineDialog::BaseLineDialog(QWidget *parent) :
    AbstractToolDialog(parent),
    ui(new Ui::BaseLineDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Base Line"));

    ui->tableWidgetNodes->setEnabled(false);
    temporaryScan = nullptr;
    headers << tr("2theta") << tr("Intensity");

    ui->comboBoxGolotInterpolation->addItem(QString("Linear"),             QVariant(SPLINE_LINEAR));
    ui->comboBoxGolotInterpolation->addItem(QString("Catmull-Rom Spline"), QVariant(SPLINE_CATMULL_ROM));
    ui->comboBoxGolotInterpolation->addItem(QString("Cubic Spline"),       QVariant(SPLINE_CUBIC));
    ui->comboBoxGolotInterpolation->addItem(QString("Akima Spline"),       QVariant(SPLINE_AKIMA));
    ui->comboBoxGolotInterpolation->addItem(QString("Monotone Spline"),    QVariant(SPLINE_MONOTONE));

    ui->comboBoxManualInterpolation->addItem(QString("Linear"),             QVariant(SPLINE_LINEAR));
    ui->comboBoxManualInterpolation->addItem(QString("Catmull-Rom Spline"), QVariant(SPLINE_CATMULL_ROM));
    ui->comboBoxManualInterpolation->addItem(QString("Cubic Spline"),       QVariant(SPLINE_CUBIC));
    ui->comboBoxManualInterpolation->addItem(QString("Akima Spline"),       QVariant(SPLINE_AKIMA));
    ui->comboBoxManualInterpolation->addItem(QString("Monotone Spline"),    QVariant(SPLINE_MONOTONE));

    ui->comboBoxManualPosition->addItem(QString("Below noise"));
    ui->comboBoxManualPosition->addItem(QString("Centered on noise"));

#ifdef Q_OS_MACOS
    ui->labelManualAdd->setText(tr("Add anchor: Cmd + double click on graph"));
    ui->labelManualMove->setText(tr("Move anchor: Middle mouse button on graph"));
    ui->labelManualRemove->setText(tr("Remove anchor: Cmd + right mouse button on graph"));
#else
    ui->labelManualAdd->setText(tr("Add anchor: Ctrl + double click on graph"));
    ui->labelManualMove->setText(tr("Move anchor: Middle mouse button on graph"));
    ui->labelManualRemove->setText(tr("Remove anchor: Ctrl + right mouse button on graph"));
#endif

    initSettings();

    connect(ui->toolButtonAddAnchor, SIGNAL(clicked(bool)), this, SLOT(addAnchor()));
    connect(ui->toolButtonRemoveAnchor, SIGNAL(clicked(bool)), this, SLOT(removeAnchor()));
    connect(ui->comboBoxAlgo, SIGNAL(currentIndexChanged(int)), this, SLOT(algoChanged(int)));
    connect(ui->comboBoxScan, SIGNAL(currentIndexChanged(int)), this, SLOT(computeSnip()));

    connect(ui->comboBoxSnipWindow, SIGNAL(currentIndexChanged(int)), this, SLOT(computeSnip()));
    connect(ui->spinBoxSnipM, SIGNAL(valueChanged(int)), this, SLOT(computeSnip()));

    connect(ui->spinBoxGolotWindow, SIGNAL(valueChanged(int)), this, SLOT(computeGolot()));
    connect(ui->doubleSpinBoxGolotvinN, SIGNAL(valueChanged(double)), this, SLOT(computeGolot()));
    connect(ui->spinBoxGolotSteps, SIGNAL(valueChanged(int)), this, SLOT(computeGolot()));
    connect(ui->comboBoxGolotLast, SIGNAL(currentIndexChanged(int)), this, SLOT(computeGolot()));
    connect(ui->spinBoxGolotSmooth, SIGNAL(valueChanged(int)), this, SLOT(computeGolot()));
    connect(ui->comboBoxGolotInterpolation, SIGNAL(currentIndexChanged(int)), this, SLOT(computeGolot()));
    connect(ui->spinBoxGolotSensitivity, SIGNAL(valueChanged(int)), this, SLOT(computeGolot()));

    // connect(ui->spinBoxManualPoints, SIGNAL(valueChanged(int)), this, SLOT(computeManual()));
    // connect(ui->checkBoxManuallyMonotonically, SIGNAL(toggled(bool)), this, SLOT(computeManual()));
    connect(ui->comboBoxManualInterpolation, SIGNAL(currentIndexChanged(int)), this, SLOT(generateManual()));
    connect(ui->spinBoxManualWindow, SIGNAL(valueChanged(int)), this, SLOT(optimizeManual()));
    connect(ui->comboBoxManualPosition, SIGNAL(currentIndexChanged(int)), this, SLOT(optimizeManual()));
    connect(ui->comboBoxManualLast, SIGNAL(currentIndexChanged(int)), this, SLOT(optimizeManual()));

    connect(ui->tableWidgetNodes, SIGNAL(cellChanged(int,int)), this, SLOT(generateCurve()));
    connect(ui->tableWidgetNodes, SIGNAL(currentItemChanged(QTableWidgetItem *, QTableWidgetItem *)),
            this, SLOT(highlightAnchor(QTableWidgetItem *, QTableWidgetItem *)));
}

BaseLineDialog::~BaseLineDialog()
{
    delete ui;
}

void BaseLineDialog::initSettings()
{
    ui->splitter->restoreState(settings->value("baseLine/splitter", QByteArray()).toByteArray());
    ui->comboBoxAlgo->setCurrentIndex(settings->value("baseLine/algorithm", 0).toInt());
    ui->stackedWidgetParams->setCurrentIndex(ui->comboBoxAlgo->currentIndex());

    ui->tableWidgetNodes->setEnabled(ui->comboBoxAlgo->currentIndex() > 0);
    ui->toolButtonAddAnchor->setEnabled(ui->comboBoxAlgo->currentIndex() > 0);
    ui->toolButtonRemoveAnchor->setEnabled(ui->comboBoxAlgo->currentIndex() > 0);

    ui->comboBoxSnipWindow->setCurrentIndex(settings->value("baseLine/algoSnip/window", 1).toInt());
    ui->spinBoxSnipM->setValue(settings->value("baseLine/algoSnip/iterations", 60).toInt());

    ui->spinBoxGolotWindow->setValue(settings->value("baseLine/algoGolot/window", 3).toInt());
    ui->doubleSpinBoxGolotvinN->setValue(settings->value("baseLine/algoGolot/noiseMulti", 1.0).toDouble());
    ui->spinBoxGolotSteps->setValue(settings->value("baseLine/algoGolot/steps", 1).toInt());
    ui->comboBoxGolotLast->setCurrentIndex(settings->value("baseLine/algoGolot/mode", 0).toInt());
    ui->spinBoxGolotSmooth->setValue(settings->value("baseLine/algoGolot/dataSmooth", 1).toInt());
    ui->comboBoxGolotInterpolation->setCurrentIndex(settings->value("baseLine/algoGolot/interpolation", SPLINE_AKIMA).toInt());
    ui->spinBoxGolotSensitivity->setValue(settings->value("baseLine/algoGolot/sensitivity", 3).toInt());

    ui->comboBoxManualLast->setCurrentIndex(settings->value("baseLine/algoManual/lastPoint", 0).toInt());
    // ui->spinBoxManualPoints->setValue(settings->value("baseLine/algoManual/numberOfPoints", 20).toInt());
    // ui->checkBoxManuallyMonotonically->setChecked(settings->value("baseLine/algoManual/monotonically", false).toBool());
    ui->comboBoxManualInterpolation->setCurrentIndex(settings->value("baseLine/algoManual/interpolation", SPLINE_AKIMA).toInt());
    ui->comboBoxManualPosition->setCurrentIndex(settings->value("baseLine/algoManual/position", 0).toInt());
    ui->spinBoxManualWindow->setValue(settings->value("baseLine/algoManual/window", 5).toInt());

    restoreGeometry(settings->value("baseLine/geometry", QByteArray()).toByteArray());
}

void BaseLineDialog::saveSettings()
{
    settings->setValue("baseLine/splitter", ui->splitter->saveState());
    settings->setValue("baseLine/algorithm", ui->comboBoxAlgo->currentIndex());

    settings->setValue("baseLine/algoSnip/window", ui->comboBoxSnipWindow->currentIndex());
    settings->setValue("baseLine/algoSnip/iterations", ui->spinBoxSnipM->value());

    settings->setValue("baseLine/algoGolot/window", ui->spinBoxGolotWindow->value());
    settings->setValue("baseLine/algoGolot/noiseMulti", ui->doubleSpinBoxGolotvinN->value());
    settings->setValue("baseLine/algoGolot/steps", ui->spinBoxGolotSteps->value());
    settings->setValue("baseLine/algoGolot/mode", ui->comboBoxGolotLast->currentIndex());
    settings->setValue("baseLine/algoGolot/dataSmooth", ui->spinBoxGolotSmooth->value());
    settings->setValue("baseLine/algoGolot/interpolation", ui->comboBoxGolotInterpolation->currentIndex());
    settings->setValue("baseLine/algoGolot/sensitivity", ui->spinBoxGolotSensitivity->value());

    settings->setValue("baseLine/algoManual/lastPoint", ui->comboBoxManualLast->currentIndex());
    // settings->setValue("baseLine/algoManual/numberOfPoints", ui->spinBoxManualPoints->value());
    // settings->setValue("baseLine/algoManual/monotonically", ui->checkBoxManuallyMonotonically->isChecked());
    settings->setValue("baseLine/algoManual/interpolation", ui->comboBoxManualInterpolation->currentIndex());
    settings->setValue("baseLine/algoManual/position", ui->comboBoxManualPosition->currentIndex());
    settings->setValue("baseLine/algoManual/window", ui->spinBoxManualWindow->value());

    settings->setValue("baseLine/geometry", saveGeometry());
}

void BaseLineDialog::preSetProject(ProjectWidget *)
{
    if (graphView) {
        disconnect(graphView, SIGNAL(sigDoubleClickB(double,double,double)), this, SLOT(addManualAnchor(double,double,double)));
        disconnect(graphView, SIGNAL(sigAnchorMoved(int,double,double)), this, SLOT(moveAnchor(int,double,double)));
        disconnect(graphView, SIGNAL(sigAnchorRemove(int)), this, SLOT(removeAnchorIndex(int)));
    }

    temporaryScan = nullptr;
}

void BaseLineDialog::postSetProject(ProjectWidget *)
{
    if (graphView) {
        connect(graphView, SIGNAL(sigDoubleClickB(double,double,double)), this, SLOT(addManualAnchor(double,double,double)));
        connect(graphView, SIGNAL(sigAnchorMoved(int,double,double)), this, SLOT(moveAnchor(int,double,double)));
        connect(graphView, SIGNAL(sigAnchorRemove(int)), this, SLOT(removeAnchorIndex(int)));
    }

    clearAnchorPoints();
    clearTemporary();

    if (!checkBackend()) {
        clearGui();
        return;
    }

    parseScans();
    computeCurve();
}

void BaseLineDialog::updateView()
{
    if (!isVisible()) return;
    parseScans();
}

void BaseLineDialog::algoChanged(int n)
{
    clearAnchorPoints();

    if ((ui->stackedWidgetParams->currentIndex() == 2) && (ui->tableWidgetNodes->columnCount() > 1)) {
        prevManualAnchorPoints.clear();

        for (int i = 0; i < ui->tableWidgetNodes->rowCount(); ++i) {
            QStringList line;
            QTableWidgetItem *itX = ui->tableWidgetNodes->item(i, 0);
            QTableWidgetItem *itY = ui->tableWidgetNodes->item(i, 1);

            if (itX && itY) {
                line << itX->text();
                line << itY->text();
            }

            prevManualAnchorPoints << line;
        }
    }

    ui->stackedWidgetParams->setCurrentIndex(n);    
    blockUpdateSignals(true);

    if (n == 0) {
        ui->tableWidgetNodes->setRowCount(0);
        ui->tableWidgetNodes->setEnabled(false);
        ui->toolButtonAddAnchor->setEnabled(false);
        ui->toolButtonRemoveAnchor->setEnabled(false);
        computeSnip();
    }

    if (n == 1) {
        ui->tableWidgetNodes->setRowCount(0);
        ui->tableWidgetNodes->setEnabled(true);
        ui->toolButtonAddAnchor->setEnabled(true);
        ui->toolButtonRemoveAnchor->setEnabled(true);
        computeGolot();
    }

    if (n == 2) {
        ui->tableWidgetNodes->setRowCount(prevManualAnchorPoints.size());
        ui->tableWidgetNodes->setColumnCount(2);

        for (int i = 0; i < prevManualAnchorPoints.size(); ++i) {
            if (prevManualAnchorPoints.at(i).size() >= 2) {
                ui->tableWidgetNodes->setItem(i, 0, new QTableWidgetItem(prevManualAnchorPoints.at(i).at(0)));
                ui->tableWidgetNodes->setItem(i, 1, new QTableWidgetItem(prevManualAnchorPoints.at(i).at(1)));
            }
        }

        ui->tableWidgetNodes->setEnabled(true);
        ui->toolButtonAddAnchor->setEnabled(true);
        ui->toolButtonRemoveAnchor->setEnabled(true);
        computeManual();
    }

    blockUpdateSignals(false);
}

QVector<global::AnchorPoint> BaseLineDialog::tableToAnchors()
{
    QVector<global::AnchorPoint> vec;

    if (ui->tableWidgetNodes->columnCount() < 2) return vec;

    for (int i = 0; i < ui->tableWidgetNodes->rowCount(); ++i) {
        QTableWidgetItem *itX = ui->tableWidgetNodes->item(i, 0);
        QTableWidgetItem *itY = ui->tableWidgetNodes->item(i, 1);

        if (itX && itY) {
            double x = itX->text().toDouble();
            double y = itY->text().toDouble();
            vec.append(global::AnchorPoint(x, y, i == ui->tableWidgetNodes->currentRow()));
        }
    }

    return vec;
}

void BaseLineDialog::computeSnip()
{
    if (!checkBackend()) return;

    int w = ui->comboBoxSnipWindow->currentIndex();
    int m = ui->spinBoxSnipM->value();

    const Scan *scan = graphControl->getScan(ui->comboBoxScan->currentData(Qt::UserRole).toUuid());
    if (!scan) return;

    Scan bl(ScanOps::baseLineSNIP(*scan, m, w));
    bl.setName(QString("Baseline %1").arg(ui->comboBoxScan->currentText()));
    bl.setColor(QString());
    bl.setTypes(Scan::XY | Scan::SYNTHETIC | Scan::BACKGROUND | Scan::TEMPORARY);

    int idx = tempScanIndex();

    if (idx < 0) {
        graphControl->appendScan(bl, true);
        temporaryScan = graphControl->getLast();
    } else {
        graphControl->replaceScan(idx, bl, true);
        temporaryScan = graphControl->getScan(idx);
    }
}

void BaseLineDialog::computeGolot()
{
    if (!checkBackend()) return;

    int sm = ui->spinBoxGolotSmooth->value();
    int c = ui->comboBoxScan->currentIndex();
    int w = ui->spinBoxGolotWindow->value();
    double n = ui->doubleSpinBoxGolotvinN->value();
    int st = ui->spinBoxGolotSteps->value();
    int sens = ui->spinBoxGolotSensitivity->value();
    int md = ui->comboBoxGolotLast->currentIndex();

    if (c < 0) return;

    const Scan *scan = graphControl->getScan(ui->comboBoxScan->currentData(Qt::UserRole).toUuid());
    if (!scan) return;

    setAnchorTable(ScanOps::baseLineGolot(*scan, sm, w, n, st, sens, md));
    generateGolot();
}

void BaseLineDialog::generateGolot()
{
    if (!checkBackend()) return;

    const Scan *scan = graphControl->getScan(ui->comboBoxScan->currentData(Qt::UserRole).toUuid());
    if (!scan) return;

    Scan bl = scan->clone();
    bl.pDataHkl().clear();
    bl.setName(QString("Baseline %1").arg(ui->comboBoxScan->currentText()));
    bl.setColor(QString());
    bl.setTypes(Scan::XY | Scan::SYNTHETIC | Scan::BACKGROUND | Scan::TEMPORARY);

    int ipol = ui->comboBoxGolotInterpolation->currentData(Qt::UserRole).toInt();

    QVector<global::AnchorPoint> anchors = tableToAnchors();
    interpolateAnchorPoints(bl, anchors, ipol);

    graphView->setAnchorPoints(anchors);

    int idx = tempScanIndex();

    if (idx < 0) {
        graphControl->appendScan(bl, true);
        temporaryScan = graphControl->getLast();
    } else {
        graphControl->replaceScan(idx, bl, true);
        temporaryScan = graphControl->getScan(idx);
    }
}

void BaseLineDialog::computeManual()
{
    const Scan *scan = graphControl->getScan(ui->comboBoxScan->currentData(Qt::UserRole).toUuid());

    if (!scan) return;
    if (scan->size() < 2) return;

    int lastPointMode = ui->comboBoxManualLast->currentIndex();
    int step = int(5.0 * scan->size() / (scan->endAngle() - scan->startAngle()));
    bool ok;
    int n = 1;

    QPointF globMin(std::numeric_limits<double>::lowest(), std::numeric_limits<double>::max());

    QVector<QPointF> v;
    v.append(scan->first());

    while (n < scan->size()) {
        QPointF segMin = scan->point(n, ok);
        if (!ok) break;

        // parse a segment
        for (int i = 1; i < step; ++i) {
            QPointF p = scan->point(n + i, ok);
            if (!ok) continue;

            // find lowest y value
            if (p.y() < segMin.y()) segMin = p;
        }

        if (segMin.y() < globMin.y()) {
            globMin = segMin;
        }

        v.append(segMin);

        n += step;
    }

    if (lastPointMode == 0) {
        v.append(scan->last());
    } else if (lastPointMode == 1) {
        v.append(QPointF(scan->last().x(), v.last().y()));
    } else {
        v.append(QPointF(scan->last().x(), globMin.y()));
    }

    setAnchorTable(v);
    generateManual();
}

void BaseLineDialog::generateManual()
{
    if (!checkBackend()) return;

    const Scan *scan = graphControl->getScan(ui->comboBoxScan->currentData(Qt::UserRole).toUuid());
    if (!scan) return;

    Scan bl = scan->clone();
    bl.pDataHkl().clear();
    bl.setName(QString("Baseline %1").arg(ui->comboBoxScan->currentText()));
    bl.setColor(QString());
    bl.setTypes(Scan::XY | Scan::SYNTHETIC | Scan::BACKGROUND | Scan::TEMPORARY);

    int ipol = ui->comboBoxManualInterpolation->currentData(Qt::UserRole).toInt();

    QVector<global::AnchorPoint> anchors = tableToAnchors();
    interpolateAnchorPoints(bl, anchors, ipol);

    graphView->setAnchorPoints(anchors);

    int idx = tempScanIndex();

    if (idx < 0) {
        graphControl->appendScan(bl, true);
        temporaryScan = graphControl->getLast();
    } else {
        graphControl->replaceScan(idx, bl, true);
        temporaryScan = graphControl->getScan(idx);
    }
}

void BaseLineDialog::computeCurve()
{
    int n = ui->stackedWidgetParams->currentIndex();

    if (n == 0) {
        computeSnip();
    } else if (n == 1) {
        computeGolot();
    } else if (n == 2) {
        computeManual();
    }
}

void BaseLineDialog::generateCurve()
{
    int n = ui->stackedWidgetParams->currentIndex();

    if (n == 1) {
        generateGolot();
    } else if (n == 2) {
        generateManual();
    }
}

void BaseLineDialog::setAnchorTable(const QVector<QPointF> &pt)
{
    blockUpdateSignals(true);

    ui->tableWidgetNodes->setColumnCount(2);
    ui->tableWidgetNodes->setRowCount(pt.size());
    ui->tableWidgetNodes->setHorizontalHeaderLabels(headers);

    for (int i = 0; i < pt.size(); ++i) {
        ui->tableWidgetNodes->setItem(i, 0, new QTableWidgetItem(QString("%1").arg(pt.at(i).x(), 0, 'f', 5)));
        ui->tableWidgetNodes->setItem(i, 1, new QTableWidgetItem(QString("%1").arg(pt.at(i).y(), 0, 'f', 2)));
    }

    blockUpdateSignals(false);
}

void BaseLineDialog::clearAnchorPoints()
{
    if (graphView) graphView->setAnchorPoints(QVector<global::AnchorPoint>());
}

void BaseLineDialog::interpolateAnchorPoints(Scan &sc, const QVector<global::AnchorPoint> &anch, int ipol)
{
    // use a map to sort the data and eliminate duplicate x values
    // note: we must store the x value as int(double*multi), else elimination will not work reliably
    // due to rounding errors of doubles. If duplicates in x occur, the spline algorithms will crash

    QMap<int, double> data;
    double multi = 10000.0;

    for (int i = 0; i < anch.size(); ++i) {
        data.insert(int(multi * anch.at(i).angle), anch.at(i).intensity);
    }

    QVector<double> mX;
    QVector<double> mY;

    QMapIterator<int, double> it(data);

    while (it.hasNext()) {
        it.next();
        mX.append(double(it.key()) / multi);
        mY.append(it.value());
    }

    if (ipol == SPLINE_LINEAR)      sc.setDataInt(ScanOps::interpolateSplineLinear(mX,     mY, sc.pDataAngle()));
    if (ipol == SPLINE_CATMULL_ROM) sc.setDataInt(ScanOps::interpolateSplineCatmullRom(mX, mY, sc.pDataAngle()));
    if (ipol == SPLINE_CUBIC)       sc.setDataInt(ScanOps::interpolateSplineCubic(mX,      mY, sc.pDataAngle()));
    if (ipol == SPLINE_AKIMA)       sc.setDataInt(ScanOps::interpolateSplineAkima(mX,      mY, sc.pDataAngle()));
    if (ipol == SPLINE_MONOTONE)    sc.setDataInt(ScanOps::interpolateSplineMonotone(mX,   mY, sc.pDataAngle()));
}

void BaseLineDialog::parseScans()
{
    bool oldState = ui->comboBoxScan->blockSignals(true);
    int n = ui->comboBoxScan->currentIndex();

    ui->comboBoxScan->clear();

    if (!graphControl) {
        ui->comboBoxScan->blockSignals(oldState);
        return;
    }

    for (int i = 0; i < graphControl->count(); ++i) {
        if (!graphControl->at(i)->hasScanData()) continue;
        if (graphControl->at(i)->isTemporary())  continue;

        const Scan * scan = graphControl->at(i);
        ui->comboBoxScan->addItem(graphControl->scanName(scan), scan->uid());
    }

    n = n < ui->comboBoxScan->count() ? n : ui->comboBoxScan->count() - 1;
    ui->comboBoxScan->setCurrentIndex(n < 0 ? 0 : n);
    ui->comboBoxScan->blockSignals(oldState);
}

void BaseLineDialog::append()
{
    keepTemporary();
    parseScans();
    computeCurve();
}

void BaseLineDialog::keepTemporary()
{
    if (!checkBackend()) return;

    clearAnchorPoints();

    if (temporaryScan) {
        temporaryScan->setTypes(Scan::XY | Scan::BACKGROUND | Scan::SYNTHETIC);
        temporaryScan = nullptr;
        graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);
    }
}

void BaseLineDialog::clearTemporary()
{
    if (!checkBackend()) return;

    clearAnchorPoints();

    if (temporaryScan) {
        QUuid uid = temporaryScan->uid();
        temporaryScan = nullptr;
        graphControl->removeScan(uid);
    }
}

void BaseLineDialog::clearGui()
{
    bool oldState = ui->comboBoxScan->blockSignals(true);
    ui->comboBoxScan->clear();
    ui->comboBoxScan->blockSignals(oldState);
}

void BaseLineDialog::resetSnip()
{
    blockUpdateSignals(true);
    if (ui->comboBoxSnipWindow->count() > 1) ui->comboBoxSnipWindow->setCurrentIndex(1);
    ui->spinBoxSnipM->setValue(60);
    computeSnip();
    blockUpdateSignals(false);
}

void BaseLineDialog::resetGolot()
{
    blockUpdateSignals(true);
    if (ui->comboBoxGolotLast->count() > 0) ui->comboBoxGolotLast->setCurrentIndex(1);
    ui->spinBoxGolotSmooth->setValue(3);
    ui->spinBoxGolotSteps->setValue(20);
    ui->spinBoxGolotWindow->setValue(10);
    ui->doubleSpinBoxGolotvinN->setValue(1.0);
    ui->comboBoxGolotInterpolation->setCurrentIndex(SPLINE_AKIMA);
    computeGolot();
    blockUpdateSignals(false);
}

void BaseLineDialog::resetManual()
{
    blockUpdateSignals(true);
    ui->tableWidgetNodes->setRowCount(0);
    ui->comboBoxManualInterpolation->setCurrentIndex(SPLINE_AKIMA);
    computeManual();
    blockUpdateSignals(false);
}

void BaseLineDialog::blockUpdateSignals(bool b)
{
    ui->comboBoxScan->blockSignals(b);
    ui->comboBoxAlgo->blockSignals(b);

    ui->comboBoxSnipWindow->blockSignals(b);
    ui->spinBoxSnipM->blockSignals(b);

    ui->comboBoxGolotLast->blockSignals(b);
    ui->spinBoxGolotSmooth->blockSignals(b);
    ui->spinBoxGolotSteps->blockSignals(b);
    ui->spinBoxGolotWindow->blockSignals(b);
    ui->comboBoxGolotInterpolation->blockSignals(b);

    ui->comboBoxManualInterpolation->blockSignals(b);

    ui->doubleSpinBoxGolotvinN->blockSignals(b);
    ui->tableWidgetNodes->blockSignals(b);
}

void BaseLineDialog::addAnchor()
{
    int i = ui->tableWidgetNodes->currentRow();
    if (i < 0) i = 0;

    QTableWidgetItem *ix = ui->tableWidgetNodes->item(i, 0);
    QTableWidgetItem *iy = ui->tableWidgetNodes->item(i, 1);

    if (ix && iy) {
        QString dx = ix->text();
        QString dy = iy->text();

        ui->tableWidgetNodes->insertRow(i);

        ui->tableWidgetNodes->setItem(i, 0, new QTableWidgetItem(dx));
        ui->tableWidgetNodes->setItem(i, 1, new QTableWidgetItem(dy));

        generateCurve();
    }
}

void BaseLineDialog::removeAnchor()
{
    int i = ui->tableWidgetNodes->currentRow();
    if (i < 0) return;

    ui->tableWidgetNodes->removeRow(i);
    generateCurve();
}

void BaseLineDialog::highlightAnchor(QTableWidgetItem *, QTableWidgetItem *)
{
    QVector<global::AnchorPoint> anchors = tableToAnchors();
    graphView->setAnchorPoints(anchors);
    graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY);
}

void BaseLineDialog::addManualAnchor(double x, double y, double)
{
    if (!isVisible()) return;
    if (ui->stackedWidgetParams->currentIndex() != 2) return;

    if (ui->tableWidgetNodes->rowCount() < 1) {
        // initiate the table
        setAnchorTable(QVector<QPointF>());
    }

    for (int i = 0; i < ui->tableWidgetNodes->rowCount(); ++i) {
        QTableWidgetItem *itNext = ui->tableWidgetNodes->item(i, 0);
        if (!itNext) continue;

        double xNext = itNext->text().toDouble();

        if (xNext > x) {
            ui->tableWidgetNodes->insertRow(i);
            ui->tableWidgetNodes->setItem(i, 0, new QTableWidgetItem(QString("%1").arg(x, 0, 'f', 5)));
            ui->tableWidgetNodes->setItem(i, 1, new QTableWidgetItem(QString("%1").arg(y, 0, 'f', 2)));
            break;
        }
    }

    generateManual();
}

void BaseLineDialog::moveAnchor(int idx, double newAng, double newInt)
{
    // only used for manual baselines
    if (ui->comboBoxAlgo->currentIndex() != 2) return;

    if (idx < 0) return;
    if (idx >= ui->tableWidgetNodes->rowCount()) return;
    if (ui->tableWidgetNodes->columnCount() < 2) return;

    const Scan *scan = graphControl->getScan(ui->comboBoxScan->currentData(Qt::UserRole).toUuid());
    if (!scan) return;

    QTableWidgetItem *itAng = ui->tableWidgetNodes->item(idx, 0);
    QTableWidgetItem *itInt = ui->tableWidgetNodes->item(idx, 1);

    if (!itAng || !itInt) return;

    double ang = newAng;

    if (newAng < scan->minAngle())      ang = scan->minAngle();
    else if (newAng > scan->maxAngle()) ang = scan->maxAngle();

    itAng->setText(QString("%1").arg(ang, 0, 'f', 5));
    itInt->setText(QString("%1").arg(newInt < 0.0 ? 0.0 : newInt, 0, 'f', 2));

    generateManual();
}

void BaseLineDialog::removeAnchorIndex(int idx)
{
    if (idx < 0) return;
    if (idx >= ui->tableWidgetNodes->rowCount()) return;

    ui->tableWidgetNodes->removeRow(idx);
    generateCurve();
}

void BaseLineDialog::optimizeManual()
{
    if (ui->comboBoxAlgo->currentIndex() != 2) return;
    if (!checkBackend()) return;

    const Scan *scan = graphControl->getScan(ui->comboBoxScan->currentData(Qt::UserRole).toUuid());
    if (!scan) return;

    int mode = ui->comboBoxManualPosition->currentIndex();      // 0 = minimum, 1 = average
    int lastPointMode = ui->comboBoxManualLast->currentIndex(); // 0 = measured, 1 = last point, 2 = global minimum
    int window = ui->spinBoxManualWindow->value();              // number of data points left and right of the angle
    double globalMin = std::numeric_limits<double>::max();

    blockUpdateSignals(true);

    for (int i = 0; i < ui->tableWidgetNodes->rowCount(); ++i) {
        QTableWidgetItem *itAng = ui->tableWidgetNodes->item(i, 0);
        QTableWidgetItem *itInt = ui->tableWidgetNodes->item(i, 1);

        if (!itAng || !itInt) return;

        double dang = itAng->text().toDouble();
        double dint = std::numeric_limits<double>::max();
        if (mode == 1) dint = 0; // different initialization required

        int idx = scan->indexOfAngle(dang, 0);
        if (dang <= scan->startAngle()) idx = 0;
        if (dang >= scan->endAngle())   idx = scan->size() - 1;

        int idx_st = idx - window;
        int idx_ed = idx + window;

        if (idx_st < 0) {
            idx_ed -= idx_st;
            idx_st = 0;
        } else if (idx_ed >= scan->size()) {
            idx_st -= idx_ed - scan->size() - 1;
            idx_ed = scan->size() - 1;
        }

        if (mode == 0) {
            for (int j = idx_st; j <= idx_ed; ++j) {
                dint = qMin(dint, scan->intensity(j));
            }
        } else if (mode == 1) {
            for (int j = idx_st; j <= idx_ed; ++j) {
                dint += scan->intensity(j);
            }

            dint /= double(idx_ed - idx_st + 1);
        }

        globalMin = qMin(globalMin, dint);
        itAng->setText(QString("%1").arg(scan->angle(idx), 0, 'f', 5));
        itInt->setText(QString("%1").arg(dint, 0, 'f', 2));
    }

    int rc = ui->tableWidgetNodes->rowCount();

    if ((lastPointMode == 1) && (rc > 1)) {
        ui->tableWidgetNodes->item(rc - 1, 1)->setText(ui->tableWidgetNodes->item(rc - 2, 1)->text());
    } else if (lastPointMode == 2) {
        ui->tableWidgetNodes->item(rc - 1, 1)->setText(QString("%1").arg(globalMin, 0, 'f', 2));
    }

    generateManual();
    blockUpdateSignals(false);
}
