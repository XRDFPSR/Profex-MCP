/***************************************************************************
                          waterfallplotwidget.h  -  description
                             -------------------
    begin                : Tue Aug 03 12:57:00 CEST 2021
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

#ifndef WATERFALLPLOTWIDGET_H
#define WATERFALLPLOTWIDGET_H

#include <QFrame>
#include <QVector>
#include <QPainter>
#include <QMouseEvent>

#include "../../libXrdIO/scan.h"
#include "../../libXrdIO/colorMaps/lutgenerator.h"

struct WpScanMetrics {
    WpScanMetrics() : wmin(0.0), wmax(0.0), imin(0.0), imax(0.0), npoints(0) {}
    WpScanMetrics(double wmi, double wma, double imi, double ima, int np) : wmin(wmi), wmax(wma), imin(imi), imax(ima), npoints(np) {}
    double wmin;
    double wmax;
    double imin;
    double imax;
    int npoints;
};

class WaterfallPlotWidget : public QFrame
{
    Q_OBJECT
public:
    explicit WaterfallPlotWidget(QWidget *parent = nullptr);
    ~WaterfallPlotWidget();

    inline void setScanData(const QHash<QUuid, Scan> *s) {_scanHeap = s;}
    inline void setUidData(const QVector<QUuid> *v) {_uidList = v;}

    void updateScanData();
    void clearScans();
    void setLut(const colorMaps::Lut &);
    void setColorTemp(int);
    void setYscale(int);
    void setGamma(double);
    void setXrange(double, double);
    void setYrange(double, double);
    void getXrange(double &, double &);
    void getYrange(double &, double &);
    void getXlimits(double &, double &);
    void getYlimits(double &, double &);
    void resetXrange();
    void resetYrange();
    inline void blockUpdate(bool b) {_blockUpdate = b;}
    inline bool updatesBlocked() const {return _blockUpdate;}
    void save(const QString &, int, int);
    void highlightScans(const QList<QUuid> &);
    inline bool hasHighlights() const {return _hlIdx.size() > 0;}

public slots:
    void updateAll();
    void setSmoothInterpolation(bool);
    void setDrawGridHorizontal(bool);
    void setDrawGridVertical(bool);
    void setShowScanLabels(bool);

private:
    QImage _lineBuffer;
    QImage _scaledBuffer;
    bool _blockUpdate;
    int _nPoints;
    colorMaps::Lut _lut;
    int _smoothInterpol;
    bool _drawGridH;
    bool _drawGridV;
    bool _showScanLabels;
    double _gamma;

    QStringList _labels;
    QRect _imageRect;
    QRect _fileNameRect;
    QRect _xaxisRect;
    QMap<int, int> _angToPixelMap;
    QList<double> _majTicks;
    QList<double> _minTicks;
    QList<QUuid> _hlIdx;
    const QHash<QUuid, Scan> *_scanHeap;
    const QVector<QUuid> *_uidList;
    QHash<QUuid, WpScanMetrics> _metricsBuffer;

    double _clickPos;
    double _wmin;
    double _wmax;
    double _wminDisp;
    double _wmaxDisp;
    double _imin;
    double _imax;
    double _iminDisp;
    double _imaxDisp;
    double _wminDispClicked;
    double _wmaxDispClicked;

    double _iRangeMinLin;
    double _iRangeMaxLin;
    double _iRangeMinDisp;
    double _iRangeMaxDisp;

    int _yScale;
    int _lutOffset;
    int _lutSign;
    int _lutBins;
    int _colTemp;

    void mouseMoveEvent(QMouseEvent *);
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void wheelEvent(QWheelEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseReleaseEvent(QMouseEvent *);

    bool hasData();
    int angleToBufferIndex(double);
    double angleToBufferIndexF(double);
    double bufferIndexToAngle(int);
    int angleToIndex(double);
    double pixelToAngle(int);
    void calcLimits();
    void calcXtickMarks();
    void calcDisplayYlimits();
    void calcDrawRegions(const QPaintDevice *canvas, const QFontMetrics &fm);
    void updateAngToPixelMap();
    void renderImageBuffer();
    void renderLine(const Scan &, QImage &, int y);
    void scaleDisplayBuffer();
    QRgb pixelValueRgb(double);
    void setLutOffset();
    void zoomStep(int, int);

    void drawFileNames(QPainter &);
    void drawXaxis(QPainter &);
    void drawGrid(QPainter &);
    void drawHighlight(QPainter &);

    QImage bufferedPixmap(int, int);
    void renderPixmap(const QString &, int, int);
    void renderSvg(const QString &);
    void exportTextGrid(const QString &);

private slots:

signals:
    void sigMouseCoordinates(QString);
    void sigXrangeWasZoomed();

};

#endif // WATERFALLPLOTWIDGET_H
