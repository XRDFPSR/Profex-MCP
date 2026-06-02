/***************************************************************************
                          bgmndatadisplay.cpp  -  description
                             -------------------
    begin                : Wed May 23 21:10:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#include "bgmndatadisplay.h"

BgmnDataDisplay::BgmnDataDisplay(QWidget *parent) : QCustomPlot(parent)
{
    settings = SettingsManager::getInstance();
    this->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    this->setInteractions(QCP::iRangeDrag | QCP::iRangeZoom);
    this->xAxis->axisRect()->setRangeDrag(Qt::Horizontal);
    this->xAxis->axisRect()->setRangeZoom(Qt::Horizontal);

    this->legend->setVisible(true);
    this->yAxis->setLabel(tr("Intensity [a.u.]"));

    connect(this->xAxis, SIGNAL(rangeChanged(QCPRange)), this, SLOT(zoomX(QCPRange)));
    connect(this, SIGNAL(mousePress(QMouseEvent*)), this, SLOT(zoomReset(QMouseEvent*)));

    zoomed = false;
    currentCurve = 0;
    xmin = 0.0;
    xmax = 0.0;
    showSubCurves = false;
}

void BgmnDataDisplay::exportPdf(const QString &fn)
{
    qApp->setOverrideCursor(Qt::WaitCursor);
    this->savePdf(fn, 0, 0, QCP::epAllowCosmetic, "Profex", tr("BGMN Data Plot"));
    qApp->restoreOverrideCursor();
}

void BgmnDataDisplay::exportPng(const QString &fn, int w, int h)
{
    qApp->setOverrideCursor(Qt::WaitCursor);
    this->savePng(fn, w, h, 1.0, -1, 300);
    qApp->restoreOverrideCursor();
}

QString BgmnDataDisplay::exportCsv(int i)
{
    return dataToCsv(i);
}

/*
 * generates equidistant x-values
 */
QVector<double> BgmnDataDisplay::xValues(double min, double max, double step)
{
    int points = int(qAbs(max - min) / qAbs(step));
    QVector<double> x(points, 0.0);

    for (int i = 0; i < points; ++i) {
        x[i] = min + (double)i * step;
    }

    return x;
}

void BgmnDataDisplay::zoomX(const QCPRange &newRange)
{
    zoomed = true;
    double wmin = qMin(xmin, xmax);
    double wmax = qMax(xmin, xmax);

    QCPRange fixedRange(newRange);

    if (fixedRange.lower < wmin)
    {
        fixedRange.lower = wmin;
        fixedRange.upper = wmin + newRange.size();

        if (fixedRange.upper > wmax || qFuzzyCompare(newRange.size(), wmax-wmin)) {
            fixedRange.upper = wmax;
        }

        this->xAxis->setRange(fixedRange);
    } else if (fixedRange.upper > wmax)
    {
        fixedRange.upper = wmax;
        fixedRange.lower = wmax - newRange.size();

        if (fixedRange.lower < wmin || qFuzzyCompare(newRange.size(), wmax-wmin)) {
            fixedRange.lower = wmin;
        }

        this->xAxis->setRange(fixedRange);
    }

    this->replot();
}

void BgmnDataDisplay::zoomReset(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        zoomed = false;
        this->xAxis->setRange(qMin(xmin, xmax), qMax(xmin, xmax));
        this->replot();
    }
}

void BgmnDataDisplay::toggleSubCurves(bool b)
{
    showSubCurves = b;
    displayCurve(currentCurve);
}

QString BgmnDataDisplay::valuesToString(const QVector<QVector<double> > &data)
{
    QString out;
    QString sep = settings->value("config/asciiFieldSeparator", " ").toString();

    int maxIdx = 0;
    for (int i = 0; i < data.size(); ++i) {
        maxIdx = qMax(maxIdx, data.at(i).count());
    }

    for (int y = 0; y < maxIdx; ++y) {
        QStringList line;

        for (int x = 0; x < data.count(); ++x) {
            line.append(y >= data.at(x).count() ? "0.00000000" : QString("%1").arg(data.at(x).at(y), 0, 'f', 8));
        }

        out.append(line.join(sep));
        out.append("\n");
    }

    return out;
}

void BgmnDataDisplay::setTitle(const QString &s)
{
    if (this->plotLayout()->rowCount() < 2) {
        this->plotLayout()->insertRow(0);
        this->plotLayout()->addElement(0, 0, new QCPTextElement(this, s, QFont("sans", 12)));
    } else {
        static_cast<QCPTextElement *>(this->plotLayout()->elementAt(0))->setText(s);
    }
}

void BgmnDataDisplay::offsetXvalues(QVector<double> &l, double d)
{
    for (int i = 0; i < l.size(); ++i) {
        l[i] += d;
    }
}
