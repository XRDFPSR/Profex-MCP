/***************************************************************************
                          optionsconfigpagerslit.cpp  -  description
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

#include "opticsconfigpagerslit.h"

OpticsConfigPageRslit::OpticsConfigPageRslit(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageRslitForm)

{
    ui->setupUi(this);
}

QMap<QString, QString> OpticsConfigPageRslit::setParameters(const QMap<QString, QString> &p)
{
    setInstalled(p.contains("RoundSlitR"));

    bool ok;
    QMap<QString, QString> err;

    ui->doubleSpinBoxRoundSlitR->setValue(p.value("RoundSlitR", "0.0").toDouble(&ok));
    if (!ok) err["RoundSlitR"] = p.value("RoundSlitR", "0.0");

    ui->doubleSpinBoxRoundSlitD->setValue(p.value("RoundSlitD", "0.0").toDouble(&ok));
    if (!ok) err["RoundSlitD"] = p.value("RoundSlitD", "0.0");

    return err;
}

QMap<QString, QString> OpticsConfigPageRslit::getParameters()
{
    QMap<QString, QString> m;
    if (!installed) return m;

    m.insert("RoundSlitR", QString("%1").arg(ui->doubleSpinBoxRoundSlitR->value(), 0, 'f', 2));
    m.insert("RoundSlitD", QString("%1").arg(ui->doubleSpinBoxRoundSlitD->value(), 0, 'f', 4));

    return m;
}

QString OpticsConfigPageRslit::helpText()
{
    QString out("<h1>Pinhole aperture</h1>"
                "<p>A pinhole aperture with a round hole placed in the primary beam.</p>"
                "<ul><li><b>Pinhole diameter:</b> Diameter of the pinhole in mm.</li>"
                "<li><b>Distance from sample:</b> Distance of the slit from the sample in mm.</li></ul>");
    return css + out;
}
