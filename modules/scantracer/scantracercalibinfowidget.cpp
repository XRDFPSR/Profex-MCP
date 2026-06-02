/***************************************************************************
                          scantracercalibinfowidget.cpp  -  description
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

#include "scantracercalibinfowidget.h"
#include <QPainter>

ScanTracerCalibInfoWidget::ScanTracerCalibInfoWidget(QWidget *parent) : QWidget(parent)
{
    _displayMode = 0;
    _strTitle = tr("Calibration hint");
    _strStart = tr("Click \"Start axis calibration\"");
    _strPt1 = tr("Click at the start of the axes");
    _strPt2 = tr("Click at the end of the x-axis");
    _strPt3 = tr("Click at the end of the y-axis");
    _strCpl1 = tr("Fine-tune pixel coordinates");
    _strCpl2 = tr("Then enter calibrated coordinates");
    _strNext = tr("All done. Proceed to next step");

    _arrBl = QString(QChar(0x2B0B));
    _arrBr = QString(QChar(0x2B0A));
    _arrTl = QString(QChar(0x2B09));

    QFontMetrics fm(font());
    _space = fm.horizontalAdvance("M");
    _vTitle = 2*_space + fm.ascent();
    _vHint = _vTitle + 2*fm.lineSpacing();
}

void ScanTracerCalibInfoWidget::setDisplayMode(int i)
{
    _displayMode = i;
    update();
}

void ScanTracerCalibInfoWidget::paintEvent(QPaintEvent *e)
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

    p.setPen(Qt::red);

    if (_displayMode > 0) p.drawEllipse(xyStart, _space/2, _space/2);
    if (_displayMode > 1) p.drawEllipse(xEnd, _space/2, _space/2);
    if (_displayMode > 2) p.drawEllipse(yEnd, _space/2, _space/2);

    if (_displayMode == 0) {
        p.drawText((width() - fm.horizontalAdvance(_strStart))/2, _vHint, _strStart);
    }

    if (_displayMode == 1) {
        p.drawText((width() - fm.horizontalAdvance(_strPt1))/2, _vHint, _strPt1);
        p.setFont(arrowFont);
        p.drawText(xyStart.x() + _space, xyStart.y() - _space, _arrBl);
        p.setFont(font());
    }

    if (_displayMode == 2) {
        p.drawText((width() - fm.horizontalAdvance(_strPt2))/2, _vHint, _strPt2);
        p.setFont(arrowFont);
        p.drawText(xEnd.x() - _space - fmArrow.horizontalAdvance(_arrBr), xEnd.y() - _space, _arrBr);
        p.setFont(font());
    }

    if (_displayMode == 3) {
        p.drawText((width() - fm.horizontalAdvance(_strPt3))/2, _vHint + 3*fm.lineSpacing(), _strPt3);
        p.setFont(arrowFont);
        p.drawText(yEnd.x() + _space, yEnd.y() + _space + fmArrow.horizontalAdvance(_arrTl), _arrTl);
        p.setFont(font());
    }

    if (_displayMode == 4) {
        p.drawText((width() - fm.horizontalAdvance(_strCpl1))/2, _vHint, _strCpl1);
        p.drawText((width() - fm.horizontalAdvance(_strCpl2))/2, _vHint + fm.lineSpacing(), _strCpl2);
    }

    if (_displayMode == 5) {
        p.drawText((width() - fm.horizontalAdvance(_strNext))/2, _vHint, _strNext);
    }
}
