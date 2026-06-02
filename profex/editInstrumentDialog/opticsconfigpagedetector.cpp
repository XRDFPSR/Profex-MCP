/***************************************************************************
                          optionsconfigpagedetector.cpp  -  description
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

#include "opticsconfigpagedetector.h"

OpticsConfigPageDetector::OpticsConfigPageDetector(const QString &t, QWidget *parent) :
 AbstractOpticsConfigPage(t, parent),
  ui(new Ui::OpticsConfigPageDetectorForm)

{
    ui->setupUi(this);

    // data structure in comboBoxModel:
    // int(type) ; double(DetH) ; double(DetW) ; [double(DetArrayW)]
    // 0=0D 1=1D ; width        ; pitch        ; active length

    ui->comboBoxModel->addItem("Point detector",            QList<QVariant>() << 0 << 0.0 << 0.0 << 0.0);
    ui->comboBoxModel->addItem("1D / 2D detector",          QList<QVariant>() << 1 << 0.0 << 0.0 << 0.0);
    ui->comboBoxModel->addItem("Bruker LynxEye",            QList<QVariant>() << 1 << 16.0 << 0.075 << 14.4);
    ui->comboBoxModel->addItem("Bruker LynxEye XE",         QList<QVariant>() << 1 << 16.0 << 0.075 << 14.4);
    ui->comboBoxModel->addItem("Bruker LynxEye-2",          QList<QVariant>() << 1 << 16.0 << 0.075 << 14.4);
    ui->comboBoxModel->addItem("Bruker LynxEye SSD160",     QList<QVariant>() << 1 << 12.0 << 0.075 << 12.0);
    ui->comboBoxModel->addItem("Bruker LynxEye SSD160-2",   QList<QVariant>() << 1 << 12.0 << 0.075 << 12.0);
    ui->comboBoxModel->addItem("Bruker Eiger2 R 500K",      QList<QVariant>() << 1 << 38.6 << 0.075 << 77.2);
    ui->comboBoxModel->addItem("Bruker Vantec-500",         QList<QVariant>() << 1 << 140.0 << 0.068 << 140.0);
    ui->comboBoxModel->addItem("Bruker Vantec-2000",        QList<QVariant>() << 1 << 140.0 << 0.068 << 140.0);
    ui->comboBoxModel->addItem("Dectris Pilatus3 R 100K-A", QList<QVariant>() << 1 << 83.8 << 0.172 << 33.5);
    ui->comboBoxModel->addItem("Dectris Pilatus3 R 200K-A", QList<QVariant>() << 1 << 83.8 << 0.172 << 70.0);
    ui->comboBoxModel->addItem("Dectris Pilatus3 R 300K"  , QList<QVariant>() << 1 << 83.8 << 0.172 << 106.5);
    ui->comboBoxModel->addItem("Dectris Pilatus3 R 300K-W", QList<QVariant>() << 1 << 253.7 << 0.172 << 33.5);
    ui->comboBoxModel->addItem("Dectris Mythen2 R 1K",      QList<QVariant>() << 1 << 64.0 << 0.050 << 8.0);
    ui->comboBoxModel->addItem("Dectris Mythen2 R 1D 8mm",  QList<QVariant>() << 1 << 32.0 << 0.050 << 8.0);
    ui->comboBoxModel->addItem("Dectris Mythen2 R 1D 4mm",  QList<QVariant>() << 1 << 32.0 << 0.050 << 4.0);
    ui->comboBoxModel->addItem("PANalytical X'celerator",   QList<QVariant>() << 1 << 15.0 << 0.070 << 8.89);
    ui->comboBoxModel->addItem("PANalytical PIXcel 1D",     QList<QVariant>() << 1 << 14.08 << 0.055 << 14.08);
    ui->comboBoxModel->addItem("PANalytical PIXcel 3D",     QList<QVariant>() << 1 << 14.08 << 0.055 << 14.08);
    ui->comboBoxModel->addItem("PANalytical PIXcel3D Medipix3", QList<QVariant>() << 1 << 14.08 << 0.055 << 14.08);
    ui->comboBoxModel->addItem("PANalytical 1Der",          QList<QVariant>() << 1 << 15.0 << 0.070 << 8.89);
    ui->comboBoxModel->addItem("PANalytical GaliPIX3D",     QList<QVariant>() << 1 << 24.16 << 0.075 << 30.06);
    ui->comboBoxModel->addItem("Rigaku D/teX Ultra",        QList<QVariant>() << 1 << 20.0 << 0.100 << 12.8);
    ui->comboBoxModel->addItem("Rigaku D/teX Ultra 250",    QList<QVariant>() << 1 << 20.0 << 0.075 << 19.4);
    ui->comboBoxModel->addItem("Rigaku HyPix-400",          QList<QVariant>() << 1 << 38.5 << 0.100 <<  9.6);
    ui->comboBoxModel->addItem("Rigaku HyPix-3000",         QList<QVariant>() << 1 << 38.5 << 0.100 << 77.5);

    connect(ui->comboBoxModel, SIGNAL(currentIndexChanged(int)), this, SLOT(setModel()));
}

QMap<QString, QString> OpticsConfigPageDetector::setParameters(const QMap<QString, QString> &p)
{
    bool ok;
    QMap<QString, QString> err;

    // using prefix A for area detector variables. These will be translated correctly when
    // writing the SAV file
    if (p.contains("ADetArrayW")) {
        double deth = p.value("ADetH", "0.0").toDouble(&ok);
        if (!ok) err["DetH"] = p.value("ADetH", "0.0");

        double detw = p.value("ADetW", "0.0").toDouble(&ok);
        if (!ok) err["DetW"] = p.value("ADetW", "0.0");

        double detaw = p.value("ADetArrayW", "0.0").toDouble(&ok);
        if (!ok) err["DetArrayW"] = p.value("ADetArrayW", "0.0");

        ui->doubleSpinBoxSensorDetH->setValue(deth);
        ui->doubleSpinBoxSensorDetW->setValue(detw);
        ui->doubleSpinBoxSensorDetArrayW->setValue(detaw);
        ui->comboBoxModel->setItemData(1, QList<QVariant>() << 1 << deth << detw << detaw);

        ui->comboBoxModel->setCurrentIndex(1);
        ui->doubleSpinBoxSensorDetArrayW->setEnabled(true);
        ui->doubleSpinBoxSensorDetH->setEnabled(true);
        ui->doubleSpinBoxSensorDetW->setEnabled(true);
    } else {
        ui->comboBoxModel->setCurrentIndex(0);
        ui->doubleSpinBoxSensorDetArrayW->setEnabled(false);
        ui->doubleSpinBoxSensorDetH->setEnabled(false);
        ui->doubleSpinBoxSensorDetW->setEnabled(false);
    }

    return err;
}

QMap<QString, QString> OpticsConfigPageDetector::getParameters()
{
    QMap<QString, QString> m;
    if (ui->comboBoxModel->currentIndex() == 0) return m;

    m.insert("ADetH", QString("%1").arg(ui->doubleSpinBoxSensorDetH->value(), 0, 'f', 2));
    m.insert("ADetW", QString("%1").arg(ui->doubleSpinBoxSensorDetW->value(), 0, 'f', 4));
    m.insert("ADetArrayW", QString("%1").arg(ui->doubleSpinBoxSensorDetArrayW->value(), 0, 'f', 2));

    return m;
}

void OpticsConfigPageDetector::setModel()
{
    QList<QVariant> lst = ui->comboBoxModel->currentData(Qt::UserRole).toList();
    if (lst.size() < 4) return;

    int type = lst.at(0).toInt();

    ui->doubleSpinBoxSensorDetArrayW->setEnabled(type > 0);
    ui->doubleSpinBoxSensorDetH->setEnabled(type > 0);
    ui->doubleSpinBoxSensorDetW->setEnabled(type > 0);

    if (type > 0) {
        ui->doubleSpinBoxSensorDetArrayW->setValue(lst.at(3).toDouble());
        ui->doubleSpinBoxSensorDetH->setValue(lst.at(1).toDouble());
        ui->doubleSpinBoxSensorDetW->setValue(lst.at(2).toDouble());
    }
}

QString OpticsConfigPageDetector::helpText()
{
    QString out("<h1>Detector</h1>"
                "<h2>Point detector with detector slit</h2>"
                "<p>For point detectors, a detector slit must be configured.</p>"
                "<h2>1D/2D detector:</h2>"
                "<p>For linear (1D) and area (2D) detectors, the dimensions and pitch of the sensor "
                "must be specified. If a sensor mask or a region of interest (ROI) is used, only the "
                "active sensor area must be entered here.</p>"
                "<ul><li><b>Sensor equatorial height:</b> Equatorial height of the active sensor area in mm.</li>"
                "<li><b>Sensor axial width:</b> Axial width of the active sensor area in mm.</li>"
                "<li><b>Pixel size:</b> Pixel size / sensor pitch in mm.</li></ul>");
    return css + out;
}
