/***************************************************************************
                          elementdatadialog.cpp  -  description
                             -------------------
    begin                : Wed Feb 10 19:16:07 CET 2021
    copyright            : (C) 2021 by Nicola Doebelin
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

#include "elementdatadialog.h"
#include "../../../libXrdIO/functions.h"
#include "../../../libXrdIO/structs.h"
#include "../../../libXrdIO/bgmnfileio.h"
#include "wavelengthcombobox.h"
#include "ui_elementdatadialog.h"
#include <limits>
#include <QDir>
#include <QFileDialog>

ElementDataDialog::ElementDataDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ElementDataDialog)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    data = scatData.getAllScatteringData();
    // scatData.writeDataToFile("/home/nic/anoScatData.bin", data);

    xAxisWl = true;
    defaultTicker = QSharedPointer<QCPAxisTicker>(new QCPAxisTicker);
    logTicker = QSharedPointer<QCPAxisTickerLog>(new QCPAxisTickerLog);

    ui->plotParam->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    ui->plotParam->xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
    ui->plotParam->xAxis->axisRect()->setRangeZoom(Qt::Horizontal);

    axLbl.append(Parameter(QString("f0 + %1f'").arg(global::Delta), QString("e/atom"),           false, DataType::F0));
    axLbl.append(Parameter(QString("%1f'").arg(global::Delta), QString("e/atom"),                false, DataType::F1));
    axLbl.append(Parameter(QString("%1f\"").arg(global::Delta), QString("e/atom"),               true,  DataType::F2));
    axLbl.append(Parameter(QString("%1f', %1f\"").arg(global::Delta), QString("e/atom"),         false, DataType::F1F2));
    axLbl.append(Parameter(QString("Mass attenuation"), QString("cm%1/g").arg(global::superTwo), true,  DataType::MAC));
    axLbl.append(Parameter(QString("Linear attenuation"), QString("1/cm"),                       true,  DataType::LAC));

    connect(ui->listWidgetDisplay, SIGNAL(itemSelectionChanged()), this, SLOT(elementSelected()));
    connect(ui->listWidgetWavelength, SIGNAL(itemSelectionChanged()), this, SLOT(displayWavelengthChanged()));
    connect(ui->comboBoxDisplayParam, SIGNAL(currentIndexChanged(int)), this, SLOT(dataSelected(int)));
    connect(ui->comboBoxEmissionLines, SIGNAL(currentIndexChanged(int)), this, SLOT(emissionLineChanged(int)));
    connect(ui->doubleSpinBoxEnergy, SIGNAL(valueChanged(double)), this, SLOT(energyChanged(double)));
    connect(ui->doubleSpinBoxWavelength, SIGNAL(valueChanged(double)), this, SLOT(wavelengthChanged(double)));
    connect(ui->plotParam, SIGNAL(mouseMove(QMouseEvent*)), this, SLOT(cursorCoordinates(QMouseEvent*)));
    connect(ui->plotParam, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(resetPlotZoom(QMouseEvent*)));
    connect(ui->checkBoxCustomWl, SIGNAL(toggled(bool)), this, SLOT(toggleCustomWl(bool)));
    connect(ui->splitterDisplay, SIGNAL(splitterMoved(int,int)), this, SLOT(splitterYchanged(int,int)));
    connect(ui->splitterList, SIGNAL(splitterMoved(int,int)), this, SLOT(splitterXchanged(int,int)));

    QStringList absHeaderLabels = QStringList()
                                        << "Element"
                                        << "Z"
                                        << QString("%1 [%2]").arg(axLbl.at(1).label).arg(axLbl.at(1).unit)
                                        << QString("%1 [%2]").arg(axLbl.at(2).label).arg(axLbl.at(2).unit)
                                        << QString("%1 [%2]").arg(axLbl.at(4).label).arg(axLbl.at(4).unit)
                                        << QString("%1 [%2]").arg(axLbl.at(5).label).arg(axLbl.at(5).unit);

    ui->treeWidgetAbsorption->setColumnCount(absHeaderLabels.size());
    ui->treeWidgetAbsorption->setHeaderLabels(absHeaderLabels);

    initWavelength();
    initSettings();
    initDisplay();
}

ElementDataDialog::~ElementDataDialog()
{
    saveSettings();
    delete ui;
}

void ElementDataDialog::initWavelength()
{
    ui->listWidgetWavelength->clear();

    QList<global::CharWaveLength> wl = global::Functions::getAllWavelengths();

    for (int i = 0; i < wl.size(); ++i) {
        QString lblKa1 = QString("%1K%2%3").arg(wl.at(i).element).arg(global::alpha).arg(global::subOne);
        QString lblKa2 = QString("%1K%2%3").arg(wl.at(i).element).arg(global::alpha).arg(global::subTwo);
        QString lblKb = QString("%1K%2").arg(wl.at(i).element).arg(global::beta);

        QListWidgetItem *itemKa1 = new QListWidgetItem(lblKa1);
        QListWidgetItem *itemKa2 = new QListWidgetItem(lblKa2);
        QListWidgetItem *itemKb = new QListWidgetItem(lblKb);

        itemKa1->setToolTip(QString("%1 %2").arg(10.0 * wl.at(i).ka1, 0, 'f', 6).arg(global::angstrom));
        itemKa2->setToolTip(QString("%1 %2").arg(10.0 * wl.at(i).ka2, 0, 'f', 6).arg(global::angstrom));
        itemKb->setToolTip(QString("%1 %2").arg(10.0 * wl.at(i).kb, 0, 'f', 6).arg(global::angstrom));

        itemKa1->setData(Qt::UserRole, wl.at(i).ka1);
        itemKa2->setData(Qt::UserRole, wl.at(i).ka2);
        itemKb->setData(Qt::UserRole, wl.at(i).kb);

        ui->listWidgetWavelength->addItem(itemKa1);
        ui->listWidgetWavelength->addItem(itemKa2);
        ui->listWidgetWavelength->addItem(itemKb);
    }

    ui->comboBoxXunit->clear();
    ui->comboBoxXunit->addItem(QString(tr("X-ray Energy [keV]")));
    ui->comboBoxXunit->addItem(QString(tr("Wavelength [%1]")).arg(global::angstrom));
}

void ElementDataDialog::initSettings()
{
    restoreGeometry(settings->value("elementDataDialog/geometry", QByteArray()).toByteArray());
    ui->splitterDisplay->restoreState(settings->value("elementDataDialog/splitterDisplay", QByteArray()).toByteArray());
    ui->splitterList->restoreState(settings->value("elementDataDialog/splitterList", QByteArray()).toByteArray());
    lineWidth = settings->value("graph/lineWidth", 1).toInt();
    initColorTable(settings->value("graph/colorTable", QList<QVariant>()).toList());
    ui->treeWidgetAbsorption->header()->restoreState(settings->value("elementDataDialog/treeWidgetAbsorption", QByteArray()).toByteArray());

    ui->comboBoxEmissionLines->showKa2(true);
    ui->comboBoxEmissionLines->showKb(true);
    ui->comboBoxEmissionLines->initData();
    ui->comboBoxEmissionLines->setCurrentIndex(settings->value("elementDataDialog/emissionLine", 4).toInt());

    ui->comboBoxXunit->setCurrentIndex(settings->value("elementDataDialog/xaxisUnit", 0).toInt());
    xAxisWl = settings->value("elementDataDialog/xaxisUnit", 0).toInt() == 1;

    ui->checkBoxCustomWl->setChecked(settings->value("elementDataDialog/customEmissionLine", false).toBool());

    toggleCustomWl(ui->checkBoxCustomWl->isChecked());
    tabToggled(0);
}

void ElementDataDialog::saveSettings()
{
    settings->setValue("elementDataDialog/geometry", saveGeometry());
    settings->setValue("elementDataDialog/treeWidgetAbsorption", ui->treeWidgetAbsorption->header()->saveState());

    settings->setValue("elementDataDialog/emissionLine", ui->comboBoxEmissionLines->currentIndex());
    settings->setValue("elementDataDialog/customEmissionLine", ui->checkBoxCustomWl->isChecked());
    settings->setValue("elementDataDialog/xaxisUnit", ui->comboBoxXunit->currentIndex());
}

void ElementDataDialog::initDisplay()
{
    for (int i = 0; i < axLbl.size(); ++i) {
        ui->comboBoxDisplayParam->addItem(axLbl.at(i).label, QVariant(axLbl.at(i).type));
    }

    QBrush brushBkgr(QGuiApplication::palette().color(QPalette::Base));
    QPen penAxis(settings->isDarkMode() ? global::Functions::colorToDarkMode(QColor(Qt::black)) : Qt::black, lineWidth);
    QPen penGrid(settings->isDarkMode() ? global::Functions::colorToDarkMode(QColor(Qt::lightGray)) : Qt::lightGray, lineWidth, Qt::DashLine);
    QPen penSubGrid(settings->isDarkMode() ? global::Functions::colorToDarkMode(QColor(Qt::lightGray)) : Qt::lightGray, lineWidth, Qt::DotLine);

    ui->plotParam->setBackground(brushBkgr);
    ui->plotParam->legend->setVisible(true);

    QSharedPointer<QCPAxisTickerLog> logTicker(new QCPAxisTickerLog);
    ui->plotParam->xAxis->setTicker(logTicker);
    ui->plotParam->xAxis->setNumberFormat("eb"); // e = exponential, b = beautiful decimal powers
    ui->plotParam->xAxis->setNumberPrecision(0); // makes sure "1*10^4" is displayed only as "10^4"
    ui->plotParam->xAxis->setScaleType(QCPAxis::stLogarithmic);

    ui->plotParam->xAxis->setBasePen(penAxis);
    ui->plotParam->xAxis->setTickPen(penAxis);
    ui->plotParam->xAxis->setSubTickPen(penAxis);
    ui->plotParam->xAxis->setLabelColor(penAxis.color());
    ui->plotParam->xAxis->setTickLabelColor(penAxis.color());
    ui->plotParam->xAxis->grid()->setPen(penGrid);
    ui->plotParam->xAxis->grid()->setSubGridPen(penSubGrid);
    ui->plotParam->xAxis->grid()->setSubGridVisible(true);

    ui->plotParam->yAxis->setBasePen(penAxis);
    ui->plotParam->yAxis->setTickPen(penAxis);
    ui->plotParam->yAxis->setSubTickPen(penAxis);
    ui->plotParam->yAxis->setLabelColor(penAxis.color());
    ui->plotParam->yAxis->setTickLabelColor(penAxis.color());
    ui->plotParam->yAxis->grid()->setPen(penGrid);
    ui->plotParam->yAxis->grid()->setSubGridPen(penSubGrid);
    ui->plotParam->yAxis->grid()->setSubGridVisible(true);

    ui->plotParam->legend->setBrush(brushBkgr);
    ui->plotParam->legend->setTextColor(penAxis.color());
    ui->plotParam->legend->setBorderPen(Qt::NoPen);

    for (int i = 0; i < data.size(); ++i) {
        QListWidgetItem *it = new QListWidgetItem(data.at(i).element());
        it->setData(Qt::UserRole, data.at(i).z());
        ui->listWidgetDisplay->addItem(it);
    }
}

void ElementDataDialog::initColorTable(const QList<QVariant> &names)
{
    colorTable.clear();

    for (int i = 0; i < names.size(); ++i) {
        QColor col = QColor(names.at(i).toString());

        if (settings->isDarkMode()) {
            colorTable.append(global::Functions::colorToDarkMode(col));
            colorTable.append(global::Functions::colorToDarkMode(col.darker(150)));
        } else {
            colorTable.append(col);
            colorTable.append(col.lighter(150));
        }
    }
}

void ElementDataDialog::resetPlotZoom(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        ui->plotParam->xAxis->rescale(true);
        ui->plotParam->replot();
    }
}

void ElementDataDialog::displayWavelengthChanged()
{
    updateDisplayGraph();
}

void ElementDataDialog::tabToggled(int i)
{
    ui->toolButtonSavePdf->setEnabled(i == 0);
    ui->toolButtonSaveXyData->setEnabled(i == 0);
    ui->toolButtonSaveMdr->setEnabled(i == 1);
    ui->toolButtonSaveAno->setEnabled(i == 1);
}

QColor ElementDataDialog::getColor(int i)
{
    return i < colorTable.size() ? colorTable.at(i) : settings->getRandomColor(i);
}

void ElementDataDialog::elementSelected()
{
    updateDisplayGraph();
}

void ElementDataDialog::dataSelected(int n)
{
    setupXAxis();
    setupYAxis(n);
    updateDisplayGraph();
}

void ElementDataDialog::setupXAxis()
{
    ui->plotParam->axisRect()->insetLayout()->setInsetAlignment(0, xAxisWl ? Qt::AlignLeft|Qt::AlignTop : Qt::AlignRight|Qt::AlignTop);
    ui->plotParam->xAxis->setLabel(xAxisWl ? QString("Wavelength [%1]").arg(global::angstrom) : QString("X-ray Energy [keV]"));
}

void ElementDataDialog::setupYAxis(int n)
{
    if (axLbl.at(n).log) {
        ui->plotParam->yAxis->setRange(0.0, 1.0);    // work around for a bug in switching scaleType
        ui->plotParam->yAxis->setTicker(logTicker);
        ui->plotParam->yAxis->setNumberFormat("eb"); // e = exponential, b = beautiful decimal powers
        ui->plotParam->yAxis->setNumberPrecision(0); // makes sure "1*10^4" is displayed only as "10^4"
        ui->plotParam->yAxis->setScaleType(QCPAxis::stLogarithmic);
    } else {
        ui->plotParam->yAxis->setTicker(defaultTicker);
        ui->plotParam->yAxis->setNumberFormat("gb");
        ui->plotParam->yAxis->setNumberPrecision(6);
        ui->plotParam->yAxis->setScaleType(QCPAxis::stLinear);
    }

    ui->plotParam->yAxis->setLabel(axLbl.at(n).label + " [" + axLbl.at(n).unit + "]");
}

void ElementDataDialog::updateDisplayGraph()
{
    DataType dt = static_cast<DataType>(ui->comboBoxDisplayParam->currentData(Qt::UserRole).toInt());
    int colIdx = 6;
    double yMin = std::numeric_limits<double>::max();
    double yMax = std::numeric_limits<double>::min();

    ui->plotParam->clearGraphs();

    for (int i = 0; i < ui->listWidgetDisplay->count(); ++i) {
        if (!ui->listWidgetDisplay->item(i)->isSelected()) {
            continue;
        }

        const ElementScatteringData *e = &data.at(i);

        QVector<double> xval;
        if (xAxisWl) {
            for (int i = 0; i < e->dataEnergy().size(); ++i) {
                xval.append(global::Functions::energyToWavelength(e->dataEnergy().at(i)));
            }
        } else {
            xval = e->dataEnergy();
        }

        if (dt == DataType::F0) {
            addDataGraph(QString("f0+%1f' %2").arg(global::Delta).arg(e->element()), e->z(), xval, e->dataF0(), colIdx, yMin, yMax);
        } else if (dt == DataType::F1) {
            addDataGraph(QString("%1f' %2").arg(global::Delta).arg(e->element()), e->z(), xval, e->dataF1(), colIdx, yMin, yMax);
        } else if (dt == DataType::F2) {
            addDataGraph(QString("%1f\" %2").arg(global::Delta).arg(e->element()), e->z(), xval, e->dataF2(), colIdx, yMin, yMax);
        } else if (dt == DataType::F1F2) {
            addDataGraph(QString("%1f' %2").arg(global::Delta).arg(e->element()), e->z(), xval, e->dataF1(), colIdx, yMin, yMax);
            addDataGraph(QString("%1f\" %2").arg(global::Delta).arg(e->element()), e->z(), xval, e->dataF2(), colIdx + 1, yMin, yMax);
        } else if (dt == DataType::MAC) {
            addDataGraph(QString("MAC %1").arg(e->element()), e->z(), xval, e->dataMac(), colIdx, yMin, yMax);
        } else if (dt == DataType::LAC) {
            addDataGraph(QString("LAC %1").arg(e->element()), e->z(), xval, e->dataLac(), colIdx, yMin, yMax);
        }

        colIdx += 2;
    }

    for (int i = 0; i < ui->listWidgetWavelength->count(); ++i) {
        if (ui->listWidgetWavelength->item(i)->isSelected()) {
            double en;

            if (xAxisWl) {
                en = ui->listWidgetWavelength->item(i)->data(Qt::UserRole).toDouble();
            } else {
                en = global::Functions::wavelengthToEnergy(ui->listWidgetWavelength->item(i)->data(Qt::UserRole).toDouble());
            }

            addWlGraph(ui->listWidgetWavelength->item(i)->text(), en, yMin, yMax, colIdx += 2);
        }
    }

    ui->plotParam->xAxis->rescale(true);
    ui->plotParam->yAxis->rescale(true);
    ui->plotParam->replot();
}

void ElementDataDialog::addDataGraph(const QString &e, int z, const QVector<double> &x, const QVector<double> &y, int colIdx, double &mi, double &ma)
{
    int n = ui->plotParam->graphCount();
    QPen penMain(getColor(colIdx));
    penMain.setWidth(lineWidth);

    for (int i = 0; i < y.count(); ++i) {
        mi = qMin(mi, y.at(i));
        ma = qMax(ma, y.at(i));
    }

    ui->plotParam->addGraph();
    ui->plotParam->graph(n)->setPen(penMain);
    ui->plotParam->graph(n)->setName(QString("%1 (%2)").arg(e).arg(z));
    ui->plotParam->graph(n)->setData(x, y, true);
}

void ElementDataDialog::addWlGraph(const QString &s, double e, double yMin, double yMax, int colIdx)
{
    int n = ui->plotParam->graphCount();
    QPen penMain(getColor(colIdx));
    penMain.setWidth(lineWidth);

    ui->plotParam->addGraph();
    ui->plotParam->graph(n)->setPen(penMain);
    ui->plotParam->graph(n)->setName(s);
    ui->plotParam->graph(n)->setData(QVector<double>() << e << e, QVector<double>() << yMin << yMax, true);
}

void ElementDataDialog::emissionLineChanged(int i)
{
    double wl = ui->comboBoxEmissionLines->itemData(i, Qt::UserRole).toDouble();
    double e  = global::Functions::wavelengthToEnergy(wl);

    bool oldStateE = ui->doubleSpinBoxEnergy->blockSignals(true);
    bool oldStateWl = ui->doubleSpinBoxWavelength->blockSignals(true);

    ui->doubleSpinBoxWavelength->setValue(wl);
    ui->doubleSpinBoxEnergy->setValue(e);

    ui->doubleSpinBoxEnergy->blockSignals(oldStateE);
    ui->doubleSpinBoxWavelength->blockSignals(oldStateWl);

    updateDisplayLists();
}

void ElementDataDialog::energyChanged(double e)
{
    bool oldState = ui->doubleSpinBoxWavelength->blockSignals(true);
    ui->doubleSpinBoxWavelength->setValue(global::Functions::energyToWavelength(e));
    ui->doubleSpinBoxWavelength->blockSignals(oldState);
    updateDisplayLists();
}

void ElementDataDialog::wavelengthChanged(double w)
{
    double e = global::Functions::wavelengthToEnergy(w);
    bool oldState = ui->doubleSpinBoxEnergy->blockSignals(true);
    ui->doubleSpinBoxEnergy->setValue(e);
    ui->doubleSpinBoxEnergy->blockSignals(oldState);
    updateDisplayLists();
}

void ElementDataDialog::toggleCustomWl(bool b)
{
    ui->comboBoxEmissionLines->setEnabled(!b);
    ui->doubleSpinBoxEnergy->setEnabled(b);
    ui->doubleSpinBoxWavelength->setEnabled(b);
    if (!b) emissionLineChanged(ui->comboBoxEmissionLines->currentIndex());
}

void ElementDataDialog::updateDisplayLists()
{
    double v = ui->doubleSpinBoxEnergy->value();
    QStringList   el;
    QList<int>    z;
    QList<double> f0;
    QList<double> f1;
    QList<double> f2;
    QList<double> mac;
    QList<double> lac;

    scatData.getElementValuesAtEnergy(v, el, z, f0, f1, f2, mac, lac);
    updateAbsWidget(el, z, f1, f2, mac, lac);
}


void ElementDataDialog::updateAbsWidget(const QStringList &el, const QList<int> &z, const QList<double> &f1, const QList<double> &f2, const QList<double> &mac, const QList<double> &lac)
{
    ui->treeWidgetAbsorption->clear();
    int n = qMin(el.size(), qMin(mac.size(), lac.size()));

    for (int i = 0; i < n; ++i) {
        QStringList lbl;
        lbl << el.at(i);
        lbl << QString("%1").arg(z.at(i));
        lbl << QString("%1").arg(f1.at(i), 0, 'f', 3);
        lbl << QString("%1").arg(f2.at(i), 0, 'f', 3);
        lbl << QString("%1").arg(mac.at(i), 0, 'f', 3);
        lbl << QString("%1").arg(lac.at(i), 0, 'f', 3);

        QTreeWidgetItem *it = new QTreeWidgetItem(lbl);

        it->setData(1, Qt::UserRole, z.at(i));
        it->setData(2, Qt::UserRole, f1.at(i) - double(z.at(i)));
        it->setData(3, Qt::UserRole, f2.at(i));
        it->setData(4, Qt::UserRole, mac.at(i));
        it->setData(5, Qt::UserRole, lac.at(i));

        ui->treeWidgetAbsorption->addTopLevelItem(it);
    }
}

void ElementDataDialog::saveMdr()
{
    QString f = QFileDialog::getSaveFileName(this, QString(tr("Save MDR file")), QDir::homePath(), QString("BGMN MDR file (*.mdr *.MDR)"));
    if (f.isEmpty()) return;

    QString out = QString("% Mass absorption coefficients for elements with %1 keV radiation\n").arg(ui->doubleSpinBoxEnergy->value(), 0, 'f', 6);
    out += "% Generated by Profex\n";

    for (int i = 0; i < ui->treeWidgetAbsorption->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetAbsorption->topLevelItem(i);
        out += QString("%1 %2\n").arg(it->text(0).toUpper()).arg(it->text(4));
    }

    BgmnFileIO::writeTextFile(f, out);
}

void ElementDataDialog::saveAno()
{
    QString f = QFileDialog::getSaveFileName(this, QString(tr("Save ANO file")), QDir::homePath(), QString("BGMN ANO file (*.ano *.ANO)"));
    if (f.isEmpty()) return;

    QString out = QString("% Anomalous scattering factors for elements with %1 keV radiation\n").arg(ui->doubleSpinBoxEnergy->value(), 0, 'f', 6);
    out += "% Generated by Profex\n";

    for (int i = 0; i < ui->treeWidgetAbsorption->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetAbsorption->topLevelItem(i);
        out += QString("%1 %2 %3\n").arg(it->text(0).toUpper()).arg(it->text(2)).arg(it->text(3));
    }

    BgmnFileIO::writeTextFile(f, out);
}

void ElementDataDialog::saveXy()
{
    QString f = QFileDialog::getSaveFileName(this, QString(tr("Save CSV file")), QDir::homePath(), QString("ASCII CSV file (*.csv *.CSV)"));
    if (f.isEmpty()) return;

    int c = ui->plotParam->graphCount();
    QList<QStringList> gData;
    QStringList header;

    for (int i = 0; i < c; ++i) {
        gData.append(QStringList());
        header << QString("Energy [keV];%1").arg(ui->plotParam->graph(i)->name());
    }

    int n = 0;

    for (int i = 0; i < c; ++i) {
        QCPDataContainer<QCPGraphData>::const_iterator it;

        for (it = ui->plotParam->graph(i)->data().data()->constBegin(); it != ui->plotParam->graph(i)->data().data()->constEnd(); ++it) {
            gData[i] << QString("%1;%2").arg(it->key, 0, 'f', 6).arg(it->value, 0, 'f', 6);
        }

        n = qMax(n, gData.at(i).size());
    }

    QStringList out(header.join(";"));

    for (int i = 0; i < n; ++i) {
        QStringList line;

        for (int j = 0; j < c; ++j) {
            if (i >= gData.at(j).size()) line.append(";");
            else                         line.append(gData.at(j).at(i));
        }

        out.append(line.join(";"));
    }

    BgmnFileIO::writeTextFile(f, out.join("\n"));
}

void ElementDataDialog::savePdf()
{
    QString f = QFileDialog::getSaveFileName(this, QString(tr("Save PDF file")), QDir::homePath(), QString("PDF file (*.pdf *.PDF)"));
    if (f.isEmpty()) return;
    ui->plotParam->savePdf(f);
}

void ElementDataDialog::cursorCoordinates(QMouseEvent *event)
{
    QString sx;
    QString sy;
    QString sa;

    double x = ui->plotParam->xAxis->pixelToCoord(event->pos().x());
    double y = ui->plotParam->yAxis->pixelToCoord(event->pos().y());
    double a = xAxisWl ? global::Functions::wavelengthToEnergy(x) : global::Functions::energyToWavelength(x);

    if ((ui->plotParam->xAxis->range().contains(x))) {
        sx = QString::number(xAxisWl ? 10.0 * x : x, 'f', xAxisWl ? 6 : 4);
        sy = QString::number(y, 'f', 6);
        sa = QString::number(xAxisWl ? a : 10.0 * a, 'f', xAxisWl ? 4 : 6);
    }

    int idx = ui->comboBoxDisplayParam->currentIndex();
    ui->labelCoordinates->setText(QString("%1 = %2 %3 (%4 = %5 %6)    %7 = %8 %9")
                                  .arg(xAxisWl ? QString(global::lambda) : "E")
                                  .arg(sx)
                                  .arg(xAxisWl ? QString(global::angstrom) : "keV")
                                  .arg(xAxisWl ? "E" : QString(global::lambda))
                                  .arg(sa)
                                  .arg(xAxisWl ? "keV" : QString(global::angstrom))
                                  .arg(axLbl.at(idx).label)
                                  .arg(sy)
                                  .arg(axLbl.at(idx).unit));
}

void ElementDataDialog::xunitChanged()
{
    xAxisWl = ui->comboBoxXunit->currentIndex() == 1;
    setupXAxis();
    updateDisplayGraph();
}

void ElementDataDialog::splitterXchanged(int,int)
{
    settings->setValue("elementDataDialog/splitterList", ui->splitterList->saveState());
}

void ElementDataDialog::splitterYchanged(int,int)
{
    settings->setValue("elementDataDialog/splitterDisplay", ui->splitterDisplay->saveState());
}
