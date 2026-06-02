/***************************************************************************
                          optionsconfigpagetslit.cpp  -  description
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

#include "opticsconfigpagetslit.h"

OpticsConfigPageTslit::OpticsConfigPageTslit(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageTslitForm)

{
    ui->setupUi(this);
}

QMap<QString, QString> OpticsConfigPageTslit::setParameters(const QMap<QString, QString> &p)
{
    setInstalled(p.contains("TSlitH"));

    bool ok;
    QMap<QString, QString> err;

    ui->doubleSpinBoxTslitH->setValue(p.value("TSlitH", "0.0").toDouble(&ok));
    if (!ok) err["TSlitH"] = p.value("TSlitH", "0.0");

    ui->doubleSpinBoxTslitR->setValue(p.value("TSlitR", "0.0").toDouble(&ok));
    if (!ok) err["TSlitR"] = p.value("TSlitR", "0.0");

    return err;
}

QMap<QString, QString> OpticsConfigPageTslit::getParameters()
{
    QMap<QString, QString> m;
    if (!installed) return m;

    m.insert("TSlitH", QString("%1").arg(ui->doubleSpinBoxTslitH->value(), 0, 'f', 2));
    m.insert("TSlitR", QString("%1").arg(ui->doubleSpinBoxTslitR->value(), 0, 'f', 2));

    return m;
}


QString OpticsConfigPageTslit::helpText()
{
    QString out("<h1>Axial beam slit</h1>"
                "<p>The usage of an additional axial slit near the X-ray tube is supported.</p>"
                "<ul><li><b>Axial slit width:</b> The axial width of the aperture in mm.</li>"
                "<li><b>Distance from sample:</b> Distance of the slit from the sample in mm.</li></ul>");
    return css + out;
}
