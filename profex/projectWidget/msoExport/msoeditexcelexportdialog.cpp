/***************************************************************************
                          msoeditexcelexport.cpp  -  description
                             -------------------
    begin                : Sun Aug 09 08:25:00 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#include "msoeditexcelexportdialog.h"
#include "ui_msoeditexcelexportdialog.h"
#include <QDir>
#include <QFileDialog>
#include <QComboBox>
#include <QSpinBox>
#include <QCheckBox>
#include <QDebug>

MsoEditExcelExportDialog::MsoEditExcelExportDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::MsoEditExcelExportDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();

    QStringList hd;
    hd << tr("Phase") << tr("Parameter") << tr("ESD") << tr("Worksheet") << tr("Row") << tr("Column") << tr("Filter RegExp") << tr("Output");
    ui->tableWidgetConfig->setColumnCount(hd.size());
    ui->tableWidgetConfig->setHorizontalHeaderLabels(hd);

    restoreGeometry(settings->value("ExcelExport/dlgGeometry", QByteArray()).toByteArray());
}

MsoEditExcelExportDialog::~MsoEditExcelExportDialog()
{
    settings->setValue("ExcelExport/dlgGeometry", saveGeometry());
    delete ui;
}

void MsoEditExcelExportDialog::initData(const QMap<QString, QVariant> &savData,
                                        const QMap<QString, QList<global::Result> > &lstData,
                                        const QList<global::AxObjectExcel> &cfg)
{
    checkFileButtons(cfg);
    updateParameterMap(savData, lstData);
    updateTable(cfg);
}

void MsoEditExcelExportDialog::checkFileButtons(const QList<global::AxObjectExcel> &cfg)
{
    if (!cfg.size()) {
        ui->radioButtonNew->setChecked(true);
    } else if (cfg.first().file == "%%new%%") {
        ui->radioButtonNew->setChecked(true);
    } else if (cfg.first().file == "%%project%%") {
        ui->radioButtonFileProject->setChecked(true);
    } else {
        ui->radioButtonFileName->setChecked(true);
        ui->lineEditExcelFile->setText(cfg.first().file);
    }
}

void MsoEditExcelExportDialog::updateParameterMap(const QMap<QString, QVariant> &savData,
                                                  const QMap<QString, QList<global::Result> > &lstData)
{
    parameterMap.clear();

    QMapIterator<QString, QVariant> itSav(savData);

    while (itSav.hasNext()) {
        itSav.next();
        if (parameterMap.contains("GLOBAL")) {
            parameterMap["GLOBAL"].append(itSav.key());
        } else {
            parameterMap.insert("GLOBAL", QStringList(itSav.key()));
        }
    }

    QMapIterator<QString, QList<global::Result> > itLst(lstData);

    while (itLst.hasNext()) {
        itLst.next();

        if (!itLst.value().size()) continue;

        if (!parameterMap.contains(itLst.key())) {
            parameterMap.insert(itLst.key(), QStringList());
        }

        for (int i = 0; i < itLst.value().size(); ++i) {
            parameterMap[itLst.key()].append(itLst.value().at(i).name);
        }
    }
}

void MsoEditExcelExportDialog::updateTable(const QList<global::AxObjectExcel> &cfg)
{
    for (int i = 0; i < cfg.size(); ++i) {
        if (parameterMap.contains(cfg.at(i).phase)) {
            QString paramName = cfg.at(i).parameter.right(3) == "ESD" ? cfg.at(i).parameter.left(cfg.at(i).parameter.length() - 3) : cfg.at(i).parameter;

            if (parameterMap.value(cfg.at(i).phase).contains(paramName)) {
                addTableRow(cfg.at(i));
            }
        }
    }
}

void MsoEditExcelExportDialog::addTableRow(const global::AxObjectExcel &par)
{
    int n = ui->tableWidgetConfig->rowCount();
    ui->tableWidgetConfig->setRowCount(n + 1);

    QString paramName = par.parameter.right(3) == "ESD" ? par.parameter.left(par.parameter.length() - 3) : par.parameter;
    bool isEsd = par.parameter.right(3) == "ESD";

    QComboBox *cbPhase = new QComboBox;
    connect(cbPhase, SIGNAL(currentIndexChanged(int)), this, SLOT(updateParamCBox()));
    cbPhase->addItems(parameterMap.keys());
    cbPhase->setCurrentText(par.phase);

    QComboBox *cbParam = new QComboBox;
    cbParam->addItems(parameterMap.value(par.phase));
    cbParam->setCurrentText(paramName);

    QCheckBox *ckbEsd = new QCheckBox;
    ckbEsd->setChecked(isEsd);

    QSpinBox *sbWorksheet = new QSpinBox;
    sbWorksheet->setMinimum(1);
    sbWorksheet->setMaximum(99);
    sbWorksheet->setValue(par.worksheet);

    QSpinBox *sbRow = new QSpinBox;
    sbRow->setMinimum(1);
    sbRow->setMaximum(1048576); //1,048,576 rows by 16,384 columns
    sbRow->setValue(par.row);

    QSpinBox *sbCol = new QSpinBox;
    sbCol->setMinimum(1);
    sbCol->setMaximum(16384);
    sbCol->setValue(par.col);

    ui->tableWidgetConfig->setCellWidget(n, 0, cbPhase);
    ui->tableWidgetConfig->setCellWidget(n, 1, cbParam);
    ui->tableWidgetConfig->setCellWidget(n, 2, ckbEsd);
    ui->tableWidgetConfig->setCellWidget(n, 3, sbWorksheet);
    ui->tableWidgetConfig->setCellWidget(n, 4, sbRow);
    ui->tableWidgetConfig->setCellWidget(n, 5, sbCol);
    ui->tableWidgetConfig->setItem(n, 6, new QTableWidgetItem(par.filter));
    ui->tableWidgetConfig->setItem(n, 7, new QTableWidgetItem(par.output));
}

QList<global::AxObjectExcel> MsoEditExcelExportDialog::getConfig()
{
    QString file;
    if (ui->radioButtonFileProject->isChecked()) file = "%%project%%";
    else if (ui->radioButtonNew->isChecked())    file = "%%new%%";
    else                                         file = ui->lineEditExcelFile->text();

    QRegularExpression rx("([^\\/])[\\/]([^\\/])");
    file.replace(rx, "\\1\\\\2");

    qDebug() << QString("MsoEditExcelExportDialog::getConfig(): File is %1").arg(file);

    // return file name with double backslashes!
    QList<global::AxObjectExcel> cfg;

    for (int i = 0; i < ui->tableWidgetConfig->rowCount(); ++i) {
        global::AxObjectExcel exclOb;

        exclOb.file = file;
        exclOb.phase     = qobject_cast<QComboBox*>(ui->tableWidgetConfig->cellWidget(i, 0))->currentText();
        QString par = qobject_cast<QComboBox*>(ui->tableWidgetConfig->cellWidget(i, 1))->currentText();
        bool isEsd = qobject_cast<QCheckBox*>(ui->tableWidgetConfig->cellWidget(i, 2))->isChecked();

        exclOb.parameter = isEsd ? par + "ESD" : par;

        exclOb.worksheet = qobject_cast<QSpinBox*>(ui->tableWidgetConfig->cellWidget(i, 3))->value();
        exclOb.row       = qobject_cast<QSpinBox*>(ui->tableWidgetConfig->cellWidget(i, 4))->value();
        exclOb.col       = qobject_cast<QSpinBox*>(ui->tableWidgetConfig->cellWidget(i, 5))->value();
        exclOb.filter    = ui->tableWidgetConfig->item(i, 6)->text();
        exclOb.output    = ui->tableWidgetConfig->item(i, 7)->text();

        cfg.append(exclOb);
    }

    return cfg;
}

void MsoEditExcelExportDialog::selectFile()
{
    QString f = ui->lineEditExcelFile->text().isEmpty() ? QDir::homePath() : ui->lineEditExcelFile->text();
    f = QFileDialog::getOpenFileName(this, tr("Select Excel File"), f);

    if (!f.isEmpty()) {
        ui->lineEditExcelFile->setText(f);
        ui->radioButtonFileName->setChecked(true);
    }
}

void MsoEditExcelExportDialog::addParam()
{
    global::AxObjectExcel exclOb;
    exclOb.phase = "GLOBAL";
    addTableRow(exclOb);
}

void MsoEditExcelExportDialog::removeParam()
{
    ui->tableWidgetConfig->model()->removeRow(ui->tableWidgetConfig->currentRow());
}

void MsoEditExcelExportDialog::updateParamCBox()
{
    int n = -1;

    for (int i = 0; i < ui->tableWidgetConfig->rowCount(); ++i) {
        if (ui->tableWidgetConfig->cellWidget(i, 0) == QObject::sender()) {
            n = i;
            break;
        }
    }

    if (n < 0) return;

    QComboBox *cbPhase  = qobject_cast<QComboBox*>(ui->tableWidgetConfig->cellWidget(n, 0));
    QComboBox *cbParams = qobject_cast<QComboBox*>(ui->tableWidgetConfig->cellWidget(n, 1));

    if (!cbPhase || !cbParams) return;

    cbParams->clear();
    cbParams->addItems(parameterMap.value(cbPhase->currentText()));
}
