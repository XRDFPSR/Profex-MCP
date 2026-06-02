/***************************************************************************
                          scantracerscene.h  -  description
                             -------------------
    begin                : Wed May 12 19:00:00 CEST 2021
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

#ifndef SCANTRACERSCENE_H
#define SCANTRACERSCENE_H

#include <QGraphicsScene>
#include <QObject>
#include <QPainterPath>
#include <QMap>
#include <QGraphicsSceneMouseEvent>

class ScanTracerScene : public QGraphicsScene
{
    Q_OBJECT

public:
    ScanTracerScene();
    void loadImage(const QString &);
    void setPixmap(const QPixmap &);

    void setCalibMode(bool);
    void setBaseLineMode(bool);
    inline void setPickScanColorMode(bool b) {_pickScanColorMode = b;}
    inline void setPickBackgroundColorMode(bool b) {_pickBackgroundColorMode = b;}

    inline bool calibMode() {return _calibMode;}
    inline bool baseLineMode() {return _baseLineMode;}
    inline bool pickScanColorMode() {return _pickScanColorMode;}
    inline bool pickBackgroundColorMode() {return _pickBackgroundColorMode;}

    bool hasPixmap();
    bool hasBaseLine();

    int imageWidth();
    int imageHeight();
    inline QPixmap pixmap() {return _pixmap;}
    void addPoint(const QPointF &);
    QPolygonF baseLinePolygon();
    void clearBaseLine();
    void clearTrace();
    void clearAll();

    void setBL(const QPoint &);
    void setBR(const QPoint &);
    void setTL(const QPoint &);

    void addTraceMark(const QPointF &);

private:
    void mousePressEvent(QGraphicsSceneMouseEvent *event);
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event);

    int _symbolSizeTrace;
    int _symbolSizePoints;
    QPixmap _pixmap;
    bool _calibMode;
    bool _baseLineMode;
    bool _pickScanColorMode;
    bool _pickBackgroundColorMode;
    QGraphicsPixmapItem* _pixmapItem;
    QGraphicsPathItem *_baseLine;
    QList<QGraphicsEllipseItem *> _calibMarks;
    QList<QGraphicsItem *> _traceMarks;
    QList<QGraphicsEllipseItem *> _baseLineMarks;
    QMap<double, QPointF> _baseLinePoints;

    QPolygonF pointMapToPolygon(const QMap<double, QPointF> &);

    void clearTraceMarks();
    void clearCalibMarks();
    void clearBaseLineMarks();
    void clearPixmap();
    void pickScanColor(const QPointF &);
    void pickBackgroundColor(const QPointF &);

signals:
    void sigMouseClickPosition(QPointF);
    void sigMouseCoordinates(QPointF);
    void sigScanColorPicked(QColor);
    void sigBackgroundColorPicked(QColor);
};

#endif // SCANTRACERSCENE_H
