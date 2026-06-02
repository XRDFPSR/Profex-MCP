/***************************************************************************
                          tubetailsimulatordialog.cpp  -  description
                             -------------------
    begin                : Wed Jan 25 18:50:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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

#include "tubetailsimulatordialog.h"
#include "ui_tubetailsimulatordialog.h"
#include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/structs.h"
#include "QFileDialog"

TubeTailSimulatorDialog::TubeTailSimulatorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::TubeTailSimulatorDialog)
{
    ui->setupUi(this);
    setWindowTitle("Tube Tail Simulation");
    settings = SettingsManager::getInstance();

    yTicksLin  = QSharedPointer<QCPAxisTickerText>(new QCPAxisTickerText);
    yTicksSqrt = QSharedPointer<QCPAxisTickerText>(new QCPAxisTickerText);

    ui->plotWidget->xAxis->setLabel(QString("Diffraction Angle [%1%2%3]").arg(global::degree, "2", global::theta));
    ui->plotWidget->yAxis->setLabel(QString("Intensity [a.u.]"));

    yTicksLin->addTick(0.00, "0.00");
    yTicksLin->addTick(0.20, "0.20");
    yTicksLin->addTick(0.40, "0.40");
    yTicksLin->addTick(0.60, "0.60");
    yTicksLin->addTick(0.80, "0.80");
    yTicksLin->addTick(1.00, "1.00");

    yTicksSqrt->addTick(0.00, "0.00");
    yTicksSqrt->addTick(std::sqrt(0.002), "0.002");
    yTicksSqrt->addTick(std::sqrt(0.01), "0.01");
    yTicksSqrt->addTick(std::sqrt(0.02), "0.02");
    yTicksSqrt->addTick(std::sqrt(0.05), "0.05");
    yTicksSqrt->addTick(std::sqrt(0.10), "0.10");
    yTicksSqrt->addTick(std::sqrt(0.20), "0.20");
    yTicksSqrt->addTick(std::sqrt(0.40), "0.40");
    yTicksSqrt->addTick(std::sqrt(0.60), "0.60");
    yTicksSqrt->addTick(std::sqrt(0.80), "0.80");
    yTicksSqrt->addTick(1.00, "1.00");

    initSettings();
    updatePlot();
}

TubeTailSimulatorDialog::~TubeTailSimulatorDialog()
{
    delete ui;
}

void TubeTailSimulatorDialog::closeEvent(QCloseEvent *e)
{
    saveSettings();
    e->accept();
}

void TubeTailSimulatorDialog::initSettings()
{
    ui->comboBoxYScale->insertItem(0, QString("Linear"));
    ui->comboBoxYScale->insertItem(1, QString("Square root"));
    // ui->comboBoxYScale->insertItem(2, QString("Logarithmic"));
    yscale = yScale::lin;

    workingDir = settings->value("tubeTailSimulationDialog/workingDir", QDir::homePath()).toString();
    restoreGeometry(settings->value("tubeTailSimulationDialog/geometry", QByteArray()).toByteArray());
    ui->splitter->restoreState(settings->value("tubeTailSimulationDialog/splitter", QByteArray()).toByteArray());

    ui->doubleSpinBoxStepSize->setValue(settings->value("tubeTailSimulationDialog/stepSize", 0.005).toDouble());
    ui->doubleSpinBoxBeamWidth->setValue(settings->value("tubeTailSimulationDialog/beamWidth", 0.02).toDouble());
    ui->doubleSpinBoxTubeTailWidth->setValue(settings->value("tubeTailSimulationDialog/ttWidth", 0.4).toDouble());
    ui->doubleSpinBoxTubeTailHeight->setValue(settings->value("tubeTailSimulationDialog/ttHeight", 0.25).toDouble());
}

void TubeTailSimulatorDialog::saveSettings()
{
    settings->setValue("tubeTailSimulationDialog/workingDir", workingDir);
    settings->setValue("tubeTailSimulationDialog/geometry", saveGeometry());
    settings->setValue("tubeTailSimulationDialog/splitter", ui->splitter->saveState());

    settings->setValue("tubeTailSimulationDialog/stepSize", ui->doubleSpinBoxStepSize->value());
    settings->setValue("tubeTailSimulationDialog/beamWidth", ui->doubleSpinBoxBeamWidth->value());
    settings->setValue("tubeTailSimulationDialog/ttWidth", ui->doubleSpinBoxTubeTailWidth->value());
    settings->setValue("tubeTailSimulationDialog/ttHeight", ui->doubleSpinBoxTubeTailHeight->value());
}

void TubeTailSimulatorDialog::updatePlot()
{
    ui->plotWidget->clearGraphs();

    double beamW = ui->doubleSpinBoxBeamWidth->value();
    double ttW   = ui->doubleSpinBoxTubeTailWidth->value();
    double ttH   = ui->doubleSpinBoxTubeTailHeight->value() * 0.01;
    double stepS = ui->doubleSpinBoxStepSize->value();

    if (qFuzzyIsNull(stepS)) stepS = 0.005;
    if (qFuzzyIsNull(beamW)) beamW = 0.02;
    if (qFuzzyIsNull(ttW))   ttW   = 0.30;

    double xmax = 1.2 * ttW;
    int nSteps = int(xmax / stepS);

    QVector<double> yvalDisp;
    xval.clear();
    yval.clear();
    xval.reserve(2 * nSteps);
    yval.reserve(2 * nSteps);
    yvalDisp.reserve(2 * nSteps);

    for (int i = 0; i <= nSteps; ++i) {
        double x = xmax * (double)i/double(nSteps);
        double ypk = (1.0 - ttH) / (1.0 + std::exp((-5.0 / beamW) * (-x + beamW)));
        double ytt = ttH / (1.0 + std::exp((-5.0 / beamW) * (-x + ttW)));
        double y = ypk + ytt;

        xval.append(x);
        yval.append(y);

        if (!qFuzzyCompare(x, -x)) {
            // avoid values at x=0 being added twice (for +0 and -0)
            xval.prepend(-x);
            yval.prepend(y);
        }

        if (yscale != yScale::lin) {
            if (yscale == yScale::sqrt) {
                yvalDisp.append(std::sqrt(y));
                yvalDisp.prepend(std::sqrt(y));
            } else {
                yvalDisp.append(qFuzzyIsNull(y) ? 0.0 : std::log10(y));
                yvalDisp.prepend(qFuzzyIsNull(y) ? 0.0 : std::log10(y));
            }
        }
    }

    if (!ui->plotWidget->graphCount()) {
        ui->plotWidget->addGraph();
    }

    ui->plotWidget->graph(0)->addData(xval, (yscale == yScale::lin ? yval : yvalDisp));
    ui->plotWidget->xAxis->setRange(xval.first(), xval.last());
    ui->plotWidget->yAxis->setRange(0.0, 1.0);
    ui->plotWidget->replot();
}

void TubeTailSimulatorDialog::exportData()
{
    QString f = QFileDialog::getSaveFileName(this, "Export data to scan file", workingDir, "ASCII scan file (*.xy *.XY)");
    if (f.isEmpty()) return;

    QFileInfo fi(f);
    workingDir = fi.absolutePath();

    QString out;
    double scale = std::pow(10.0, 6.0);

    for (int i = 0; i < qMin(xval.size(), yval.size()); ++i) {
        double v = scale * yval.at(i);
        out.append(QString("%1 %2\n").arg(xval.at(i), 0, 'f', 6).arg(v, 0, 'f', 4));
    }

    BgmnFileIO::writeTextFile(fi.absoluteFilePath(), out);
}

void TubeTailSimulatorDialog::axisScaleChanged(int i)
{
    switch (i) {
        case 0: yscale = yScale::lin;
            ui->plotWidget->yAxis->setTicker(yTicksLin);
            break;
        case 1: yscale = yScale::sqrt;
            ui->plotWidget->yAxis->setTicker(yTicksSqrt);
            break;
        case 2: yscale = yScale::log;
            break;
        default: yscale = yScale::lin;
            ui->plotWidget->yAxis->setTicker(yTicksLin);
    }

    updatePlot();
}

void TubeTailSimulatorDialog::resetValues()
{
    ui->doubleSpinBoxStepSize->setValue(0.005);
    ui->doubleSpinBoxBeamWidth->setValue(0.02);
    ui->doubleSpinBoxTubeTailWidth->setValue(0.4);
    ui->doubleSpinBoxTubeTailHeight->setValue(0.25);
    ui->comboBoxYScale->setCurrentIndex(0);
}
