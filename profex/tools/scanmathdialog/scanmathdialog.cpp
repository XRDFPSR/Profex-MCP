/***************************************************************************
                          scanmathdialog.cpp  -  description
                             -------------------
    begin                : Wed Sep 27 15:10:00 CEST 2017
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

#include "scanmathdialog.h"
#include "ui_scanmathdialog.h"
#include "../libXrdIO/scan.h"
#include <QJSEngine>
#include <QVector>
#include <QColor>
#include <QMenu>
#include <QMessageBox>
#include <QDebug>

ScanMathDialog::ScanMathDialog(QWidget *parent) :
    AbstractToolDialog(parent),
    ui(new Ui::ScanMathDialog)
{
    ui->setupUi(this);
    setWindowTitle(tr("Scan Math Operations"));

    helpDlg   = nullptr;
    uid2tt = QUuid::createUuid();
    uidd   = QUuid::createUuid();

    ui->treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(ui->treeWidget,
            SIGNAL(itemActivated(QTreeWidgetItem*,int)),
            this,
            SLOT(scanSelected(QTreeWidgetItem*, int)));

    connect(ui->treeWidget,
            SIGNAL(customContextMenuRequested(QPoint)),
            this,
            SLOT(scanListContextMenu(QPoint)));

    connect(ui->treeWidget,
            SIGNAL(itemClicked(QTreeWidgetItem*,int)),
            this,
            SLOT(scanClicked(QTreeWidgetItem*,int)));

    restoreGeometry(settings->value("scanMathDialog/geometry", QByteArray()).toByteArray());
    ui->treeWidget->header()->restoreState(settings->value("scanMathDialog/scanListState", QByteArray()).toByteArray());
}

ScanMathDialog::~ScanMathDialog()
{
    settings->setValue("scanMathDialog/geometry", saveGeometry());
    settings->setValue("scanMathDialog/scanListState", ui->treeWidget->header()->saveState());
    if (helpDlg) delete helpDlg;
    delete ui;
}

void ScanMathDialog::postSetProject(ProjectWidget *)
{
    parseScans();
}

void ScanMathDialog::clearGui()
{
    ui->treeWidget->clear();
}

void ScanMathDialog::updateView()
{
    if (!isVisible()) return;
    parseScans();
}

void ScanMathDialog::parseScans()
{
    ui->treeWidget->clear();
    if (!checkBackend()) return;

    QVector<const Scan *> xyScans = graphControl->xyScans();

    for (int i = 0; i < xyScans.count(); ++i) {
        if (xyScans.at(i)->isTemporary()) continue;

        QStringList scanLabels;
        scanLabels << QString("#%1").arg(i+1) << graphControl->scanName(xyScans.at(i));

        QTreeWidgetItem *twi = new QTreeWidgetItem(scanLabels);
        twi->setData(0, Qt::UserRole, xyScans.at(i)->uid());
        twi->setCheckState(2, i == 0 ? Qt::Checked : Qt::Unchecked);
        ui->treeWidget->addTopLevelItem(twi);
    }

    QStringList ttLabels("#t");
    ttLabels << QString(tr("2%1 angle in degrees").arg(global::theta)) << QString();
    QTreeWidgetItem *twtt = new QTreeWidgetItem(ttLabels);
    twtt->setData(0, Qt::UserRole, uid2tt);
    ui->treeWidget->addTopLevelItem(twtt);

    QStringList dLabels("#d");
    dLabels << QString(tr("d value in %1").arg(global::angstrom)) <<  QString();
    QTreeWidgetItem *twd = new QTreeWidgetItem(dLabels);
    twd->setData(0, Qt::UserRole, uidd);
    ui->treeWidget->addTopLevelItem(twd);
}

void ScanMathDialog::scanSelected(QTreeWidgetItem *current, int)
{
    QString string = ui->lineEdit->text();
    string += current->text(0);
    ui->lineEdit->setText(string.simplified());
    ui->lineEdit->setFocus();
}

void ScanMathDialog::compute()
{
    qDebug() << QString("ScanMathDialog::compute(): Invoked");

    if (ui->lineEdit->text().isEmpty()) {
        qDebug() << QString("ScanMathDialog::compute(): Expression is empty. Exiting.");
        return;
    }

    if (!checkBackend()) {
        qDebug() << QString("ScanMathDialog::compute(): No data model available. Exiting.");
        return;
    }

    if (!graphControl->hasData()) {
        qDebug() << QString("ScanMathDialog::compute(): Data model is empty. Exiting.");
        return;
    }

    if (!ui->treeWidget->topLevelItemCount()) {
        qDebug() << QString("ScanMathDialog::compute(): No scans in treewidget. Exiting.");
        return;
    }

    // prepare a 2D vector with the following structure:
    // vector(
    //   vector<double>: intensities of scan 1
    //   vector<double>: intensities of scan 2
    //   ...
    //   vector<double>: intensities of scan n
    //   vector<double>: 2theta values of anchor scan
    //   vector<double>: d values of anchor scan
    // )
    //
    // all intensities will be quantized to the anchor scan's angle values
    QVector<double> anchorAngle = getAnchorScan()->pDataAngle();
    std::sort(anchorAngle.begin(), anchorAngle.end());

    int n = ui->treeWidget->topLevelItemCount();
    QVector< QVector<double> > intensities(n, QVector<double>(anchorAngle.size(), 0.0));

    double wl = graphControl->first()->waveLength();
    if (qFuzzyIsNull(wl)) wl = settings->defaultWavelength();

    QStringList args;

    for (int i = 0; i < n - 2; ++i) {
        QUuid uid = ui->treeWidget->topLevelItem(i)->data(0, Qt::UserRole).toUuid();
        const Scan *scan = graphControl->getScan(uid);

        if (scan) {
            qDebug() << QString("ScanMathDialog::compute(): Scan is valid %1").arg(graphControl->scanName(scan));
            QVector<double> quantInt = intensityQuantized(anchorAngle, scan->dataMap(true));
            if (quantInt.size()) {
                intensities[i] = quantInt;
                args << QString("scan%1").arg(i + 1);
            }
        } else {
            qDebug() << QString("ScanMathDialog::compute(): Invalid scan: %1").arg(uid.toString());
        }
    }

    // append one vector with 2theta angles, to be addressed as #t
    intensities[intensities.size() - 2] = anchorAngle;
    args << QString("scant");

    // append one vector with d values, to be addressed as #d
    intensities[intensities.size() - 1] = dvalues(anchorAngle, wl);
    args << QString("scand");

    QString lstring = ui->lineEdit->text().replace(QString("#"), "scan");
    QString fstring = QString("(function(%1) {return %2;})").arg(args.join(", "), lstring);
    QJSEngine engine;
    QJSValue  function = engine.evaluate(fstring);

    if (function.isError()) {
        qDebug() << QString("ScanMathDialog::compute(): Parsing \"%1\" failed.").arg(fstring);
        qDebug() << QString("                           Error message: %1").arg(function.toString());

        QMessageBox::warning(this, tr("Error parsing expression"), QString(tr("Could not parse the expression:\n%1")).arg(function.toString()));
        return;
    }

    qDebug() << QString("ScanMathDialog::compute(): Parsing \"%1\"").arg(fstring);

    QVector<double> resultsIntensities(anchorAngle.size(), 0.0);

    for (int i = 0; i < anchorAngle.size(); ++i) {
        QJSValueList values;

        for (int j = 0; j < intensities.size(); ++j) {
            values << intensities[j][i];
        }

        QJSValue result = function.call(values);
        resultsIntensities[i] = result.isNumber() ? result.toNumber() : 0.0;
    }

    Scan outputScan(QString("I = %1").arg(ui->lineEdit->text()));
    outputScan.setDataAng(anchorAngle);
    outputScan.setDataInt(resultsIntensities);
    outputScan.setTypes(Scan::XY | Scan::SYNTHETIC);

    graphControl->appendScan(outputScan, true);
    parseScans();
}

void ScanMathDialog::scanListContextMenu(QPoint p)
{
    QString str = ui->treeWidget->currentItem()->text(0);
    QString lbl = ui->treeWidget->currentItem()->text(1);

    QMenu slContextMenu(lbl, this);
    slContextMenu.addAction(QString("+%1").arg(str), this, SLOT(addPlus()));
    slContextMenu.addAction(QString("-%1").arg(str), this, SLOT(addMinus()));
    slContextMenu.addAction(QString("*%1").arg(str), this, SLOT(addMultiply()));
    slContextMenu.addAction(QString("/%1").arg(str), this, SLOT(addDivide()));
    slContextMenu.addAction(QString("pow(%1, 2)").arg(str), this, SLOT(addPow()));
    slContextMenu.addAction(QString("sqrt(%1)").arg(str), this, SLOT(addSqrt()));
    slContextMenu.addAction(QString("abs(%1)").arg(str), this, SLOT(addAbs()));
    slContextMenu.exec(ui->treeWidget->mapToGlobal(p));
}

void ScanMathDialog::addPlus()
{
    QString s(ui->lineEdit->text());
    QString a(ui->treeWidget->currentItem()->text(0));
    QString b(QString("%1+%2").arg(s, a));
    ui->lineEdit->setText(b.simplified());
}

void ScanMathDialog::addMinus()
{
    QString s(ui->lineEdit->text());
    QString a(ui->treeWidget->currentItem()->text(0));
    QString b(QString("%1-%2").arg(s, a));
    ui->lineEdit->setText(b.simplified());
}

void ScanMathDialog::addMultiply()
{
    QString s(ui->lineEdit->text());
    QString a(ui->treeWidget->currentItem()->text(0));
    QString b(QString("%1*%2").arg(s, a));
    ui->lineEdit->setText(b.simplified());
}

void ScanMathDialog::addDivide()
{
    QString s(ui->lineEdit->text());
    QString a(ui->treeWidget->currentItem()->text(0));
    QString b(QString("%1/%2").arg(s, a));
    ui->lineEdit->setText(b.simplified());
}

void ScanMathDialog::addPow()
{
    QString s(ui->lineEdit->text());
    QString a(ui->treeWidget->currentItem()->text(0));
    QString b(QString("%1 Math.pow(%2, 2)").arg(s, a));
    ui->lineEdit->setText(b.simplified());
}

void ScanMathDialog::addSqrt()
{
    QString s(ui->lineEdit->text());
    QString a(ui->treeWidget->currentItem()->text(0));
    QString b(QString("%1 Math.sqrt(%2)").arg(s, a));
    ui->lineEdit->setText(b.simplified());
}

void ScanMathDialog::addAbs()
{
    QString s(ui->lineEdit->text());
    QString a(ui->treeWidget->currentItem()->text(0));
    QString b(QString("%1 Math.abs(%2)").arg(s, a));
    ui->lineEdit->setText(b.simplified());
}

/*
 * we only check if the current item's check state in col 2 was changed.
 */
void ScanMathDialog::scanClicked(QTreeWidgetItem *current, int col)
{
    if (col != 2) return;

    int n = ui->treeWidget->indexOfTopLevelItem(current);

    if (n >= ui->treeWidget->topLevelItemCount() - 2) return;

    for (int i = 0; i < ui->treeWidget->topLevelItemCount() - 2; ++i) {
        if (i == n) {
            ui->treeWidget->topLevelItem(i)->setCheckState(2, Qt::Checked);
        } else {
            ui->treeWidget->topLevelItem(i)->setCheckState(2, Qt::Unchecked);
        }
    }
}

const Scan * ScanMathDialog::getAnchorScan()
{
    int n = 0;

    for (int i = 0; i < ui->treeWidget->topLevelItemCount() - 2; ++i) {
        if (ui->treeWidget->topLevelItem(i)->checkState(2) == Qt::Checked) {
            n = i;
            break;
        }
    }

    const Scan *ascan = graphControl->getScan(ui->treeWidget->topLevelItem(n)->data(0, Qt::UserRole).toUuid());

    if (ascan) return ascan;
    return graphControl->first();
}

void ScanMathDialog::showInfo()
{
    if (!helpDlg) helpDlg = new ScanMathHelpDialog(this);
    helpDlg->show();
}

QVector<double> ScanMathDialog::intensityQuantized(const QVector<double> &anchorAng, const QMap<double, double> &scan)
{
    QVector<double> intQuantized(anchorAng.size(), 0.0);
    if (!scan.size()) return intQuantized;

    for (int i = 0; i < anchorAng.size(); ++i) {
        if (anchorAng.at(i) < scan.firstKey()) continue;
        if (anchorAng.at(i) > scan.lastKey())  break;

        QMap<double, double>::const_iterator iterUp = scan.lowerBound(anchorAng.at(i));
        QMap<double, double>::const_iterator iterLow;

        if (iterUp != scan.constEnd()) {
            if (iterUp == scan.constBegin()) { // first value in scan
                intQuantized[i] = iterUp.value();
            } else {
                // the next two lines were: iterLow = iterUp - 1; which is deprecated
                iterLow = iterUp;
                --iterLow;

                double d = (anchorAng.at(i) - iterLow.key()) / (iterUp.key() - iterLow.key());
                intQuantized[i] = (1.0 - d) * iterLow.value() + d * iterUp.value();
            }
        }
    }

    return intQuantized;
}

QVector<double> ScanMathDialog::dvalues(const QVector<double> &tt, double wl)
{
    QVector<double> dval(tt.size(), 0.0);
    int idx = tt.size() - 1;

    for (int i = 0; i < tt.size(); ++i) {
         dval[idx - i] = wl / (2.0 * qSin(qDegreesToRadians(tt.at(i) / 2.0)));
    }

    return dval;
}
