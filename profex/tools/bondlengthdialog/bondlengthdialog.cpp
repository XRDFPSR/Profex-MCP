/***************************************************************************
                          bondlengthdialog.cpp  -  description
                             -------------------
    begin                : Mon Apr 22 20:51:00 CEST 2024
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

#include "bondlengthdialog.h"
#include "ui_bondlengthdialog.h"
#include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/structs.h"
#include "projectWidget/bgmnprojectwidget.h"
#include <QStringList>
#include <QCloseEvent>

BondLengthDialog::BondLengthDialog(QWidget *parent)
    : AbstractToolDialog(parent)
    , ui(new Ui::BondLengthDialog)
{
    ui->setupUi(this);

    QString strWidth;
    strWidth.fill('O', 6);
    QFontMetrics fm(ui->tableWidget->font());
    ui->tableWidget->horizontalHeader()->setDefaultSectionSize(fm.horizontalAdvance(strWidth));
    ui->labelTable->setText(tr("Interatomic distances [") + global::angstrom + tr("]"));

    project = nullptr;
    prgDlg = nullptr;
    initSettings();
}

BondLengthDialog::~BondLengthDialog()
{
    delete ui;
}

void BondLengthDialog::initSettings()
{
    restoreGeometry(settings->value("bondLengthDialog/geometry", QByteArray()).toByteArray());
    ui->splitter->restoreState(settings->value("bondLengthDialog/splitter", QByteArray()).toByteArray());
    ui->checkBoxFilter->setChecked(settings->value("bondLengthDialog/filterChecked", false).toBool());
    ui->doubleSpinBoxFilter->setValue(settings->value("bondLengthDialog/filterValue", 0.2).toDouble());
}

void BondLengthDialog::saveSettings()
{
    settings->setValue("bondLengthDialog/geometry", saveGeometry());
    settings->setValue("bondLengthDialog/splitter", ui->splitter->saveState());
    settings->setValue("bondLengthDialog/filterChecked", ui->checkBoxFilter->isChecked());
    settings->setValue("bondLengthDialog/filterValue", ui->doubleSpinBoxFilter->value());
}

void BondLengthDialog::closeEvent(QCloseEvent *e)
{
    saveSettings();
    e->accept();
}

void BondLengthDialog::postSetProject(ProjectWidget *p)
{
    project = dynamic_cast<BgmnProjectWidget *>(p);
    updateData();
}

void BondLengthDialog::updateData()
{
    if (!project) return;

    QStringList phases = project->getPhaseNames();
    bool oldState = ui->listWidgetPhases->blockSignals(true);
    ui->listWidgetPhases->clear();

    for (int i = 0; i < phases.size(); ++i) {
        ui->listWidgetPhases->addItem(phases.at(i));
    }

    ui->listWidgetPhases->setCurrentRow(0);
    ui->listWidgetPhases->blockSignals(oldState);

    if (!prgDlg) {
        prgDlg = new QProgressDialog(this);
        prgDlg->setCancelButton(nullptr);
        connect(&_data, SIGNAL(setProgress(int,QString)), this, SLOT(updateProgress(int,QString)));
    }

    prgDlg->setMaximum(phases.size());
    prgDlg->show();

    qApp->processEvents();

    double filter = ui->checkBoxFilter->isChecked() ? ui->doubleSpinBoxFilter->value() : -1.0;
    _data.setData(project->getStructuresRefined(), filter);
    updateTable();
    prgDlg->close();
    prgDlg->reset();
}

void BondLengthDialog::clearGui()
{
    bool oldStateTable = ui->tableWidget->blockSignals(true);
    bool oldStateList = ui->listWidgetPhases->blockSignals(true);

    ui->tableWidget->clear();
    ui->tableWidget->setRowCount(0);
    ui->tableWidget->setColumnCount(0);

    ui->listWidgetPhases->clear();

    ui->tableWidget->blockSignals(oldStateTable);
    ui->listWidgetPhases->blockSignals(oldStateList);
}

void BondLengthDialog::updateTable()
{
    QString phase = ui->listWidgetPhases->currentItem()->text();
    if (phase.isEmpty()) return;

    bool oldState = ui->tableWidget->blockSignals(true);
    ui->tableWidget->clear();

    const QStringList header = _data.tableHeader(phase);

    ui->tableWidget->setRowCount(_data.rowCount(phase));
    ui->tableWidget->setColumnCount(_data.columnCount(phase));

    ui->tableWidget->setHorizontalHeaderLabels(header);
    ui->tableWidget->setVerticalHeaderLabels(header);

    populateTable(phase);

    ui->tableWidget->blockSignals(oldState);
}

void BondLengthDialog::populateTable(const QString &phase)
{
    if (!_data.contains(phase)) return;
    QList<QStringList> tableData = _data.tableData(phase);

    int rc = _data.rowCount(phase);
    int cc = _data.columnCount(phase);

    for (int r = 0; r < rc; ++r) {
        for (int c = 0; c < cc; ++c) {
            QString txt(tableData.at(r).at(c));
            QTableWidgetItem *it = ui->tableWidget->item(r, c);

            if (it) {
                it->setText(txt);
            } else {
                it = new QTableWidgetItem(txt);
                ui->tableWidget->setItem(r, c, it);
            }
        }
    }
}

void BondLengthDialog::saveData()
{
    QString f = QFileDialog::getSaveFileName(this, tr("Save data"), graphProject->workingDir(), tr("Comma separated values (*.csv *.CSV)"));
    if (f.isEmpty()) return;

    QStringList out(ui->listWidgetPhases->currentItem()->text());
    QStringList hheader("");

    for (int c = 0; c < ui->tableWidget->columnCount(); ++c) {
        hheader.append(ui->tableWidget->horizontalHeaderItem(c)->text());
    }

    out.append(hheader.join(";"));

    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        QStringList line(ui->tableWidget->verticalHeaderItem(r)->text());

        for (int c = 0; c < ui->tableWidget->columnCount(); ++c) {
            line.append(ui->tableWidget->item(r, c)->text());
        }

        out.append(line.join(";"));
    }

    BgmnFileIO::writeTextFile(f, out.join("\n"));
}

void BondLengthDialog::updateProgress(int n, QString s)
{
    if (prgDlg) {
        prgDlg->setValue(n);
        prgDlg->setLabelText(s);
        qApp->processEvents();
    }
}
