/***************************************************************************
                          optionsconfigpagefocus.cpp  -  description
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

#include "opticsconfigpagefocus.h"
#include "projectWidget/bgmnbackendconfig.h"
#include <QtMath>
#include <QFileDialog>

OpticsConfigPageFocus::OpticsConfigPageFocus(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageFocusForm)

{
    ui->setupUi(this);

    BgmnBackendConfig bkgCfg;
    QStringList lams = bkgCfg.getAllLamFiles().keys();
    ui->comboBoxLambda->addItems(lams);

    connect(ui->toolButtonTubeTails, SIGNAL(clicked(bool)), this, SLOT(selectTubeTails()));
    connect(ui->toolButtonTubeTailsClear, SIGNAL(clicked(bool)), this, SLOT(clearTubeTails()));
}

QMap<QString, QString> OpticsConfigPageFocus::setParameters(const QMap<QString, QString> &p)
{
    bool ok;
    QMap<QString, QString> err;

    QString lam = p.value("LAMBDA", "");
    double  syn = p.value("SYNCHROTRON", "-1.0").toDouble(&ok);
    if (!ok) err["SYNCHROTRON"] = p.value("SYNCHROTRON", "-1.0");

    if (syn <= 0.0) setLambda(lam);
    else            setSynchrotron(syn);

    double fh = p.value("FocusH", "12.0").toDouble(&ok);
    if (!ok) err["FocusH"] = p.value("FocusH", "12.0");

    double fw = p.value("FocusW", "0.04").toDouble(&ok);
    if (!ok) err["FocusW"] = p.value("FocusW", "0.04");

    ui->doubleSpinBoxFocusH->setValue(fh);
    ui->doubleSpinBoxFocusW->setValue(fw);

    ui->lineEditTubeTails->setText(p.value("TubeTails", ""));

    return err;
}

QMap<QString, QString> OpticsConfigPageFocus::getParameters()
{
    QMap<QString, QString> m;

    if (ui->radioButtonCharacteristic->isChecked()) {
        m.insert("LAMBDA", ui->comboBoxLambda->currentText());
    } else {
        m.insert("SYNCHROTRON", QString("%1").arg(ui->doubleSpinBoxSynchrotron->value(), 0, 'f', 6));
    }
    m.insert("FocusH", QString("%1").arg(ui->doubleSpinBoxFocusH->value(), 0, 'f', 2));
    m.insert("FocusW", QString("%1").arg(ui->doubleSpinBoxFocusW->value(), 0, 'f', 4));

    if (!ui->lineEditTubeTails->text().isEmpty()) {
        m.insert("TubeTails", ui->lineEditTubeTails->text());
    }

    return m;
}

QString OpticsConfigPageFocus::helpText()
{
    QString out("<h1>X-ray source</h1>"
                "<p>The optical focus dimensions on the anode must be specified. They are derived "
                "from the thermal focus dimensions, which are usually printed on the label of "
                "the X-ray tube.</p>"
                "<ul><li><b>Target width:</b> Axial dimension of the X-ray tube's focus (target).</li>"
                "<li><b>Apparent height:</b> The optical equatorial focus dimension is "
                "reduced by the take-off angle of X-rays from the anode surface (usually 6°). "
                "In following, the optical equatorial focus dimension equals the 10th part of "
                "the thermal one.</li>"
                "<li><b>Tube tails:</b> A scan file containing the tube tails scan. Note that the file format "
                "must be natively supported by BGMN. No conversion will take place.</li></ul>"
                "<p>In case of synchrotron radiation, specify the diameter of the beam in mm as the "
                "width and apparent height of the target.</p>");
    return css + out;
}

void OpticsConfigPageFocus::selectTubeTails()
{
    QStringList filters;
    filters.append("Free format XY scan (*.xy *.XY *.csv *.CSV *.xyp *.XYP)");
    filters.append("ASCII scan file (*.dat *.DAT *.asc *.ASC *.txt *.TXT)");
    filters.append("Seifert/FPM VAL scan (*.val *.VAL)");
    filters.append("All files (*.*)");

    QFileInfo fi(ui->lineEditTubeTails->text());
    QString f = QFileDialog::getOpenFileName(this, tr("Tube tail scan"), fi.absolutePath(), filters.join(";;"));

    if (!f.isEmpty()) {
        ui->lineEditTubeTails->setText(f);
        ui->lineEditTubeTails->setToolTip(f);
    }
}

void OpticsConfigPageFocus::clearTubeTails()
{
    ui->lineEditTubeTails->clear();
    ui->lineEditTubeTails->setToolTip(QString());
}

void OpticsConfigPageFocus::setLambda(const QString &s)
{
    ui->radioButtonCharacteristic->setChecked(true);
    ui->radioButtonSynchrotron->setChecked(false);

    int i = ui->comboBoxLambda->findText(s, Qt::MatchFlags(Qt::MatchFixedString));
    if (i >= 0) ui->comboBoxLambda->setCurrentIndex(i);
}

void OpticsConfigPageFocus::setSynchrotron(double d)
{
    ui->radioButtonCharacteristic->setChecked(false);
    ui->radioButtonSynchrotron->setChecked(true);
    ui->doubleSpinBoxSynchrotron->setValue(d);
}

QString OpticsConfigPageFocus::getLambda() const
{
    return ui->comboBoxLambda->currentText();
}

double OpticsConfigPageFocus::getSynchrotron() const
{
    return ui->doubleSpinBoxSynchrotron->value();
}

bool OpticsConfigPageFocus::hasLambda() const
{
    return ui->radioButtonCharacteristic->isChecked();
}

bool OpticsConfigPageFocus::hasSynchrotron() const
{
    return ui->radioButtonSynchrotron->isChecked();
}
