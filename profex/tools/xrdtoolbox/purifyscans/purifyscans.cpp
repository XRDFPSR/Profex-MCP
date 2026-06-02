/***************************************************************************
                          purifyscans.cpp  -  description
                             -------------------
    begin                : Sat Jun 24 11:00:00 CEST 2017
    copyright            : (C) 2017 by Nicola Doebelin
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

#include <QFile>
#include <QFileInfo>
#include <QTextStream>
#include <QString>
#include <QFileDialog>
#include <QDir>
#include <QRegularExpression>
#include <QDebug>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QFontMetrics>

#include "../libXrdIO/import/importhandler.h"
#include "../libXrdIO/bgmnfileio.h"
#include "purifyscans.h"
#include "ui_purifyscans.h"

PurifyScans::PurifyScans(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PurifyScans)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();

    plotWidget = new XrdCustomPlot(this);

    QGridLayout *layout = new QGridLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(plotWidget, 0, 0, 1, 1);
    ui->plotWidgetContainer->setLayout(layout);

    ui->pushButtonSaveAs->setEnabled(false);
    ui->pushButtonPurge->setEnabled(false);

    ui->labelReferenceGewichtLabel->setText("GEWICHT:");
    ui->labelReferenceGewichtMacLabel->setText("K:");
    ui->labelReferenceSampleMacLabel->setText(QString("%1%2*:").arg(global::mu).arg(global::subM));
    ui->doubleSpinBoxRefMac->setSuffix(QString(" cm%1/g").arg(global::superTwo));

    ui->labelSampleGewichtMacLabel->setText("K:");
    ui->labelSampleGewichtLabel->setText("Bulk GEWICHT:");
    ui->labelSampleMacLabel->setText(QString("%1%2*:").arg(global::mu).arg(global::subM));

    ui->labelReferenceGewichtMac->setText("0.0000");
    ui->labelSampleGewicht->setText("0.0000");
    ui->labelSampleMac->setText("0.0000");
    ui->labelSampleGewichtMac->setText("0.0000");
    ui->labelDiaFileName->setText("");

    ui->splitter->restoreState(settings->value("purifyScans/splitterState", QByteArray()).toByteArray());
    ui->treeWidgetPhases->header()->restoreState(settings->value("purifyScans/treeViewHeader", QByteArray()).toByteArray());

    ui->doubleSpinBoxRefGewicht->setValue(settings->value("purifyScans/refGewicht", 0.0).toDouble());
    ui->doubleSpinBoxRefMac->setValue(settings->value("purifyScans/refMac", 0.0).toDouble());

    ui->doubleSpinBoxTubeInitial->setValue(settings->value("purifyScans/tubeInitial", 0.0).toDouble());
    ui->doubleSpinBoxTubeCurrent->setValue(settings->value("purifyScans/tubeCurrent", 0.0).toDouble());

    referenceValuesChanged();

    plotWidget->legend->setVisible(true);
}

void PurifyScans::saveSettings()
{
    // if the MAC of a phase is != 0.0, store it in the settings so
    // we can retrieve it later as a default value
    QMap<QString, QVariant> macList;

    for (int i = 0; i < ui->treeWidgetPhases->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetPhases->topLevelItem(i);
        if (!qFuzzyIsNull(it->text(2).toDouble())) {
            macList[it->text(0)] = QVariant(it->text(2));
        }
    }

    settings->setValue("purifyScans/macs", macList);
    settings->setValue("purifyScans/splitterState", ui->splitter->saveState());
    settings->setValue("purifyScans/treeViewHeader", ui->treeWidgetPhases->header()->saveState());
    settings->setValue("purifyScans/refGewicht", ui->doubleSpinBoxRefGewicht->value());
    settings->setValue("purifyScans/refMac", ui->doubleSpinBoxRefMac->value());
    settings->setValue("purifyScans/tubeInitial", ui->doubleSpinBoxTubeInitial->value());
    settings->setValue("purifyScans/tubeCurrent", ui->doubleSpinBoxTubeCurrent->value());
}

PurifyScans::~PurifyScans()
{
    saveSettings();
    delete ui;
}

void PurifyScans::load()
{
    QString s(QFileDialog::getOpenFileName(this,
                                           tr("Load DIA file"),
                                           settings->value("purifyScans/workingDir", QDir::homePath()).toString(),
                                           tr("DIA file (*.dia *.DIA)")));

    if (!s.isEmpty()) loadFile(s);
}

void PurifyScans::loadFile(const QString &s)
{
    diaFileName.setFile(s);

    if (!diaFileName.exists()) {
        diaFileName = QFileInfo();
        return;
    }

    qDebug() << QString("PurifyScans::load: Parsing DIA file %1").arg(diaFileName.absoluteFilePath());
    settings->setValue("purifyScans/workingDir", diaFileName.absolutePath());
    ui->labelDiaFileName->setText(diaFileName.fileName());

    ui->treeWidgetPhases->clear();

    scanHeap.clear();
    ImportHandler iHandler;
    if (iHandler.uidByFileName(diaFileName.absoluteFilePath()) != "BGMN_DIA") return;

    iHandler.load(diaFileName.absoluteFilePath(), iHandler.uidByFileName(diaFileName.absoluteFilePath()), scanHeap, true);
    plotWidget->clearGraphs();
    plotDia(-1);

    QFileInfo fiLst(diaFileName.absolutePath() + "/" + diaFileName.completeBaseName() + ".lst");

    if (!fiLst.exists()) {
        qDebug() << QString("PurifyScans::load: File does not exist: %1").arg(fiLst.absoluteFilePath());
        return;
    }

    qDebug() << QString("PurifyScans::load: Parsing LST file %1").arg(fiLst.absoluteFilePath());

    parseContent(BgmnFileIO::readTextFile(fiLst.absoluteFilePath()));
    precheckItems();
    ui->pushButtonPurge->setEnabled(true);
}

// n = -1: Draw all scans in the dia file
// n >= 0: Draw scan No. n only
void PurifyScans::plotDia(int n)
{
    if (!scanHeap.size()) return;

    QMap<QString, QVariant> macList = settings->value("purifyScans/macs", QMap<QString, QVariant>()).toMap();

    int iFirst = 0;
    int iLast = scanHeap.size() - 1;

    if (n >= 0) {
        iFirst = qMin(n, scanHeap.size() - 1);
        iLast  = iFirst;
    }

    double xmin = 180.0;
    double xmax = 0.0;

    for (int i = iFirst; i <= iLast; ++i) {
        if (i == 2) continue; // skip Idiff
        plotWidget->addGraph();
        plotWidget->graph(plotWidget->graphCount() - 1)->addData(scanHeap[i].pDataAngle(), scanHeap[i].pDataIntensity());
        plotWidget->graph(plotWidget->graphCount() - 1)->setName(scanHeap[i].name());
        plotWidget->graph(plotWidget->graphCount() - 1)->rescaleAxes(true);

        xmin = qMin(xmin, scanHeap[i].pDataAngle().first());
        xmax = qMax(xmax, scanHeap[i].pDataAngle().last());

        if (i > 3) {
            QString name = scanHeap[i].name();

            QTreeWidgetItem *it = new QTreeWidgetItem(ui->treeWidgetPhases);
            it->setFlags(it->flags() | Qt::ItemIsEditable);
            it->setText(1, name);
            it->setText(2, "0.00000000");
            it->setText(3, macList.contains(name) ? macList[name].toString() : "0.000000");
            it->setCheckState(0, Qt::Unchecked);
        }
    }

    if (plotWidget->graphCount() > 0) plotWidget->graph(0)->setPen(QPen(Qt::black));
    if (plotWidget->graphCount() > 1) plotWidget->graph(1)->setPen(QPen(Qt::red));
    if (plotWidget->graphCount() > 2) plotWidget->graph(2)->setPen(QPen(Qt::blue));

    if (plotWidget->graphCount() > 3) {
        for (int i = 3; i < plotWidget->graphCount(); ++i) {
            plotWidget->graph(i)->setPen(settings->getRandomColor(i));
        }
    }

    plotWidget->setXLimits(xmin, xmax);
    plotWidget->xAxis->setRange(xmin, xmax);
    plotWidget->replot();
}

void PurifyScans::plotPurged()
{
    if (!purgedScan.size()) return;

    plotDia(0);

    int n = plotWidget->graphCount();

    plotWidget->addGraph();
    plotWidget->graph(n)->addData(purgedScan.pDataAngle(), purgedScan.pDataIntensity());
    plotWidget->graph(n)->setName(purgedScan.name());
    plotWidget->graph(n)->rescaleAxes(true);

    if (n > 0) {
        plotWidget->graph(n)->setPen(QPen(Qt::red));
    } else {
        plotWidget->graph(n)->setPen(QPen(Qt::black));
    }

    plotWidget->replot();
}

void PurifyScans::precheckItems()
{
    double gw = 0.0;
    int c = 0;

    // check all items, but remember the one with the highest GEWICHT
    for (int i = 0; i < ui->treeWidgetPhases->topLevelItemCount(); ++i) {
        ui->treeWidgetPhases->topLevelItem(i)->setCheckState(0, Qt::Checked);

        double igw = ui->treeWidgetPhases->topLevelItem(i)->text(2).toDouble();

        if (igw > gw) {
            gw = igw;
            c = i;
        }
    }

    // uncheck the one with the highest GEWICHT, assuming that it is the main phase
    ui->treeWidgetPhases->topLevelItem(c)->setCheckState(0, Qt::Unchecked);
}

void PurifyScans::parseContent(const QString &s)
{
    QVector<double> defaultVal(2, 0.0);
    QMap<QString, QVector<double> > mValues;

    QRegularExpression rxPhase("Local\\s+parameters\\s+and\\s+GOALs\\s+for\\s+phase\\s+(\\S+)");
    QRegularExpression rxGw("\\b\\(?GEWICHT\\)?=(\\d+\\.?\\d*)");
    QRegularExpression rxMc("\\bMAC[a-zA-Z0-9]*=(\\d+\\.?\\d*)");

    int offset = 0;
    QRegularExpressionMatch rmPhase = rxPhase.match(s, offset);

    while (rmPhase.hasMatch()) {
        QString phase = rmPhase.captured(1).simplified();
        offset = rmPhase.capturedEnd(1) + 1;

        mValues[phase] = defaultVal;

        QRegularExpressionMatch rmGw = rxGw.match(s, offset);
        QRegularExpressionMatch rmMc = rxMc.match(s, offset);

        if (rmGw.hasMatch()) mValues[phase][0] = rmGw.captured(1).toDouble();
        if (rmMc.hasMatch()) mValues[phase][1] = rmMc.captured(1).toDouble();

        qDebug() << QString("PurifyScans::parseContent(): Phase = %1, GEWICHT = %2, MAC = %3")
                    .arg(phase)
                    .arg(mValues[phase][0])
                    .arg(mValues[phase][1]);

        rmPhase = rxPhase.match(s, offset);
    }

    QMapIterator<QString, QVector<double> > it(mValues);

    while (it.hasNext()) {
        it.next();

        for (int i = 0; i < ui->treeWidgetPhases->topLevelItemCount(); ++i) {
            QTreeWidgetItem *item = ui->treeWidgetPhases->topLevelItem(i);

            if (item->text(1).simplified() == it.key().simplified()) {
                item->setText(2, QString("%1").arg(it.value()[0], 0, 'f', 8));
                item->setText(3, QString("%1").arg(it.value()[1], 0, 'f', 6));
            }
        }
    }
}

void PurifyScans::saveAs()
{
    QString file(QFileDialog::getSaveFileName(this,
                                              tr("Save scan"),
                                              settings->value("purifyScans/workingDir", QDir::homePath()).toString(),
                                              tr("ASCII Scan file (*.xy *.XY)")));

    if (file.isEmpty()) return;
    if (!purgedScan.size()) return;

    writeToFile(file, purgedScan.pDataAngle(), purgedScan.pDataIntensity());
}

void PurifyScans::purge()
{
    purgedScan.reset();
    if (!uiValuesCheck()) return;

    double kRef = ui->doubleSpinBoxRefMac->value() * ui->doubleSpinBoxRefGewicht->value();
    double kSamp = getMainPhaseK();

    if (qFuzzyIsNull(kRef) || qFuzzyIsNull(kSamp)) {
        qDebug() << QString("PurifyScans::purge(): No MAC or GEWICHT found for the main phase. Please enter valid values.");
        return;
    }

    // scale factor to normalize main phase to 100% crystallinity (affects only diffraction signal, not background)
    double scaleGw = kRef / kSamp;

    // compensate for tube intensity loss (affects diffraction signal and background)
    double scaleNetIntens = ui->doubleSpinBoxTubeInitial->value() / ui->doubleSpinBoxTubeCurrent->value();

    qDebug() << QString("Scale factors: kref = %1, ksamp = %2, gw = %3, intens = %4").arg(kRef).arg(kSamp).arg(scaleGw).arg(scaleNetIntens);

    QList<int> strip = getStripPhases();

    // number in scanHeap of the checked scan (scanHeap also contains Iobs, Icalc, Idiff, Ibkgr,
    // but they are not displayed)
    for (int i = 0; i < strip.size(); ++i) {
        strip[i] += 4;
    }

    QVector<double> ang(scanHeap.first().pDataAngle());
    QVector<double> iobs(scanHeap.first().pDataIntensity());

    for (int i = 0; i < iobs.size(); ++i) {
        // subtract the background
        iobs[i] -= scanHeap[3].intensity(i);

        // strip all checked phases
        for (int s = 0; s < strip.size(); ++s) {
            int z = strip[s];

            if (z < scanHeap.size()) {
                // subtract the phase signal without background
                iobs[i] -= (scanHeap[z].intensity(i) - scanHeap[3].intensity(i));
            }
        }

        // scale to target K value
        iobs[i] *= scaleGw;

        // add back the background signal
        iobs[i] += scanHeap[3].intensity(i);

        // apply tube intensity scaling
        iobs[i] *= scaleNetIntens;
    }

    purgedScan.setDataAng(ang);
    purgedScan.setDataInt(iobs);
    purgedScan.setName("Purified Scan");

    plotWidget->clearGraphs();
    plotPurged();
    ui->pushButtonSaveAs->setEnabled(true);
}

QList<int> PurifyScans::getStripPhases()
{
    QList<int> lst;

    for (int i = 0; i < ui->treeWidgetPhases->topLevelItemCount(); ++i) {
        if (ui->treeWidgetPhases->topLevelItem(i)->checkState(0) == Qt::Checked) {
            lst.append(i);
        }
    }

    return lst;
}

double PurifyScans::getMainPhaseK()
{
    double ksamp = 0.0;

    for (int i = 0; i < ui->treeWidgetPhases->topLevelItemCount(); ++i) {
        QTreeWidgetItem *it = ui->treeWidgetPhases->topLevelItem(i);

        if (it->checkState(0) == Qt::Unchecked) {
            ksamp += it->text(2).toDouble() * it->text(3).toDouble();
        }
    }

    return ksamp;
}

void PurifyScans::writeToFile(const QString &file, const QVector<double> &ang, const QVector<double> &iobs)
{
    QString sep = settings->value("config/asciiFieldSeparator", " ").toString();
    QString out;

    for (int i = 0; i < iobs.size(); ++i) {
        out += QString("%1%2%3\n").arg(ang.at(i), 0, 'f', 6).arg(sep).arg(iobs.at(i), 0, 'f', 6);
    }

    BgmnFileIO::writeTextFile(file, out);
}

void PurifyScans::updateSampleValues()
{
    double sumGw = 0.0;
    double sumMc = 0.0;

    for (int i = 0; i < ui->treeWidgetPhases->topLevelItemCount(); ++i) {
        sumGw += ui->treeWidgetPhases->topLevelItem(i)->text(2).toDouble();
    }

    for (int i = 0; i < ui->treeWidgetPhases->topLevelItemCount(); ++i) {
        double curGw = ui->treeWidgetPhases->topLevelItem(i)->text(2).toDouble();
        double curMc = ui->treeWidgetPhases->topLevelItem(i)->text(3).toDouble();
        sumMc += curMc * curGw / sumGw;
    }

    ui->labelSampleGewicht->setText(QString("%1").arg(sumGw, 0, 'f', 8));
    ui->labelSampleMac->setText(QString("%1 cm%2/g").arg(sumMc, 0, 'f', 4).arg(global::superTwo));
    ui->labelSampleGewichtMac->setText(QString("%1").arg(sumGw*sumMc, 0, 'f', 4));
}

/*
 * updates the current sample's mac
 */
void PurifyScans::itemChanged(QTreeWidgetItem *, int)
{
    updateSampleValues();
}

void PurifyScans::referenceValuesChanged()
{
    double refK = ui->doubleSpinBoxRefGewicht->value() * ui->doubleSpinBoxRefMac->value();
    double tarK = 0.0;

    if (!qFuzzyIsNull(ui->doubleSpinBoxTubeCurrent->value())) {
            tarK = refK * ui->doubleSpinBoxTubeInitial->value() / ui->doubleSpinBoxTubeCurrent->value();
    }

    ui->labelReferenceGewichtMac->setText(QString("%1").arg(refK, 0, 'f', 4));
    ui->labelTargetK->setText(QString("%1").arg(tarK, 0, 'f', 4));
}

bool PurifyScans::uiValuesCheck()
{
    if (scanHeap.size() < 4) {
        qDebug() << QString("PurifyScans::uiValuesCheck(): No Scans found. Please load a DIA file first.");
        return false;
    }

    if (qFuzzyIsNull(ui->doubleSpinBoxRefMac->value())) {
        qDebug() << QString("PurifyScans::uiValuesCheck(): No reference MAC value found. Please enter a valid MAC.");
        return false;
    }

    if (qFuzzyIsNull(ui->doubleSpinBoxTubeCurrent->value()) || qFuzzyIsNull(ui->doubleSpinBoxTubeInitial->value())) {
        qDebug() << QString("PurifyScans::uiValuesCheck():  No tube intensities found. Please enter valid intensities.");
        return false;
    }

    return true;
}
