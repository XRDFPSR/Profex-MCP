/***************************************************************************
                          scantracerimagetracer.cpp  -  description
                             -------------------
    begin                : Thu May 13 14:50:00 CEST 2021
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

#include "scantracerimagetracer.h"
#include <limits>
#include <QDebug>

ScanTracerImageTracer::ScanTracerImageTracer(QObject *parent) : QObject(parent)
{
    _nPoints = 100;
    _thresR = 128;
    _thresG = 128;
    _thresB = 128;
    _scanCol = Qt::black;
    _bgCol = Qt::white;
    _scene = nullptr;
    _lineCenterMode = 1;
    _colorSensitivity = 4;
}

void ScanTracerImageTracer::setPixmap(const QPixmap &p)
{
    _image = p.toImage().convertToFormat(QImage::Format_RGB888);
}

/* calibration points in pixel coordinates */
void ScanTracerImageTracer::setCalibrationPoints(const QPointF &bl, const QPointF &br, const QPointF &tl)
{
    _xAxis = QLineF(bl, br);
    _yAxis = QLineF(bl, tl);
}

QPolygonF ScanTracerImageTracer::startTrace()
{
    qDebug() << QString("ScanTracerImageTracer::startTrace(): Invoked");
    if (!checkData()) return QPolygonF();

    if (!adjustBaseLineEndPoint()) {
        qDebug() << QString("  Could not find intersection of Y-axis with baseline. Exiting.");
        return QPolygonF();
    }

    qDebug() << QString("ScanTracerImageTracer::startTrace(): Rasterizing with %1 points").arg(_nPoints);

    if (_scene) _scene->clearTrace();
    setThreshold();

    QPolygonF rBl = rasterizedBaseLine(_nPoints);
    QPolygonF trace = runTrace(rBl);

    return trace;
}

void ScanTracerImageTracer::setThreshold()
{
    int minTh = 10 * (10 - _colorSensitivity);

    _thresR = qMax(minTh, qAbs(_bgCol.red()   - _scanCol.red())   / _colorSensitivity);
    _thresG = qMax(minTh, qAbs(_bgCol.green() - _scanCol.green()) / _colorSensitivity);
    _thresB = qMax(minTh, qAbs(_bgCol.blue()  - _scanCol.blue())  / _colorSensitivity);
}

bool ScanTracerImageTracer::checkData()
{
    if (_image.isNull()) {
        qDebug() << QString("ScanTracerImageTracer::checkData(): No pixmap available. Exiting.");
        return false;
    }

    if (_baseLine.count() < 2) {
        qDebug() << QString("ScanTracerImageTracer::checkData(): Baseline contains %1 points, "
                            "but at least 2 points are required. Exiting.").arg(_baseLine.count());
        return false;
    }

    if (_xAxis.isNull() || _yAxis.isNull()) {
        qDebug() << QString("ScanTracerImageTracer::checkData(): Invalid calibrations points. Could not define axes. Exiting.");
        return false;
    }

    return true;
}

bool ScanTracerImageTracer::adjustBaseLineEndPoint()
{
    int s = _baseLine.count();
    QPointF ctr(_xAxis.x2() + _yAxis.x2() - _yAxis.x1(), _xAxis.y2() + _yAxis.y2() - _yAxis.y1());
    QLineF firstBlSegment(_baseLine.at(0), _baseLine.at(1));
    QLineF lastBlSegment(_baseLine.at(s-2), _baseLine.at(s-1));
    QLineF leftYaxis = _yAxis;
    QLineF rightYaxis(_xAxis.p2(), ctr);
    qDebug() << QString("  Calibration point TR set to %1 %2").arg(ctr.x()).arg(ctr.y());

    QPointF iSecL;
    QPointF iSecR;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    if (leftYaxis.intersects(firstBlSegment, &iSecL) == QLineF::NoIntersection) return false;
    if (rightYaxis.intersects(lastBlSegment, &iSecR) == QLineF::NoIntersection) return false;
#else
    if (leftYaxis.intersect(firstBlSegment, &iSecL) == QLineF::NoIntersection) return false;
    if (rightYaxis.intersect(lastBlSegment, &iSecR) == QLineF::NoIntersection) return false;
#endif

    _baseLine[0] = iSecL;
    _baseLine[s-1] = iSecR;

    qDebug() << QString("  Left Y-axis intersects baseline at %1 %2").arg(iSecL.x()).arg(iSecL.y());
    qDebug() << QString("  Right Y-axis intersects baseline at %1 %2").arg(iSecR.x()).arg(iSecR.y());

    return true;
}

QPolygonF ScanTracerImageTracer::rasterizedBaseLine(int nPoints)
{
    double xStep = (qMax(_xAxis.x2(), _xAxis.x1()) - qMin(_xAxis.x1(), _xAxis.x2())) / double(nPoints);
    double yStep = (qMax(_xAxis.y2(), _xAxis.y1()) - qMin(_xAxis.y1(), _xAxis.y2())) / double(nPoints);
    QLineF vLine = _yAxis;

    QPolygonF rBl;

    while (vLine.x1() <= _xAxis.x2()) {
        int cSeg = getBaseLineSegment(vLine.x1());
        if (cSeg < 0) break;

        QLineF blSeg(_baseLine.at(cSeg), _baseLine.at(cSeg + 1));
        QPointF iSec;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        if (vLine.intersects(blSeg, &iSec) == QLineF::NoIntersection) break;
#else
        if (vLine.intersect(blSeg, &iSec) == QLineF::NoIntersection) break;
#endif
        rBl.append(iSec);

        vLine.translate(xStep, yStep);
    }

    return rBl;
}

int ScanTracerImageTracer::getBaseLineSegment(double x)
{
    if (_baseLine.size() < 2) return -1;

    for (int i = 1; i < _baseLine.size(); ++i) {
        if (qFuzzyCompare(x, _baseLine.at(i-1).x())) return i-1;
        if (qFuzzyCompare(x, _baseLine.at(i).x()))   return i-1; // end point still belongs to the previous segment

        if (_baseLine.at(i).x() > x) {
            return i-1;
        }
    }

    return -1;
}

QPolygonF ScanTracerImageTracer::runTrace(const QPolygonF &bl)
{
    QPolygonF trace;
    QLineF top(QPointF(0.0, 0.0), QPointF(double(_image.width()), 0.0));
    QPointF tPoint;

    for (int i = 0; i < bl.size(); ++i) {
        QLineF l = _yAxis.translated(bl.at(i) - _yAxis.p1());

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
        if (l.intersects(top, &tPoint) != QLineF::NoIntersection) {
#else
        if (l.intersect(top, &tPoint) != QLineF::NoIntersection) {
#endif
            l.setP2(tPoint);
            QPointF trPt = scanLine(l);
            if (!trPt.isNull()) {
                trace.append(trPt);
                if (_scene) _scene->addTraceMark(trPt);
            }
        }
    }

    return trace;
}

QPointF ScanTracerImageTracer::scanLine(const QLineF &l)
{
    int xStart = int(l.x1()+0.5);
    int xEnd   = int(l.x2()+0.5);
    int yStart = int(l.y1()+0.5);
    int yEnd   = int(l.y2()+0.5);

    QList<QPointF> pts;
    bool hasData = false;

    int minVal = std::numeric_limits<int>::max();

    for (int y = yStart; y > yEnd; --y) {
        // y-axis is invertet in pixel coordinates (0 at the top)
        double d = double(yStart - y) / double(yStart - yEnd);
        int x = int(0.5 + double(xStart) + d * double(xEnd - xStart));

        int vr = qAbs(qRed(_image.pixel(x, y))   - _scanCol.red());
        int vg = qAbs(qGreen(_image.pixel(x, y)) - _scanCol.green());
        int vb = qAbs(qBlue(_image.pixel(x, y))  - _scanCol.blue());
        int v = vr + vg + vb;

        if (vr > _thresR || vg > _thresG || vb > _thresB) {
            if (hasData) break;      // we are past the line, stop scanning
            else         continue;   // we haven't met the line yet, continue
        } else {
            hasData = true;          // we found a dark pixel
        }

        if (v == minVal) {
            pts.append(QPointF(x, y));
        } else if (v < minVal) {
            pts.clear();
            pts.append(QPointF(x, y));
            minVal = v;
        }
    }

    if (!pts.size()) return l.p1();

    if (_lineCenterMode == 0) return pts.last();
    if (_lineCenterMode == 2) return pts.first();

    double sx = 0.0;
    double sy = 0.0;

    for (int i = 0; i < pts.size(); ++i) {
        sx += pts.at(i).x();
        sy += pts.at(i).y();
    }

    return QPointF(sx / double(pts.size()), sy / double(pts.size()));
}
