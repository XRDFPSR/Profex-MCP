/***************************************************************************
                          graphtosvg.h  -  description
                             -------------------
    begin                : Tue Sep 04 19:42:15 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#ifndef GRAPHTOSVG_H
#define GRAPHTOSVG_H

#include "scan.h"
#include "settingsmanager.h"
#include "functions.h"

#include <QBuffer>
#include <QFont>
#include <QRect>

#if defined XRDIO
#define XRDIO_EXPORT Q_DECL_EXPORT
#else
#define XRDIO_EXPORT Q_DECL_IMPORT
#endif


enum Scale {YSCALELIN, YSCALESQRT, YSCALELOG10, YSCALELOGIT, XSCALETWOTHETA, XSCALED, XSCALEQ};

struct Tick {
    double value;  // true value
    double scaled; // scaled value
    int position;  // screen position

    Tick() : value(), scaled(), position() {}
    Tick(double v, double s, int p) : value(v), scaled(s), position(p) {}
};

class XRDIO_EXPORT GraphToSvg
{
public:
    GraphToSvg(const QVector<Scan> &_sheap, double _eps1, double _eps2, double _eps3);
    QByteArray getSvg(double ar);
    double getWavelength();

private:
    SettingsManager *settings;
    QVector<Scan> scanHeap;
    QFont fontTitle;
    QFont fontAxis;
    QFont fontTicks;
    QFont fontLegend;
    int margin_t;
    int margin_b;
    int margin_l;
    int margin_r;
    int margin_o;
    int margin_m;
    int margin_i;
    double minAng, maxAng, minIntens, maxIntens;
    double pScale;
    QRect canvas;
    QRect plot;
    Scale xScaling, yScaling;
    int lineWidth;
    int symbolSize;
    double wavelength;
    double difOffset;
    bool useCountsPerSecond;
    bool showGridMinorX;
    bool showGridMajorX;
    bool showGridMinorY;
    bool showGridMajorY;
    int tickDensityX;
    int tickDensityY;
    int tickDensityD;
    double eps1, eps2, eps3;
    bool drawDifferenceTickLabels;

    void renderSvg(QBuffer *, double a = 0.0);
    void calcMargins(const QPainter &, bool border);
    void initPlot(const QRect &rc);
    void drawWindow(QPainter &p, bool replaceChars);
    void drawPlot(QPainter &p);
    void drawHklTicks(QPainter &p);
    void drawTicks(QPainter &p);
    void drawLegend(QPainter &p);

    QList<int> drawTicksXaxis(QPainter &p, int tLength);
    QList<int> drawTicksYaxis(QPainter &p, int tLength);
    QList<int> drawTicksDiff(QPainter &p, int tLength);
    QList<Tick> getTickPositionsX(Scale xscale, int tDensity, int tType);
    QList<Tick> getTickPositionsY(Scale yscale, int tDensity);
    QList<Tick> getTickPositionsD(Scale yscale, int tDensity);

    void drawLabels(QPainter &p);
    QList<int> drawLabelsXaxis(QPainter &p, const QFontMetrics &fm, int tLength, int lSpacing);
    QList<int> drawLabelsYaxis(QPainter &p, const QFontMetrics &fm, int tLength, int lSpacing);
    QList<int> drawLabelsDiff(QPainter &p, const QFontMetrics &fm, int tLength, int lSpacing);

    QPolygon getScanScreenPolygon(const Scan &scan, bool clipManually);
    double yValueToScale(double d) const;
    void updateTickDensity();

    inline double getLog10(double d) const {return d < 1.0 ? d < -1.0 ? -log10(-d) : 0.0 : log10(d);}
    inline double getSqrt(double d) const  {return d < 0.0 ? -sqrt(-d) : sqrt(d);}
    inline double getInvLog10(double d) const {return d < 0.0 ? -pow(10.0, -d) : pow(10.0, d);}
    inline double getInvSqrt(double d) const  {return d < 0.0 ? -d * d : d * d;}

    int getXcoord(double i) const;
    int getYcoord(double i) const;
    double getRealX(int i) const;
    double getRealY(int i, bool) const;
    double yScaleToValue(double d) const;
    inline double angularCorrection(double tt) const {return global::Functions::angularCorrection(tt, eps1, eps2, eps3);}
    QPoint interpolate(const QPoint &a, const QPoint &b, int x);

    void updateScanColors(bool force);
    void updateScanStyles(bool force);
};

#endif // GRAPHTOSVG_H
