/***************************************************************************
                          convergencedisplay.cpp  -  description
                             -------------------
    begin                : Wed Mar 02 20:00:00 CEST 2016
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

#include "convergencedisplay.h"
#include "../libXrdIO/structs.h"
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QColor>
#include <QVariant>
#include <QDebug>

ConvergenceDisplay::ConvergenceDisplay(QWidget *parent)
    : QCustomPlot(parent)
{
    settings = SettingsManager::getInstance();

    bgmnFirstValue = 0.0;
    yZoom = 100.0;
    bool showLegendDefault = settings->value("convergenceDisplay/showLegend", true).toBool();

    rxBgmnFirst.setPattern("^\\s*0\\s+(\\d\\.\\d{6}E[\\+-]\\d{2})\\s*$");
    rxBgmn.setPattern("^\\d+\\s+(\\d\\.\\d{6}E[\\+-]\\d{2})(?:\\s+\\d\\.\\d{3}E[\\+-]\\d{2}){2}\\s+\\d\\.\\d+\\s*$");
    rxBgmnP.setPattern("N=\\s*(\\d+)\\s+IT=\\s*\\d+");

    actionLegendVisible = new QAction(showLegendDefault ? tr("Hide Legend") : tr("Show Legend"), this);
    actionLegendVisible->setChecked(showLegendDefault);

    actionSaveAscii = new QAction(tr("Save Data"), this);
    actionSavePdf   = new QAction(tr("Save PDF"), this);

    chi2accept = settings->value("convergenceDisplay/chi2acceptance", 1.5).toDouble();
    lineWidth = settings->value("graph/lineWidth", 1).toInt();

    connect(actionLegendVisible, SIGNAL(triggered(bool)), this, SLOT(toggleLegend()));
    connect(actionSaveAscii, SIGNAL(triggered(bool)), this, SLOT(exportAscii()));
    connect(actionSavePdf,   SIGNAL(triggered(bool)), this, SLOT(exportPdf()));

    bool darkMode = settings->isDarkMode();
    QColor bgCol(darkMode ? QGuiApplication::palette().color(QPalette::Base) : Qt::white);

    this->setCursor(Qt::ArrowCursor);

    this->xAxis->setLabel("Iterations");
    this->xAxis->setRange(0.0, 1.0);
    this->yAxis->setRange(0.0, 100.0);

    this->xAxis->setTickLabels(false);
    this->xAxis->grid()->setVisible(false);
    this->yAxis->grid()->setVisible(false);

    this->legend->setVisible(showLegendDefault);

    QPen penRwp(darkMode ? global::Functions::colorToDarkMode(Qt::red) : Qt::red);
    QPen penRexp(darkMode ? global::Functions::colorToDarkMode(Qt::blue) : Qt::blue);
    QPen penLimit(darkMode ? global::Functions::colorToDarkMode(QColor(127, 127, 255)) : QColor(127, 127, 255));
    QPen penAxes(darkMode ? global::Functions::colorToDarkMode(Qt::black) : Qt::black);

    this->setBackground(QBrush(bgCol));
    this->legend->setBrush(bgCol);
    this->legend->setTextColor(penAxes.color());
    this->legend->setBorderPen(penAxes);

    penRwp.setWidth(lineWidth);
    penRexp.setWidth(lineWidth);
    penLimit.setWidth(lineWidth);
    penAxes.setWidth(lineWidth);

    this->xAxis->setBasePen(penAxes);
    this->yAxis->setBasePen(penAxes);

    this->xAxis->setTickPen(penAxes);
    this->yAxis->setTickPen(penAxes);

    this->xAxis->setSubTickPen(penAxes);
    this->yAxis->setSubTickPen(penAxes);

    this->xAxis->setLabelColor(darkMode ? Qt::white : Qt::black);
    this->yAxis->setLabelColor(darkMode ? Qt::white : Qt::black);
    this->xAxis->setTickLabelColor(darkMode ? Qt::white : Qt::black);
    this->yAxis->setTickLabelColor(darkMode ? Qt::white : Qt::black);

    this->addGraph();
    this->addGraph();
    this->addGraph();

    this->graph(0)->setName("Rwp");
    this->graph(1)->setName("Rexp");
    this->graph(2)->setName(QString("Acceptance Limit (%1%2 = %3)").arg(global::chi).arg(global::superTwo).arg(chi2accept));

    this->graph(0)->setPen(penRwp);
    this->graph(1)->setPen(penRexp);
    this->graph(2)->setPen(penLimit);
    this->graph(2)->setBrush(QBrush(QColor(127, 127, 255, 20)));
}

ConvergenceDisplay::~ConvergenceDisplay()
{
    delete actionLegendVisible;
    delete actionSaveAscii;
    delete actionSavePdf;
}

void ConvergenceDisplay::clear()
{
    bgmnFirstValue = 0.0;
    yZoom = 100.0;

    rXdata.clear();
    rwpYdata.clear();
    rexpYdata.clear();
    chi2Ydata.clear();

    this->graph(0)->setData(rXdata, rwpYdata);
    this->graph(1)->setData(rXdata, rexpYdata);
    this->graph(2)->setData(rXdata, chi2Ydata);
    this->replot();
}

void ConvergenceDisplay::setValues(double _rwp, double _rex, double _first)
{
    if (qFuzzyIsNull(bgmnFirstValue)) {
        setBgmnFirstValue(_first);
    }

    double chi2 = sqrt(chi2accept) * _rex;

    rXdata.append(double(rXdata.size()));
    rwpYdata.append(_rwp);
    rexpYdata.append(_rex);
    chi2Ydata.append(chi2);

    this->graph(0)->setData(rXdata, rwpYdata);
    this->graph(0)->setName(QString("Rwp = %1 %").arg(_rwp, 0, 'f', 2));

    this->graph(1)->setData(rXdata, rexpYdata);
    this->graph(1)->setName(QString("Rexp = %1 %").arg(_rex, 0, 'f', 2));

    this->graph(2)->setData(rXdata, chi2Ydata);

    this->xAxis->setLabel(QString("%1 Iterations").arg(rXdata.size() - 1));

    if (settings->value("convergenceDisplay/zoom", false).toBool()) {
        this->yAxis->setRange(zoomedYmin(), zoomedYmax());
    } else {
        this->yAxis->setRange(0.0, yZoom);
    }

    this->xAxis->rescale(false);
    this->replot();
}

void ConvergenceDisplay::setBgmnFirstValue(double d)
{
    if (d > bgmnFirstValue) {
        bgmnFirstValue = d;

        rXdata.clear();
        rwpYdata.clear();

        rXdata.append(0.0);
        rwpYdata.append(100.0);

        chi2Ydata.append(rexpYdata.size() ? sqrt(1.50) * rexpYdata.first() : 0.0);
    }
}

void ConvergenceDisplay::setRexpDenom(int m, double d)
{
    if (!qFuzzyIsNull(d)) {
        rexpYdata.append(100.0 * sqrt(double(m)/d));
    }
}

void ConvergenceDisplay::exportAscii()
{
    QFile file(QFileDialog::getSaveFileName(this,
                                            QString(tr("Export data")),
                                            settings->value("config/workingdir", QDir::homePath()).toString(),
                                            QString("*.csv *.CSV")));

    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) return;

    QTextStream out(&file);
    out << dataToAscii();
}

void ConvergenceDisplay::exportPdf()
{
    QString file(QFileDialog::getSaveFileName(this,
                                            QString(tr("Export graph")),
                                            settings->value("config/workingdir", QDir::homePath()).toString(),
                                            QString("PDF File (*.pdf *.PDF)")));

    if (file.isEmpty()) return;

    qApp->setOverrideCursor(Qt::WaitCursor);
    this->savePdf(file);
    qApp->restoreOverrideCursor();

}

QString ConvergenceDisplay::dataToAscii()
{
    QString str("Iteration;Rwp;Rexp;Chi^2\n");
    int n = qMin(qMin(qMin(rwpYdata.size(), rexpYdata.size()), rXdata.size()), chi2Ydata.size());

    for (int i = 0; i < n; ++i) {
        str += QString("%1;%2;%3\n").arg(rXdata.at(i)).arg(rwpYdata.at(i)).arg(rexpYdata.at(i)).arg(chi2Ydata.at(i));
    }

    return str;
}

void ConvergenceDisplay::contextMenuEvent(QContextMenuEvent *event)
{
    QMenu *menu = new QMenu(this);

    menu->addAction(actionLegendVisible);
    menu->addAction(actionSaveAscii);
    menu->addAction(actionSavePdf);

    menu->exec(event->globalPos());
    delete menu;
    event->ignore();
}

void ConvergenceDisplay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() == Qt::LeftButton) {
        bool b = !settings->value("convergenceDisplay/zoom", false).toBool();
        settings->setValue("convergenceDisplay/zoom", b);

        if (b) {
            this->yAxis->setRange(zoomedYmin(), zoomedYmax());
            yZoom = 100.0;
        } else {
            this->yAxis->setRange(0.0, yZoom);
        }

        replot();
    }
}

double ConvergenceDisplay::zoomedYmax()
{
    double d = 100.0;

    if (rwpYdata.size() && rexpYdata.size() && chi2Ydata.size()) {
        d = qMax(qMax(rwpYdata.last(), rexpYdata.last()), chi2Ydata.last());
    }

    return d * 1.2 > 100.0 ? 100.0 : d * 1.2;
}

double ConvergenceDisplay::zoomedYmin()
{
    double d = 0.0;

    if (rwpYdata.size() && rexpYdata.size() && chi2Ydata.size()) {
        d = qMin(qMin(rwpYdata.last(), rexpYdata.last()), chi2Ydata.last());
    }

    return d * 0.9;
}

void ConvergenceDisplay::toggleLegend()
{
    bool b = !settings->value("convergenceDisplay/showLegend", true).toBool();
    settings->setValue("convergenceDisplay/showLegend", b);

    this->legend->setVisible(b);
    actionLegendVisible->setText(b ? tr("Hide Legend") : tr("Show Legend"));
    replot();
}

void ConvergenceDisplay::wheelEvent(QWheelEvent *event)
{
    if (settings->value("convergenceDisplay/zoom", false).toBool()) {
        return;
    }

    if (event->angleDelta().y() > 0) {
        if (yZoom > 1.0) {
            yZoom -= qSqrt(yZoom);
        }
    } else {
        // add at least 0.1, else the increment of e.g. sqrt(1.0001) is too small
        yZoom += qMax(0.1, qSqrt(yZoom));
        if (yZoom > 100.0) yZoom = 100.0;
    }

    this->yAxis->setRange(0.0, yZoom);
    replot();
}
