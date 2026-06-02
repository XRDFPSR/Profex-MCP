/***************************************************************************
                          optionsconfigpageairscat.cpp  -  description
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

#include "opticsconfigpageairscat.h"

OpticsConfigPageAirscat::OpticsConfigPageAirscat(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageAirscatForm)

{
    ui->setupUi(this);
}

QMap<QString, QString> OpticsConfigPageAirscat::setParameters(const QMap<QString, QString> &p)
{
    setInstalled(p.contains("AirScat"));

    bool ok;
    QMap<QString, QString> err;

    ui->doubleSpinBoxAirscat->setValue(p.value("AirScat", "0.0").toDouble(&ok));
    if (!ok) err["AirScat"] = p.value("AirScat", "0.0");

    return err;
}

QMap<QString, QString> OpticsConfigPageAirscat::getParameters()
{
    QMap<QString, QString> m;
    if (!installed) return m;

    m.insert("AirScat", QString("%1").arg(ui->doubleSpinBoxAirscat->value(), 0, 'f', 2));
    return m;
}

QString OpticsConfigPageAirscat::helpText()
{
    QString out("<h1>Beam knife</h1>"
                "<p>Beam knifes are only available for GEOMETRY=REFLEXION.</p>"
                "<ul><li><b>Height over sample surface:</b> Height of the beam knife edge "
                "over the sample surface in mm.</li></ul>"
                "<p>Only used for stationary beam knifes. Motorized beam knifes to not "
                "interact with the beam. Disable the beam knife option in case a motorized one is used.</p>");
    return css + out;
}
