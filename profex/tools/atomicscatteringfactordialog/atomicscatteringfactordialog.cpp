/***************************************************************************
                          atomicscatteringfactordialog.h  -  description
                             -------------------
    begin                : Mon Feb 09 08:50:00 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#include "atomicscatteringfactordialog.h"
#include "ui_atomicscatteringfactordialog.h"
#include "imageresolutiondialog.h"
#include "../libXrdIO/bgmnfileio.h"
#include "3rdparty/qcustomplot/qcustomplot.h"
#include "wavelengthcombobox.h"

#include <QDebug>
#include <QList>
#include <QFontMetrics>
#include <math.h>

AtomicScatteringFactorDialog::AtomicScatteringFactorDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AtomicScatteringFactorDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    darkMode = false;

    ui->spinBoxB->setSuffix(QString(" %1%2").arg(global::angstrom).arg(global::superTwo));
    ui->labelCoordinatesX->setText(QString());
    ui->labelCoordinatesY->setText(QString());
    ui->labelU->setText(QString());

    ui->comboBoxWavelength->showKa2(false);
    ui->comboBoxWavelength->showKb(false);
    ui->comboBoxWavelength->initData(true);
    initSettings();
    initPlot();
    initListWidgetItems();
}

AtomicScatteringFactorDialog::~AtomicScatteringFactorDialog()
{
    delete ui;
}

void AtomicScatteringFactorDialog::initSettings()
{
    darkMode = settings->isDarkMode();

    restoreGeometry(settings->value("afaParmDialog/geometry", QByteArray()).toByteArray());
    ui->comboBoxWavelength->setCurrentIndex(settings->value("afaParmDialog/wavelength", 0).toInt());
    initColorTable(settings->value("graph/colorTable", QList<QVariant>()).toList());
    lineWidth = settings->value("graph/lineWidth", 1).toInt();
    stepSizeTheta = settings->value("afaParmDialog/stepSize2t", 1.0).toDouble();
    if (stepSizeTheta <= 0.0) stepSizeTheta = 1.0; // that would cause infinite loops
    wavelength = 10.0 * ui->comboBoxWavelength->currentData(Qt::UserRole).toDouble();

    QByteArray splitterSizes(settings->value("afaParmDialog/splitter", QByteArray()).toByteArray());

    if (splitterSizes.isNull()) {
        QList<int> defaultSplitterSizes;
        defaultSplitterSizes << 100 << 400;
        ui->splitter->setSizes(defaultSplitterSizes);
    } else {
        ui->splitter->restoreState(splitterSizes);
    }

    QFontMetrics fm(font());
    ui->labelCoordinatesX->setMinimumWidth(fm.horizontalAdvance("2m = mmm.mm"));
    ui->labelCoordinatesY->setMinimumWidth(fm.horizontalAdvance("f = mmm.mm"));
}

void AtomicScatteringFactorDialog::closeEvent(QCloseEvent *e)
{
    saveSettings();
    e->accept();
}

void AtomicScatteringFactorDialog::saveSettings()
{
    settings->setValue("afaParmDialog/geometry", saveGeometry());
    settings->setValue("afaParmDialog/splitter", ui->splitter->saveState());
    settings->setValue("afaParmDialog/wavelength", ui->comboBoxWavelength->currentIndex());
    settings->setValue("afaParmDialog/stepSize2t", stepSizeTheta);
}

void AtomicScatteringFactorDialog::initPlot()
{
    QPen penAxis(darkMode ? global::Functions::colorToDarkMode(QColor(Qt::black)) : Qt::black, lineWidth);
    QPen penGrid(darkMode ? global::Functions::colorToDarkMode(QColor(Qt::lightGray)) : Qt::lightGray, lineWidth, Qt::DotLine);
    QBrush brushBkgr(QGuiApplication::palette().color(QPalette::Base));

    ui->plotWidget->setBackground(brushBkgr);

    ui->plotWidget->legend->setVisible(true);
    ui->plotWidget->xAxis->setLabel(QString(tr("sin(%1)/%2 [%3%4%5]"))
                                    .arg(global::theta)
                                    .arg(global::lambda)
                                    .arg(global::angstrom)
                                    .arg(global::superMinus)
                                    .arg(global::superOne));

    ui->plotWidget->yAxis->setLabel(QString(tr("Atomic form factor [e]")));

    ui->plotWidget->xAxis->setBasePen(penAxis);
    ui->plotWidget->xAxis->setTickPen(penAxis);
    ui->plotWidget->xAxis->setSubTickPen(penAxis);
    ui->plotWidget->xAxis->setLabelColor(penAxis.color());
    ui->plotWidget->xAxis->setTickLabelColor(penAxis.color());
    ui->plotWidget->xAxis->grid()->setPen(penGrid);

    ui->plotWidget->yAxis->setBasePen(penAxis);
    ui->plotWidget->yAxis->setTickPen(penAxis);
    ui->plotWidget->yAxis->setSubTickPen(penAxis);
    ui->plotWidget->yAxis->setLabelColor(penAxis.color());
    ui->plotWidget->yAxis->setTickLabelColor(penAxis.color());
    ui->plotWidget->yAxis->grid()->setPen(penGrid);

    ui->plotWidget->legend->setBrush(brushBkgr);
    ui->plotWidget->legend->setTextColor(penAxis.color());
    ui->plotWidget->legend->setBorderPen(Qt::NoPen);

    connect(ui->plotWidget, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(cursorCoordinates(QMouseEvent*)));
}

void AtomicScatteringFactorDialog::initColorTable(const QList<QVariant> &names)
{
    colorTable.clear();

    for (int i = 0; i < names.size(); ++i) {
        QColor col = QColor(names.at(i).toString());

        if (darkMode) {
            colorTable.append(global::Functions::colorToDarkMode(col));
            colorTable.append(global::Functions::colorToDarkMode(col.darker(150)));
        } else {
            colorTable.append(col);
            colorTable.append(col.lighter(150));
        }
    }
}

void AtomicScatteringFactorDialog::clearPlot()
{
    ui->plotWidget->clearGraphs();
    ui->plotWidget->replot();
}

void AtomicScatteringFactorDialog::initListWidgetItems()
{
    for (int j = 0; j < global::BgmnScatteringFactorSymbols.size(); ++j) {
        QListWidgetItem *it = new QListWidgetItem(global::BgmnScatteringFactorSymbols.at(j), ui->listWidget);
        it->setData(Qt::UserRole, global::BgmnScatteringFactorSymbols.at(j));

        ui->listWidget->addItem(it);
    }
}

void AtomicScatteringFactorDialog::drawData()
{
    clearPlot();
    xYYdata.clear();

    ui->plotWidget->xAxis->setRange(0.0, 0.1);
    ui->plotWidget->yAxis->setRange(0.0, 0.1);

    QList<QListWidgetItem*> selectedItems = ui->listWidget->selectedItems();

    double b = ui->spinBoxB->value();
    bool drawBRange = !qFuzzyIsNull(b);
    int colIndex = 6;
    int n = 0;

    xYYdata.resize(drawBRange ? 2 * selectedItems.count() + 1 : selectedItems.count() + 1);

    for (int i = 0; i < selectedItems.count(); ++i) {
        QString name(selectedItems.at(i)->data(Qt::UserRole).toString());

        getData(global::ScatteringFactors.value(name), 0.0, xYYdata[0], xYYdata[n+1]);

        QPen penMain(getColor(colIndex));
        penMain.setWidth(lineWidth);

        ui->plotWidget->addGraph();
        ui->plotWidget->graph(n)->addData(xYYdata[0].data, xYYdata[n+1].data);
        ui->plotWidget->graph(n)->setName(selectedItems.at(i)->text());
        ui->plotWidget->graph(n)->setPen(penMain);
        ui->plotWidget->graph(n)->rescaleAxes(true);

        if (drawBRange)  {
            ++n;
            QString bname = QString("%1 (B=%2)").arg(selectedItems.at(i)->text()).arg(b, 0, 'f', 2);
            getData(global::ScatteringFactors.value(name), b, xYYdata[0], xYYdata[n+1]);

            QPen penBrange(getColor(colIndex + 1));
            penBrange.setWidth(lineWidth);

            ui->plotWidget->addGraph();
            ui->plotWidget->graph(n)->addData(xYYdata[0].data, xYYdata[n+1].data);
            ui->plotWidget->graph(n)->setName(bname);
            ui->plotWidget->graph(n)->setPen(penBrange);
            ui->plotWidget->graph(n)->rescaleAxes(true);
        }

        ++n;
        colIndex += 2;
    }

    ui->plotWidget->replot();
}

void AtomicScatteringFactorDialog::atomSelectionChanged()
{
    drawData();
}

void AtomicScatteringFactorDialog::waveLengthChanged()
{
    wavelength = 10.0 * ui->comboBoxWavelength->currentData(Qt::UserRole).toDouble();
    drawData();
}

void AtomicScatteringFactorDialog::getData(const global::ScatteringFactorFunction fct, double b, DataSet &xdata, DataSet &ydata)
{
    xdata.data.clear();
    ydata.data.clear();
    xdata.name = QStringLiteral("sin(theta)/lambda");
    ydata.name = fct.element;

    if (qFuzzyIsNull(wavelength)) return;

    for (double tt = 0.0; tt <= 180.0; tt += 2.0 * stepSizeTheta) {
        double x = sin(tt * M_PI / 360.0) / wavelength;
        double y = global::Functions::getScatteringFactor(fct, tt, wavelength, b);

        xdata.data.append(x);
        ydata.data.append(y);
    }
}

QColor AtomicScatteringFactorDialog::getColor(int i)
{
    return i < colorTable.size() ? colorTable.at(i) : settings->getRandomColor(i);
}

void AtomicScatteringFactorDialog::exportData()
{
    QString fn = QFileDialog::getSaveFileName(this,
                                              tr("Export Data"),
                                              settings->value("config/workingdir", QDir::homePath()).toString(),
                                              tr("CSV file (*.csv *.CSV)"));

    if (fn.isEmpty()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);

    if (!BgmnFileIO::writeTextFile(fn, dataToCsv())) {
        qDebug() << QString("AtomicScatteringFactorDialog::exportData(): Could not open file for writing: %1").arg(fn);
    }

    qApp->restoreOverrideCursor();
}

QString AtomicScatteringFactorDialog::dataToCsv()
{
    QString csvString;
    int nPoints = xYYdata.first().data.size();

    double t = 0.0;

    QStringList header(xYYdata.first().name);
    header.append("2theta");

    for (int n = 1; n < xYYdata.size(); ++n) {
        header.append(xYYdata.at(n).name);
    }

    csvString += header.join(";") + "\n";

    for (int i = 0; i < nPoints; ++i) {
        QStringList line;

        // add sin(t)/lambda
        line.append(QString::number(xYYdata.at(0).data.at(i),'f', 4));

        // add 2theta
        line.append(QString::number(2.0 * t, 'f', 4));
        t += stepSizeTheta;

        for (int j = 1; j < xYYdata.size(); ++j) {
            line.append(QString::number(xYYdata.at(j).data.at(i),'f', 4));
        }

        csvString += line.join(";") + "\n";
    }

    return csvString;
}

void AtomicScatteringFactorDialog::savePdf()
{
    QString fn = QFileDialog::getSaveFileName(this,
                                              tr("PDF File Name"),
                                              settings->value("config/workingdir", QDir::homePath()).toString(),
                                              tr("PDF File (*.pdf *.PDF)"));

    if (fn.isEmpty()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);
    ui->plotWidget->savePdf(fn);
    qApp->restoreOverrideCursor();
}


void AtomicScatteringFactorDialog::savePng()
{
    QString fn = QFileDialog::getSaveFileName(this,
                                              tr("PNG File Name"),
                                              settings->value("config/workingdir", QDir::homePath()).toString(),
                                              tr("PNG Image File (*.png *.PNG)"));

    if (fn.isEmpty()) return;

    int w = 1280;
    int h = 1024;

    ImageResolutionDialog *irdlg = new ImageResolutionDialog(this);

    irdlg->setPixelWidth(w);
    irdlg->setPixelHeight(h);

    if (irdlg->exec() == QDialog::Accepted) {
        w = irdlg->pixelWidth();
        h = irdlg->pixelHeight();
        delete irdlg;
    } else {
        delete irdlg;
        return;
    }

    qApp->setOverrideCursor(Qt::WaitCursor);
    ui->plotWidget->savePng(fn, w, h);
    qApp->restoreOverrideCursor();
}

void AtomicScatteringFactorDialog::cursorCoordinates(QMouseEvent *event)
{
    QString sx;
    QString sy;

    double x = ui->plotWidget->xAxis->pixelToCoord(event->pos().x());
    double y = ui->plotWidget->yAxis->pixelToCoord(event->pos().y());

    if ((ui->plotWidget->xAxis->range().contains(x))
            && (ui->plotWidget->yAxis->range().contains(y))) {
        double ztheta = 2.0 * asin(x * wavelength) * 180.0 / M_PI;

        sx = QString::number(ztheta, 'f', 2);
        sy = QString::number(y, 'f', 2);
    }

    ui->labelCoordinatesX->setText(QString("2%1 = %2°").arg(global::theta).arg(sx));
    ui->labelCoordinatesY->setText(QString("f = %3").arg(sy));
}

void AtomicScatteringFactorDialog::bChanged(double)
{
    ui->labelU->setText(QString("u = %1 %2").arg(qSqrt(ui->spinBoxB->value()/(8*M_PI*M_PI)), 0, 'f', 2).arg(global::angstrom));
    drawData();
}

void AtomicScatteringFactorDialog::resetB()
{
    ui->spinBoxB->setValue(0.0);
}

void AtomicScatteringFactorDialog::changeSettings()
{
    bool ok;
    double d = QInputDialog::getDouble(this, tr("Settings"), QString(tr("2%1 Step Size")).arg(global::theta), 2.0*stepSizeTheta, 0.01, 10.0, 2, &ok);

    if (ok) {
        stepSizeTheta = d/2.0;
        drawData();
    }
}
