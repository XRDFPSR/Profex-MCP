/***************************************************************************
                          prefpagefullprofconfig.cpp  -  description
                             -------------------
    begin                : Wed May 10 10:00:00 CEST 2017
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

#include "prefpagechemicalcomp.h"
#include "oxidedialog.h"
#include "../libXrdIO/structs.h"
#include "ui_prefpagechemicalcomp.h"

#include <QMessageBox>
#include <QComboBox>

PrefPageChemicalComp::PrefPageChemicalComp(QWidget *parent) :
    PrefPageTemplate(parent),
    ui(new Ui::PrefPageChemicalComp)
{
    ui->setupUi(this);
}

PrefPageChemicalComp::~PrefPageChemicalComp()
{
    delete ui;
}

void PrefPageChemicalComp::initUi()
{
    connect(ui->oxideTableWidget, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(editChemTableCell(int,int)));
    connect(ui->toolButtonChemReset, SIGNAL(clicked(bool)), this, SLOT(resetChemistry()));

    ui->comboBoxMode->addItem("Elements by weight-%");
    ui->comboBoxMode->addItem("Elements by atomic-%");
    ui->comboBoxMode->addItem("Oxides by weight-%");

    initSettings();
}

void PrefPageChemicalComp::initSettings()
{
    setOxides(settings->value("chemistry/oxides", defaultOxideWeights()).toStringList());

    int m = settings->value("chemistry/mode", 0).toInt();
    ui->comboBoxMode->setCurrentIndex(m);

    ui->oxideTableWidget->setEnabled(m == 2);
    ui->oxideTableWidget->horizontalHeader()->restoreState(settings->value("preferencesDialog/chemCompHeaderState", QByteArray()).toByteArray());
}

void PrefPageChemicalComp::saveSettings()
{
    settings->setValue("chemistry/oxides", getOxides());
    settings->setValue("chemistry/mode", ui->comboBoxMode->currentIndex());
    settings->setValue("preferencesDialog/chemCompHeaderState", ui->oxideTableWidget->horizontalHeader()->saveState());
}

/*
 * populates the tableWidget with oxide molecular weights
 */
void PrefPageChemicalComp::setOxides(const QStringList &l)
{
    // a collection of all known element symbols. This is used to loop over the elements in the correct order
    QStringList atomList = global::atoms.split(";", Qt::KeepEmptyParts);
    QStringList allElements;

    for (int i = 0; i < atomList.size() - 4; i += 5) {
        allElements << atomList.at(i+1);
    }

    // create a hash with all element symbols and empty oxide data
    QHash<QString, QStringList> allElementData;
    for (int i = 0; i < allElements.size(); ++i) {
        allElementData.insert(allElements.at(i), QStringList());
    }

    // now copy all oxide information obtained from the function call to the hash for easy access
    for (int i = 0; i < l.size() - 2; i += 3) {
        QStringList data;
        //      oxide name     mol weight     used
        data << l.at(i + 1) << l.at(i + 2);
        allElementData[l.at(i)] = data;
    }

    // prepare the table widget
    ui->oxideTableWidget->setRowCount(allElements.size());
    ui->oxideTableWidget->setColumnCount(3);
    QStringList headers;
    headers << "Element" << "Oxide" << "Molecular Weight";
    ui->oxideTableWidget->setHorizontalHeaderLabels(headers);

    // loop over all elements and populate the table. We will loop over the stringList allElements, because
    // it contains the elements in the correct order. A QHash::iterator would be randomly sorted.
    for (int i = 0; i < allElements.size(); ++i) {
        QString at = allElements.at(i);

        if (at.isEmpty()) {
            continue;
        }

        QStringList data = allElementData.value(at);

        QTableWidgetItem *itAtom = new QTableWidgetItem(at);
        itAtom->setFlags(Qt::ItemIsEnabled);
        ui->oxideTableWidget->setItem(i, 0, itAtom);

        if (data.size() > 1) {
            ui->oxideTableWidget->setItem(i, 1, new QTableWidgetItem(data.at(0)));
            ui->oxideTableWidget->setItem(i, 2, new QTableWidgetItem(data.at(1)));
        }
    }
}

/*
 * returns the oxides and molecular weights as a stringlist
 */
QStringList PrefPageChemicalComp::getOxides()
{
    QStringList l;

    for (int i = 0; i < ui->oxideTableWidget->rowCount(); ++i) {
        for (int j = 0; j < ui->oxideTableWidget->columnCount(); ++j) {
            QTableWidgetItem *it = ui->oxideTableWidget->item(i, j);
            if (it) {
                l << it->text().simplified();
            } else {
                l << QString();
            }

        }
    }

    return l;
}

/*
 * this is a set of hard-coded default oxide molecular weights. It will be
 * used if no custom values were entered.
 */
QStringList PrefPageChemicalComp::defaultOxideWeights()
{
    QStringList atomList = global::atoms.split(";", Qt::KeepEmptyParts);
    QStringList oxideList;

    for (int i = 0; i < atomList.size() - 4; i += 5) {
        oxideList << atomList.at(i+1) << atomList.at(i+3) << atomList.at(i+4);
    }

    return oxideList;
}

void PrefPageChemicalComp::editChemTableCell(int row, int col)
{
    // only process cells in col 0
    if (col) {
        return;
    }

    OxideDialog *oxdlg = new OxideDialog(this);
    QTableWidgetItem *itElement = ui->oxideTableWidget->item(row, 0);
    QTableWidgetItem *itOxide = ui->oxideTableWidget->item(row, 1);
    QTableWidgetItem *itWeight = ui->oxideTableWidget->item(row, 2);

    // if the table did not contain an item with the element, exit
    if (!itElement) {
        return;
    }

    // if the table did not contain an item for oxide, create one
    if (!itOxide) {
        itOxide = new QTableWidgetItem();
        ui->oxideTableWidget->setItem(row, 1, itOxide);
    }

    // if the table did not contain an item for the weight, create one
    if (!itWeight) {
        itWeight = new QTableWidgetItem();
        ui->oxideTableWidget->setItem(row, 2, itWeight);
    }

    if (itOxide->text().isEmpty()) {
        oxdlg->setIon(itElement->text());
    } else {
        oxdlg->setOxide(itOxide->text());
    }

    if (oxdlg->exec() == QDialog::Accepted) {
        itOxide->setText(oxdlg->getOxide());
        itWeight->setText(QString::number(oxdlg->getMolWeight(), 'f', 6));
    }

    delete oxdlg;
}

void PrefPageChemicalComp::resetChemistry()
{
    if (QMessageBox::question(this,
                              tr("Reset Table"),
                              tr("Restore default values?\nAll custom values will be lost."))
            == QMessageBox::No) {
        return;
    }

    ui->oxideTableWidget->clear();
    setOxides(defaultOxideWeights());
}

void PrefPageChemicalComp::currentMode(int i)
{
    ui->oxideTableWidget->setEnabled(i == 2);
}
