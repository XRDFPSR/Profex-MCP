/***************************************************************************
                          scantracerimagetracer.h  -  description
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

#ifndef SCANTRACERIMAGETRACER_H
#define SCANTRACERIMAGETRACER_H

#include <QObject>
#include <QPixmap>
#include <QImage>
#include <QTransform>
#include "scantracerscene.h"

class ScanTracerImageTracer : public QObject
{
    Q_OBJECT
public:
    explicit ScanTracerImageTracer(QObject *parent = nullptr);

    void setPixmap(const QPixmap &);
    inline void setBaseLine(const QPolygonF &p) {_baseLine = p;}
    void setCalibrationPoints(const QPointF &, const QPointF &, const QPointF &);
    inline void setNPoints(int n) {_nPoints = n;}
    QPolygonF startTrace();
    inline void setScene(ScanTracerScene *s) {_scene = s;}
    inline void setScanColor(const QColor &c) {_scanCol = c;}
    inline void setBackgroundColor(const QColor &c) {_bgCol = c;}
    inline QColor getScanColor() {return _scanCol;}
    inline QColor getBackgroundColor() {return _bgCol;}
    inline void setLineCenterMode(int i) {_lineCenterMode = i;}
    inline int getLineCenterMode() {return _lineCenterMode;}
    inline void setColorSensitivity(int i) {_colorSensitivity = i;}
    inline int getColorSensitivity() {return _colorSensitivity;}

private:
    QImage _image;
    QPolygonF _baseLine;
    QLineF _xAxis;
    QLineF _yAxis;
    int _nPoints;
    ScanTracerScene *_scene;
    int _lineCenterMode; // 0 = top darkes pixel, 1 = center darkest pixel, 2 = bottom darkest pixel
    QColor _scanCol;
    QColor _bgCol;
    int _thresR;
    int _thresG;
    int _thresB;
    int _colorSensitivity;

    bool checkData();
    bool adjustBaseLineEndPoint();
    QPolygonF rasterizedBaseLine(int);
    QPolygonF runTrace(const QPolygonF &);
    QPointF scanLine(const QLineF &);
    int getBaseLineSegment(double);
    void setThreshold();

signals:

};

#endif // SCANTRACERIMAGETRACER_H
