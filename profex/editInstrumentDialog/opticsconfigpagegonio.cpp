/***************************************************************************
                          optionsconfigpagegonio.cpp  -  description
                             -------------------
    begin                : Tue Jul 16 18:00:00 CEST 2020
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

#include "opticsconfigpagegonio.h"
#include <QFileInfo>
#include <QFileDialog>

OpticsConfigPageGonio::OpticsConfigPageGonio(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageGonioForm)

{
    ui->setupUi(this);
    connect(ui->toolButtonUNTload, SIGNAL(clicked(bool)), this, SLOT(loadBackground()));
    connect(ui->toolButtonUNTclear, SIGNAL(clicked(bool)), this, SLOT(clearBackground()));
}

QMap<QString, QString> OpticsConfigPageGonio::setParameters(const QMap<QString, QString> &p)
{
    bool ok;
    QMap<QString, QString> err;

    ui->doubleSpinBoxRadius->setValue(p.value("R", "0.0").toDouble(&ok));
    if (!ok) err["R"] = p.value("R", "0.0");

    ui->doubleSpinBoxWMin->setValue(p.value("WMIN", "2.0").toDouble(&ok));
    if (!ok) err["WMIN"] = p.value("WMIN", "2.0");

    ui->doubleSpinBoxWMax->setValue(p.value("WMAX", "150.0").toDouble(&ok));
    if (!ok) err["WMAX"] = p.value("WMAX", "150.0");

    return err;
}

QMap<QString, QString> OpticsConfigPageGonio::getParameters()
{
    QMap<QString, QString> map;

    map.insert("R", QString("%1").arg(ui->doubleSpinBoxRadius->value(), 0, 'f', 1));
    map.insert("WMIN", QString("%1").arg(ui->doubleSpinBoxWMin->value(), 0, 'f', 2));
    map.insert("WMAX", QString("%1").arg(ui->doubleSpinBoxWMax->value(), 0, 'f', 2));

    return map;
}

QString OpticsConfigPageGonio::helpText()
{
    QString out("<h1>Goniometer</h1>"
                "<ul><li><b>Goniometer radius:</b> Radius of the goniometer circle in mm. This length "
                "is equivalent to the distance form the sample to the focus point.</li>"
                "<li><b>Minimum angle:</b> Lower limit of the profile simulation range. Choose the lowest angle "
                "that is ever going to be measured on the instrument.</li>"
                "<li><b>Maximum angle:</b> Upper limit of the profile simulation range. Choose the highest angle "
                "that is ever going to be measured on the instrument.</li></ul>"
                "<h2>Project template parameters</h2>"
                "<p>The following parameters are not used for the profile calculation, but they are written "
                "to the project template file associated with the instrument configuration.</p>"
                "<ul><li><b>Background coefficients:</b> Specify a constant number of coefficients for the background "
                "polynomial. This value will override the default automatic choice of coefficients by BGMN.</li>"
                "<li><b>Measured background:</b> Select a scan only containing background signal, to be used "
                "with the UNT parameter in the project control file.</li></ul>");
    return css + out;
}

void OpticsConfigPageGonio::loadBackground()
{
    QStringList filters;
    filters.append("Free format XY scan (*.xy *.XY *.csv *.CSV *.xyp *.XYP)");
    filters.append("ASCII scan file (*.dat *.DAT *.asc *.ASC *.txt *.TXT)");
    filters.append("Seifert/FPM VAL scan (*.val *.VAL)");
    filters.append("All files (*.*)");

    QFileInfo fi(ui->lineEditUNT->text());
    QString f = QFileDialog::getOpenFileName(this, tr("Background scan"), fi.absolutePath(), filters.join(";;"));

    if (!f.isEmpty()) {
        ui->lineEditUNT->setText(f);
        ui->lineEditUNT->setToolTip(f);
    }
}

void OpticsConfigPageGonio::clearBackground()
{
    ui->lineEditUNT->clear();
    ui->lineEditUNT->setToolTip("UNT");
}

int OpticsConfigPageGonio::getRU() const
{
    if (ui->checkBoxRU->isChecked()) {
        return ui->spinBoxRU->value();
    }

    return -1;
}

QString OpticsConfigPageGonio::getUNT() const
{
    return ui->lineEditUNT->text();
}

void OpticsConfigPageGonio::setRU(int i)
{
    if (i < 0) {
        ui->checkBoxRU->setChecked(false);
    } else {
        ui->checkBoxRU->setChecked(true);
        ui->spinBoxRU->setValue(i);
    }
}

void OpticsConfigPageGonio::setUNT(const QString &s)
{
    ui->lineEditUNT->setText(s);
    ui->lineEditUNT->setToolTip(QString("UNT=%1").arg(s));
}
