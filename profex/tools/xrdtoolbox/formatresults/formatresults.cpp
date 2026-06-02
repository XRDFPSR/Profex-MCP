/***************************************************************************
                          formatresults.cpp  -  description
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

#include "formatresults.h"
#include "ui_formatresults.h"

#include <QDir>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QRegularExpression>
#include <math.h>
#include "../libXrdIO/bgmnfileio.h"
#include "../libXrdIO/structs.h"

FormatResults::FormatResults(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::FormatResults)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();
    ui->toolButtonSaveAs->setEnabled(false);
    ui->toolButtonReload->setEnabled(false);

    QString sep = settings->value("config/asciiFieldSeparator", " ").toString();

    ui->checkBoxSkipR->setChecked(settings->value("formatResults/skipR", true).toBool());
    ui->checkBoxMeanSd->setChecked(settings->value("formatResults/meanSd", true).toBool());
    ui->checkBoxEsd->setChecked(settings->value("formatResults/includeEsd", true).toBool());
    ui->lineEditFieldSeparator->setText(settings->value("formatResults/fieldSeparator", sep).toString());
    ui->splitter->restoreState(settings->value("formatResults/splitter", QByteArray()).toByteArray());

    skipParams << "Chi2" << "Rexp" << "Rwp" << "GOF";

    connect(ui->checkBoxEsd, SIGNAL(clicked(bool)), this, SLOT(reload()));
    connect(ui->checkBoxMeanSd, SIGNAL(clicked(bool)), this, SLOT(reload()));
    connect(ui->checkBoxSkipR, SIGNAL(clicked(bool)), this, SLOT(reload()));
}

FormatResults::~FormatResults()
{
    delete ui;
}

void FormatResults::closeEvent(QCloseEvent *e)
{
    saveSettings();
    e->accept();
}

void FormatResults::saveSettings()
{
    settings->setValue("formatResults/skipR", ui->checkBoxSkipR->isChecked());
    settings->setValue("formatResults/meanSd", ui->checkBoxMeanSd->isChecked());
    settings->setValue("formatResults/includeEsd", ui->checkBoxEsd->isChecked());
    settings->setValue("formatResults/fieldSeparator", ui->lineEditFieldSeparator->text());
    settings->setValue("formatResults/splitter", ui->splitter->saveState());
}

void FormatResults::load()
{
    QString workingDir(settings->value("config/workingdir", QDir::homePath()).toString());
    if (loadedFile.isReadable()) workingDir = loadedFile.absolutePath();

    loadedFile = QFileInfo(QFileDialog::getOpenFileName(this, tr("Open CSV file"), workingDir, tr("CSV file (*.csv *.CSV)")));
    if (!loadedFile.isReadable()) return;

    parseCsv(BgmnFileIO::readTextFile(loadedFile.absoluteFilePath()));
    ui->toolButtonReload->setEnabled(true);
    ui->toolButtonSaveAs->setEnabled(true);
}

void FormatResults::saveAs()
{
    QString out;
    QString sep = ui->lineEditFieldSeparator->text();

    QFileInfo file(QFileDialog::getSaveFileName(this, tr("Save to File"), loadedFile.absolutePath(), tr("CSV file (*.csv *.CSV")));

    for (int r = 0; r < ui->tableWidget->rowCount(); ++r) {
        QStringList line;

        for (int c = 0; c < ui->tableWidget->columnCount(); ++c) {
            if (ui->tableWidget->item(r, c)) {
                line.append(ui->tableWidget->item(r, c)->text());
            } else {
                line.append(QString());
            }
        }

        out += line.join(sep) + "\n";
    }

    if (BgmnFileIO::writeTextFile(file.absoluteFilePath(), out)) {
        qDebug() << QString("FormatResults::saveAs(): Data saved to %1").arg(file.absoluteFilePath());
    } else {
        qDebug() << QString("FormatResults::saveAs(): Could not save data to %1").arg(file.absoluteFilePath());
    }
}

void FormatResults::reload()
{
    parseCsv(BgmnFileIO::readTextFile(loadedFile.absoluteFilePath()));
}

void FormatResults::parseCsv(const QString &s)
{
    ui->tableWidget->clear();
    QMap<QString, ResultsDataSet> data(splitCsv(s));
    resizeTable(data);

    if ((ui->tableWidget->rowCount() == 0) || (ui->tableWidget->columnCount() == 0)) {
        qDebug() << QString("FormatResults::parseCsv: Illegal table size: r=%1 c=%2").arg(ui->tableWidget->rowCount()).arg(ui->tableWidget->columnCount());
        return;
    }

    QMap<QString, QMap<int, QStringList> > dataTable(transposeData(data)); //  <parameter,   <simulation, values;esds> >
    if (ui->checkBoxMeanSd->isChecked()) dataTable = addMeanSd(dataTable);

    createTableHeader();
    createTableData(dataTable);
}

void FormatResults::resizeTable(const QMap<QString, ResultsDataSet> &data)
{
    QList<int> sizes(getTableSize(data));
    int skip = ui->checkBoxEsd->isChecked() ? 2 : 1;

    int w = 1 + skip * sizes.at(0);
    int h = 1 + sizes.at(1) * sizes.at(2);

    if (ui->checkBoxMeanSd->isChecked()) {
        h += sizes.at(2) * 2;
    }

    ui->tableWidget->setColumnCount(w);
    ui->tableWidget->setRowCount(h);
}

/*
 * returns ResultsDataSets:
 *     QMap<filename, ResultsDataSet>
 */
QMap<QString, ResultsDataSet> FormatResults::splitCsv(const QString &s)
{
    QMap<QString, ResultsDataSet> list;
    QStringList lines(s.split(global::rxLineEnding));

    for (int i = 0; i < lines.size(); ++i) {
        QStringList line(lines.at(i).split(";", Qt::KeepEmptyParts)); // contains file;sample;sampleID;Parameter;Value;ESD

        ResultsDataSet data = lineToResultsDataSet(line);
        if (data.isEmpty) continue;

        QString id(data.file);

        if (list.contains(id)) {
            QString param(data.parameters.last());
            list[id].composition = data.composition;
            list[id].simulation = data.simulation;
            list[id].parameters.append(param);
            list[id].values[param] = line.at(4).toDouble();
            list[id].esds[param] = line.at(5).toDouble();
        } else {
            list[id] = data;
        }
    }

    return list;
}

QMap<QString, QMap<int, QStringList> > FormatResults::transposeData(const QMap<QString, ResultsDataSet> &data)
{
    QMap<QString, QMap<int, QStringList> > dataTable;
    //  <parameter,   <simulation, values;esds> >

    QMapIterator<QString, ResultsDataSet> it(data);
    while (it.hasNext()) {
        it.next();

        ResultsDataSet d = it.value();
        if (d.isEmpty) continue;

        for (int i = 0; i < d.parameters.size(); ++i) {
            QString p(d.parameters.at(i));
            dataTable[p][d.simulation].append(QString("%1").arg(d.values[p], 3));
            dataTable[p][d.simulation].append(QString("%1").arg(d.esds[p], 3));
        }
    }

    return dataTable;
}

QMap<QString, QMap<int, QStringList> > FormatResults::addMeanSd(const QMap<QString, QMap<int, QStringList> > &dataTableIn)
{
    QMap<QString, QMap<int, QStringList> > dataTable = dataTableIn;
    //  <parameter,   <simulation, values;esds> >

    QMapIterator<QString, QMap<int, QStringList> > it(dataTable);
    while (it.hasNext()) {
        it.next();
        QStringList means(getAllMeans(it.value()));
        QStringList sds(getAllSds(it.value()));
        dataTable[it.key()][meanIndex] = means;
        dataTable[it.key()][sdIndex] = sds;
    }

    return dataTable;
}

QList<int> FormatResults::getTableSize(const QMap<QString, ResultsDataSet> &t)
{
    QList<int> comps;
    QList<int> sims;
    QStringList params;

    QMapIterator<QString, ResultsDataSet> it(t);

    while (it.hasNext()) {
        it.next();

        ResultsDataSet d = it.value();
        if (d.isEmpty) continue;

        if (!comps.contains(d.composition)) comps.append(d.composition);
        if (!sims.contains(d.simulation))   sims.append(d.simulation);

        for (int j = 0; j < d.parameters.size(); ++j) {
            if (!params.contains(d.parameters.at(j))) params.append(d.parameters.at(j));
        }
    }

    qDebug() << QString("FormatResults::splitCsv: Found %1 compositions with %2 simulations and %3 parameters").arg(comps.size()).arg(sims.size()).arg(params.size());

    QList<int> sizes(3, 0);
    sizes[0] = comps.size();
    sizes[1] = sims.size();
    sizes[2] = params.size();

    return sizes;
}

ResultsDataSet FormatResults::lineToResultsDataSet(const QStringList &l)
{
    ResultsDataSet data;
    data.isEmpty = true;

    if (l.size() < 5) return data;

    QRegularExpression rx("comp(\\d+)-sim(\\d+)\\.[a-zA-Z0-9]+");
    QRegularExpressionMatch rm;
    QFileInfo fi(l.first());

    QString param(l.at(3));

    QRegularExpression rxsum("([^/]+)/sum");
    QRegularExpressionMatch rmsum = rxsum.match(l.at(3));

    if (rmsum.hasMatch()) {
        param = rmsum.captured(1);
    }

    if (ui->checkBoxSkipR->isChecked() && skipParams.contains(param)) {
        return data;
    }

    rm = rx.match(fi.fileName());
    if (!rm.hasMatch()) return data;

    data.file = l.first();
    data.composition = rm.captured(1).toInt();
    data.simulation  = rm.captured(2).toInt();
    data.parameters.append(param);
    data.values[param] = l.at(4).toDouble();

    if (l.size() > 5) {
        data.esds[param] = l.at(5).toDouble();
    } else {
        data.esds[param] = std::numeric_limits<double>::min();
    }

    data.isEmpty = false;
    return data;
}

void FormatResults::createTableHeader()
{
    int comp = 0;
    int skip = ui->checkBoxEsd->isChecked() ? 2 : 1;

    ui->tableWidget->setItem(0, 0, new QTableWidgetItem(tr("Phase")));

    for (int i = 1; i < ui->tableWidget->columnCount(); i += skip) {
        ui->tableWidget->setItem(0, i, new QTableWidgetItem(QString("comp-%1").arg(comp, 3, 10, QLatin1Char('0'))));
        ++comp;
    }
}

/*
 * Format of the map:
 *     <parameter,   <simulation, values;esds> >
 */
void FormatResults::createTableData(const QMap<QString, QMap<int, QStringList> > &dataTable)
{
    QMapIterator<QString, QMap<int, QStringList> > par(dataTable);
    int row = 1;
    int skip = ui->checkBoxEsd->isChecked() ? 1 : 2;

    while (par.hasNext()) {
        par.next();
        QMapIterator<int, QStringList> sim(par.value());

        while (sim.hasNext()) {
            sim.next();
            QStringList valEsd = sim.value();

            if (row >= ui->tableWidget->rowCount()) break;

            if (sim.key() == meanIndex) ui->tableWidget->setItem(row, 0, new QTableWidgetItem(par.key() + " Mean"));
            else if (sim.key() == sdIndex) ui->tableWidget->setItem(row, 0, new QTableWidgetItem(par.key() + " Std.Dev."));
            else ui->tableWidget->setItem(row, 0, new QTableWidgetItem(par.key()));

            int col = 1;
            for (int i = 0; i < valEsd.size(); i += skip) {
                if (i >= valEsd.size()) break;
                if (col >= ui->tableWidget->columnCount()) break;

                ui->tableWidget->setItem(row, col, new QTableWidgetItem(valEsd.at(i)));
                ++col;
            }

            ++row;
        }
    }
}

double FormatResults::mean(const QVector<double> &data)
{
    if (!data.size()) return 0.0;

    double mean = 0.0;

    for (int i = 0; i < data.size(); ++i) {
        mean += data.at(i);
    }

    mean /= double(data.size());
    return mean;
}

double FormatResults::standardDeviation(const QVector<double> &data, double mean)
{
    int n = data.size();
    if (n < 2) return 0.0;

    double sigma = 0.0;

    for (int i = 0; i < n; ++i) {
        sigma += pow(mean - data.at(i), 2.0);
    }

    sigma /= double(n-1);
    sigma = sqrt(sigma);
    return sigma;
}

QStringList FormatResults::getAllMeans(const QMap<int, QStringList> &data)
{
    if (!data.size()) return QStringList();

    QMapIterator<int, QStringList> it(data);
    int h = data.size();
    int w = 0;

    // determine the size of the value matrix
    while (it.hasNext()) {
        it.next();
        w = qMax(w, it.value().size());
    }

    // initialize a matrix filled with 0.0
    QVector<double> column(h, 0.0);
    QVector<QVector<double> > matrix(w, column);

    int row = 0;

    // fill the matrix with the real values
    it.toFront();
    while (it.hasNext()) {
        it.next();

        QStringList line(it.value());
        for (int col = 0; col < line.size(); ++col) {
            matrix[col][row] = line.at(col).toDouble();
        }

        ++row;
    }

    QStringList means;
    for (int i = 0; i < matrix.size(); ++i) {
        means.append(QString("%1").arg(mean(matrix.at(i)), 0, 'f', 6));
    }

    return means;
}

QStringList FormatResults::getAllSds(const QMap<int, QStringList> &data)
{
    if (!data.size()) return QStringList();

    QMapIterator<int, QStringList> it(data);
    int h = data.size();
    int w = 0;

    // determine the size of the value matrix
    while (it.hasNext()) {
        it.next();
        w = qMax(w, it.value().size());
    }

    // initialize a matrix filled with 0.0
    QVector<double> column(h, 0.0);
    QVector<QVector<double> > matrix(w, column);

    int row = 0;

    // fill the matrix with the real values
    it.toFront();
    while (it.hasNext()) {
        it.next();

        QStringList line(it.value());
        for (int col = 0; col < line.size(); ++col) {
            matrix[col][row] = line.at(col).toDouble();
        }

        ++row;
    }

    QStringList sds;
    for (int i = 0; i < matrix.size(); ++i) {
        sds.append(QString("%1").arg(standardDeviation(matrix.at(i), mean(matrix.at(i))), 0, 'f', 6));
    }

    return sds;
}

