/***************************************************************************
                          scantracerbaselineinfowidget.cpp  -  description
                             -------------------
    begin                : Wed June 29 19:00:00 CEST 2021
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

#include "scantracerbaselineinfowidget.h"
#include <QPainter>

ScanTracerBaseLineInfoWidget::ScanTracerBaseLineInfoWidget(QWidget *parent) : QWidget(parent)
{
    _displayMode = 0;
    _strTitle = tr("Baseline hint");
    _strStart = tr("Click \"Draw baseline\"");
    _strPt1 = tr("Draw a baseline below the scan");
    _strPt2 = tr("Start and end outside of the");
    _strPt3 = tr("calibrated range");
    _strNext = tr("All done. Proceed to next step");

    _arrBl = QString(QChar(0x2B0B));
    _arrBr = QString(QChar(0x2B0A));
    _arrTl = QString(QChar(0x2B09));

    _dummyData[0.00] = 0.010313517000830;
    _dummyData[0.02] = 0.014153892025837;
    _dummyData[0.04] = 0.033058960325962;
    _dummyData[0.06] = 0.026954721346811;
    _dummyData[0.08] = 0.032713630319938;
    _dummyData[0.10] = 0.037942073212484;
    _dummyData[0.12] = 0.045952559076445;
    _dummyData[0.14] = 0.010556815140051;
    _dummyData[0.16] = 0.004972828758865;
    _dummyData[0.18] = 0.018831092670276;
    _dummyData[0.20] = 0.102969673388485;
    _dummyData[0.22] = 0.236107241962959;
    _dummyData[0.24] = 0.539306004649702;
    _dummyData[0.26] = 1.000000000000000;
    _dummyData[0.28] = 0.444553066337485;
    _dummyData[0.30] = 0.543613347039664;
    _dummyData[0.32] = 0.126794120545386;
    _dummyData[0.34] = 0.019107617204813;
    _dummyData[0.36] = 0.015544311724274;
    _dummyData[0.38] = 0.049894727042611;
    _dummyData[0.40] = 0.034375371129823;
    _dummyData[0.42] = 0.024583611281714;
    _dummyData[0.44] = 0.015647413006533;
    _dummyData[0.46] = 0.002834989624878;
    _dummyData[0.48] = 0.049301990882436;
    _dummyData[0.50] = 0.043530101464305;
    _dummyData[0.52] = 0.020413970067701;
    _dummyData[0.54] = 0.022467087914451;
    _dummyData[0.56] = 0.020526279445600;
    _dummyData[0.58] = 0.030230218107194;
    _dummyData[0.60] = 0.097154732771243;
    _dummyData[0.62] = 0.128516019433262;
    _dummyData[0.64] = 0.278519987670145;
    _dummyData[0.66] = 0.548219664421313;
    _dummyData[0.68] = 0.201083751461419;
    _dummyData[0.70] = 0.293014726327682;
    _dummyData[0.72] = 0.070338721099933;
    _dummyData[0.74] = 0.031591933888420;
    _dummyData[0.76] = 0.032726057364982;
    _dummyData[0.78] = 0.028316167977847;
    _dummyData[0.80] = 0.109700879790504;
    _dummyData[0.82] = 0.164397342965423;
    _dummyData[0.84] = 0.449215440728126;
    _dummyData[0.86] = 0.849511063504041;
    _dummyData[0.88] = 0.346263221141072;
    _dummyData[0.90] = 0.432144788434948;
    _dummyData[0.92] = 0.084890937276402;
    _dummyData[0.94] = 0.034052991296764;
    _dummyData[0.96] = 0.008976168941322;
    _dummyData[0.98] = 0.037867250526177;
    _dummyData[1.00] = 0.013695305470480;

    _dummyBl[-0.05] = 0.200;
    _dummyBl[0.30] = 0.150;
    _dummyBl[0.70] = 0.120;
    _dummyBl[1.05] = 0.120;

    QFontMetrics fm(font());
    _space = fm.horizontalAdvance("M");
    _vTitle = 2*_space + fm.ascent();
    _vHint = _vTitle + 2*fm.lineSpacing();
}

void ScanTracerBaseLineInfoWidget::setDisplayMode(int i)
{
    _displayMode = i;
    update();
}

void ScanTracerBaseLineInfoWidget::paintEvent(QPaintEvent *e)
{
    Q_UNUSED(e);
    QFont arrowFont = font();
    arrowFont.setPointSize(4 * font().pointSize());

    QFontMetrics fm(font());
    QFontMetrics fmArrow(arrowFont);

    QPoint xyStart(2*_space, height() - 2*_space);
    QPoint xEnd(width() - 2*_space, height() - 2*_space);
    QPoint yEnd(2*_space, 2*_space);

    QPainter p(this);
    p.fillRect(rect(), QBrush(Qt::white));

    p.setPen(Qt::black);
    p.drawLine(xyStart, xEnd);
    p.drawLine(xyStart, yEnd);
    p.drawText((width() - fm.horizontalAdvance(_strTitle))/2, _vTitle, _strTitle);

    QPolygonF ddata = dummyScanToScreenCoordinates(_dummyData);

    p.drawPolyline(ddata);
    p.setPen(Qt::blue);

    if (_displayMode > 0) {
        QPolygonF dbl = dummyBaseLineToScreenCoordinates(_dummyBl);
        p.drawPolyline(dbl);

        for (int i = 0; i < dbl.size(); ++i) {
            p.drawEllipse(dbl.at(i), _space/2, _space/2);
        }
    }

    if (_displayMode == 0) {
        p.drawText((width() - fm.horizontalAdvance(_strStart))/2, _vHint, _strStart);
    }

    if (_displayMode == 1) {
        p.drawText((width() - fm.horizontalAdvance(_strPt1))/2, _vHint, _strPt1);
        p.drawText((width() - fm.horizontalAdvance(_strPt2))/2, _vHint + fm.lineSpacing(), _strPt2);
        p.drawText((width() - fm.horizontalAdvance(_strPt3))/2, _vHint + 2*fm.lineSpacing(), _strPt3);
    }

    if (_displayMode == 2) {
        p.drawText((width() - fm.horizontalAdvance(_strNext))/2, _vHint, _strNext);
    }
}

QPolygonF ScanTracerBaseLineInfoWidget::dummyScanToScreenCoordinates(const QMap<double, double> &m)
{
    double mx = double(width() - 4 * _space);
    double my = double(height() - 4 * _space);

    QPolygonF ddata;
    QMapIterator<double, double> itData(m);

    while (itData.hasNext()) {
        itData.next();
        double dx = itData.key() * mx + 2*_space;
        double dy = height() - _space - (0.25 + itData.value() * 0.5) * my;
        ddata.append(QPointF(dx, dy));
    }

    return ddata;
}

QPolygonF ScanTracerBaseLineInfoWidget::dummyBaseLineToScreenCoordinates(const QMap<double, double> &m)
{
    double mx = double(width() - 4 * _space);
    double my = double(height() - 4 * _space);

    QPolygonF dbl;
    QMapIterator<double, double> itBl(m);

    while (itBl.hasNext()) {
        itBl.next();
        double dx = itBl.key() * mx + 2*_space;
        double dy = height() - _space - itBl.value() * my;

        if (itBl.key() < 0.0) dx = double(_space);
        else if (itBl.key() > 1.0) dx = double(width() - _space);
        dbl.append(QPointF(dx, dy));
    }

    return dbl;
}
