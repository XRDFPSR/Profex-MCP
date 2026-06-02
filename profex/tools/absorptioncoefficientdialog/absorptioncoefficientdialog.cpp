/***************************************************************************
                          absorptioncoefficientdialog.cpp  -  description
                             -------------------
    begin                : Tue Aug 10 20:51:00 CEST 2017
    copyright            : (C) 2017 by Nicola Doebelin
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

#include <QMessageBox>
#include <QFile>
#include <QFileDialog>
#include <QTextStream>
#include <QtMath>

#include "absorptioncoefficientdialog.h"
#include "ui_absorptioncoefficientdialog.h"
#include "../libXrdIO/structs.h"
#include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/parser/bgmnlstparser.h"
#include "../libXrdIO/absorptioncoefficientcalculator.h"
#include "wavelengthcombobox.h"

AbsorptionCoefficientDialog::AbsorptionCoefficientDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AbsorptionCoefficientDialog)
{
    ui->setupUi(this);

    settings = SettingsManager::getInstance();

    sampleMac = 0.0;
    sampleLac = 0.0;

    suffixLac = QString("cm%1%2").arg(global::superMinus).arg(global::superOne);
    suffixMac = QString("cm%1/g").arg(global::superTwo);
    suffixRho = QString("g/cm%1").arg(global::superThree);
    suffixMum = QString("%1m").arg(global::micro);

    lstFile = QString();

    headers << QString("Phase");
    headers << QString("Formula");
    headers << QString("Density [%1]").arg(suffixRho);
    headers << QString("Quantity");
    headers << QString("Phase MAC [%1]").arg(suffixMac);
    headers << QString("Phase LAC [%1]").arg(suffixLac);

    ui->labelDensity->setText(QString(tr("Density [%1]:")).arg(suffixRho));
    ui->labelQuantity->setText(QString(tr("Phase quantity:")));
    ui->labelPhaseMac->setText(QString(tr("Phase MAC [%1]:")).arg(suffixMac));
    ui->labelPhaseLac->setText(QString(tr("Phase LAC [%1]:")).arg(suffixLac));
    ui->labelSampleMac->setText(QString(tr("Sample MAC [%1]:")).arg(suffixMac));
    ui->labelSampleLac->setText(QString(tr("Sample LAC [%1]:")).arg(suffixLac));
    ui->labelDepthPath->setText(QString("Path length [%1]:").arg(suffixMum));
    ui->labelDepthLayer->setText(QString("Layer thickness [%1]:").arg(suffixMum));
    ui->lineEditSumFormula->setToolTip(AbsorptionCoefficientCalculator::getToolTip());

    model = new QStandardItemModel(this);
    model->setHorizontalHeaderLabels(headers);
    ui->treeViewCollection->setModel(model);

    mapper = new QDataWidgetMapper;
    mapper->setOrientation(Qt::Horizontal);
    mapper->setSubmitPolicy(QDataWidgetMapper::ManualSubmit);
    mapper->setModel(model); /*****/
    mapper->addMapping(ui->lineEditPhase, 0);
    mapper->addMapping(ui->lineEditSumFormula, 1);
    mapper->addMapping(ui->lineEditDensity, 2);
    mapper->addMapping(ui->lineEditQuantity, 3);
    mapper->addMapping(ui->lineEditMAC, 4);
    mapper->addMapping(ui->lineEditLAC, 5);

    QItemSelectionModel *selectionModel = ui->treeViewCollection->selectionModel();

    connect(selectionModel, SIGNAL(currentChanged(QModelIndex,QModelIndex)),
                this, SLOT(slotCurrentChanged(QModelIndex,QModelIndex)));

    connect(ui->lineEditPhase, SIGNAL(editingFinished()), this, SLOT(updateItems()));
    connect(ui->lineEditSumFormula, SIGNAL(editingFinished()), this, SLOT(updateItems()));
    connect(ui->lineEditDensity, SIGNAL(editingFinished()), this, SLOT(updateItems()));
    connect(ui->lineEditQuantity, SIGNAL(editingFinished()), this, SLOT(updateItems()));
    connect(ui->comboBoxWaveLength,  SIGNAL(currentIndexChanged(int)), this, SLOT(updateItems()));

    connect(ui->doubleSpinBoxDepthAttenuation, SIGNAL(valueChanged(double)), this, SLOT(updateItems()));
    connect(ui->doubleSpinBoxDepthPacking, SIGNAL(valueChanged(double)), this, SLOT(updateItems()));
    connect(ui->doubleSpinBoxDepthIncident, SIGNAL(valueChanged(double)), this, SLOT(updateItems()));

    connect(ui->doubleSpinBoxCustomWl, SIGNAL(valueChanged(double)), this, SLOT(updateItems()));
    connect(ui->radioButtonWLcharact, SIGNAL(toggled(bool)), this, SLOT(updateItems()));

    initData();
    initSettings();
    recalcCoefficients();
}

AbsorptionCoefficientDialog::~AbsorptionCoefficientDialog()
{
    // apparently mapper has to be deleted manually before the model is deleted by
    // destruction of the class
    if (mapper) delete mapper;
    delete ui;
}

void AbsorptionCoefficientDialog::closeEvent(QCloseEvent *e)
{
    saveSettings();
    e->accept();
}
void AbsorptionCoefficientDialog::initSettings()
{
    restoreGeometry(settings->value("absCoeffDialog/geometry", QByteArray()).toByteArray());
    ui->splitterV->restoreState(settings->value("absCoeffDialog/splitterV", QByteArray()).toByteArray());
    ui->splitterH->restoreState(settings->value("absCoeffDialog/splitterH", QByteArray()).toByteArray());
    ui->comboBoxWaveLength->setCurrentText(settings->value("absCoeffDialog/waveLength", "").toString());
    ui->doubleSpinBoxCustomWl->setValue(settings->value("absCoeffDialog/customWl", 0.154056).toDouble());
    ui->doubleSpinBoxDepthAttenuation->setValue(settings->value("absCoeffDialog/attenuation", 99.0).toDouble());
    ui->doubleSpinBoxDepthPacking->setValue(settings->value("absCoeffDialog/packingDensity", 100.0).toDouble());
    ui->doubleSpinBoxDepthIncident->setValue(settings->value("absCoeffDialog/incidentAngle", 30.0).toDouble());

    bool useCustomWl = settings->value("absCoeffDialog/useCustomWl", false).toBool();
    ui->radioButtonWLcharact->setChecked(!useCustomWl);
    ui->radioButtonWLcustom->setChecked(useCustomWl);

    QList<QVariant> wlst = settings->value("absCoeffDialog/columnWidths", QList<QVariant>()).toList();

    for (int i = 0; i < headers.size(); ++i) {
        ui->treeViewCollection->setColumnWidth(i, i < wlst.size() ? wlst.at(i).toInt() : 50);
    }
}

void AbsorptionCoefficientDialog::saveSettings()
{
    settings->setValue("absCoeffDialog/geometry", saveGeometry());
    settings->setValue("absCoeffDialog/splitterV", ui->splitterV->saveState());
    settings->setValue("absCoeffDialog/splitterH", ui->splitterH->saveState());
    settings->setValue("absCoeffDialog/waveLength", ui->comboBoxWaveLength->currentText());
    settings->setValue("absCoeffDialog/customWl", ui->doubleSpinBoxCustomWl->value());
    settings->setValue("absCoeffDialog/useCustomWl", ui->radioButtonWLcustom->isChecked());
    settings->setValue("absCoeffDialog/attenuation", ui->doubleSpinBoxDepthAttenuation->value());
    settings->setValue("absCoeffDialog/packingDensity", ui->doubleSpinBoxDepthPacking->value());
    settings->setValue("absCoeffDialog/incidentAngle", ui->doubleSpinBoxDepthIncident->value());

    QList<QVariant> wlst;

    for (int i = 0; i < headers.size(); ++i) {
        wlst.append(QVariant(ui->treeViewCollection->columnWidth(i)));
    }

    settings->setValue("absCoeffDialog/columnWidths", wlst);
}

void AbsorptionCoefficientDialog::initData()
{
    mapMolWeight.clear();
    QStringList atms = global::atoms.split(";");

    for (int i = 0; i < atms.size() - 5; i += 5) {
        mapMolWeight[atms.at(i+1).toUpper()] = atms.at(i+2).toDouble();
    }

    ui->comboBoxWaveLength->showKa2(false);
    ui->comboBoxWaveLength->showKb(false);
    ui->comboBoxWaveLength->initData(true);
}

void AbsorptionCoefficientDialog::parseLstFile(const QString &s)
{
    model->removeRows(0, model->rowCount(model->invisibleRootItem()->index()));
    lstFile = s;

    bool ok;
    BgmnLstParser lparser(lstFile, ok);

    QStringList phases(lparser.getPhaseNames());
    if (phases.isEmpty()) return;

    QString globalGoals = settings->value("bgmnProject/reportedGlobalGoals", global::defaultBgmnGlobalGoals).toString();
    // check again due to an incompatibility in Profex 5.4.0 with previous settings format (was a QStringList before)
    if (globalGoals.isEmpty()) globalGoals = global::defaultBgmnGlobalGoals;

    mapQuantities = phaseQuantities(lparser.getGlobalGoals(globalGoals.split("\n")));

    for (int i = 0; i < phases.size(); ++i) {
        CrystalStructure structure = lparser.getCrystalStructure(phases.at(i));
        QString formulaStr = lparser.getSumFormula(phases.at(i));

        double waveLength = ui->radioButtonWLcustom->isChecked()
                            ? ui->doubleSpinBoxCustomWl->value()
                            : ui->comboBoxWaveLength->currentData(Qt::UserRole).toDouble();

        double density = structure.density();
        double mac = AbsorptionCoefficientCalculator::getMacFromFormula(formulaStr, waveLength);
        double lac = AbsorptionCoefficientCalculator::getLacFromFormula(formulaStr, waveLength, density);

        QString quant = qFuzzyCompare(getQuantity(phases.at(i)), -1.0) ? QString() : QString("%1").arg(getQuantity(phases.at(i)), 0, 'f', 4);
        QStringList data;

        data << phases.at(i);
        data << structure.toSumFormula();
        data << QString("%1").arg(density, 0, 'f', 4);
        data << quant;
        data << QString("%1").arg(mac, 0, 'f', 5);
        data << QString("%1").arg(lac, 0, 'f', 5);

        appendItem(data);
    }

    mapper->toFirst();
}

void AbsorptionCoefficientDialog::recalcCoefficients()
{
    QStandardItem *item = model->invisibleRootItem();
    double waveLength = ui->radioButtonWLcustom->isChecked()
                            ? ui->doubleSpinBoxCustomWl->value()
                            : ui->comboBoxWaveLength->currentData(Qt::UserRole).toDouble();

    for (int i = 0; i < item->rowCount(); ++i) {
        QString formulaStr = item->child(i, 1)->data(Qt::DisplayRole).toString();
        double density = item->child(i, 2)->data(Qt::DisplayRole).toDouble();
        double mac = AbsorptionCoefficientCalculator::getMacFromFormula(formulaStr, waveLength);
        double lac = AbsorptionCoefficientCalculator::getLacFromFormula(formulaStr, waveLength, density);

        item->child(i, 4)->setData(QString("%1").arg(mac, 0, 'f', 5), Qt::DisplayRole);
        item->child(i, 5)->setData(QString("%1").arg(lac, 0, 'f', 5), Qt::DisplayRole);
    }

    sampleMac = calcSampleMac();
    sampleLac = calcSampleLac();

    QString strSampleMac = QString("%1").arg(sampleMac, 0, 'f', 5);
    QString strSampleLac = QString("%1").arg(sampleLac, 0, 'f', 5);

    int idx = mapper->currentIndex();

    if (idx >= 0) {
        ui->lineEditMAC->setText(model->item(idx, 4)->text());
        ui->lineEditLAC->setText(model->item(idx, 5)->text());
    }

    ui->lineEditSampleMAC->setText(qFuzzyCompare(sampleMac, -1.0) ? QString() : strSampleMac);
    ui->lineEditSampleLAC->setText(qFuzzyCompare(sampleLac, -1.0) ? QString() : strSampleLac);
}

void AbsorptionCoefficientDialog::recalcPenetration()
{
    if (qFuzzyIsNull(sampleLac) || ui->lineEditSampleLAC->text().isEmpty()) {
        ui->lineEditDepthLayer->clear();
        ui->lineEditDepthPath->clear();
        return;
    }

    double attIntensity = 1.0 - 0.01 * ui->doubleSpinBoxDepthAttenuation->value();
    double pdensity = 0.01 * ui->doubleSpinBoxDepthPacking->value();
    double incident = ui->doubleSpinBoxDepthIncident->value();

    if (qFuzzyCompare(attIntensity, 0.0) || qFuzzyCompare(pdensity, 0.0)) {
        ui->lineEditDepthLayer->setText("inf");
        ui->lineEditDepthPath->setText("inf");
        return;
    }

    double path = -qLn(attIntensity) / (0.0001 * sampleLac * pdensity); // LAC from cm^-1 to um^-1
    double layer = path * qSin(qDegreesToRadians(incident));

    ui->lineEditDepthPath->setText(QString("%1").arg(path, 0, '2', 4));
    ui->lineEditDepthLayer->setText(QString("%1").arg(layer, 0, '2', 4));
}

/*
double AbsorptionCoefficientDialog::calcMac(const QMap<QString, double> &fracFormula)
{
    double mac = 0.0;
    bool useCustomWl = ui->radioButtonWLcustom->isChecked();

    double waveLength = useCustomWl ?
                          ui->doubleSpinBoxCustomWl->value()
                          : ui->comboBoxWaveLength->currentData(Qt::UserRole).toDouble();
    double en = global::Functions::wavelengthToEnergy(waveLength);

    QMapIterator<QString, double> it(fracFormula);

    while (it.hasNext()) {
        it.next();
        mac += scatData.getMacForKeV(it.key(), en) * it.value();
    }

    return mac;
}

double AbsorptionCoefficientDialog::calcLac(double mac, double rho)
{
    return mac * rho;
}
*/

double AbsorptionCoefficientDialog::calcSampleMac()
{
    bool hasEmpty = false;
    bool ok = false;
    double smac = 0.0;
    double pd = 0.01 * ui->doubleSpinBoxDepthPacking->value();

    QStandardItem *item = model->invisibleRootItem();

    for (int i = 0; i < item->rowCount(); ++i) {
        QVariant vq(item->child(i, 3)->data(Qt::DisplayRole));
        QVariant vm(item->child(i, 4)->data(Qt::DisplayRole));

        double q = vq.toDouble(&ok);
        if (!ok || vq.toString().isEmpty()) hasEmpty = true;

        double m = vm.toDouble(&ok);
        if (!ok || vq.toString().isEmpty()) hasEmpty = true;

        smac += q * m * pd;
    }

    if (hasEmpty) return -1.0;
    return smac;
}

double AbsorptionCoefficientDialog::calcSampleLac()
{
    bool hasEmpty = false;
    bool ok = false;
    double slac = 0.0;
    double pd = 0.01 * ui->doubleSpinBoxDepthPacking->value();

    QStandardItem *item = model->invisibleRootItem();

    for (int i = 0; i < item->rowCount(); ++i) {
        QVariant vd(item->child(i, 2)->data(Qt::DisplayRole));
        QVariant vq(item->child(i, 3)->data(Qt::DisplayRole));
        QVariant vm(item->child(i, 4)->data(Qt::DisplayRole));

        double d = vd.toDouble(&ok);
        if (!ok || vd.toString().isEmpty()) hasEmpty = true;

        double q = vq.toDouble(&ok);
        if (!ok || vq.toString().isEmpty()) hasEmpty = true;

        double m = vm.toDouble(&ok);
        if (!ok || vm.toString().isEmpty()) hasEmpty = true;

        slac += d * q * m * pd;
    }

    if (hasEmpty) return -1.0;
    return slac;
}

/*
 * returns the sum formula as a map [element string][number of atoms double]
 */
/*
QMap<QString, double> AbsorptionCoefficientDialog::parseFormula(const QString &form)
{
    QMap<QString, double> formula;
    if (form.isEmpty()) return formula;

    static QRegularExpression rxEl("([A-Za-z]{1,2})(\\d+\\.?\\d*)?");
    static QRegularExpression rxSep("[\\s;_]+");
    QRegularExpressionMatch rm;
    QStringList elements(form.split(rxSep));

    for (int i = 0; i < elements.size(); ++i) {
        rm = rxEl.match(elements.at(i));
        if (!rm.hasMatch()) continue;

        QString el = rm.captured(1).toUpper();
        double idx = rm.captured(2).isEmpty() ? 1.0 : rm.captured(2).toDouble();

        if (formula.contains(el)) formula[el] += idx;
        else                      formula[el] = idx;
    }

    return formula;
}
*/
/*
 * returns a map with the sum formula elements by weight [element string][weight in g pfu double]
 */
/*
QMap<QString, double> AbsorptionCoefficientDialog::fractionalFormula(const QMap<QString, double> &formula)
{
    double molWt = 0.0;
    QMap<QString, double> fformula;
    QMapIterator<QString, double> it(formula);

    while (it.hasNext()) {
        it.next();
        molWt += mapMolWeight.value(it.key()) * it.value();
    }

    it.toFront();

    while (it.hasNext()) {
        it.next();
        fformula[it.key()] = (mapMolWeight.value(it.key()) * it.value()) / molWt;
    }

    return fformula;
}
*/

void AbsorptionCoefficientDialog::setProjectData(const QString &lst, double wl)
{
    ui->comboBoxWaveLength->setCurrentText(selectWavelength(wl));

    QFileInfo fi(lst);
    if (fi.exists()) parseLstFile(fi.absoluteFilePath());
}

QString AbsorptionCoefficientDialog::selectWavelength(double wl)
{
    for (int i = 0; i < ui->comboBoxWaveLength->count(); ++i) {
        double d = ui->comboBoxWaveLength->itemData(i).toDouble();

        if (qAbs(wl - d) < 0.01) {
            return ui->comboBoxWaveLength->itemText(i);
        }
    }

    return QString();
}

double AbsorptionCoefficientDialog::getQuantity(const QString &phase)
{
    QMapIterator<QString, double> it(mapQuantities);

    while (it.hasNext()) {
        it.next();

        if (it.key().toLower().simplified() == phase.toLower().simplified()) {
            return it.value();
        }
    }

    return -1.0;
}

QMap<QString, double> AbsorptionCoefficientDialog::phaseQuantities(const QList<global::Result> &goals)
{
    // we need to test for the old and new goal nomenclature:
    // old: phase/sum=dd.ddd
    // new: Qphase=dd.ddd
    QMap<QString, double> map;
    static QRegularExpression rxa("^([^\\/]+)\\/sum(?:=\\d+\\.?\\d*)?$");
    static QRegularExpression rxb("^Q([^=]+)(?:=\\d+\\.?\\d*)?$");
    QRegularExpressionMatch rma;
    QRegularExpressionMatch rmb;

    for (int i = 0; i < goals.size(); ++i) {
        global::Result r = goals.at(i);

        rma = rxa.match(r.name);
        rmb = rxb.match(r.name);

        if (rma.hasMatch()) {
            map[rma.captured(1)] = r.value;
        }

        if (rmb.hasMatch()) {
            map[rmb.captured(1)] = r.value;
        }
    }

    return map;
}

void AbsorptionCoefficientDialog::updateItems()
{
    mapper->submit();
    recalcCoefficients();
    recalcPenetration();
}

void AbsorptionCoefficientDialog::appendItem()
{
    QStringList l;

    l << ui->lineEditPhase->text();
    l << ui->lineEditSumFormula->text();
    l << ui->lineEditDensity->text();
    l << ui->lineEditQuantity->text();
    l << ui->lineEditMAC->text();
    l << ui->lineEditLAC->text();

    appendItem(l);
}

void AbsorptionCoefficientDialog::appendItem(const QStringList &l)
{
    QList<QStandardItem *> rowItems;

    for (int i = 0; i < l.size(); ++i) {
        rowItems << new QStandardItem(l.at(i));
        rowItems.last()->setEditable(false);
    }

    QStandardItem *root = model->invisibleRootItem();
    root->appendRow(rowItems);
    updateItems();
}

void AbsorptionCoefficientDialog::slotCurrentChanged(QModelIndex current, QModelIndex)
{
    mapper->setCurrentModelIndex(current);
    updateItems();
}

void AbsorptionCoefficientDialog::clearCurrent()
{
    QItemSelectionModel *selectionModel = ui->treeViewCollection->selectionModel();
    QModelIndex idx(selectionModel->currentIndex());

    if (!idx.isValid()) return;

    int row = model->itemFromIndex(idx)->row();

    QList<QStandardItem *> rowItems = model->takeRow(row);

    while (rowItems.size()) {
        if (rowItems.first()) {
            QStandardItem *it = rowItems.takeFirst();
            delete it;
        }
    }
}

void AbsorptionCoefficientDialog::clearAll()
{
    if (QMessageBox::question(this,
                              tr("Remove all phases"),
                              tr("Do you really want to remove all phases?"))
            == QMessageBox::No) {
        return;
    }

    while (model->invisibleRootItem()->rowCount()) {
        QList<QStandardItem *> items = model->takeRow(0);

        while (items.count()) {
            QStandardItem *item = items.takeFirst();
            if (item) delete item;
        }
    }

    ui->lineEditPhase->clear();
    ui->lineEditSumFormula->clear();
    ui->lineEditDensity->clear();
    ui->lineEditQuantity->clear();
    ui->lineEditMAC->clear();
    ui->lineEditLAC->clear();
    ui->lineEditSampleMAC->clear();
    ui->lineEditSampleLAC->clear();
}

void AbsorptionCoefficientDialog::saveAs()
{
    QString fn = QFileDialog::getSaveFileName(this,
                                              tr("Output File Name"),
                                              settings->value("config/workingdir", QDir::homePath()).toString(),
                                              tr("Text file (*.csv *.CSV)"));

    if (fn.isEmpty()) return;

    QString output(QString("Source file;%1\n\n").arg(lstFile));
    output += QString("%1\n").arg(headers.join(";"));

    for (int i = 0; i < model->invisibleRootItem()->rowCount(); ++i) {
        QStringList values;

        for (int j = 0; j < model->invisibleRootItem()->columnCount(); ++j) {
            QStandardItem *it = model->invisibleRootItem()->child(i, j);
            values << (it ? it->data(Qt::DisplayRole).toString() : QString());
        }

        output += QString("%1\n").arg(values.join(";"));
    }

    output += QString("\n");
    output += QString("Wavelength;%1\n").arg(ui->comboBoxWaveLength->currentText());
    output += QString("Attenuation;%1;%\n").arg(ui->doubleSpinBoxDepthAttenuation->value(), 0, 'f', 2);
    output += QString("Packing density;%1;%\n").arg(ui->doubleSpinBoxDepthPacking->value(), 0, 'f', 2);
    output += QString("Incident angle;%1;°\n").arg(ui->doubleSpinBoxDepthIncident->value(), 0, 'f', 2);
    output += QString("\n");
    output += QString("Sample MAC;%1;%2\n").arg(ui->lineEditSampleMAC->text(), suffixMac);
    output += QString("Sample LAC;%1;%2\n").arg(ui->lineEditSampleLAC->text(), suffixLac);
    output += QString("Path length;%1;%2\n").arg(ui->lineEditDepthPath->text(), suffixMum);
    output += QString("Layer thickness;%1;%2\n").arg(ui->lineEditDepthLayer->text(), suffixMum);

    BgmnFileIO::writeTextFile(fn, output);
}

void AbsorptionCoefficientDialog::loadFile()
{
    QFileInfo fi(QFileDialog::getOpenFileName(this,
                                              tr("Open Results File"),
                                              settings->value("config/workingdir", QDir::homePath()).toString(),
                                              tr("BGMN results file (*.lst *.LST)")));

    if (!fi.exists()) return;

    parseLstFile(fi.absoluteFilePath());
}

