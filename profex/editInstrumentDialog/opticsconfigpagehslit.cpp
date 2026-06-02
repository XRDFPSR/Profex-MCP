/***************************************************************************
                          optionsconfigpagehslit.cpp  -  description
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

#include "opticsconfigpagehslit.h"

OpticsConfigPageHslit::OpticsConfigPageHslit(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageHslitForm)

{
    ui->setupUi(this);

    ui->comboBoxMode->addItem(tr("Fixed"));
    ui->comboBoxMode->addItem(tr("Variable"));

    connect(ui->comboBoxMode, SIGNAL(currentIndexChanged(int)), ui->stackedWidget, SLOT(setCurrentIndex(int)));
    connect(ui->radioButtonBeamDiv, SIGNAL(toggled(bool)), ui->doubleSpinBoxDiv, SLOT(setEnabled(bool)));
    connect(ui->radioButtonBeamDiv, SIGNAL(toggled(bool)), ui->doubleSpinBoxHslitW, SLOT(setDisabled(bool)));

    ui->stackedWidget->widget(0)->layout()->setContentsMargins(0, 0, 0, 0);
    ui->stackedWidget->widget(1)->layout()->setContentsMargins(0, 0, 0, 0);

    hSlitWstringFix = "2*tan(div*pi/360)*(R-HSlitR)";
    hSlitWstringVar = "(2*(R-HSlitR)*irr*sin(pi*zweiTheta/360))/(2*R+irr*cos(pi*zweiTheta/360))";
}

QMap<QString, QString> OpticsConfigPageHslit::setParameters(const QMap<QString, QString> &p)
{
    enum Mode {none, fixpos, fixdiv, var};
    Mode mode = none;

    if (p.contains("irr"))         mode = var;
    else if (p.contains("div"))    mode = fixdiv;
    else if (p.contains("HSlitR")) mode = fixpos;

    setInstalled(mode != none);

    ui->comboBoxMode->setCurrentIndex(mode == var ? 1 : 0);
    ui->stackedWidget->setCurrentIndex(mode == var ? 1 : 0);

    bool ok;
    QMap<QString, QString> err;

    if (mode == var) {
        ui->doubleSpinBoxIrr->setValue(p.value("irr", "0.0").toDouble(&ok));
        if (!ok) err["irr"] = p.value("irr", "0.0");

        ui->doubleSpinBoxHslitR->setValue(p.value("HSlitR", "0.0").toDouble(&ok));
        if (!ok) err["HSlitR"] = p.value("HSlitR", "0.0");

        if (p.value("HSlitW", "") != hSlitWstringVar) err["HSlitW"] = p.value("HSlitW", "");
    } else if (mode == fixdiv) {
        ui->doubleSpinBoxDiv->setValue(p.value("div", "0.0").toDouble(&ok));
        if (!ok) err["div"] = p.value("div", "0.0");

        ui->doubleSpinBoxHslitR->setValue(p.value("HSlitR", "0.0").toDouble(&ok));
        if (!ok) err["HSlitR"] = p.value("HSlitR", "0.0");

        if (p.value("HSlitW", "") != hSlitWstringFix) err["HSlitW"] = p.value("HSlitW", "");
    } else if (mode == fixpos) {
        ui->doubleSpinBoxHslitR->setValue(p.value("HSlitR", "0.0").toDouble(&ok));
        if (!ok) err["HSlitR"] = p.value("HSlitR", "0.0");

        ui->doubleSpinBoxHslitW->setValue(p.value("HSlitW", "0.0").toDouble(&ok));
        if (!ok) err["HSlitW"] = p.value("HSlitW", "0.0");
    }

    ui->radioButtonBeamDiv->setChecked(mode != fixpos);
    ui->radioButtonSlitPos->setChecked(mode == fixpos);
    ui->doubleSpinBoxDiv->setEnabled(mode != fixpos);
    ui->doubleSpinBoxHslitW->setEnabled(mode == fixpos);

    return err;
}

QMap<QString, QString> OpticsConfigPageHslit::getParameters()
{
    QMap<QString, QString> m;
    if (!installed) return m;

    if (ui->comboBoxMode->currentIndex() == 0) {
        if (ui->radioButtonBeamDiv->isChecked()) {
            m.insert("div", QString("%1").arg(ui->doubleSpinBoxDiv->value(), 0, 'f', 4));
            m.insert("HSlitR", QString("%1").arg(ui->doubleSpinBoxHslitR->value(), 0, 'f', 2));
            m.insert("HSlitW", hSlitWstringFix);
        } else {
            m.insert("HSlitR", QString("%1").arg(ui->doubleSpinBoxHslitR->value(), 0, 'f', 2));
            m.insert("HSlitW", QString("%1").arg(ui->doubleSpinBoxHslitW->value(), 0, 'f', 4));
        }
    }

    if (ui->comboBoxMode->currentIndex() == 1) {
        m.insert("irr", QString("%1").arg(ui->doubleSpinBoxIrr->value(), 0, 'f', 4));
        m.insert("HSlitR", QString("%1").arg(ui->doubleSpinBoxHslitR->value(), 0, 'f', 2));
        m.insert("HSlitW", hSlitWstringVar);
    }

    return m;
}

QString OpticsConfigPageHslit::helpText()
{
    QString out("<h1>Divergence slit</h1>"
                "<h2>Fixed slit</h2>"
                "<p>In fixed mode, the slit parameters can be specified in two different ways. Either as a "
                "constant beam divergence angle, for which the distance from the sample and slit width will "
                "be calculated automatically, or as the position from the sample and effective slit width.</p>"
                "<ul><li><b>Beam divergence</b> Specify a constant beam divergence angle. The distance from the "
                "sample and slit opening will be calculated automatically.</li>"
                "<li><p><b>Slit opening:</b> Specify the equatorial slit width in mm.</li>"
                "<li><b>Distance from sample:</b> Distance of the slit from the sample.</li></ul>"
                "<h2>Variable slit</h2>"
                "<p>In variable mode, the equatorial length of the irradiated area on the sample surface "
                "must be specified. The distance from the sample and variable slit opening will "
                "be calculated automatically.</p>"
                "<ul><li><b>Irradiated length</b> Equatorial length of the irradiated area on the "
                "sample surface.</li></ul>"
                "<p>If used in conjunction with TubeTails, you should use the switch GSUM=Y.</p>");
    return css + out;
}
