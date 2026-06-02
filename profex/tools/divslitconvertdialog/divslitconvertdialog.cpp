/***************************************************************************
                          divslitconvertdialog.cpp  -  description
                             -------------------
    begin                : Fri Mar 22 21:00:00 CEST 2024
    copyright            : (C) 2024 by Nicola Doebelin
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

#include "divslitconvertdialog.h"
#include "ui_divslitconvertdialog.h"

DivSlitConvertDialog::DivSlitConvertDialog(QWidget *parent)
    : AbstractToolDialog(parent)
    , ui(new Ui::DivSlitConvertDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Convert Divergence Slit"));

    temporaryScan = nullptr;

    initSettings();

    connect(ui->comboBoxScan, SIGNAL(currentIndexChanged(int)), this, SLOT(compute()));
    connect(ui->radioButtonAdsToFds, SIGNAL(toggled(bool)), this, SLOT(compute()));
    connect(ui->doubleSpinBoxGonioRadius, SIGNAL(valueChanged(double)), this, SLOT(compute()));
    connect(ui->doubleSpinBoxIrrLength, SIGNAL(valueChanged(double)), this, SLOT(compute()));
    connect(ui->doubleSpinBoxDivAngle, SIGNAL(valueChanged(double)), this, SLOT(compute()));
}

DivSlitConvertDialog::~DivSlitConvertDialog()
{
    delete ui;
}

void DivSlitConvertDialog::preSetProject(ProjectWidget *)
{
    temporaryScan = nullptr;
}

void DivSlitConvertDialog::postSetProject(ProjectWidget *)
{
    clearTemporary();

    if (!checkBackend()) {
        clearGui();
        return;
    }

    parseScans();
    compute();
}

void DivSlitConvertDialog::clearGui()
{
    bool oldState = ui->comboBoxScan->blockSignals(true);
    ui->comboBoxScan->clear();
    ui->comboBoxScan->blockSignals(oldState);
}

void DivSlitConvertDialog::initSettings()
{
    int m = settings->value("divSlitConvertDialog/conversionMode", 0).toInt();
    ui->radioButtonAdsToFds->setChecked(m == 0);
    ui->radioButtonFdsToAds->setChecked(m == 1);

    ui->doubleSpinBoxGonioRadius->setValue(settings->value("divSlitConvertDialog/goniometerRadius", 200.0).toDouble());
    ui->doubleSpinBoxIrrLength->setValue(settings->value("divSlitConvertDialog/irradiatedLength", 10.0).toDouble());
    ui->doubleSpinBoxDivAngle->setValue(settings->value("divSlitConvertDialog/divergenceAngle", 0.25).toDouble());

    restoreGeometry(settings->value("divSlitConvertDialog/geometry", QByteArray()).toByteArray());
}

void DivSlitConvertDialog::saveSettings()
{
    int m = ui->radioButtonAdsToFds->isChecked() ? 0 : 1;

    settings->setValue("divSlitConvertDialog/conversionMode", m);
    settings->setValue("divSlitConvertDialog/goniometerRadius", ui->doubleSpinBoxGonioRadius->value());
    settings->setValue("divSlitConvertDialog/irradiatedLength", ui->doubleSpinBoxIrrLength->value());
    settings->setValue("divSlitConvertDialog/divergenceAngle", ui->doubleSpinBoxDivAngle->value());

    settings->setValue("divSlitConvertDialog/geometry", saveGeometry());
}

void DivSlitConvertDialog::parseScans()
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

void DivSlitConvertDialog::keepTemporary()
{
    if (!checkBackend()) return;

    if (temporaryScan) {
        temporaryScan->setTypes(Scan::XY | Scan::SYNTHETIC);
        temporaryScan = nullptr;
        graphControl->updateViews(QSet<global::ViewUpdateMode>() << global::ViewUpdateMode::DISPLAY << global::ViewUpdateMode::RESULTS);
    }
}

void DivSlitConvertDialog::clearTemporary()
{
    if (!checkBackend()) return;

    if (temporaryScan) {
        QUuid _uid = temporaryScan->uid();
        temporaryScan = nullptr;
        graphControl->removeScan(_uid);
    }
}

void DivSlitConvertDialog::blockUpdateSignals(bool b)
{
    ui->comboBoxScan->blockSignals(b);

    ui->doubleSpinBoxGonioRadius->blockSignals(b);
    ui->doubleSpinBoxIrrLength->blockSignals(b);
    ui->doubleSpinBoxDivAngle->blockSignals(b);
}

void DivSlitConvertDialog::updateView()
{
    if (!isVisible()) return;
    parseScans();
}

void DivSlitConvertDialog::append()
{
    keepTemporary();
    parseScans();
    compute();
}

void DivSlitConvertDialog::compute()
{
    if (!checkBackend()) return;
    if (ui->comboBoxScan->currentIndex() < 0) return;

    const Scan *_scan = graphControl->getScan(ui->comboBoxScan->currentData(Qt::UserRole).toUuid());
    if (!_scan) return;

    DsMode _mode = ui->radioButtonAdsToFds->isChecked() ? DsMode::ADSTOFDS : DsMode::FDSTOADS;

    double _gonioRad = ui->doubleSpinBoxGonioRadius->value();
    double _irrLen   = ui->doubleSpinBoxIrrLength->value();
    double _divAng   = ui->doubleSpinBoxDivAngle->value();

    Scan _sm;
    QString _labl;

    switch (_mode) {
    case DsMode::ADSTOFDS:
        _sm = ScanOps::convertDivSlitToFDS(*_scan, _irrLen, _divAng, _gonioRad);
        _labl = "ADS to FDS";
        break;
    case DsMode::FDSTOADS:
        _sm = ScanOps::convertDivSlitToADS(*_scan, _irrLen, _divAng, _gonioRad);
        _labl = "FDS to ADS";
        break;
    default:
        _sm = ScanOps::convertDivSlitToFDS(*_scan, _irrLen, _divAng, _gonioRad);
        _labl = "ADS to FDS";
        break;
    }

    static QRegularExpression rx("([^\\(]+)(?:\\(.*\\))?");
    QRegularExpressionMatch rm = rx.match(ui->comboBoxScan->currentText());
    QString scanBaseName = rm.hasMatch() ? rm.captured(1).trimmed() : ui->comboBoxScan->currentText();
    _sm.setName(QString("%1 %2").arg(scanBaseName, _labl));
    _sm.setTypes(Scan::XY | Scan::SYNTHETIC | Scan::TEMPORARY);
    _sm.setColor(QString());

    int _idx = tempScanIndex();

    if (_idx < 0) {
        graphControl->appendScan(_sm, true);
        temporaryScan = graphControl->getLast();
    } else {
        graphControl->replaceScan(_idx, _sm, true);
        temporaryScan = graphControl->getScan(_idx);
    }
}
