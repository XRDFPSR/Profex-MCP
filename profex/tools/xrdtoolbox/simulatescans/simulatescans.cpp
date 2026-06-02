/***************************************************************************
                          simulatescans.cpp  -  description
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

#include <QFileDialog>
#include <QInputDialog>
#include <QTextStream>
#include <QProgressBar>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QMessageBox>
#include <QDebug>
#include "../libXrdIO/bgmnfileio.h"

#include <random>
#include <math.h>
#include <limits>

#include "simulatescans.h"

SimulateScans::SimulateScans(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::SimulateScans)
{
    ui->setupUi(this);
    settings = SettingsManager::getInstance();

    initSettings();

    seedTime.start();

    // these are the default quantities. They will be overwritten if other values
    // are found in the settings
    QString q1("100.00;99.00;98.00;95.00;90.00;80.00;70.00;50.00;30.00;20.00;10.00;5.00;2.00;1.00;0.00");
    QString q2("0.00;1.00;2.00;5.00;10.00;20.00;30.00;50.00;70.00;80.00;90.00;95.00;98.00;99.00;100.00");

    defaultQuant.append(q1.split(";"));
    defaultQuant.append(q2.split(";"));

    QStringList quantities(settings->value("simulateScans/quantities", QStringList()).toStringList());

    if (quantities.size()) {
        defaultQuant.clear();
        for (int i = 0; i < quantities.size(); ++i) {
            defaultQuant.append(quantities.at(i).split(";"));
        }
    }

    ui->spinBoxCompositions->setValue(defaultQuant.first().size());
    ui->tableWidgetCompositions->setRowCount(defaultQuant.first().size());
}

SimulateScans::~SimulateScans()
{
    delete ui;
}

/*
 * Note: Phase MACs and scan files are stored in the phase listwidgetitem's data in a stringlist
 * of the following format:
 *
 * <mac>;<scan1>;<scan2>;...;<scanN>
 */

void SimulateScans::initSettings()
{
    restoreGeometry(settings->value("simulateScans/geometry", QByteArray()).toByteArray());
    ui->progressBar->setValue(0);
    ui->spinBoxSimulations->setValue(settings->value("simulateScans/simulations", 30).toInt());
    ui->checkBoxNoise->setChecked(settings->value("simulateScans/addnoise", true).toBool());
    ui->doubleSpinBoxScale->setValue(settings->value("simulateScans/scale", 1.0).toDouble());
    ui->splitterV->restoreState(settings->value("simulateScans/splitterV", QByteArray()).toByteArray());
    ui->splitterH1->restoreState(settings->value("simulateScans/splitterH1", QByteArray()).toByteArray());
    ui->splitterH2->restoreState(settings->value("simulateScans/splitterH2", QByteArray()).toByteArray());

    ui->progressBar->setEnabled(false);
    ui->toolButtonSimulate->setEnabled(false);
    ui->toolButtonPhaseRemove->setEnabled(false);
    ui->toolButtonScanAdd->setEnabled(false);
    ui->toolButtonScanRemove->setEnabled(false);

    isRunning = false;
    abort = false;

    connect(this, SIGNAL(emitLogMessage(QString)), this, SLOT(logMessage(QString)));
}

void SimulateScans::saveSettings()
{
    settings->setValue("simulateScans/geometry", saveGeometry());
    settings->setValue("simulateScans/simulations", ui->spinBoxSimulations->value());
    settings->setValue("simulateScans/addnoise", ui->checkBoxNoise->isChecked());
    settings->setValue("simulateScans/scale", ui->doubleSpinBoxScale->value());
    settings->setValue("simulateScans/splitterV", ui->splitterV->saveState());
    settings->setValue("simulateScans/splitterH1",  ui->splitterH1->saveState());
    settings->setValue("simulateScans/splitterH2",  ui->splitterH2->saveState());

    QStringList quantities;

    for (int i = 0; i < defaultQuant.size(); ++i) {
        quantities.append(defaultQuant.at(i).join(";"));
    }

    settings->setValue("simulateScans/quantities", quantities);
}

void SimulateScans::close()
{
    saveSettings();
    QDialog::close();
}

void SimulateScans::runSynthesis()
{
    if (isRunning) {
        abort = true;
        return;
    }

    if (!checkMacs()) {
        QMessageBox::information(this, tr("Invalid MAC"), tr("Please enter a valid MAC (> 0.0) for all phases first."));
        return;
    }

    isRunning = true;
    abort = false;
    ui->toolButtonSimulate->setIcon(QIcon::fromTheme("profex-run-abort"));
    ui->textEditLog->clear();
    execSimulations();
}

void SimulateScans::execSimulations()
{
    QString dname(QFileDialog::getExistingDirectory(this, tr("Output file"),
                                                    settings->value("config/workingdir", QDir::homePath()).toString()));

    if (dname.isEmpty()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);
    qApp->processEvents();
    disableGUI(true);

    int nCompositions = ui->tableWidgetCompositions->rowCount();
    int nPhases       = ui->listWidgetPhases->count();
    int nSimulations  = ui->spinBoxSimulations->value();
    double scale      = ui->doubleSpinBoxScale->value();
    bool bNoise       = ui->checkBoxNoise->isChecked();

    emit emitLogMessage(QString("SimulateScans::create(): Computing %1 x %2 compositions from %3 phases").arg(nCompositions).arg(nSimulations).arg(nPhases));
    emit emitLogMessage(QString());

    ui->progressBar->setEnabled(true);
    ui->progressBar->setMaximum(nCompositions * nSimulations);
    int progress = 0;

    emit emitLogMessage(QString("    Using the following phase MACs:"));

    for (int i = 0; i < ui->listWidgetPhases->count(); ++i) {
        emit emitLogMessage(QString("        %1 = %2").arg(ui->listWidgetPhases->item(i)->text(), -20, ' ')
                    .arg(ui->listWidgetPhases->item(i)->data(Qt::UserRole).toStringList().first().toDouble(), 0, 'f', 6));
    }

    emit emitLogMessage(QString());

    unsigned long seed = seedTime.elapsed();
    std::default_random_engine generator(seed);

    for (int nc = 0; nc < nCompositions; ++nc) {
        if (abort) {
            emit emitLogMessage(QString("    Simulation aborted"));
            break;
        }

        emit emitLogMessage(QString());
        emit emitLogMessage(QString("    Composition %1 contains the following phase quantities:").arg(nc + 1));

        for (int i = 0; i < nPhases; ++i) {
            emit emitLogMessage(QString("        %1 = %2").arg(ui->listWidgetPhases->item(i)->text(), -20, ' ')
                        .arg(100.0 * quantValue(i, nc), 0, 'f', 2));
        }

        double ms = macSample(nc);

        emit emitLogMessage(QString());
        emit emitLogMessage(QString("        Sample MAC           = %1").arg(ms, 0, 'f', 6));
        emit emitLogMessage(QString("        Global scale factor  = %1").arg(scale, 0, 'f', 4));
        emit emitLogMessage(QString("        Apply Poisson noise  = %1").arg(bNoise ? "yes" : "no"));
        emit emitLogMessage(QString());

        for (int ns = 0; ns < nSimulations; ++ns) {
            emit emitLogMessage(QString("        Computing simulation no. %1").arg(ns + 1));
            QString fname(QString("%1/comp%2-sim%3.xy").arg(dname).arg(nc, 3, 10, QLatin1Char('0')).arg(ns, 5, 10, QLatin1Char('0')));

            QVector< QVector<double> > vecSum;

            // pick a random file of phase number np
            for (int np = 0; np < nPhases; ++np) {
                QString pname = ui->listWidgetPhases->item(np)->text();
                int nf = ui->listWidgetPhases->item(np)->data(Qt::UserRole).toStringList().size() - 1;
                std::uniform_int_distribution<int> distribution(0, nf - 1);
                int nr = distribution(generator);
                QFileInfo fiPhase(fileInfo(np, nr));

                double quantity = quantValue(np, nc);
                double mp = macPhase(np);
                double scaleFact = scale * quantity * mp / ms;

                emit emitLogMessage(QString("            Phase '%1'").arg(pname));
                emit emitLogMessage(QString("                Randomly selecting file %1 of %2").arg(nr+1).arg(nf));
                emit emitLogMessage(QString("                Using scale factor %1").arg(scaleFact, 0, 'f', 6));

                mergeFiles(vecSum, readFile(fiPhase.absoluteFilePath()), scaleFact);
            }

            emit emitLogMessage(QString());
            if (bNoise) emit emitLogMessage(QString("            Applying Poisson noise pattern"));

            QVector< QVector<double> > vecNoise = bNoise ? applyNoise(vecSum) : vecSum;
            writeFile(fname, vecNoise);
            ui->progressBar->setValue(progress++);
            qApp->processEvents();

            emit emitLogMessage(QString("            Data synthesis complete"));
            emit emitLogMessage(QString("            Output file written to %1").arg(fname));
            emit emitLogMessage(QString());
        }
    }

    ui->progressBar->reset();
    ui->progressBar->setEnabled(false);

    isRunning = false;
    abort = false;
    ui->toolButtonSimulate->setIcon(QIcon::fromTheme("profex-run"));

    qApp->restoreOverrideCursor();
    disableGUI(false);
}

double SimulateScans::quantValue(int phase, int quantity)
{
    if (phase >= ui->tableWidgetCompositions->columnCount()) return 0.0;
    if (quantity >= ui->tableWidgetCompositions->rowCount()) return 0.0;
    return ui->tableWidgetCompositions->item(quantity, phase)->text().toDouble() * 0.01;
}

double SimulateScans::macPhase(int phase)
{
    if (phase >= ui->listWidgetPhases->count()) return 0.0;
    return ui->listWidgetPhases->item(phase)->data(Qt::UserRole).toStringList().first().toDouble();
}

double SimulateScans::macSample(int composition)
{
    double mac = 0.0;

    int p = ui->listWidgetPhases->count();
    int n = ui->tableWidgetCompositions->rowCount();

    if (composition >= n) return 0.0;

    for (int i = 0; i < p; ++i) {
        mac += quantValue(i, composition) * macPhase(i);
    }

    return mac;
}

QFileInfo SimulateScans::fileInfo(int phase, int file)
{
    if (phase >= ui->listWidgetPhases->count()) return QFileInfo();
    if (file >= ui->listWidgetPhases->item(phase)->data(Qt::UserRole).toStringList().count() - 1) return QFileInfo();
    return QFileInfo(ui->listWidgetPhases->item(phase)->data(Qt::UserRole).toStringList().at(file + 1));
}

PhaseContrib SimulateScans::getPhaseContrib(int phase, int file, int quantity)
{
    PhaseContrib pContr;
    pContr.file = QString();
    pContr.quantity = 0.0;

    if (phase >= ui->listWidgetPhases->count()) return pContr;

    QFileInfo fi(fileInfo(phase, file));
    if (!fi.exists()) return pContr;

    pContr.file = fi.absoluteFilePath();
    pContr.quantity = quantValue(phase, quantity);

    return pContr;
}

QVector< QVector<double> > SimulateScans::readFile(const QString &f)
{
    QRegularExpression rx("\\s+|;");
    QVector< QVector<double> > vec;
    QStringList data(BgmnFileIO::readTextFileLines(f));

    for (int i = 0; i < data.size(); ++i) {
        QStringList line = data.at(i).split(rx);

        if (line.size() < 2) continue;

        QVector<double> vline(2);
        vline[0] = line.at(0).toDouble();
        vline[1] = line.at(1).toDouble();

        vec.append(vline);
    }

    return vec;
}

void SimulateScans::writeFile(const QString &f, const QVector<QVector<double> > &vec)
{
    QString sep = settings->value("config/asciiFieldSeparator", " ").toString();
    QString out;

    for (int i = 0; i < vec.size(); ++i) {
        out += QString("%1%2%3\n").arg(vec.at(i).at(0), 0, 'f', 6).arg(sep).arg(vec.at(i).at(1), 0, 'f', 6);
    }

    BgmnFileIO::writeTextFile(f, out);
}

void SimulateScans::mergeFiles(QVector<QVector<double> > &va, const QVector<QVector<double> > &vb, double factor)
{
    QVector<double> line(2);

    for (int i = 0; i < vb.size(); ++i) {
        if (i >= va.size()) {
            line[0] = vb.at(i).at(0);
            line[1] = vb.at(i).at(1) * factor;
            va.append(line);
        } else {
            va[i][1] += vb.at(i).at(1) * factor;
        }
    }
}

QVector< QVector<double> > SimulateScans::applyNoise(const QVector<QVector<double> > &vec)
{
    QVector< QVector<double> > vo = vec;

    unsigned long seed = seedTime.elapsed();
    std::default_random_engine generator(seed);

    for (int i = 0; i < vo.size(); ++i) {
        std::poisson_distribution<int> distribution(vo.at(i).at(1));
        vo[i][1] = double(distribution(generator));
    }

    return vo;
}

void SimulateScans::updateTotalInt()
{
    int n = ui->listWidgetPhases->count();
    ui->tableWidgetCompositions->setColumnCount(n + 1);

    ui->tableWidgetCompositions->setHorizontalHeaderItem(n, new QTableWidgetItem("Total"));

    for (int ro = 0; ro < ui->tableWidgetCompositions->rowCount(); ++ro) {
        double sum = 0.0;

        for (int co = 0; co < ui->tableWidgetCompositions->columnCount() - 1; ++co) {
            QTableWidgetItem *it = ui->tableWidgetCompositions->item(ro, co);

            double val = 0.0;

            if (it) {
                val = it->text().toDouble();
            }

            sum += val;
        }

        QTableWidgetItem *it = new QTableWidgetItem(QString("%1").arg(sum, 0, 'f', 2));
        it->setFlags(it->flags() & ~Qt::ItemIsEditable);

        if (qFuzzyCompare(sum, 100.0)) it->setBackground(QColor::fromRgb(200,255,200));
        else                           it->setBackground(QColor::fromRgb(255,200,200));

        ui->tableWidgetCompositions->setItem(ro, n, it);
    }
}

void SimulateScans::addPhase()
{
    QString name = QInputDialog::getText(this, tr("Phase name"), tr("Enter a name for the phase"));
    if (name.isEmpty()) return;

    QStringList refScans(settings->value(QString("simulateScans/mac/%1").arg(name), "0.00000").toString());

    refScans.append(getScanFileNames());

    bool oldStateP = ui->listWidgetPhases->blockSignals(true);
    bool oldStateM = ui->doubleSpinBoxMAC->blockSignals(true);

    QListWidgetItem *it = new QListWidgetItem(name, ui->listWidgetPhases);

    ui->listWidgetPhases->addItem(it);
    ui->listWidgetPhases->setCurrentItem(it);
    ui->listWidgetPhases->currentItem()->setData(Qt::UserRole, refScans);

    ui->doubleSpinBoxMAC->setValue(refScans.first().toDouble());

    ui->toolButtonPhaseRemove->setEnabled(true);
    ui->toolButtonScanAdd->setEnabled(true);
    ui->toolButtonSimulate->setEnabled(true);

    updateRefScans(refScans);

    ui->listWidgetPhases->blockSignals(oldStateP);
    ui->doubleSpinBoxMAC->blockSignals(oldStateM);

    addTableRow(name);
}

void SimulateScans::removePhase()
{
    ui->listWidgetScanFiles->clear();
    ui->toolButtonScanRemove->setEnabled(false);

    QString name;
    QListWidgetItem *it = ui->listWidgetPhases->takeItem(ui->listWidgetPhases->currentRow());

    if (it) {
        name = it->text();
        delete it;
    }

    if (!ui->listWidgetPhases->count()) {
        ui->toolButtonScanAdd->setEnabled(false);
        ui->toolButtonPhaseRemove->setEnabled(false);
        ui->toolButtonScanRemove->setEnabled(false);
        ui->toolButtonSimulate->setEnabled(false);
    }

    removeTableRow(name);
}

void SimulateScans::addScan()
{
    QListWidgetItem *it = ui->listWidgetPhases->currentItem();
    if (!it) return;

    QStringList refScans(getScanFileNames());
    if (refScans.isEmpty()) return;

    QStringList data(it->data(Qt::UserRole).toStringList());
    data.append(refScans);
    it->setData(Qt::UserRole, data);
    updateRefScans(data);
}

void SimulateScans::removeScan()
{
    QListWidgetItem *its = ui->listWidgetScanFiles->currentItem();
    QListWidgetItem *itp = ui->listWidgetPhases->currentItem();

    if (!its) {
        qDebug() << QString("*** no scan item found");
        return;
    }

    if (!itp) {
        qDebug() << QString("*** no phase item found");
        return;
    }

    QStringList l = itp->data(Qt::UserRole).toStringList();

    int i = ui->listWidgetScanFiles->row(its);

    if (i < l.size() - 1) {
        l.removeAt(i + 1);
        ui->listWidgetPhases->currentItem()->setData(Qt::UserRole, l);
        updateRefScans(l);
    }
}

void SimulateScans::addTableRow(const QString &name)
{
    int n = ui->listWidgetPhases->count();
    int m = ui->tableWidgetCompositions->rowCount();

    if ((m == 0) && (defaultQuant.size())) {
        m = defaultQuant.first().size();
    }

    ui->tableWidgetCompositions->setColumnCount(n);
    ui->tableWidgetCompositions->setRowCount(m);
    ui->tableWidgetCompositions->setHorizontalHeaderItem(n - 1, new QTableWidgetItem(QString("%1 [wt-%]").arg(name)));

    for (int i = 0; i < m; ++i) {
        QTableWidgetItem *it = new QTableWidgetItem(getDefaultCellText(i, n - 1));
        ui->tableWidgetCompositions->setItem(i, n - 1, it);
    }

    updateTotalInt();
}

void SimulateScans::removeTableRow(const QString &name)
{
    for (int i = 0; i < ui->tableWidgetCompositions->columnCount() - 2; ++i) {
        QTableWidgetItem *it = ui->tableWidgetCompositions->horizontalHeaderItem(i);

        if (it->text() == name) {
            ui->tableWidgetCompositions->removeColumn(i);
            break;
        }
    }

    updateTotalInt();
}

QString SimulateScans::getDefaultCellText(int r, int c)
{
    QString s("0.00");
    if (c >= defaultQuant.count()) return s;
    if (r >= defaultQuant.at(c).count()) return s;
    return defaultQuant.at(c).at(r);
}

void SimulateScans::macChanged(double d)
{
    Q_UNUSED(d);
    QListWidgetItem *it = ui->listWidgetPhases->currentItem();
    if (!it) return;

    QStringList data = it->data(Qt::UserRole).toStringList();

    if (data.size()) {
        data[0] = macToString();
    } else {
        data = QStringList(macToString());
    }

    it->setData(Qt::UserRole, data);
    settings->setValue(QString("simulateScans/mac/%1").arg(it->text()), QVariant(macToString()));
}

bool SimulateScans::checkMacs()
{
    int e = 0;

    for (int i = 0; i < ui->listWidgetPhases->count(); ++i) {
        QStringList d = ui->listWidgetPhases->item(i)->data(Qt::UserRole).toStringList();

        if (d.isEmpty()) {
            e++;
        } else {
            if (d.first().toDouble() <= 0.0) {
                e++;
            }
        }
    }

    return e == 0 ? true : false;
}

void SimulateScans::currentPhaseChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(previous);
    if (!current) {
        ui->doubleSpinBoxMAC->setValue(0.0);
        return;
    }

    QStringList l = current->data(Qt::UserRole).toStringList();

    updateRefScans(l);

    if (l.size()) {
        ui->doubleSpinBoxMAC->setValue(l.first().toDouble());
    } else {
        ui->doubleSpinBoxMAC->setValue(0.0);
    }
}

void SimulateScans::currentScanChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    Q_UNUSED(current);
    Q_UNUSED(previous);
    /* nothing to do */
}

QString SimulateScans::macToString()
{
    return QString("%1").arg(ui->doubleSpinBoxMAC->value(), 0, 'f', 6);
}

void SimulateScans::updateRefScans(const QStringList &l)
{
    ui->listWidgetScanFiles->clear();

    if (l.size() <= 1) {
        ui->toolButtonScanRemove->setEnabled(false);
        return;
    }

    for (int i = 1; i < l.size(); ++i) {
        QFileInfo fi(l.at(i));
        ui->listWidgetScanFiles->addItem(fi.fileName());
    }

    ui->toolButtonScanRemove->setEnabled(true);
}

QStringList SimulateScans::getScanFileNames()
{
    return QFileDialog::getOpenFileNames(this, tr("Select reference scans"),
                                  settings->value("config/workingdir", QDir::homePath()).toString(),
                                  tr("Reference Scans (*.xy *.XY);;All files (*.*)"));
}

void SimulateScans::phaseQuantityChanged(QTableWidgetItem *it)
{
    // if *it is a cell in the "total" row, do nothing
    if (it->column() == ui->tableWidgetCompositions->columnCount() - 1) return;

    // make sure the text is of format d.dddd
    bool oldState = ui->tableWidgetCompositions->blockSignals(true);

    double d = it->text().toDouble();
    it->setText(QString("%1").arg(d, 0, 'f', 2));

    ui->tableWidgetCompositions->blockSignals(oldState);

    // update the "total" row
    updateTotalInt();
}

void SimulateScans::disableGUI(bool b)
{
    ui->groupBoxPhases->setEnabled(!b);


    ui->doubleSpinBoxScale->setEnabled(!b);
    ui->checkBoxNoise->setEnabled(!b);
    ui->spinBoxSimulations->setEnabled(!b);

    ui->pushButtonClose->setEnabled(!b);
}

void SimulateScans::logMessage(const QString &s)
{
    qDebug() << s;
    ui->textEditLog->appendPlainText(s);
}

void SimulateScans::numberOfCompositionsChanged(int n)
{
    ui->tableWidgetCompositions->setRowCount(n);
}
