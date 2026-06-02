/***************************************************************************
                          calcdspacingdialog.cpp  -  description
                             -------------------
    begin                : Tue Jul 24 18:10:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#include <QFileDialog>
#include <QClipboard>
#include "calcdspacingdialog.h"
#include "ui_calcdspacingdialog.h"
#include "../libXrdIO/functions.h"
#include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/crystal/crystalstructure.h"
#include "../libXrdIO/parser/bgmnlstparser.h"

CalcDspacingDialog::CalcDspacingDialog(QWidget *parent)
    : AbstractToolDialog(parent),
      ui(new Ui::CalcDspacingDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Calculate Lattice Spacing"));
    project = nullptr;

    settings = SettingsManager::getInstance();

    workingDir = QDir::homePath();
    initSettings();
    connect(ui->treeWidget->header(), SIGNAL(geometriesChanged()), this, SLOT(treeHeaderChanged()));
}

CalcDspacingDialog::~CalcDspacingDialog()
{
    delete ui;
}


void CalcDspacingDialog::preSetProject(ProjectWidget *)
{
}

void CalcDspacingDialog::postSetProject(ProjectWidget *p)
{
    workingDir = p->workingDir();
    project = dynamic_cast<BgmnProjectWidget *>(p);
    updateCrystalSystem();
}

void CalcDspacingDialog::closeEvent(QCloseEvent *e)
{
    saveSettings();
    e->accept();
}

void CalcDspacingDialog::initSettings()
{
    ui->comboBoxCrystalSystem->addItem(tr("Triclinic"),             CrystalSystem::TRICLINIC);
    ui->comboBoxCrystalSystem->addItem(tr("Monoclinic (unique a)"), CrystalSystem::MONOCLINICA);
    ui->comboBoxCrystalSystem->addItem(tr("Monoclinic (unique b)"), CrystalSystem::MONOCLINICB);
    ui->comboBoxCrystalSystem->addItem(tr("Monoclinic (unique c)"), CrystalSystem::MONOCLINICC);
    ui->comboBoxCrystalSystem->addItem(tr("Orthorhombic"),          CrystalSystem::ORTHORHOMBIC);
    ui->comboBoxCrystalSystem->addItem(tr("Tetragonal"),            CrystalSystem::TETRAGONAL);
    ui->comboBoxCrystalSystem->addItem(tr("Rhombohedral"),          CrystalSystem::RHOMBOHEDRAL);
    ui->comboBoxCrystalSystem->addItem(tr("Trigonal / Hexagonal"),  CrystalSystem::HEXAGONAL);
    ui->comboBoxCrystalSystem->addItem(tr("Cubic"),                 CrystalSystem::CUBIC);

    ui->comboBoxCrystalSystem->setCurrentIndex(settings->value("dSpacingDialog/crystalSystem", 0).toInt());

    ui->comboBoxRadiationCharacteristic->showKa2(false);
    ui->comboBoxRadiationCharacteristic->showKb(false);
    ui->comboBoxRadiationCharacteristic->initData(true);

    ui->comboBoxRadiationCharacteristic->setCurrentIndex(settings->value("dSpacingDialog/radCharacteristic", 4).toInt());
    ui->doubleSpinBoxRadiationSynchrotron->setValue(settings->value("dSpacingDialog/radSynchrotron", 0.07).toDouble());

    restoreGeometry(settings->value("dSpacingDialog/geometry", QByteArray()).toByteArray());
    ui->treeWidget->header()->restoreState(settings->value("dSpacingDialog/treeHeader", QByteArray()).toByteArray());

    ui->radioButtonRadiationCharacteristic->setChecked(true);
    updateCrystalSystem();
}

void CalcDspacingDialog::saveSettings()
{
    settings->setValue("dSpacingDialog/geometry", saveGeometry());
    settings->setValue("dSpacingDialog/radCharacteristic", ui->comboBoxRadiationCharacteristic->currentIndex());
    settings->setValue("dSpacingDialog/radSynchrotron", ui->doubleSpinBoxRadiationSynchrotron->value());
    settings->setValue("dSpacingDialog/crystalSystem", ui->comboBoxCrystalSystem->currentIndex());
}

void CalcDspacingDialog::updateCrystalSystem()
{
    CrystalSystem syst = static_cast<CrystalSystem>(ui->comboBoxCrystalSystem->currentData(Qt::UserRole).toInt());
    QList<bool>   status;

    // override angle values given by the crystal system, and enable/disable spinboxes
    switch (syst) {
    case CrystalSystem::TRICLINIC:
        status << true << true << true << true << true << true;
        break;
    case CrystalSystem::MONOCLINICA:
        ui->doubleSpinBoxCellBeta->setValue(90.0);
        ui->doubleSpinBoxCellGamma->setValue(90.0);
        status << true << true << true << true << false << false;
        break;
    case CrystalSystem::MONOCLINICB:
        ui->doubleSpinBoxCellAlpha->setValue(90.0);
        ui->doubleSpinBoxCellGamma->setValue(90.0);
        status << true << true << true << false << true << false;
        break;
    case CrystalSystem::MONOCLINICC:
        ui->doubleSpinBoxCellAlpha->setValue(90.0);
        ui->doubleSpinBoxCellBeta->setValue(90.0);
        status << true << true << true << false << false << true;
        break;
    case CrystalSystem::ORTHORHOMBIC:
        ui->doubleSpinBoxCellAlpha->setValue(90.0);
        ui->doubleSpinBoxCellBeta->setValue(90.0);
        ui->doubleSpinBoxCellGamma->setValue(90.0);
        status << true << true << true << false << false << false;
        break;
    case CrystalSystem::TETRAGONAL:
        ui->doubleSpinBoxCellAlpha->setValue(90.0);
        ui->doubleSpinBoxCellBeta->setValue(90.0);
        ui->doubleSpinBoxCellGamma->setValue(90.0);
        status << true << false << true << false << false << false;
        break;
    case CrystalSystem::RHOMBOHEDRAL:
        ui->doubleSpinBoxCellBeta->setValue(90.0);
        ui->doubleSpinBoxCellGamma->setValue(90.0);
        status << true << false << false << true << false << false;
        break;
    case CrystalSystem::HEXAGONAL:
        ui->doubleSpinBoxCellAlpha->setValue(90.0);
        ui->doubleSpinBoxCellBeta->setValue(90.0);
        ui->doubleSpinBoxCellGamma->setValue(120.0);
        status << true << false << true << false << false << false;
        break;
    case CrystalSystem::CUBIC:
        ui->doubleSpinBoxCellAlpha->setValue(90.0);
        ui->doubleSpinBoxCellBeta->setValue(90.0);
        ui->doubleSpinBoxCellGamma->setValue(90.0);
        status << true << false << false << false << false << false;
        break;
    default:
        status << true << true << true << true << true << true;
        break;
    }

    ui->doubleSpinBoxCellA->setEnabled(status.at(0));
    ui->doubleSpinBoxCellB->setEnabled(status.at(1));
    ui->doubleSpinBoxCellC->setEnabled(status.at(2));
    ui->doubleSpinBoxCellAlpha->setEnabled(status.at(3));
    ui->doubleSpinBoxCellBeta->setEnabled(status.at(4));
    ui->doubleSpinBoxCellGamma->setEnabled(status.at(5));

    updateData();
}

void CalcDspacingDialog::updateData()
{
    int h, k, l;
    double a, b, c, alpha, beta, gamma;

    getValues(a, b, c, alpha, beta, gamma, h, k, l);
    calculateResults(a, b, c, alpha, beta, gamma, h, k, l, getLambda());
}

void CalcDspacingDialog::getValues(double &a, double &b, double &c, double &al, double &be, double &ga, int &h, int &k, int &l)
{
    CrystalSystem syst = static_cast<CrystalSystem>(ui->comboBoxCrystalSystem->currentData(Qt::UserRole).toInt());

    h = ui->spinBoxH->value();
    k = ui->spinBoxK->value();
    l = ui->spinBoxL->value();

    a = ui->doubleSpinBoxCellA->value();
    b = ui->doubleSpinBoxCellB->value();
    c = ui->doubleSpinBoxCellC->value();
    al = ui->doubleSpinBoxCellAlpha->value();
    be = ui->doubleSpinBoxCellBeta->value();
    ga = ui->doubleSpinBoxCellGamma->value();

    // override variables restricted by the crystal system
    switch (syst) {
    case CrystalSystem::TRICLINIC:
        break;
    case CrystalSystem::MONOCLINICA:
        be = 90.0;
        ga = 90.0;
        break;
    case CrystalSystem::MONOCLINICB:
        al = 90.0;
        ga = 90.0;
        break;
    case CrystalSystem::MONOCLINICC:
        al = 90.0;
        be = 90.0;
        break;
    case CrystalSystem::ORTHORHOMBIC:
        al = 90.0;
        be = 90.0;
        ga = 90.0;
        break;
    case CrystalSystem::TETRAGONAL:
        b = a;
        al = 90.0;
        be = 90.0;
        ga = 90.0;
        break;
    case CrystalSystem::RHOMBOHEDRAL:
        b = a;
        c = a;
        be = al;
        ga = al;
        break;
    case CrystalSystem::HEXAGONAL:
        b = a;
        al = 90.0;
        be = 90.0;
        ga = 120.0;
        break;
    case CrystalSystem::CUBIC:
        b = a;
        c = a;
        al = 90.0;
        be = 90.0;
        ga = 90.0;
        break;
    default:
        break;
    }
}

void CalcDspacingDialog::calculateResults(double a, double b, double c, double al, double be, double ga, int h, int k, int l, double wl)
{
    if (qFuzzyIsNull(a) || qFuzzyIsNull(b) || qFuzzyIsNull(c) || qFuzzyIsNull(al) || qFuzzyIsNull(be) || qFuzzyIsNull(ga)) return;

    double v = global::Functions::cellVolume(a, b, c, al, be, ga);
    ui->lineEditVolume->setText(QString("%1").arg(v, 0, 'f', 6));

    if ((h == 0) && (k == 0) && (l == 0)) {
        ui->lineEditResultD->clear();
        ui->lineEditResult2t->clear();
        return;
    }

    double d = global::Functions::dSpacing(a, b, c, al, be, ga, h, k, l);
    ui->lineEditResultD->setText(QString("%1").arg(d, 0, 'f', 6));

    if (qFuzzyIsNull(wl)) return;

    double tt = global::Functions::dToTwoTheta(d, wl);
    ui->lineEditResult2t->setText(QString("%1").arg(tt, 0, 'f', 4));
}

double CalcDspacingDialog::getLambda()
{
    return ui->radioButtonRadiationCharacteristic->isChecked() ?
               ui->comboBoxRadiationCharacteristic->currentData(Qt::UserRole).toDouble()
                                                               : ui->doubleSpinBoxRadiationSynchrotron->value();

}

void CalcDspacingDialog::appendLine()
{
    QStringList l;
    l << ui->lineEditPhaseName->text();
    l << QString("%1").arg(ui->spinBoxH->value());
    l << QString("%1").arg(ui->spinBoxK->value());
    l << QString("%1").arg(ui->spinBoxL->value());
    l << ui->lineEditResultD->text();
    l << ui->lineEditResult2t->text();

    ui->treeWidget->addTopLevelItem(new QTreeWidgetItem(l));
}

void CalcDspacingDialog::removeLine()
{
    QTreeWidgetItem *it = ui->treeWidget->currentItem();
    it = ui->treeWidget->takeTopLevelItem(ui->treeWidget->indexOfTopLevelItem(it));
    if (it) delete it;
}

void CalcDspacingDialog::clearList()
{
    // do not use QTreeWidget::clear(), because it resets the header
    while (ui->treeWidget->topLevelItemCount()) {
        QTreeWidgetItem *it = ui->treeWidget->takeTopLevelItem(0);
        if (it) delete it;
    }
}

void CalcDspacingDialog::saveAs()
{
    QString s = QFileDialog::getSaveFileName(this, tr("Save hkl List"), workingDir, tr("Text file (*.csv *.CSV)"));
    if (s.isEmpty()) return;

    BgmnFileIO::writeTextFile(s, getOutputText());
}

QString CalcDspacingDialog::getOutputText()
{
    QStringList out;
    out << QString("########################################################");
    out << QString("# Lattice plane spacings                               #");
    out << QString("# calculated with Profex %1.%2.%3                         #").arg(VERSION_MAJOR).arg(VERSION_MINOR).arg(VERSION_BUILD);
    out << QString("########################################################\n");

    out << QString("Wavelength;%1\n").arg(getLambda(), 0, 'f', 8);
    out << QString("# Phase;h;k;l;d [nm];2theta [deg]");

    for (int i = 0; i < ui->treeWidget->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidget->topLevelItem(i);
        QStringList line;

        for (int c = 0; c < it->columnCount(); ++c) {
            line << it->text(c);
        }

        out.append(line.join(";"));
    }

    return out.join("\n");
}

void CalcDspacingDialog::treeHeaderChanged()
{
    settings->setValue("dSpacingDialog/treeHeader", ui->treeWidget->header()->saveState());
}

void CalcDspacingDialog::copyData()
{
    QClipboard *clipboard = QGuiApplication::clipboard();
    clipboard->setText(getOutputText());
}

void CalcDspacingDialog::readFromProject()
{
    if (!project) {
        qDebug() << QString("CalcDspacingDialog::readFromProject(): No valid project");
        return;
    }

    const Scan *scan = graphControl->hasActiveScan() ? graphControl->activeScans().constFirst() : nullptr;

    if (!scan) {
        qDebug() << QString("CalcDspacingDialog::readFromProject(): No scan selected");
        return;
    }

    BgmnLstParser lparser = project->getLstParser();
    QStringList phases = lparser.getPhaseNames();

    if (!phases.contains(scan->name())) {
        qDebug() << QString("CalcDspacingDialog::readFromProject(): Phase %1 not found in project").arg(scan->name());
        return;
    }

    CrystalStructure structure = lparser.getCrystalStructure(scan->name());
    CrystalUnitCell cell = structure.unitCell();

    ui->lineEditPhaseName->setText(scan->name());
    int itNum = cell.itNumber();

    if (itNum >= 1 && itNum <= 2) {
        // triclinic
        ui->comboBoxCrystalSystem->setCurrentIndex(0);
    } else if (itNum >= 3 && itNum <= 15) {
        if (qFuzzyCompare(cell.beta(), cell.gamma())) {
            // monoclinic unique a
            ui->comboBoxCrystalSystem->setCurrentIndex(1);
        } else if (qFuzzyCompare(cell.alpha(), cell.gamma())) {
            // monoclinic unique b
            ui->comboBoxCrystalSystem->setCurrentIndex(2);
        } else {
            // monoclinic unique c
            ui->comboBoxCrystalSystem->setCurrentIndex(3);
        }
    } else if (itNum >= 16 && itNum <= 74) {
        // orthorhombic
        ui->comboBoxCrystalSystem->setCurrentIndex(4);
    } else if (itNum >= 75 && itNum <= 142) {
        // tetragonal
        ui->comboBoxCrystalSystem->setCurrentIndex(5);
    } else if (itNum >= 143 && itNum <= 194) {
        // trigonal/hexagonal
        ui->comboBoxCrystalSystem->setCurrentIndex(7);
    } else if (itNum >= 195 && itNum <= 230) {
        // cubic
        ui->comboBoxCrystalSystem->setCurrentIndex(8);
    }

    ui->doubleSpinBoxCellA->setValue(0.1 * cell.a());
    ui->doubleSpinBoxCellB->setValue(0.1 * cell.b());
    ui->doubleSpinBoxCellC->setValue(0.1 * cell.c());
    ui->doubleSpinBoxCellAlpha->setValue(cell.alpha());
    ui->doubleSpinBoxCellBeta->setValue(cell.beta());
    ui->doubleSpinBoxCellGamma->setValue(cell.gamma());

    updateCrystalSystem();
}
