/***************************************************************************
                          appenddialog.cpp  -  description
                             -------------------
    begin                : Tue July 12 17:00:00 CEST 2011
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

#include <QFileInfo>
#include <QDir>
#include <QFile>
#include <QTextStream>
#include "fpaddremovedialog.h"
#include "../libXrdIO/parser/fppcrparser.h"
#include "ui_addremovephasedialog.h"


FpAddRemoveDialog::FpAddRemoveDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AddRemovePhaseDialog)
{
    ui->setupUi(this);
    ui->checkBoxOverwrite->hide();
    ui->checkBoxDeleteFiles->hide();
    ui->toolButtonFavorites->hide();
    ui->labelFilter->hide();
    ui->lineEditFilter->hide();
    ui->toolButtonClearFilter->hide();
    ui->toolButtonFilterOptions->hide();

    settings = SettingsManager::getInstance();

    QStringList header;
    header << "Phase";
    ui->treeWidgetStructures->setColumnCount(header.size());
    ui->treeWidgetStructures->setHeaderLabels(header);

    QRect geo = settings->value("fpProject/appendDialog/geometry", QVariant(QRect())).toRect();
    if (!geo.isEmpty()) resize(geo.width(), geo.height());
    initSettings();
}

FpAddRemoveDialog::~FpAddRemoveDialog()
{
    settings->setValue("fpProject/appendDialog/geometry", geometry());
    settings->setValue("fpProject/defaultInstrument", ui->comboBoxDevices->currentText());
    // settings->setValue("fpProject/addRemoveDialogSplitter", ui->splitter->saveState());
}

void FpAddRemoveDialog::initSettings()
{
    setStrDir(settings->value("fpProject/structureDatabase", "").toString());
    setDevDir(settings->value("fpProject/deviceDatabase", "").toString());
    // ui->splitter->restoreState(settings->value("fpProject/addRemoveDialogSplitter", "").toByteArray());
}

void FpAddRemoveDialog::setFilters()
{
    devExt << "*.pcd" << "*.PCD";
    strExt << "*.pcp" << "*.PCP";
}

QString FpAddRemoveDialog::getString()
{
    FpPcrParser pparser;
    FpPcrParser tplParser;
    int phases = 0;
    double dparam[11] = {0.0};

    // create a new file (including the header) from a template file
    if (ui->checkBoxGenerateControlFile->isChecked()) {
        // read the header from the template
        QString dfs = ui->comboBoxDevices->itemData(ui->comboBoxDevices->currentIndex(), Qt::UserRole).toString();
        QFile tplSource(dfs);

        if (!tplSource.open(QIODevice::ReadOnly | QIODevice::Text)) {
            qDebug() << QString("FpAppendDialog: Could not read template file %1").arg(dfs);
            return QString();
        }

        // create a temporary pcrparser and set the content from the template
        QTextStream in(&tplSource);
        text = in.readAll();
        tplSource.close();

        tplParser.setContentsString(text);
        tplParser.getDevice(dparam[0], dparam[1], dparam[2], dparam[3], dparam[4], dparam[5], dparam[6], dparam[7], dparam[8], dparam[9], dparam[10]);

        // read the header and device info
        text = tplParser.getHeader();

        // write the parsed header to the pcrParser
        pparser.setContentsString(text);
        pparser.setNumberOfPhases(0);

        // set the dat file name
        QFileInfo fiDatFile(projectDir + "/" + projectScanFile);
        pparser.setDataFileName(fiDatFile.absolutePath() + "/" + fiDatFile.baseName() + ".dat");
    } else {
        // read the content of the pcrParser from the existing file
        pparser.setContentsString(text);

        // read the device parameters
        // CAUTION: If the file only contained the header but no phase, this will fail.
        pparser.getDevice(dparam[0], dparam[1], dparam[2], dparam[3], dparam[4], dparam[5], dparam[6], dparam[7], dparam[8], dparam[9], dparam[10]);
    }

    // remove phases
    // *************

    phases = pparser.getNumberOfPhases();

    for (int i = 0; i < ui->treeWidgetRemovePhases->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetRemovePhases->topLevelItem(i);
        qDebug() << QString("FpRemoveDialog::getString(): found phase %1").arg(it->text(0));

        if (it->checkState(0) == Qt::Checked) {
            QString phase = it->text(0);
            pparser.removePhase(phase);
            phases--;
            qDebug() << QString("FpRemoveDialog::getString(): removing phase %1").arg(phase);
        }
    }

    pparser.setNumberOfPhases(phases);

    // add phases
    // **********

    phases = pparser.getNumberOfPhases();

    // compose all checked files
    for (int i = 0; i < ui->treeWidgetStructures->topLevelItemCount(); ++i) {
        if (ui->treeWidgetStructures->topLevelItem(i)->checkState(0) == Qt::Checked) {
            // read the content of the structure file
            QString dfp = ui->treeWidgetStructures->topLevelItem(i)->data(0, Qt::UserRole).toString();
            QFile strSource(dfp);

            if (! strSource.open(QIODevice::ReadOnly | QIODevice::Text)) {
                qDebug() << QString("FpAppendDialog: Could not read template file %1").arg(dfp);
                return QString();
            }

            QTextStream in(&strSource);
            QString ph = in.readAll();

            // append the phase to the text
            text.append(ph);
            phases++;
            pparser.appendPhase(ph);
        }
    }

    // set the new text and update the number of phases and device configuration
    pparser.setNumberOfPhases(phases);
    pparser.setDevice(dparam[0], dparam[1], dparam[2], dparam[3], dparam[4], dparam[5], dparam[6], dparam[7], dparam[8], dparam[9], dparam[10]);

    // return the string
    return pparser.getContentsString();
}

void FpAddRemoveDialog::setDevDir(const QString &s)
{
    QDir dir(s);
    setFilters();

    QFileInfoList l = dir.entryInfoList(devExt, QDir::Files | QDir::NoSymLinks);

    for (int i = 0; i < l.size(); ++i) {
        ui->comboBoxDevices->addItem(l.at(i).baseName(), l.at(i).absoluteFilePath());
    }

    int i = ui->comboBoxDevices->findText(settings->value("fpProject/defaultInstrument", "").toString());

    if (i >= 0) {
        ui->comboBoxDevices->setCurrentIndex(i);
    }

}

/*
 * reads all structure files in the directory "s" and populates the listWidget
 */
void FpAddRemoveDialog::setStrDir(const QString &s)
{
    QDir dir(s);
    setFilters();

    // get a list of all str files in the directory
    QFileInfoList l = dir.entryInfoList(strExt, QDir::Files | QDir::NoSymLinks);

    for (int i = 0; i < l.size(); ++i) {
        QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetStructures, QStringList(l.at(i).baseName()));
        it->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
        it->setCheckState(0, Qt::Unchecked);
        it->setData(0, Qt::UserRole, l.at(i).absoluteFilePath());
        ui->treeWidgetStructures->addTopLevelItem(it);
    }
}

/*
 * sets the content of the pcr file to s, and populates the removephase list
 */
void FpAddRemoveDialog::setText(const QString &s)
{
    text = s;

    if ((!text.contains("Job")) && (!text.contains("Npr")) && (!text.contains("Nph"))) {
        ui->checkBoxGenerateControlFile->setChecked(true);
    } else {
        FpPcrParser pparser;
        pparser.setContentsString(s);
        QStringList lst = pparser.getPhaseNames();

        for (int i = 0; i < lst.size(); ++i) {
            QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetRemovePhases);
            it->setFlags(Qt::ItemIsUserCheckable | Qt::ItemIsEnabled);
            it->setCheckState(0, Qt::Unchecked);
            it->setText(0, lst.at(i));
            ui->treeWidgetRemovePhases->addTopLevelItem(it);
            qDebug() << QString("FpAddRemoveDialog::setText(): Appending phase %1 to list widget").arg(lst.at(i));
        }
    }
}

void FpAddRemoveDialog::applyFilter(QString)
{
    /* todo */
}

void FpAddRemoveDialog::filterOptions(QAction *)
{
    /* todo */
}
