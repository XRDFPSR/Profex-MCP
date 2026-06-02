/***************************************************************************
                          optionsconfigpagescol.cpp  -  description
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

#include "opticsconfigpagescol.h"
#include <QtMath>

OpticsConfigPageScol::OpticsConfigPageScol(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageScolForm)

{
    ui->setupUi(this);

    connect(ui->radioButtonRad, SIGNAL(toggled(bool)), ui->doubleSpinBoxScollR, SLOT(setEnabled(bool)));
    connect(ui->radioButtonRad, SIGNAL(toggled(bool)), ui->doubleSpinBoxScollD, SLOT(setDisabled(bool)));
    connect(ui->doubleSpinBoxScollR, SIGNAL(valueChanged(double)), this, SLOT(radChanged(double)));
    connect(ui->doubleSpinBoxScollD, SIGNAL(valueChanged(double)), this, SLOT(degChanged(double)));
}

QMap<QString, QString> OpticsConfigPageScol::setParameters(const QMap<QString, QString> &p)
{
    setInstalled(p.contains("SColl"));

    bool ok;
    QMap<QString, QString> err;

    double d = p.value("SColl", "0.0").toDouble(&ok);
    if (!ok) err["SColl"] = p.value("SColl", "0.0");

    if (d < 0.2) { // assume radians
        ui->radioButtonRad->setChecked(true);
        ui->radioButtonDeg->setChecked(false);
        ui->doubleSpinBoxScollR->setEnabled(true);
        ui->doubleSpinBoxScollD->setEnabled(false);
        ui->doubleSpinBoxScollR->setValue(d);
        ui->doubleSpinBoxScollD->setValue(qRadiansToDegrees(d));
    } else { // assume degrees
        ui->radioButtonRad->setChecked(false);
        ui->radioButtonDeg->setChecked(true);
        ui->doubleSpinBoxScollR->setEnabled(false);
        ui->doubleSpinBoxScollD->setEnabled(true);
        ui->doubleSpinBoxScollR->setValue(qDegreesToRadians(d));
        ui->doubleSpinBoxScollD->setValue(d);
    }

    return err;
}

QMap<QString, QString> OpticsConfigPageScol::getParameters()
{
    QMap<QString, QString> m;
    if (!installed) return m;

    if (ui->radioButtonRad->isChecked()) {
        m.insert("SColl", QString("%1").arg(ui->doubleSpinBoxScollR->value(), 0, 'f', 4));
    } else if (ui->radioButtonDeg->isChecked()) {
        m.insert("SColl", QString("%1").arg(qDegreesToRadians(ui->doubleSpinBoxScollD->value()), 0, 'f', 4));
    }

    return m;
}

void OpticsConfigPageScol::radChanged(double d)
{
    bool oldState = ui->doubleSpinBoxScollD->blockSignals(true);
    ui->doubleSpinBoxScollD->setValue(qRadiansToDegrees(d));
    ui->doubleSpinBoxScollD->blockSignals(oldState);
}

void OpticsConfigPageScol::degChanged(double d)
{
    bool oldState = ui->doubleSpinBoxScollR->blockSignals(true);
    ui->doubleSpinBoxScollR->setValue(qDegreesToRadians(d));
    ui->doubleSpinBoxScollR->blockSignals(oldState);
}

QString OpticsConfigPageScol::helpText()
{
    QString out("<h1>Secondary-beam Soller slit</h1>"
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
