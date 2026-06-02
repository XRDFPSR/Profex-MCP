/***************************************************************************
                          oxidedialog.cpp  -  description
                             -------------------
    begin                : Wed Aug 20 12:12:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#include "oxidedialog.h"
#include "ui_oxidedialog.h"
#include "../libXrdIO/structs.h"

OxideDialog::OxideDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::OxideDialog)
{
    ui->setupUi(this);

    // defaultAtoms is read from structs.h
    QStringList lst = global::atoms.split(";");
    for (int i = 0; i < lst.size() - 4; i += 5) {
        atWeights.insert(lst.at(i + 1).toUpper(), lst.at(i + 2).toDouble());
    }
}

OxideDialog::~OxideDialog()
{
    delete ui;
}

void OxideDialog::recalc()
{
    int ion = ui->spinBoxIon->value();
    int oxi = ui->spinBoxOxygen->value();
    QString strIon = ui->labelIon->text();
    QString strOxi = "O";
    QString strIonIdx = ion > 1 ? QString::number(ion) : "";
    QString strOxiIdx = oxi > 1 ? QString::number(oxi) : "";
    double wtIon = 0.0;

    if (atWeights.contains(strIon.toUpper())) {
        wtIon = atWeights.value(strIon.toUpper());
    }

    mWeight = wtIon * (double)ion + atWeights.value(strOxi) * (double)oxi;

    oxName = QString();
    QString oxNameDisplay;

    if (ion > 0) {
        oxName += strIon + strIonIdx;
        oxNameDisplay = strIon;
        if (ion > 1) {
            oxNameDisplay += QString("<sub>%1</sub>").arg(strIonIdx);
        }
    }

    if (oxi > 0) {
        oxName += strOxi + strOxiIdx;
        oxNameDisplay += strOxi;
        if (oxi > 1) {
            oxNameDisplay += QString("<sub>%1</sub>").arg(strOxiIdx);
        }
    }

    ui->labelMolWeight->setText(QString("%1 = %5 g/mol").arg(oxNameDisplay).arg(mWeight, 0, 'f', 6));
}

void OxideDialog::setOxide(const QString &s)
{
    QRegularExpression rx("([A-Za-z]+)(\\d*)O(\\d*)");
    QRegularExpressionMatch rm = rx.match(s);

    if (rm.hasMatch()) {
        ui->labelIon->setText(rm.captured(1));

        if (rm.captured(2).isEmpty()) {
            ui->spinBoxIon->setValue(1);
        } else {
            ui->spinBoxIon->setValue(rm.captured(2).toInt());
        }

        if (rm.captured(3).isEmpty()) {
            ui->spinBoxOxygen->setValue(1);
        } else {
            ui->spinBoxOxygen->setValue(rm.captured(3).toInt());
        }
    } else {
        ui->labelIon->setText(s);
        ui->spinBoxIon->setValue(1);
        ui->spinBoxOxygen->setValue(0);
    }

    recalc();
}

void OxideDialog::setIon(const QString &s)
{
    ui->labelIon->setText(s);
    ui->spinBoxIon->setValue(1);
    ui->spinBoxOxygen->setValue(0);
    recalc();
}

QString OxideDialog::getOxide()
{
    recalc();
    return oxName;
}

double OxideDialog::getMolWeight()
{
    recalc();
    return mWeight;
}
