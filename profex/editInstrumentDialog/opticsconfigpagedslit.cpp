/***************************************************************************
                          optionsconfigpagedslit.cpp  -  description
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

#include "opticsconfigpagedslit.h"

OpticsConfigPageDslit::OpticsConfigPageDslit(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageDslitForm)

{
    ui->setupUi(this);
}

QMap<QString, QString> OpticsConfigPageDslit::setParameters(const QMap<QString, QString> &p)
{
    bool hasData = (p.contains("DetW") && !p.contains("DetArrayW"));
    setInstalled(hasData);

    bool ok;
    QMap<QString, QString> err;

    if (hasData) {
        ui->doubleSpinBoxDetW->setValue(p.value("DetW", "0.0").toDouble(&ok));
        if (!ok) err["DetW"] = p.value("DetW", "0.0");

        ui->doubleSpinBoxDetH->setValue(p.value("DetH", "0.0").toDouble(&ok));
        if (!ok) err["DetH"] = p.value("DetH", "0.0");
    }

    return err;
}

QMap<QString, QString> OpticsConfigPageDslit::getParameters()
{
    QMap<QString, QString> m;
    if (!installed) return m;

    m.insert("DetW", QString("%1").arg(ui->doubleSpinBoxDetW->value(), 0, 'f', 2));
    m.insert("DetH", QString("%1").arg(ui->doubleSpinBoxDetH->value(), 0, 'f', 4));

    return m;
}

QString OpticsConfigPageDslit::helpText()
{
    QString out("<h1>Detector slit</h1>"
                "<p>A slit used to mask the window of point detectors.</p>"
                "<ul><li><b>Axial width:</b> Axial width of the slit in mm.</li>"
                "<li><b>Slit opening:</b> Equatorial opening of the slit in mm.</li></ul>");
    return css + out;
}
