/***************************************************************************
                          graphwindow.h  -  description
                             -------------------
    begin                : Die Jun 17 2003
    copyright            : (C) 2003 by Nicola Doebelin
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

#ifndef GRAPHWINDOW_H
#define GRAPHWINDOW_H

#include <QtCore>
#include <QMenu>
#include <QWidget>
#include <QRubberBand>
#include <QPainter>
#include <QPrinter>
#include <QMap>
#include <QVector>
#include <QLabel>
#include <math.h>

#include "abstractgraphview.h"
#include "graphwindowlegend.h"
#include "graphwindowkeyeventhandler.h"

#include "../../../libXrdIO/scan.h"
#include "../../../libXrdIO/hkl.h"
#include "../../../libXrdIO/import/importhandler.h"
#include "../../../libXrdIO/structs.h"
#include "../../../libXrdIO/functions.h"

/**
  *@author Nicola Doebelin
  */

struct tTip {
    QRect pos;
    QStringList str;
};

struct Tick {
    double value;  // true value
    double scaled; // scaled value
    int position;  // screen position

    Tick() : value(), scaled(), position() {}
    Tick(double v, double s, int p) : value(v), scaled(s), position(p) {}
};

enum Scale {YSCALELIN, YSCALESQRT, YSCALELOG10, YSCALELOGIT, XSCALETWOTHETA, XSCALED, XSCALEQ};
enum PeakPreviewMode {PPMNONE, PPMLIN, PPMPEAK};

class GraphWindow : public AbstractGraphView  {
   Q_OBJECT

    friend class GraphWindowKeyEventHandler;

public:
  GraphWindow(GraphDataController *c, QWidget *parent=nullptr);
  ~GraphWindow();

  void initSettings();
  bool setStatus(const global::RefinementStatus &) override;

  void changeGraphStyles();

  void moveLeft(double);
  void moveRight(double);
  void moveUp(double);
  void moveDown(double);
  void moveToStart();
  void moveToEnd();
  void zoomStepAngle(double);
  void zoomStepIntensity(double);
  inline double getTmax() const           {return _maxAngAbs;}
  inline double getTmin() const           {return _minAngAbs;}
  inline double getImax() const           {return _maxIntensAbs;}
  inline double getImin() const           {return _minIntensAbs;}
  inline double getWaveLength() const     {return waveLength[0];} //in Angstrom
  void getMaxRange(double &, double &, double &, double &);
  void getZoomRange(double &, double &, double &, double &);
  void setZoomRange(double, double, double, double, bool checkLimits = true);
  inline double getScanMaxIntensity() const {return scanMaxIntensity;}
  void print(QPrinter &, QPainter &);
  void setIntegralRanges(const QList<global::HighlightRegion> &);
  void setPeakFitRanges(const QList<global::HighlightRegion> &);
  inline void setAnchorPoints(const QVector<global::AnchorPoint> &a) {anchorPoints = a;}

  void setReferenceReflections(const Scan &);

  QString tip(const QPoint &) const;
  QString getXaxisUnit();
  inline bool isZoomed() const {return zoomed;}
  inline Scale getXScaling() const {return xScaling;}
  inline Scale getYScaling() const {return yScaling;}

  void renderBitmap(const QString &s, int, int);
  QPixmap renderBitmap(int, int, bool fillbg = false);

  void saveSvg(const QString &);
  QByteArray getSvg(double a); // h = w / a

  void normalizeHkl(Scan *);

  void yOffsetUp();
  void yOffsetDown();
  void xOffsetRight();
  void xOffsetLeft();
  void xyOffsetReset();
  void resetZoom();
  inline double angularCorrection(double tt) const {return global::Functions::angularCorrection(tt, eps1, eps2, eps3);}

  void setPeakPreviewMode(const PeakPreviewMode &, const QUuid &);
  inline PeakPreviewMode currentPeakPreviewMode() const {return peakPreviewMode;}
  void setRangeSelectMode(bool, const QUuid &);
  inline bool rangeSelectModeActive() const {return rangeSelectMode;}
  void overrideWaveLength(double, Scan::WavelengthMode);
  void setOverrideHklBaseLine(int);
  void setWavelengthMode(const Scan::WavelengthMode &);

  inline void getPreset(QDomDocument &) override {};
  inline void applyPreset(const QDomElement &) override {};

private:
  bool drawRubberBand, zoomed, antiAliasing,
       crossHair, middleMouseButtonDragging, noiseCursor, inspector,
       dragging, specLines,
       rangeSelectMode, fillActive;

  ImportHandler *ihandler;
  GraphWindowKeyEventHandler *keyHandler;
  QColor bgColor, activeColor, abortedColor, completedColor, idleColor, peakSelectColor, printingBgColor;
  QColor marginColor;
  QColor axisColor;
  QColor differenceColor;
  QList<QColor> colorTable;
  QList<uint> styleTable;
  QPixmap pBuffer;
  QRect canvas;
  int margin_t, margin_b, margin_l, margin_r, margin_o, margin_m, margin_i;
  double difOffset;
  int hklReferenceBase;
  int hklScanBase;
  unsigned int points;
  double _minAngDisp, _maxAngDisp, _minAngAbs, _maxAngAbs;
  QPoint pointMouseOperationStart;
  QRect plot, legendRect;
  QString completeFileName, fileName;
  double _minIntensDisp, _maxIntensDisp, _maxIntensAbs, _minIntensAbs;
  double pScale;
  bool drawLeg;
  bool recalcPlot;
  bool useBackgroundColors;
  bool drawDifferenceTickLabels;
  QRubberBand *rubberBand;
  int lineWidth;
  int symbolSize;
  int printingLineWidth;
  Scale xScaling, yScaling;
  double waveLength[3];
  Scan::WavelengthMode wavelengthMode;
  QFont fontTitle, fontAxis, fontTicks, fontLegend;
  QFont oFontTitle, oFontAxis, oFontTicks, oFontLegend;
  QMap<int, QMap<int, tTip> > tt;
  QVector<Hkl> strucRefls;
  QVector<Scan> graphHeap;
  Scan refStructure;
  QVector<global::AnchorPoint> anchorPoints;
  QMap<QString, QColor> hklColorList;
  double scanMaxIntensity;
  double prevStrucScaleFactor;
  double prevScanScaleFactor;
  bool hasOverrideWaveLength;
  QString geometry;
  QTimer updateCoordinateTimer;
  QString fileUid;
  bool useCountsPerSecond;
  bool showGridMajorX, showGridMinorX, showGridMajorY, showGridMinorY;
  int tickDensityX, tickDensityY, tickDensityD;
  double eps1, eps2, eps3;
  QPoint clickedPosition;
  QPoint firstDoubleClickPoint;
  PeakPreviewMode peakPreviewMode;
  QUuid rangeCallerUid;
  bool darkTheme;
  QRect leftMarginRect;
  QRect topMarginRect;
  QRect rightMarginRect;
  QRect bottomMarginRect;
  QMenu *contextMenuYaxis;
  QMenu *contextMenuXaxis;
  QMenu *contextMenuDataArea;
  QList<QList<global::HighlightRegion> > highlightRegions;
  QString _keyCommands;
  GraphWindowLegend *legend;
  int anchorDragged;
  QPointF anchorStartPosition;

  bool event(QEvent *) override;
  void paintEvent(QPaintEvent *) override;
  void resizeEvent(QResizeEvent *) override;
  void mousePressEvent(QMouseEvent *) override;
  void mouseReleaseEvent(QMouseEvent *) override;
  void mouseMoveEvent(QMouseEvent *) override;
  void wheelEvent(QWheelEvent *) override;
  void mouseDoubleClickEvent(QMouseEvent *) override;
  void keyPressEvent(QKeyEvent *) override;
  void keyReleaseEvent(QKeyEvent *) override;
  void changeCursor();

  void mouseButtonLeft(QMouseEvent *);
  void mouseButtonMiddle(QMouseEvent *);
  void mouseButtonRight(QMouseEvent *);

  void mouseButtonStartStrucScaling(QMouseEvent *);
  void mouseButtonStartRubberBand(QMouseEvent *);
  void mouseButtonStartDragging(QMouseEvent *);
  void mouseButtonRemoveAnchor(QMouseEvent *);

  void mouseButtonStopStruScaling(QMouseEvent *);
  void mouseButtonStopRubberBand(QMouseEvent *);
  void mouseButtonStopDragging(QMouseEvent *);

  void mouseMoveStrucScaling(QMouseEvent *);
  void mouseMoveRubberBand(QMouseEvent *);
  void mouseMoveDragging(QMouseEvent *);
  void mouseMoveAnchorPointDragging(QMouseEvent *);

  void calcLimits();
  double getVisibleMaxIntensity(const Scan *s = nullptr);
  double getVisibleMinIntensity(const Scan *s = nullptr);
  void renderSvg(QBuffer *, double a = 0.0);

  int getXcoord(double) const;
  int getYcoord(double) const;
  double getRealX(int) const;
  double getRealY(int, bool dif = false) const;

  void zoom(QPoint, QPoint);
  void zoomToPoint(QPoint, int, bool, bool);

  void initPlot(const QRect &);
  void calcMargins(const QPainter &, bool border);
  void updateTickDensity();

  void drawMargin(QPainter &);
  void drawStyledBox(QPainter &, const QRect &box, int wdt, const QColor &colOuter, const QColor &colInner);
  void drawWindow(QPainter &, bool replaceChars = false);

  void drawTicks(QPainter &);
  QList<int> drawTicksXaxis(QPainter &, int);
  QList<int> drawTicksYaxis(QPainter &, int);
  QList<int> drawTicksDiff(QPainter &, int);

  void drawLabels(QPainter &);
  QList<int> drawLabelsXaxis(QPainter &, const QFontMetrics &, int ,int);
  QList<int> drawLabelsYaxis(QPainter &, const QFontMetrics &, int ,int);
  QList<int> drawLabelsDiff(QPainter &, const QFontMetrics &, int ,int);

  QList<Tick> getTickPositionsX(Scale, int, int);
  QList<Tick> getTickPositionsY(Scale, int);
  QList<Tick> getTickPositionsD(Scale, int);

  void drawGridXaxis(QPainter &, const QList<int> &);
  void drawGridYaxis(QPainter &, const QList<int> &);

  void drawLegend(QPainter &);
  void drawHighlightedRegions(QPainter &);
  void drawPlainHightlightedRegion(QPainter &, double, double, global::HighlightRegion &);
  void drawBoxHightlightedRegion(QPainter &, double, double, global::HighlightRegion &);
  void drawStyledHighlightedRegion(QPainter &, double, double, global::HighlightRegion &);
  void drawHklTicks(QPainter &);
  void drawHklScans(QPainter &);
  void drawHklReferenceLines(QPainter &);
  void drawHklLines(QPainter &, const Scan *, int mode);
  void drawPlot(QPainter &, bool clipManually);
  void drawCrossHairCursor(QPainter &);
  void drawNoiseCursor(QPainter &);
  void drawSpecLineCursor(QPainter &);
  void drawInspector(QPainter &);
  void drawAnchorPoints(QPainter &);
  void drawPeakPreviews(QPainter &);
  void drawRange(QPainter &);
  void drawKeySequenceOverlay(QPainter &);

  QList<QList<int> > getHklScreenCoordinates(const Scan *, const Scan *, int mode);
  QPolygon getScanScreenPolygon(const Scan *, bool clipManually);
  QPolygon getBackgroundScreenPolygon(bool clipManually);
  QPolygon appendBackgroundPolygon(const QPolygon &, const QPolygon &);
  void broadcastCoordinates(const QPoint &);
  bool showToolTip(QEvent *);
  void setupNewScans();

  void regionSetBegin(int);
  void regionSetEnd(int);
  QPoint interpolate(const QPoint &, const QPoint &, int);
  void selectClickedScan(const QPoint &);
  int getNearestScanIndex(const QPoint &);
  const Scan * getNearestScan(const QPoint &);
  int getNearestAnchorIndex(const QPoint);

  inline double getLog10(double d) const {return d < 1.0 ? d < -1.0 ? -log10(-d) : 0.0 : log10(d);}
  inline double getSqrt(double d) const  {return d < 0.0 ? -sqrt(-d) : sqrt(d);}

  inline double getInvLog10(double d) const {return d < 0.0 ? -pow(10.0, -d) : pow(10.0, d);}
  inline double getInvSqrt(double d) const  {return d < 0.0 ? -d * d : d * d;}

  double yValueToScale(double) const;
  double yScaleToValue(double) const;

  void setXaxisScale(Scale, bool);
  void setYaxisScale(Scale, bool);

  QPoint getScreenCoordinates(double x, double y, const Scan *) const;

  void rightClickPlot(QMouseEvent *);
  void rightClickLeftMargin(QMouseEvent *);
  void rightClickBottomMargin(QMouseEvent *);
  void rightClickLegend(QMouseEvent *);

  void applyKeySequence(const QString &);
  void applyZoomAngleKeySequence(const QString &, const QString &);
  void applyZoomIntensityKeySequence(const QString &, const QString &);
  void applyCenterAngleKeySequence(const QString &, const QString &);
  void applyWidthAngleKeySequence(const QString &);

  QPainter::CompositionMode compositionModeLines() const;
  QPainter::CompositionMode compositionModeFill() const;

public slots:
  void updateView() override;
  void resetView() override;
  void setIdle();
  void setActive();
  void setComplete();
  void slotSetDrawLegend(bool);
  void forceUpdate();

private slots:
  void calcCoordinates(const QPoint &, double &x, double &y, double &d);
  void setXaxis2theta(bool upd = true);
  void setXaxisD(bool upd = true);
  void setXaxisQ(bool upd = true);
  void setYaxisLinear(bool upd = true);
  void setYaxisSqrt(bool upd = true);
  void setYaxisLog10(bool upd = true);
  void resetZoomX();
  void resetZoomY();
  void zoomFromZeroY(bool upd = true);
  void zoomFromMinY(bool upd = true);
  void togglePhaseVisibility();
  void copyPixmapToClipboard();

signals:
  void sigCoordinates( double, double, double );
  void sigDoubleClickA(double, double, double);
  void sigDoubleClickB(double, double, double);
  void sigIntegration(double, double);
  void sigClearIntegrals();
  void sigPeakPreviewPoints(QPointF, QPointF, PeakPreviewMode, QUuid, bool);
  void sigRangePoints(QPointF, QPointF, QUuid);
  void sigCursorMessage(QString, QUuid);
  void sigAnchorMoved(int, double, double);
  void sigAnchorRemove(int);
  void sigRangeDoubleClicked(int, int);
};

#endif
