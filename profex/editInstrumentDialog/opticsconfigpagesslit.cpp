/***************************************************************************
                          optionsconfigpagesslit.cpp  -  description
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

#include "opticsconfigpagesslit.h"

OpticsConfigPageSslit::OpticsConfigPageSslit(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageSslitForm)

{
    ui->setupUi(this);

    ui->comboBoxMode->addItem(tr("Fixed"));
    ui->comboBoxMode->addItem(tr("Variable"));

    connect(ui->comboBoxMode, SIGNAL(currentIndexChanged(int)), ui->stackedWidget, SLOT(setCurrentIndex(int)));
    connect(ui->radioButtonBeamDiv, SIGNAL(toggled(bool)), ui->doubleSpinBoxDiv, SLOT(setEnabled(bool)));
    connect(ui->radioButtonBeamDiv, SIGNAL(toggled(bool)), ui->doubleSpinBoxSslitW, SLOT(setDisabled(bool)));

    ui->stackedWidget->widget(0)->layout()->setContentsMargins(0, 0, 0, 0);
    ui->stackedWidget->widget(1)->layout()->setContentsMargins(0, 0, 0, 0);

    sSlitWstringFix = "2*tan(adiv*pi/360)*(R-SSlitR)";
    sSlitWstringVar = "(2*(R-SSlitR)*airr*sin(pi*zweiTheta/360))/(2*R+airr*cos(pi*zweiTheta/360))";
}

QMap<QString, QString> OpticsConfigPageSslit::setParameters(const QMap<QString, QString> &p)
{
    enum Mode {none, fixpos, fixdiv, var};
    Mode mode = none;

    if (p.contains("airr"))        mode = var;
    else if (p.contains("adiv"))   mode = fixdiv;
    else if (p.contains("HSlitR")) mode = fixpos;

    setInstalled(mode != none);

    ui->comboBoxMode->setCurrentIndex(mode == var ? 1 : 0);
    ui->stackedWidget->setCurrentIndex(mode == var ? 1 : 0);

    bool ok;
    QMap<QString, QString> err;

    if (mode == var) {
        ui->doubleSpinBoxIrr->setValue(p.value("airr", "0.0").toDouble(&ok));
        if (!ok) err["airr"] = p.value("airr", "0.0");

        ui->doubleSpinBoxSslitR->setValue(p.value("SSlitR", "0.0").toDouble(&ok));
        if (!ok) err["SSlitR"] = p.value("SSlitR", "0.0");

        if (p.value("SSlitW", "") != sSlitWstringVar) err["SSlitW"] = p.value("SSlitW", "");
    } else if (mode == fixdiv) {
        ui->doubleSpinBoxDiv->setValue(p.value("adiv", "0.0").toDouble(&ok));
        if (!ok) err["adiv"] = p.value("adiv", "0.0");

        ui->doubleSpinBoxSslitR->setValue(p.value("SSlitR", "0.0").toDouble(&ok));
        if (!ok) err["SSlitR"] = p.value("SSlitR", "0.0");

        if (p.value("SSlitW", "") != sSlitWstringFix) err["SSlitW"] = p.value("SSlitW", "");
    } else if (mode == fixpos) {
        ui->doubleSpinBoxSslitR->setValue(p.value("SSlitR", "0.0").toDouble(&ok));
        if (!ok) err["SSlitR"] = p.value("SSlitR", "0.0");

        ui->doubleSpinBoxSslitW->setValue(p.value("SSlitW", "0.0").toDouble(&ok));
        if (!ok) err["SSlitW"] = p.value("SSlitW", "0.0");
    }

    ui->radioButtonBeamDiv->setChecked(mode != fixpos);
    ui->radioButtonSlitPos->setChecked(mode == fixpos);
    ui->doubleSpinBoxDiv->setEnabled(mode != fixpos);
    ui->doubleSpinBoxSslitW->setEnabled(mode == fixpos);

    return err;
}

QMap<QString, QString> OpticsConfigPageSslit::getParameters()
{
    QMap<QString, QString> m;
    if (!installed) return m;

    if (ui->comboBoxMode->currentIndex() == 0) {
        if (ui->radioButtonBeamDiv->isChecked()) {
            m.insert("adiv", QString("%1").arg(ui->doubleSpinBoxDiv->value(), 0, 'f', 4));
            m.insert("SSlitR", QString("%1").arg(ui->doubleSpinBoxSslitR->value(), 0, 'f', 2));
            m.insert("SSlitW", sSlitWstringFix);
        } else {
            m.insert("SSlitR", QString("%1").arg(ui->doubleSpinBoxSslitR->value(), 0, 'f', 2));
            m.insert("SSlitW", QString("%1").arg(ui->doubleSpinBoxSslitW->value(), 0, 'f', 4));
        }
    }

    if (ui->comboBoxMode->currentIndex() == 1) {
        m.insert("airr", QString("%1").arg(ui->doubleSpinBoxIrr->value(), 0, 'f', 4));
        m.insert("SSlitR", QString("%1").arg(ui->doubleSpinBoxSslitR->value(), 0, 'f', 2));
        m.insert("SSlitW", sSlitWstringVar);
    }

    return m;
}


QString OpticsConfigPageSslit::helpText()
{
    QString out("<h1>Anti-scatter slit</h1>"
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
