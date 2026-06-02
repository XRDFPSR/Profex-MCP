/***************************************************************************
                          optionsconfigpagepcol.cpp  -  description
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

#include "opticsconfigpagepcol.h"
#include <QtMath>

OpticsConfigPagePcol::OpticsConfigPagePcol(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPagePcolForm)

{
    ui->setupUi(this);

    connect(ui->radioButtonRad, SIGNAL(toggled(bool)), ui->doubleSpinBoxPcollR, SLOT(setEnabled(bool)));
    connect(ui->radioButtonRad, SIGNAL(toggled(bool)), ui->doubleSpinBoxPcollD, SLOT(setDisabled(bool)));
    connect(ui->doubleSpinBoxPcollR, SIGNAL(valueChanged(double)), this, SLOT(radChanged(double)));
    connect(ui->doubleSpinBoxPcollD, SIGNAL(valueChanged(double)), this, SLOT(degChanged(double)));
}

QMap<QString, QString> OpticsConfigPagePcol::setParameters(const QMap<QString, QString> &p)
{
    setInstalled(p.contains("PColl"));

    bool ok;
    QMap<QString, QString> err;

    double d = p.value("PColl", "0.0").toDouble(&ok);
    if (!ok) err["PColl"] = p.value("PColl", "0.0");

    if (d < 0.2) { // assume radians
        ui->radioButtonRad->setChecked(true);
        ui->radioButtonDeg->setChecked(false);
        ui->doubleSpinBoxPcollR->setEnabled(true);
        ui->doubleSpinBoxPcollD->setEnabled(false);
        ui->doubleSpinBoxPcollR->setValue(d);
        ui->doubleSpinBoxPcollD->setValue(qRadiansToDegrees(d));
    } else { // assume degrees
        ui->radioButtonRad->setChecked(false);
        ui->radioButtonDeg->setChecked(true);
        ui->doubleSpinBoxPcollR->setEnabled(false);
        ui->doubleSpinBoxPcollD->setEnabled(true);
        ui->doubleSpinBoxPcollR->setValue(qDegreesToRadians(d));
        ui->doubleSpinBoxPcollD->setValue(d);
    }

    return err;
}

QMap<QString, QString> OpticsConfigPagePcol::getParameters()
{
    QMap<QString, QString> m;
    if (!installed) return m;

    if (ui->radioButtonRad->isChecked()) {
        m.insert("PColl", QString("%1").arg(ui->doubleSpinBoxPcollR->value(), 0, 'f', 4));
    } else if (ui->radioButtonDeg->isChecked()) {
        m.insert("PColl", QString("%1").arg(qDegreesToRadians(ui->doubleSpinBoxPcollD->value()), 0, 'f', 4));
    }

    return m;
}

void OpticsConfigPagePcol::radChanged(double d)
{
    bool oldState = ui->doubleSpinBoxPcollD->blockSignals(true);
    ui->doubleSpinBoxPcollD->setValue(qRadiansToDegrees(d));
    ui->doubleSpinBoxPcollD->blockSignals(oldState);
}

void OpticsConfigPagePcol::degChanged(double d)
{
    bool oldState = ui->doubleSpinBoxPcollR->blockSignals(true);
    ui->doubleSpinBoxPcollR->setValue(qDegreesToRadians(d));
    ui->doubleSpinBoxPcollR->blockSignals(oldState);
}

QString OpticsConfigPagePcol::helpText()
{
    QString out("<h1>Primary-beam Soller slit</h1>"
                "<ul><li><b>Opening angle (radians):</b> If selected, the opening angle of the collimator "
                "must be given in radians.</li>"
                "<li><b>Opening angle (degrees):</b> If selected, the opening angle of the collimator "
                "must be given in degrees. It will be converted to radians automatically for "
                "the configuration file.</li></ul>"
                "<p>Some manufacturers specify the full axial divergence angle on the collimator module, "
                "others specify half of the full angle, i.e. the divergence angle from a straight beam. "
                "Here we must provide the half angle. As a rule of thumb:</p>"
                "<ul><li>Bruker: Divide the value printed on the soller slit module by 2</li>"
                "<li>PANalytical: Use the value printed on the soller slit module</li>"
                "<li>Rigaku: Divide the value printed on the soller slit module by 2</li></ul>");
    return css + out;
}
