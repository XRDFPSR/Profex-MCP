/***************************************************************************
                          scantracerscene.cpp  -  description
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

#include "scantracerscene.h"
#include <QGraphicsItem>
#include <QFontMetrics>
#include <QDebug>

ScanTracerScene::ScanTracerScene()
{
    _calibMode = false;
    _baseLineMode = false;
    _pixmapItem = nullptr;
    _baseLine = nullptr;
}

void ScanTracerScene::loadImage(const QString &s)
{
    clearAll();
    _pixmap = QPixmap(s);
    _pixmapItem = addPixmap(_pixmap);
    _baseLine = addPath(QPainterPath(), QPen(Qt::blue));
    _symbolSizeTrace = _pixmap.width() / 500;
    _symbolSizePoints = _pixmap.width() / 200;
    setSceneRect(itemsBoundingRect());
}

void ScanTracerScene::setPixmap(const QPixmap &px)
{
    clearAll();
    _pixmap = px;
    _pixmapItem = addPixmap(_pixmap);
    _baseLine = addPath(QPainterPath(), QPen(Qt::blue));
    _symbolSizeTrace = _pixmap.width() / 500;
    _symbolSizePoints = _pixmap.width() / 200;
    setSceneRect(itemsBoundingRect());
}

void ScanTracerScene::setCalibMode(bool b)
{
    _calibMode = b;
    if (b) clearCalibMarks();
}

void ScanTracerScene::setBaseLineMode(bool b)
{
    _baseLineMode = b;
    if (b) clearBaseLine();
}

void ScanTracerScene::mousePressEvent(QGraphicsSceneMouseEvent *event)
{
    if (_calibMode || _baseLineMode)   addPoint(event->scenePos());
    else if (_pickScanColorMode)       pickScanColor(event->scenePos());
    else if (_pickBackgroundColorMode) pickBackgroundColor(event->scenePos());
}

void ScanTracerScene::mouseMoveEvent(QGraphicsSceneMouseEvent *event)
{
    emit sigMouseCoordinates(event->scenePos());
}

void ScanTracerScene::addPoint(const QPointF &p)
{
    QFontMetrics fm(font());
    int w = fm.horizontalAdvance("0");

    if (_calibMode) {
        QGraphicsEllipseItem *eitm = new QGraphicsEllipseItem(-w, -w, 2*w, 2*w);

        eitm->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        eitm->setPen(QPen(Qt::red));
        eitm->setBrush(QBrush(QColor(255, 0, 0, 64)));
        eitm->setPos(p);

        addItem(eitm);
        _calibMarks.append(eitm);
    }

    if (_baseLineMode) {
        // using this map to guarantee the points are sorted by x coordinate
        _baseLinePoints[p.x()] = QPointF(p.x(), p.y());
        QGraphicsEllipseItem *bitm = new QGraphicsEllipseItem(-w/2, -w/2, w, w);

        bitm->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        bitm->setPen(QPen(Qt::blue));
        bitm->setBrush(QBrush(QColor(0, 0, 255, 64)));
        bitm->setPos(p);

        addItem(bitm);
        _baseLineMarks.append(bitm);

        QPolygonF poly = pointMapToPolygon(_baseLinePoints);
        QPainterPath pth_c;

        pth_c.addPolygon(poly);
        if (_baseLine) _baseLine->setPath(pth_c);
        update();
    }

    emit sigMouseClickPosition(p);
}

int ScanTracerScene::imageWidth()
{
    if (!_pixmap.isNull()) return _pixmap.width();
    return 0;
}

int ScanTracerScene::imageHeight()
{
    if (!_pixmap.isNull()) return _pixmap.height();
    return 0;
}

void ScanTracerScene::setBL(const QPoint &p)
{
    if (_calibMarks.size() < 1) return;
    _calibMarks[0]->setPos(p);
    update();
}

void ScanTracerScene::setBR(const QPoint &p)
{
    if (_calibMarks.size() < 2) return;
    _calibMarks[1]->setPos(p);
    update();
}

void ScanTracerScene::setTL(const QPoint &p)
{
    if (_calibMarks.size() < 3) return;
    _calibMarks[2]->setPos(p);
    update();
}

QPolygonF ScanTracerScene::pointMapToPolygon(const QMap<double, QPointF> &m)
{
    QPolygonF poly;
    QMapIterator<double, QPointF> it(m);

    while (it.hasNext()) {
        it.next();
        poly.append(it.value());
    }

    return poly;
}

QPolygonF ScanTracerScene::baseLinePolygon()
{
    return pointMapToPolygon(_baseLinePoints);
}

void ScanTracerScene::addTraceMark(const QPointF &p1)
{
    QFontMetrics fm(font());
    int w = fm.horizontalAdvance("0") / 2;
    QPen pen(Qt::darkGreen);
    QBrush brush(QColor(0, 255, 0, 64));

    QGraphicsLineItem *eitmA = new QGraphicsLineItem(-w, -w,  w, w);
    QGraphicsLineItem *eitmB = new QGraphicsLineItem( w, -w, -w, w);

    eitmA->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
    eitmB->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

    eitmA->setPen(pen);
    eitmB->setPen(pen);

    eitmA->setPos(p1);
    eitmB->setPos(p1);

    addItem(eitmA);
    addItem(eitmB);

    _traceMarks.append(eitmA);
    _traceMarks.append(eitmB);
}

void ScanTracerScene::clearAll()
{
    clearPixmap();
    clearBaseLine();
    clearCalibMarks();
    clearTrace();
}

void ScanTracerScene::clearPixmap()
{
    if (!_pixmapItem) return;
    removeItem(_pixmapItem);
    delete _pixmapItem;
}

void ScanTracerScene::clearBaseLine()
{
    clearBaseLineMarks();
    if (_baseLine) _baseLine->setPath(QPainterPath());
}

void ScanTracerScene::clearTrace()
{
    clearTraceMarks();
}

void ScanTracerScene::clearTraceMarks()
{
    while (_traceMarks.size()) {
        QGraphicsItem *e = qgraphicsitem_cast<QGraphicsItem*>(_traceMarks.takeFirst());

        if (e) {
            removeItem(e);
            delete e;
        }
    }
}

void ScanTracerScene::clearCalibMarks()
{
    while (_calibMarks.size()) {
        QGraphicsItem *e = qgraphicsitem_cast<QGraphicsItem*>(_calibMarks.takeFirst());

        if (e) {
            removeItem(e);
            delete e;
        }
    }
}

void ScanTracerScene::clearBaseLineMarks()
{
    _baseLinePoints.clear();

    while (_baseLineMarks.size()) {
        QGraphicsItem *e = qgraphicsitem_cast<QGraphicsItem*>(_baseLineMarks.takeFirst());

        if (e) {
            removeItem(e);
            delete e;
        }
    }
}

void ScanTracerScene::pickScanColor(const QPointF &p)
{
    if (!_pixmap.rect().contains(QPoint(int(p.x()+0.5), int(p.y()+0.5)))) return;
    QImage _img = _pixmap.toImage().convertToFormat(QImage::Format_RGB888);
    emit sigScanColorPicked(QColor(_img.pixel(p.x(), p.y())));
}

void ScanTracerScene::pickBackgroundColor(const QPointF &p)
{
    if (!_pixmap.rect().contains(QPoint(int(p.x()+0.5), int(p.y()+0.5)))) return;
    QImage _img = _pixmap.toImage().convertToFormat(QImage::Format_RGB888);
    emit sigBackgroundColorPicked(QColor(_img.pixel(p.x(), p.y())));
}

bool ScanTracerScene::hasPixmap()
{
    if (!_pixmap) return false;
    if (_pixmap.isNull()) return false;
    return true;
}

bool ScanTracerScene::hasBaseLine()
{
    if (_baseLinePoints.count() < 2) return false;
    return true;
}
