/***************************************************************************
                          gentemplatedialog.cpp  -  description
                             -------------------
    begin                : Tue Mar 12 11:00:00 CEST 2013
    copyright            : (C) 2012 by Nicola Doebelin
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

#include "gentemplatedialog.h"
#include "ui_gentemplatedialog.h"
#include <QtCore>
#include <QDebug>

GenTemplateDialog::GenTemplateDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::GenTemplateDialog)
{
    ui->setupUi(this);
    ui->spinBoxAngle->setValue(26.60);

    settings = SettingsManager::getInstance();

    tplString = "% Theoretical instrumental function\nVERZERR=\n% Wavelength\nLAMBDA=%%WL%%\n";
    tplString += "% Phases\n% Measured data\nVAL[1]=\n% Minimum Angle (2theta)\n% WMIN=10\n";
    tplString += "% Maximum Angle (2theta)\n% WMAX=60\n% Result list output\nLIST=\n";
    tplString += "% Peak list output\nOUTPUT=\n% Diagram output\nDIAGRAMM=\n";
    tplString += "% Global parameters for zero point and sample displacement\n";
    tplString += "EPS1=0\nPARAM[1]=EPS2=0_-0.01^0.01\nNTHREADS=2\nPROTOKOLL=Y\n";

    polString = "% Polarization\n";
    polString += "POL=sqr(cos(%%ANG%%*pi/180))\npi=2*acos(0)\n";
}

GenTemplateDialog::~GenTemplateDialog()
{
    delete ui;
}

void GenTemplateDialog::accept()
{
    settings->setValue("bgmnProject/lastTemplateWavelength", ui->comboBoxRadiation->currentText());
    QDialog::accept();
}

/*
 * sets the directory of BGMN, this is where we look for *.lam files
 */
void GenTemplateDialog::setBgmnDir(const QString &s)
{
    qDebug() << QString("GenTemplateDialog::setBgmnDir(): Searching LAM files in %1").arg(s);
    ui->comboBoxRadiation->clear();
    QStringList lam = getWavelengths(s);
    ui->comboBoxRadiation->addItems(lam);
    ui->comboBoxRadiation->setCurrentText(settings->value("bgmnProject/lastTemplateWavelength", "CU").toString());
}

/*
 * returns the complete string of a template file
 */
QString GenTemplateDialog::getString()
{
    QString str = tplString;
    str.replace(QRegularExpression("%%WL%%"), ui->comboBoxRadiation->currentText());

    if (ui->radioButtonMonochromator->isChecked()) {
        str += polString;
        double ang = ui->spinBoxAngle->value();
        str.replace(QRegularExpression("%%ANG%%"), QString("%1").arg(ang));
    }

    return str;
}

/*
 * returns the basenames of all *.lam files in the directory "s"
 */
QStringList GenTemplateDialog::getWavelengths(const QString &s)
{
    qDebug() << QString("GenTemplateDialog::getWavelengths(): Scanning BGMN Directory %1").arg(s);

    QStringList filters;
    filters << "*.lam" << "*.LAM";

    QDir dir(s);
    QFileInfoList flist = dir.entryInfoList(filters, QDir::Files);

    QStringList lst;
    for (int i = 0; i < flist.size(); ++i) {
        QFileInfo fi = flist.at(i);
        qDebug() << QString("GenTemplateDialog::getWavelengths(): Found file %1").arg(fi.completeBaseName());

        lst.append(fi.completeBaseName());
    }

    return lst;
}
