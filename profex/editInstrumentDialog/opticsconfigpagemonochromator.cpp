/***************************************************************************
                          optionsconfigpagemonochromator.cpp  -  description
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

#include "opticsconfigpagemonochromator.h"

OpticsConfigPageMonochromator::OpticsConfigPageMonochromator(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageMonochromatorForm)

{
    ui->setupUi(this);
}

QMap<QString, QString> OpticsConfigPageMonochromator::setParameters(const QMap<QString, QString> &p)
{
    setInstalled(p.contains("MonR"));

    bool ok;
    QMap<QString, QString> err;

    ui->doubleSpinBoxMonr->setValue(p.value("MonR", "0.0").toDouble(&ok));
    if (!ok) err["MonR"] = p.value("MonR", "0.0");

    QRegularExpression rx("sqr\\(cos\\((\\d+\\.?\\d*)\\*pi/180\\)\\)");
    QRegularExpressionMatch rm = rx.match(p.value("POL", ""));
    if (rm.hasMatch()) ui->doubleSpinBoxAngle->setValue(rm.captured(1).toDouble(&ok));
    if (!ok) err["POL"] = p.value("POL", "");

    return err;
}

QMap<QString, QString> OpticsConfigPageMonochromator::getParameters()
{
    QMap<QString, QString> m;
    if (!installed) return m;

    m.insert("MonR", QString("%1").arg(ui->doubleSpinBoxMonr->value(), 0, 'f', 2));
    m.insert("POL", QString("sqr(cos(%1*pi/180))").arg(ui->doubleSpinBoxAngle->value(), 0, 'f', 2));

    return m;
}

QString OpticsConfigPageMonochromator::helpText()
{
    QString out("<h1>Monochromator</h1>"
                "<ul><li><b>Monochromator angle:</b> Angle in degrees 2&theta; of the monochromator crystal.</li>"
                "<li><b>Distance form sample:</b> Distance of the monochromator center axis from the sample surface.</li></ul>");
    return css + out;
}
