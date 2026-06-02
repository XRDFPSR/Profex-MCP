/***************************************************************************
                          scansmoothdialog.cpp  -  description
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
#include <limits.h>
#include <QDebug>
#include "../libXrdIO/structs.h"
#include "scansmoothdialog.h"
#include "ui_scansmoothdialog.h"

ScanSmoothDialog::ScanSmoothDialog(QWidget *parent) :
    AbstractToolDialog(parent),
    ui(new Ui::ScanSmoothDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Smooth Scans"));
    ui->treeWidgetDct->setHeaderLabels(QStringList()
                                       << QString("#")
                                       << QString("Periodicity [%1%2%3]").arg(global::degree, "2", global::theta));

    temporaryScan = nullptr;

    initSettings();

    connect(ui->comboBoxAlgo, SIGNAL(currentIndexChanged(int)), this, SLOT(algoChanged(int)));
    connect(ui->comboBoxScan, SIGNAL(currentIndexChanged(int)), this, SLOT(scanChanged()));

    connect(ui->spinBoxSmaWindow,  SIGNAL(valueChanged(int)),    this, SLOT(computeSma()));
    connect(ui->spinBoxEmaWindow,  SIGNAL(valueChanged(double)), this, SLOT(computeEma()));
    connect(ui->spinBoxLrmaWindow, SIGNAL(valueChanged(int)),    this, SLOT(computeLrma()));

    connect(ui->spinBoxDCTWindow,     SIGNAL(valueChanged(int)), this, SLOT(dctWindowChanged(int)));
    connect(ui->spinBoxDCTOverlap,    SIGNAL(valueChanged(int)), this, SLOT(dctOverlapChanged(int)));
    connect(ui->treeWidgetDct, SIGNAL(itemChanged(QTreeWidgetItem*, int)), this, SLOT(computeDct()));

    connect(ui->verticalSliderDCTcoefficients, SIGNAL(valueChanged(int)), this, SLOT(checkDCTcoefficients(int)));
}

ScanSmoothDialog::~ScanSmoothDialog()
{
    delete ui;
}

void ScanSmoothDialog::initSettings()
{
    ui->comboBoxAlgo->setCurrentIndex(settings->value("smoothDialog/algorithm", 0).toInt());
    ui->stackedWidgetParams->setCurrentIndex(ui->comboBoxAlgo->currentIndex());

    ui->spinBoxSmaWindow->setValue(settings->value("smoothDialog/algoSma/window", 5).toInt());
    ui->spinBoxEmaWindow->setValue(settings->value("smoothDialog/algoEma/window", 0.5).toDouble());
    ui->spinBoxLrmaWindow->setValue(settings->value("smoothDialog/algoLrma/window", 5).toInt());

    ui->spinBoxDCTWindow->setValue(settings->value("smoothDialog/alogoDct/window", 24).toInt());
    ui->spinBoxDCTOverlap->setValue(settings->value("smoothDialog/algoDct/overlap", 6).toInt());

    ui->spinBoxDCTOverlap->setMaximum(ui->spinBoxDCTWindow->value() / 2);
    ui->verticalSliderDCTcoefficients->setMaximum(ui->spinBoxDCTWindow->value());
    ui->verticalSliderDCTcoefficients->setValue(ui->spinBoxDCTWindow->value());

    restoreGeometry(settings->value("smoothDialog/geometry", QByteArray()).toByteArray());
}

void ScanSmoothDialog::saveSettings()
{
    settings->setValue("smoothDialog/algorithm", ui->comboBoxAlgo->currentIndex());

    settings->setValue("smoothDialog/algoSma/window", ui->spinBoxSmaWindow->value());
    settings->setValue("smoothDialog/algoEma/window", ui->spinBoxEmaWindow->value());
    settings->setValue("smoothDialog/algoLrma/window", ui->spinBoxLrmaWindow->value());

    settings->setValue("smoothDialog/algoDct/window", ui->spinBoxDCTWindow->value());
    settings->setValue("smoothDialog/algoDct/overlap", ui->spinBoxDCTOverlap->value());

    settings->setValue("smoothDialog/geometry", saveGeometry());
}

void ScanSmoothDialog::preSetProject(ProjectWidget *)
{
    temporaryScan = nullptr;
}

void ScanSmoothDialog::postSetProject(ProjectWidget *)
{
    clearTemporary();

    if (!checkBackend()) {
        clearGui();
        return;
    }

    parseScans();
    setDCTcoefficients(ui->spinBoxDCTWindow->value());
    scanChanged();
}

void ScanSmoothDialog::updateView()
{
    if (!isVisible()) return;
    parseScans();
}

void ScanSmoothDialog::compute()
{
    switch (ui->stackedWidgetParams->currentIndex()) {
        case 0:
            computeAlgo(SMA);
            break;
        case 1:
            computeAlgo(EMA);
            break;
        case 2:
            computeAlgo(LRMA);
            break;
        case 3:
            computeAlgo(DCT);
            break;
    }
}

void ScanSmoothDialog::scanChanged()
{
    const Scan *scan = graphControl->getScan(ui->comboBoxScan->currentData(Qt::UserRole).toUuid());
    if (!scan) return;

    ui->spinBoxDCTWindow->setMaximum(scan->size());
    compute();
}

void ScanSmoothDialog::algoChanged(int i)
{
    ui->stackedWidgetParams->setCurrentIndex(i);
    compute();
}

void ScanSmoothDialog::computeAlgo(SmoothAlgoType a)
{
    if (!checkBackend()) return;

    int c = ui->comboBoxScan->currentIndex();
    if (c < 0) return;

    const Scan *scan = graphControl->getScan(ui->comboBoxScan->currentData(Qt::UserRole).toUuid());
    if (!scan) return;

    Scan sm = scan->clone();
    sm.pDataHkl().clear();

    switch (a) {
        case SMA:
            sm.setDataInt(ScanOps::smoothMovingAverageSimple(scan->pDataIntensity(),
                                                             ui->spinBoxSmaWindow->value()));
            break;
        case EMA:
            sm.setDataInt(ScanOps::smoothMovingAverageExponential(scan->pDataIntensity(),
                                                                  ui->spinBoxEmaWindow->value()));
            break;
        case LRMA:
            sm.setDataInt(ScanOps::smoothMovingAverageLinearRegression(scan->pDataIntensity(),
                                                                       ui->spinBoxLrmaWindow->value()));
            break;
        case DCT:
            int dctWin = ui->spinBoxDCTWindow->value();
            int dctOlap = ui->spinBoxDCTOverlap->value();
            QList<bool> dctComp = getDCTcoefficients();
            sm.setDataInt(ScanOps::smoothDCTtypeIV(scan->pDataIntensity(), dctWin, dctOlap, dctComp));
            break;
    }

    sm.setName(QString("Smoothed %1").arg(ui->comboBoxScan->currentText()));
    sm.setTypes(Scan::XY | Scan::SYNTHETIC | Scan::TEMPORARY);
    sm.setColor(QString());

    int idx = tempScanIndex();

    if (idx < 0) {
        graphControl->appendScan(sm, true);
        temporaryScan = graphControl->getLast();
    } else {
        graphControl->replaceScan(idx, sm, true);
        temporaryScan = graphControl->getScan(idx);
    }
}

void ScanSmoothDialog::computeSma()
{
    computeAlgo(SMA);
}

void ScanSmoothDialog::computeEma()
{
    computeAlgo(EMA);
}

void ScanSmoothDialog::computeLrma()
{
    computeAlgo(LRMA);
}

void ScanSmoothDialog::computeDct()
{
    computeAlgo(DCT);
}

void ScanSmoothDialog::parseScans()
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

        const Scan *scan = graphControl->at(i);
        ui->comboBoxScan->addItem(graphControl->scanName(scan), scan->uid());
    }

    n = n < ui->comboBoxScan->count() ? n : ui->comboBoxScan->count() - 1;
    ui->comboBoxScan->setCurrentIndex(n < 0 ? 0 : n);
    ui->comboBoxScan->blockSignals(oldState);
}

void ScanSmoothDialog::append()
{
    keepTemporary();
    parseScans();
    compute();
}

void ScanSmoothDialog::keepTemporary()
{
    if (!checkBackend()) return;

    if (temporaryScan) {
        temporaryScan->setTypes(Scan::XY | Scan::SYNTHETIC);
        temporaryScan = nullptr;
        graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);
    }
}

void ScanSmoothDialog::clearTemporary()
{
    if (!checkBackend()) return;

    if (temporaryScan) {
        QUuid uid = temporaryScan->uid();
        temporaryScan = nullptr;
        graphControl->removeScan(uid);
    }
}

void ScanSmoothDialog::clearGui()
{
    bool oldState = ui->comboBoxScan->blockSignals(true);
    ui->comboBoxScan->clear();
    ui->comboBoxScan->blockSignals(oldState);
}

void ScanSmoothDialog::resetSma()
{
    blockUpdateSignals(true);
    ui->spinBoxSmaWindow->setValue(5);
    computeSma();
    blockUpdateSignals(false);
}

void ScanSmoothDialog::resetEma()
{
    blockUpdateSignals(true);
    ui->spinBoxEmaWindow->setValue(0.5);
    computeEma();
    blockUpdateSignals(false);
}

void ScanSmoothDialog::resetLrma()
{
    blockUpdateSignals(true);
    ui->spinBoxLrmaWindow->setValue(5);
    computeLrma();
    blockUpdateSignals(false);
}

void ScanSmoothDialog::resetDct()
{
    blockUpdateSignals(true);
    ui->spinBoxDCTWindow->setValue(24);
    ui->spinBoxDCTOverlap->setValue(6);
    setDCTcoefficients(24);
    computeDct();
    blockUpdateSignals(false);
}

void ScanSmoothDialog::blockUpdateSignals(bool b)
{
    ui->comboBoxScan->blockSignals(b);
    ui->comboBoxAlgo->blockSignals(b);

    ui->spinBoxSmaWindow->blockSignals(b);
    ui->spinBoxEmaWindow->blockSignals(b);
    ui->spinBoxLrmaWindow->blockSignals(b);

    ui->treeWidgetDct->blockSignals(b);
    ui->verticalSliderDCTcoefficients->blockSignals(b);
}

void ScanSmoothDialog::dctWindowChanged(int n)
{
    setDCTcoefficients(n);
    computeDct();
}

void ScanSmoothDialog::dctOverlapChanged(int)
{
    computeDct();
}

void ScanSmoothDialog::dctComponentsChanged(int)
{
    computeDct();
}

QList<bool> ScanSmoothDialog::getDCTcoefficients() const
{
    QList<bool> coeff;

    for (int i = 0; i < ui->treeWidgetDct->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetDct->topLevelItem(i);

        if (it) coeff.append(it->checkState(0) == Qt::Checked);
        else    coeff.append(false);
    }

    return coeff;
}

void ScanSmoothDialog::setDCTcoefficients(int n)
{
    bool oldState = ui->spinBoxDCTOverlap->blockSignals(true);
    ui->spinBoxDCTOverlap->setMaximum(n / 2);
    ui->spinBoxDCTOverlap->blockSignals(oldState);

    oldState = ui->verticalSliderDCTcoefficients->blockSignals(true);
    ui->verticalSliderDCTcoefficients->setMaximum(n);
    ui->verticalSliderDCTcoefficients->blockSignals(oldState);

    const Scan *scan = graphControl->getScan(ui->comboBoxScan->currentData(Qt::UserRole).toUuid());
    if (!scan) return;

    double basePeriod = scan->stepSize() * n * 2;

    oldState = ui->treeWidgetDct->blockSignals(true);

    QList<bool> prevCheckStates = getDCTcoefficients();
    ui->treeWidgetDct->clear();

    for (int i = 0; i < n; ++i) {
        QString per = (i == 0) ? QString("inf") : QString("%1").arg(basePeriod / i, 0, 'f', 4);
        QTreeWidgetItem *it = new QTreeWidgetItem(QStringList() << QString("%1").arg(i + 1) << per);

        if (i < prevCheckStates.size()) {
            it->setCheckState(0, prevCheckStates.at(i) ? Qt::Checked : Qt::Unchecked);
        } else {
            if (prevCheckStates.size()) {
                it->setCheckState(0, prevCheckStates.constLast() ? Qt::Checked : Qt::Unchecked);
            } else {
                it->setCheckState(0, Qt::Checked);
            }
        }

        ui->treeWidgetDct->addTopLevelItem(it);
    }

    ui->treeWidgetDct->blockSignals(oldState);
}

void ScanSmoothDialog::checkDCTcoefficients(int n)
{
    bool oldState = ui->treeWidgetDct->blockSignals(true);

    for (int i = 0; i < ui->treeWidgetDct->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetDct->topLevelItem(i);

        if (it) it->setCheckState(0, i < n ? Qt::Checked : Qt::Unchecked);
    }

    ui->treeWidgetDct->blockSignals(oldState);
    computeDct();
}
