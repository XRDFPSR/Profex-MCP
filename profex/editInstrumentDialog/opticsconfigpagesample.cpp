/***************************************************************************
                          optionsconfigpagesample.cpp  -  description
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

#include "opticsconfigpagesample.h"
#include "../libXrdIO/structs.h"

OpticsConfigPageSample::OpticsConfigPageSample(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageSampleForm)

{
    ui->setupUi(this);
    ui->comboBoxGeometry->addItem("Reflexion",    QVariant("REFLEXION"));
    ui->comboBoxGeometry->addItem("Capillary",    QVariant("CAPILLARY"));
    ui->comboBoxGeometry->addItem("Transmission", QVariant("TRANSMISSION"));

    ui->comboBoxReflShape->addItem("Round",       QVariant("ROUND"));
    ui->comboBoxReflShape->addItem("Rectangular", QVariant("RECTANGULAR"));

    ui->stackedWidget->widget(0)->layout()->setContentsMargins(0, 0, 0, 0);
    ui->stackedWidget->widget(1)->layout()->setContentsMargins(0, 0, 0, 0);
    ui->stackedWidget->widget(2)->layout()->setContentsMargins(0, 0, 0, 0);

    ui->labelReflLac->setText(tr("Linear abs. coeff. (cm%1%2)").arg(global::superMinus).arg(global::superOne));
    ui->labelCapLac->setText(tr("Linear abs. coeff. (cm%1%2)").arg(global::superMinus).arg(global::superOne));
    ui->labelTransLac->setText(tr("Linear abs. coeff. (cm%1%2)").arg(global::superMinus).arg(global::superOne));

    connect(ui->comboBoxGeometry, SIGNAL(currentIndexChanged(int)), ui->stackedWidget, SLOT(setCurrentIndex(int)));
    connect(ui->comboBoxGeometry, SIGNAL(currentIndexChanged(int)), this, SLOT(instrumentGeometryChanged()));
    connect(ui->comboBoxReflShape, SIGNAL(currentIndexChanged(int)), this, SLOT(sampleShapeChanged(int)));
    connect(ui->comboBoxDeltaOmega, SIGNAL(currentIndexChanged(int)), this, SLOT(deltaOmegaModeChanged(int)));
}

QMap<QString, QString> OpticsConfigPageSample::setParameters(const QMap<QString, QString> &p)
{
    enum Geo {REFL, CAP, TRANS};
    Geo mode;

    if (p.contains("GEOMETRY")) {
        if (p.value("GEOMETRY") == "CAPILLARY")         mode = Geo::CAP;
        else if (p.value("GEOMETRY") == "TRANSMISSION") mode = Geo::TRANS;
        else                                            mode = Geo::REFL;
    } else {
        mode = Geo::REFL;
    }

    if (mode == Geo::REFL)     ui->comboBoxGeometry->setCurrentIndex(0);
    else if (mode == Geo::CAP) ui->comboBoxGeometry->setCurrentIndex(1);
    else                       ui->comboBoxGeometry->setCurrentIndex(2);

    if (p.contains("SHAPE")) {
        if (p.value("SHAPE") == "ROUND")            ui->comboBoxReflShape->setCurrentIndex(0);
        else if (p.value("SHAPE") == "RECTANGULAR") ui->comboBoxReflShape->setCurrentIndex(1);
        else                                        ui->comboBoxReflShape->setCurrentIndex(0);
    }

    bool ok;
    QMap<QString, QString> err;

    if (p.contains("SamplD")) {
        ui->doubleSpinBoxReflSamplD->setValue(p.value("SamplD", "0.0").toDouble());
        ui->doubleSpinBoxTransSamplD->setValue(p.value("SamplD", "0.0").toDouble(&ok));
        if (!ok) err["SamplD"] = p.value("SamplD", "0.0");
    }

    if (p.contains("SamplH")) {
        ui->doubleSpinBoxReflSamplH->setValue(p.value("SamplH", "0.0").toDouble());
        ui->doubleSpinBoxCapSamplH->setValue(p.value("SamplH", "0.0").toDouble(&ok));
        if (!ok) err["SamplH"] = p.value("SamplH", "0.0");
    }

    if (p.contains("SamplW")) {
        ui->doubleSpinBoxReflSamplW->setValue(p.value("SamplW", "0.0").toDouble(&ok));
        if (!ok) err["SamplW"] = p.value("SamplW", "0.0");
    }

    if (p.contains("T")) {
        ui->doubleSpinBoxReflT->setValue(p.value("T", "0.0").toDouble());
        ui->doubleSpinBoxCapT->setValue(p.value("T", "0.0").toDouble());
        ui->doubleSpinBoxTransT->setValue(p.value("T", "0.0").toDouble(&ok));
        if (!ok) err["T"] = p.value("T", "0.0");
    }

    if (p.contains("D")) {
        double d = p.value("D", "0.0").toDouble(&ok);
        if (!ok) err["D"] = p.value("D", "0.0");
        double dinv = qFuzzyIsNull(d) ? 0.0 : 10.0 / d;
        ui->doubleSpinBoxReflD->setValue(dinv);
        ui->doubleSpinBoxCapD->setValue(dinv);
        ui->doubleSpinBoxTransD->setValue(dinv);
    }

    if (p.contains("DeltaOmega")) {
        static QRegularExpression rx("0.5\\*zweiTheta-(\\d+\\.?\\d*)");
        QString s = p.value("DeltaOmega", "");

        ui->checkBoxDeltaOmega->setChecked(true);

        QRegularExpressionMatch rm = rx.match(s);
        if (rm.hasMatch()) {
            ui->comboBoxDeltaOmega->setCurrentIndex(1);
            ui->labelDeltaOmega->setText("Theta (°)");
            ui->doubleSpinBoxDeltaOmega->setValue(rm.captured(1).toDouble(&ok));
            if (!ok) err["DeltaOmega"] = p.value("DeltaOmega", "");
        } else {
            ui->comboBoxDeltaOmega->setCurrentIndex(0);
            ui->labelDeltaOmega->setText("Omega (°)");
            ui->doubleSpinBoxDeltaOmega->setValue(s.toDouble(&ok));
            if (!ok) err["DeltaOmega"] = p.value("DeltaOmega", "");
        }
    } else {
        ui->checkBoxDeltaOmega->setChecked(false);
    }

    if ((mode == Geo::REFL) && p.contains("SamplW") && p.contains("SamplH")) {
        ui->comboBoxReflShape->setCurrentIndex(1);
    } else {
        ui->comboBoxReflShape->setCurrentIndex(0);
    }

    sampleShapeChanged(ui->comboBoxReflShape->currentIndex());
    return err;
}

QMap<QString, QString> OpticsConfigPageSample::getParameters()
{
    QMap<QString, QString> map;
    QString geo = ui->comboBoxGeometry->currentData(Qt::UserRole).toString();

    map.insert("GEOMETRY", geo);

    if (geo == "REFLEXION") {
        if (ui->comboBoxReflShape->currentIndex() == 0) {
            map.insert("SamplD", QString("%1").arg(ui->doubleSpinBoxReflSamplD->value(), 0, 'f', 2));
        } else {
            map.insert("SamplW", QString("%1").arg(ui->doubleSpinBoxReflSamplW->value(), 0, 'f', 2));
            map.insert("SamplH", QString("%1").arg(ui->doubleSpinBoxReflSamplH->value(), 0, 'f', 2));
        }

        double lac = ui->doubleSpinBoxReflD->value();
        double d = qFuzzyIsNull(lac) ? 0.0 : 10.0 / lac;
        map.insert("D",      QString("%1").arg(d, 0, 'f', 4));
        map.insert("T",      QString("%1").arg(ui->doubleSpinBoxReflT->value(), 0, 'f', 4));

        if (ui->checkBoxDeltaOmega->isChecked()) {
            if (ui->comboBoxDeltaOmega->currentIndex() == 0) {
                map.insert("DeltaOmega", QString("%1").arg(ui->doubleSpinBoxDeltaOmega->value(), 0, 'f', 4));
            } else {
                map.insert("DeltaOmega", QString("0.5*zweiTheta-%1").arg(ui->doubleSpinBoxDeltaOmega->value(), 0, 'f', 4));
            }
        }
    } else if (geo == "CAPILLARY") {
        double lac = ui->doubleSpinBoxCapD->value();
        double d = qFuzzyIsNull(lac) ? 0.0 : 10.0 / lac;
        map.insert("D",      QString("%1").arg(d, 0, 'f', 4));
        map.insert("T",      QString("%1").arg(ui->doubleSpinBoxCapT->value(), 0, 'f', 4));
        map.insert("SamplH", QString("%1").arg(ui->doubleSpinBoxCapSamplH->value(), 0, 'f', 2));
    } else if (geo == "TRANSMISSION") {
        double lac = ui->doubleSpinBoxTransD->value();
        double d = qFuzzyIsNull(lac) ? 0.0 : 10.0 / lac;
        map.insert("D",      QString("%1").arg(d, 0, 'f', 4));
        map.insert("T",      QString("%1").arg(ui->doubleSpinBoxTransT->value(), 0, 'f', 4));
        map.insert("SamplD", QString("%1").arg(ui->doubleSpinBoxTransSamplD->value(), 0, 'f', 2));
    }

    return map;
}

QString OpticsConfigPageSample::helpText()
{
    QString out("<h1>Sample</h1>"
                "<h2>Instrument geometry: Reflexion</h2>"
                "<p>In reflection (Bragg-Brentano) geometry, the flat sample holder "
                "can have a round or rectangular cavity. The size of the cavity must be "
                "specified depending on the shape:</p>"
                "<h3>Sample shape: Round</h3>"
                "<ul><li><b>Diameter:</b> Diameter of the sample holder cavity in mm.</p></li></ul>"
                "<h3>Sample shape: Rectangular</h3>"
                "<ul><li><b>Size:</b> Width of the sample holder cavity in axial direction in mm by "
                "Length of the sample holder cavity in equatorial direction in mm.</li></ul>"
                "<p><b>Thickness:</b> Thickness of the sample in mm. If left at 0, infinite thickness (sufficiently thick) is used.</p>"
                "<p><b>Linear absorption coefficient (LAC):</b> Effective sample LAC (theoretical LAC multiplied with the packing density). "
                "If left at 0, the profile will not be corrected for penetration depth.</p>"
                "<h3>Omega twist</h3>"
                "<p>Inclination of the sample surface. Changes the profile shapes, intensities, and penetration depths depending on the 2&theta; angle.</p>"
                "<ul><li><b>Fix Omega:</b> A constant twist of the sample surface (&omega; axis) independent of the 2&theta; angle.</li>"
                "<li><b>Fix Theta:</b> The sample surface twist &omega; changes with 2&theta; to maintain a constant incident angle &theta;. "
                "This mode can be used for GA-XRD setups.</li></ul>"
                "<h2>Instrument geometry: Capillary</h2>"
                "<p>The dimensions of the capillary must be specified.</p>"
                "<ul><li><b>Capillary length:</b> Length of the capillary in mm.</li>"
                "<li><b>Capillary inner diameter:</b> Inner diameter of the capillary in mm.</li>"
                "<li><b>Linear abs. coeff. (cm<sup>-1</sup>):</b> The sample's effective linear absorption coefficient (LAC). "
                "The theoretical LAC must be multiplied with the packing density. This value must be entered. If left at 0.0, "
                "profile calculation will abort.</li></ul>"
                "<h2>Instrument geometry: Transmission</h2>"
                "<p>The dimensions of the sample cell must be specified.</p>"
                "<ul><li><b>Diameter:</b> Open diameter of the sample holder in mm.</li>"
                "<li><b>Thickness: </b>Thickness of the sample in mm.</li>"
                "<li><b>Linear abs. coeff. (cm<sup>-1</sup>):</b> The sample's effective linear absorption coefficient (LAC). "
                "The theoretical LAC must be multiplied with the packing density. This value must be entered. If left at 0.0, "
                "profile calculation will abort.</li></ul>");
    return css + out;
}

void OpticsConfigPageSample::instrumentGeometryChanged()
{
    emit updateLayout(tag, ui->comboBoxGeometry->currentData(Qt::UserRole));
}

void OpticsConfigPageSample::sampleShapeChanged(int i)
{
    ui->stackedWidgetSampleSize->setCurrentIndex(i);
}

void OpticsConfigPageSample::deltaOmegaModeChanged(int i)
{
    if (i == 0) ui->labelDeltaOmega->setText("Omega");
    else ui->labelDeltaOmega->setText("Theta");
}
