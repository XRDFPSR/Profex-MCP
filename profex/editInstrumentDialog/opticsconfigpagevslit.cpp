/***************************************************************************
                          optionsconfigpagevslit.cpp  -  description
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

#include "opticsconfigpagevslit.h"

OpticsConfigPageVslit::OpticsConfigPageVslit(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageVslitForm)

{
    ui->setupUi(this);
}

QMap<QString, QString> OpticsConfigPageVslit::setParameters(const QMap<QString, QString> &p)
{
    setInstalled(p.contains("VSlitH"));

    bool ok;
    QMap<QString, QString> err;

    ui->doubleSpinBoxVslitH->setValue(p.value("VSlitH", "0.0").toDouble(&ok));
    if (!ok) err["VSlitH"] = p.value("VSlitH", "0.0");

    ui->doubleSpinBoxVslitR->setValue(p.value("VSlitR", "0.0").toDouble(&ok));
    if (!ok) err["VSlitR"] = p.value("VSlitR", "0.0");

    return err;
}

QMap<QString, QString> OpticsConfigPageVslit::getParameters()
{
    QMap<QString, QString> m;
    if (!installed) return m;

    m.insert("VSlitR", QString("%1").arg(ui->doubleSpinBoxVslitR->value(), 0, 'f', 2));
    m.insert("VSlitH", QString("%1").arg(ui->doubleSpinBoxVslitH->value(), 0, 'f', 2));

    return m;
}


QString OpticsConfigPageVslit::helpText()
{
    QString out("<h1>Beam mask</h1>"
                "<ul><li><b>Mask width:</b> The axial width of the mask in mm. "
                "The effective width may be different from the beam width "
                "on the sample surface.</li>"
                "<li><b>Distance from sample:</b> Distance of the mask from "
                "the sample in mm.</li></ul>");
    return css + out;
}
