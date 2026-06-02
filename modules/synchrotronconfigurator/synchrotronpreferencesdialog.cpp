/***************************************************************************
                          synchrotronpreferencesdialog.h  -  description
                             -------------------
    begin                : Tue Jan 21 18:39:00 CET 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#include <QDoubleValidator>
#include "synchrotronpreferencesdialog.h"
#include "ui_synchrotronpreferencesdialog.h"

SynchrotronPreferencesDialog::SynchrotronPreferencesDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SynchrotronPreferencesDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Preferences"));
    settings = SettingsManager::getInstance();

    QDoubleValidator *validator = new QDoubleValidator(0.0, 1.0, 4, this);
    validator->setNotation(QDoubleValidator::ScientificNotation);

    ui->lineEditHeightCutoff->setValidator(validator);
    ui->lineEditConvergenceCutoff->setValidator(validator);
    ui->lineEditAbsError->setValidator(validator);
    ui->lineEditRelError->setValidator(validator);

    initSettings();
}

SynchrotronPreferencesDialog::~SynchrotronPreferencesDialog()
{
    delete ui;
}

void SynchrotronPreferencesDialog::initSettings()
{
    ui->checkBoxSaveCurves->setChecked(settings->value("Data/saveCurveData", false).toBool());
    ui->checkBoxExpertOptions->setChecked(settings->value("Data/showExpertOptions", false).toBool());

    double heightCutoff = settings->value("Fitting/heightCutoff", 1e-9).toDouble();
    double convergenceCutoff = settings->value("Fitting/convergenceCutoff", 1e-6).toDouble();
    double absErrorCutoff = settings->value("Fitting/absoluteErrorCutoff", 1e-6).toDouble();
    double relErrorCutoff = settings->value("Fitting/relativeErrorCutoff", 1e-6).toDouble();

    ui->lineEditHeightCutoff->setText(QString("%1").arg(heightCutoff, 0, 'e', 4));
    ui->lineEditConvergenceCutoff->setText(QString("%1").arg(convergenceCutoff, 0, 'e', 4));
    ui->lineEditAbsError->setText(QString("%1").arg(absErrorCutoff, 0, 'e', 4));
    ui->lineEditRelError->setText(QString("%1").arg(relErrorCutoff, 0, 'e', 4));

    ui->doubleSpinBoxProfileCalcRange->setValue(settings->value("Data/profileRange", 25.0).toDouble());
    ui->spinBoxMaxCurves->setValue(settings->value("Fitting/maxCurves", 24).toInt());

    ui->checkBoxCurveElimination->setChecked(settings->value("Fitting/allowCurveElimination", false).toBool());
    ui->lineEditHeightCutoff->setEnabled(ui->checkBoxCurveElimination->isChecked());

    QList<QVariant> defaultDetectors;
    defaultDetectors << "Unknown" << -1.0 << "Si" << 2.3296 << "Cd Te" << 5.86;
    QList<QVariant> detectors = settings->value("Data/detectorList", defaultDetectors).toList();

    addDetectorList(detectors);
}

void SynchrotronPreferencesDialog::saveSettings()
{
    settings->setValue("Data/saveCurveData", ui->checkBoxSaveCurves->isChecked());
    settings->setValue("Data/showExpertOptions", ui->checkBoxExpertOptions->isChecked());
    settings->setValue("Fitting/heightCutoff", ui->lineEditHeightCutoff->text().toDouble());
    settings->setValue("Fitting/convergenceCutoff", ui->lineEditConvergenceCutoff->text().toDouble());
    settings->setValue("Fitting/absoluteErrorCutoff", ui->lineEditAbsError->text().toDouble());
    settings->setValue("Fitting/relativeErrorCutoff", ui->lineEditRelError->text().toDouble());
    settings->setValue("Data/profileRange", ui->doubleSpinBoxProfileCalcRange->value());
    settings->setValue("Fitting/maxCurves", ui->spinBoxMaxCurves->value());
    settings->setValue("Fitting/allowCurveElimination", ui->checkBoxCurveElimination->isChecked());

    QList<QVariant> detectors;
    for (int i = 0; i < ui->treeWidgetDetectors->topLevelItemCount(); ++i) {
        detectors << ui->treeWidgetDetectors->topLevelItem(i)->text(0);
        detectors << ui->treeWidgetDetectors->topLevelItem(i)->text(1).toDouble();
    }

    settings->setValue("Data/detectorList", detectors);
}

void SynchrotronPreferencesDialog::accept()
{
    saveSettings();
    QDialog::accept();
}

void SynchrotronPreferencesDialog::reset()
{
    ui->checkBoxSaveCurves->setChecked(false);
    ui->lineEditHeightCutoff->setText("1.0000e-09");
    ui->lineEditConvergenceCutoff->setText("1.0000e-06");
    ui->lineEditAbsError->setText("1.0000e-06");
    ui->lineEditRelError->setText("1.0000e-06");
    ui->doubleSpinBoxProfileCalcRange->setValue(10.0);
    ui->spinBoxMaxCurves->setValue(24);
    ui->checkBoxCurveElimination->setChecked(false);

    ui->treeWidgetDetectors->clear();
    QList<QVariant> defaultDetectors;
    defaultDetectors << "Unknown" << -1.0 << "Si" << 2.3296 << "Cd Te" << 5.86;
    addDetectorList(defaultDetectors);
}

void SynchrotronPreferencesDialog::removeDetector()
{
    QTreeWidgetItem *it = ui->treeWidgetDetectors->currentItem();
    if (!it) return;

    int i = ui->treeWidgetDetectors->indexOfTopLevelItem(it);
    if (i < 0) return;

    ui->treeWidgetDetectors->takeTopLevelItem(i);
    delete it;
}

void SynchrotronPreferencesDialog::addDetector()
{
    QTreeWidgetItem *it = new QTreeWidgetItem(QStringList()
                                              << "<Material>"
                                              << "<Density >");
    it->setFlags(it->flags() | Qt::ItemIsEditable);
    ui->treeWidgetDetectors->addTopLevelItem(it);
}

void SynchrotronPreferencesDialog::addDetectorList(const QList<QVariant> &lst)
{
    for (int i = 0; i < lst.size() - 1; i += 2) {
        QTreeWidgetItem *it = new QTreeWidgetItem;
        it->setFlags(it->flags() | Qt::ItemIsEditable);
        it->setText(0, lst.at(i).toString());
        it->setText(1, QString::number(lst.at(i+1).toDouble()));
        ui->treeWidgetDetectors->addTopLevelItem(it);
    }
}
