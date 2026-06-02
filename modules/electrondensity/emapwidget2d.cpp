/***************************************************************************
                          emappainter2d.cpp  -  description
                             -------------------
    begin                : Mon Sep 22 09:00:00 CEST 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#include "math.h"
#include "emapwidget2d.h"
#include "emapdatahandler.h"
#include "../libXrdIO/colorMaps/imageeffects.h"
#include <QVector3D>
#include <QVector>
#include <QList>
#include <QTime>
#include <QGraphicsPixmapItem>
#include <QFileInfo>
#include <QDir>

EMapWidget2D::EMapWidget2D(QWidget *parent) :
    QGraphicsView(parent)
{
    scene = new QGraphicsScene(this);
    setScene(scene);
    setMouseTracking(true);
    setInteractive(false);
    setDragMode(QGraphicsView::ScrollHandDrag);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);

    vmap = nullptr;
    imgIt = nullptr;
    imgBuffer = new QList<QImage>;
    image = QImage();

    axLength = 0.25;
    drawAxes = true;
    projection = AB_PLANE;
    level = 0;
    flevel = 0.0;
    activeThreads = 0;
    interpolation = 2;
    sfact = 1.0;
    superSampling = 1;

    bRectC = QRectF(0.0, 0.0, 1.0, 1.0);
    lastMousePos = QPointF(0.0, 0.0);

    atomCircleColor = QColor(255, 0, 0);
    atomTextColor = QColor(0, 0, 0);
    atomCircleWidth = 3;
    atomCircleFill = 0;

    setAlignment(Qt::AlignCenter);
    show();
}

EMapWidget2D::~EMapWidget2D()
{
    if (imgIt)     delete imgIt;
    if (imgBuffer) delete imgBuffer;
    if (scene)     delete scene;
}

void EMapWidget2D::showDefaultText()
{

}

void EMapWidget2D::setVmap(const EDataMap *v)
{
    vmap = v;
    *imgBuffer = QList<QImage>(EMapDataHandler::numberOfImages(vmap, projection), QImage());
}

void EMapWidget2D::setLut(const colorMaps::Lut &l, float max, float min)
{
    dmax = max;
    dmin = min;
    lut = l;
}

/*
 * sets the current level, calculates the fractional value,
 * and creates a matrix from fractional cartesian -> fractional
 * unit cell for level i
 */
bool EMapWidget2D::initLevel(int i)
{
    int nImgs = EMapDataHandler::numberOfImages(vmap, projection);

    if (nImgs == 0) {
        qDebug() << QString("EMapWidget2D::setCurrentLevel(): No images available");
        return false;
    }

    level = i;
    flevel = float(i)/float(nImgs);

    // re-calculate the matrix cartesian -> fractional
    mc2f = calcC2FMatrix(mf2c, flevel);

    return true;
}

/*
 * checks if rendering the image at level i is required,
 * and calls drawSceneElements() if upd=true.
 *
 * set upd=false if a call to zoomToContent follows, to avoid
 * double redrawings
 */
void EMapWidget2D::drawLevel(int i)
{
    if (!initLevel(i)) {
        return;
    }

    if (i >= imgBuffer->size()) {
        qDebug() << QString("EMapWidget2D::drawLevel(): Trying to access image %1 of %2, exiting").arg(i).arg(imgBuffer->size() - 1);
        return;
    }

    if (imgBuffer->at(i).isNull()) {
        emit acceptsInput(false);
        image = renderImage(i);
        imgBuffer->replace(i, image);
        emit acceptsInput(true);
    } else {
        image = imgBuffer->at(i);
    }

    updateCoordinates();
    drawSceneItems();
}

/*
 * constructs the matrix for fractional cartesian to fractional
 * unit cell for fractional level fl (0.0 <= fl <= 1.0)
 *
 * for certain projections and lattice types, the cartesian coordinates
 * can extend to the negative range. Here the origin is at "O", and
 * the vertical axis ends in the negative range:
 *
 * -------------------
 * |------------     |
 * | \          \    |
 * |  \          \   |
 * |   \          \  |
 * |    \          \ |
 * |     O-----------|
 * -------------------
 *
 * Therefore we must determine the matrix' bounding rect, calculate the
 * aspect ratio (because the image with is yet unknown), scale it to
 * the image height, calculate the image width, and move into
 * the positive range.
 */
QMatrix4x4 EMapWidget2D::calcC2FMatrix(const QMatrix4x4 &f2c, float fl)
{
    QMatrix4x4 m = f2c.inverted();

    // these will hold cartesian coordinates in angstrom...
    QVector3D vecOrigin; // ... of the origin at this level
    QVector3D vecH;      // ... of the end of the horizontal axis
    QVector3D vecV;      // ... of the end of the vertical axis

    if (projection == AB_PLANE) {
        vecOrigin = f2c.map(QVector3D(0.0, 0.0, fl));
        vecH      = f2c.map(QVector3D(1.0, 0.0, fl));
        vecV      = f2c.map(QVector3D(0.0, 1.0, fl));
    }

    if (projection == AC_PLANE) {
        vecOrigin = f2c.map(QVector3D(0.0, 0.0, -fl));
        vecH      = f2c.map(QVector3D(1.0, 0.0, -fl));
        vecV      = f2c.map(QVector3D(0.0, 1.0, -fl));
    }

    if (projection == BC_PLANE) {
        vecOrigin = f2c.map(QVector3D(0.0, 0.0, fl));
        vecH      = f2c.map(QVector3D(1.0, 0.0, fl));
        vecV      = f2c.map(QVector3D(0.0, 1.0, fl));
    }

    // check if any axis ends in the negative range (due to angles > 90)
    float minH = qMin(vecH.x() - vecOrigin.x(), vecV.x() - vecOrigin.x());
    float minV = qMin(vecH.y() - vecOrigin.y(), vecV.y() - vecOrigin.y());

    // check if any axis ends in the negative range (due to angles > 90)
    float maxH = qMax(vecH.x() - vecOrigin.x(), vecV.x() - vecOrigin.x());
    float maxV = qMax(vecH.y() - vecOrigin.y(), vecV.y() - vecOrigin.y());

    bRectC = QRectF(qMin(minH, float(0.0)), qMin(minV, float(0.0)),
                    maxH + fabs(minH), maxV + fabs(minV));

    if (bRectC.width() > bRectC.height()) {
        imageWidth = imageSize;
        imageHeight = int(imageWidth * bRectC.height() / bRectC.width() + 0.5);
    } else {
        imageHeight = imageSize;
        imageWidth = int(imageHeight * bRectC.width() / bRectC.height() + 0.5);
    }

    // the matrix from fractional cartesian to fractional unit cell can be
    // calculated here, because it does not depend on the image size in pixels
    m.translate(bRectC.left(), bRectC.top(), 0.0);
    m.scale(bRectC.width(), bRectC.height(), 1.0);

    return m;
}

void EMapWidget2D::drawSceneItems()
{
    drawPixmap();
    drawOverlay(imgIt->boundingRect());
    scene->setSceneRect(bRectD);
}

void EMapWidget2D::drawPixmap()
{
    scene->clear();
    imgIt = scene->addPixmap(QPixmap::fromImage(image));
    updateSceneRect();
}

void EMapWidget2D::updateSceneRect()
{
    QRectF r = scene->itemsBoundingRect();
    QMargins mrg(0.02*r.width(), 0.02*r.height(), 0.02*r.width(), 0.02*r.height());

    bRectD = scene->itemsBoundingRect() + mrg;
}

void EMapWidget2D::drawOverlay(const QRectF &bound)
{
    int nImgs = EMapDataHandler::numberOfImages(vmap, projection);
    if (nImgs == 0) return;

    double fracLevel = double(level) / (double)nImgs;
    sfact = 1.0 / qMin(transform().m11(), transform().m22());

    // the matrix fractional-to-pixels must be updated
    // because it depends on the size of the image in pixels.
    // we start from the fractional-to-cartesian matrix, and then
    // scale it to the drawing area.

    // get a fresh copy of the mf2c matrix
    mf2px = mf2c;

    // scale it to the image width
    mf2px.scale(bound.width() / bRectC.width(), bound.height() / bRectC.height(), 1.0);

    // get origin and axis endpoints
    QVector3D f00 = mf2px.map(QVector3D(0.0, 0.0, 0.0));
    QVector3D f10 = mf2px.map(QVector3D(1.0, 0.0, 0.0));
    QVector3D f01 = mf2px.map(QVector3D(0.0, 1.0, 0.0));
    QVector3D f11 = mf2px.map(QVector3D(1.0, 1.0, 0.0));

    // determine how far the endpoints extend to the negative range
    float dx = qMin(f00.x(), qMin(f10.x(), f01.x()));
    float dy = qMin(f00.y(), qMin(f10.y(), f01.y()));

    // reduce the size of the axis to an arbitrary length (just visually appealing)
    QVector3D faxisH = f10 * axLength;
    QVector3D faxisV = f01 * axLength;

    QString textH;
    QString textV;

    if (projection == AB_PLANE) {
        textH = "a";
        textV = "b";
    }

    if (projection == AC_PLANE) {
        textH = "a";
        textV = "c";
    }

    if (projection == BC_PLANE) {
        textH = "b";
        textV = "c";
    }

    // translate the vectors to the position of the origin on the widget
    f00    -= QVector3D(dx, dy, 0.0);
    f10    -= QVector3D(dx, dy, 0.0);
    f01    -= QVector3D(dx, dy, 0.0);
    f11    -= QVector3D(dx, dy, 0.0);
    faxisH -= QVector3D(dx, dy, 0.0);
    faxisV -= QVector3D(dx, dy, 0.0);

    QPolygonF border;
    border << QPointF(f00.x(), f00.y()) << QPointF(f01.x(), f01.y());
    border << QPointF(f11.x(), f11.y()) << QPointF(f10.x(), f10.y());
    border << QPointF(f00.x(), f00.y());

    int strokeLineWidth = 1 + (sfact < 1.0 ? 1 : int(sfact + 0.5));

    // draw the unit cell outline
    QGraphicsPolygonItem *cellOutline = scene->addPolygon(border, QPen(QBrush(Qt::black), strokeLineWidth));
    cellOutline->setFlag(QGraphicsItem::ItemClipsChildrenToShape, true);

    // needed for cursor coordinates
    bRectP.setCoords(int(qMin(f00.x(), qMin(f10.x(), f01.x()))),
                     int(qMin(f00.y(), qMin(f10.y(), f01.y()))),
                     int(qMax(f11.x(), qMax(f01.x(), f10.x()))),
                     int(qMax(f11.y(), qMax(f01.y(), f10.y()))));

    QPen penLines(Qt::red);
    penLines.setWidth(strokeLineWidth);

    if (drawAxes) {
        QGraphicsSimpleTextItem *htxt = scene->addSimpleText(textH);
        QGraphicsSimpleTextItem *vtxt = scene->addSimpleText(textV);

        int lblDst = fontMetrics().horizontalAdvance("aa");
        QVector3D dax(lblDst, lblDst, 0.0);

        htxt->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);
        vtxt->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

        htxt->setPos(faxisH.x() - dax.x(),     faxisH.y() + int(1.3 * dax.y()));
        vtxt->setPos(faxisV.x() - 2 * dax.x(), faxisV.y() + dax.y());

        scene->addLine(f00.x() - dax.x(), f00.y() + dax.y(), faxisH.x() - dax.x(), faxisH.y() + dax.y(), penLines);
        scene->addLine(f00.x() - dax.x(), f00.y() + dax.y(), faxisV.x() - dax.x(), faxisV.y() + dax.y(), penLines);

        updateSceneRect();
    }

    if (drawAtoms || drawAtomLabels) { // drawing atom circles and/or labels
        QVector3D patom;

        for (int i = 0; i < atoms.size(); ++i) {
            QString name = atoms.at(i).name;
            float radF = 0.0;

            if (projection == AB_PLANE) {
                // determine distance of atom from current level
                QVector3D distF(0.0, 0.0, fabs(atoms.at(i).z - fracLevel)); // fractional
                QVector3D distC = mf2c.map(distF); // cartesian

                if (distC.z() < atoms.at(i).r) {
                    patom = mf2px.map(QVector3D(atoms.at(i).x, atoms.at(i).y, 0.0));

                    // radius of the circle the current level cuts through the atom ball, in A
                    radF = sqrt(pow(double(atoms.at(i).r), 2.0) - pow(double(distC.z()), 2.0));
                }
            }

            if (projection == AC_PLANE) {
                // determine distance of atom from current level
                QVector3D distF(0.0, fabs(atoms.at(i).y - fracLevel), 0.0); // fractional
                QVector3D distC = mf2c.map(distF); // cartesian

                if (distC.y() < atoms.at(i).r) {
                    patom = mf2px.map(QVector3D(atoms.at(i).x, atoms.at(i).z, 0.0));

                    // radius of the circle the current level cuts through the atom ball, in A
                    radF = sqrt(pow(double(atoms.at(i).r), 2.0) - pow(double(distC.y()), 2.0));
                }
            }

            if (projection == BC_PLANE) {
                // determine distance of atom from current level
                QVector3D distF(fabs(atoms.at(i).x - fracLevel), 0.0, 0.0); // fractional
                QVector3D distC = mf2c.map(distF); // cartesian

                if (distC.x() < atoms.at(i).r) {
                    patom = mf2px.map(QVector3D(atoms.at(i).y, atoms.at(i).z, 0.0));

                    // radius of the circle the current level cuts through the atom ball, in A
                    radF = sqrt(pow(double(atoms.at(i).r), 2.0) - pow(double(distC.x()), 2.0));
                }
            }

            if (radF > 0.0) {
                // scaled to image size
                patom -= QVector3D(dx, dy, 0.0);

                if (drawAtoms) {
                    int rad = int(radF * imageWidth / bRectC.width());

                    QGraphicsEllipseItem *itAtomCircle = new QGraphicsEllipseItem(patom.x() - rad, patom.y() - rad, 2 * rad, 2 * rad, cellOutline);
                    itAtomCircle->setPen(penLines);

                    itAtomCircle->setToolTip(QString("%1: x=%2 y=%3 z=%4")
                                     .arg(atoms.at(i).name)
                                     .arg(atoms.at(i).x, 0, 'f', 4)
                                     .arg(atoms.at(i).y, 0, 'f', 4)
                                     .arg(atoms.at(i).z, 0, 'f', 4));
                }

                if (drawAtomLabels) {
                    QGraphicsSimpleTextItem *itLbl = new QGraphicsSimpleTextItem(name, cellOutline);
                    itLbl->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

                    QRectF textBox = itLbl->boundingRect();

                    QGraphicsRectItem *itBox = new QGraphicsRectItem(0, 0, textBox.width(), textBox.height(), cellOutline);
                    itBox->setFlag(QGraphicsItem::ItemIgnoresTransformations, true);

                    itBox->setPen(QPen(Qt::white));
                    itBox->setBrush(QBrush(Qt::white));

                    itLbl->setZValue(1);

                    QRectF rmap = mapToScene(textBox.toRect()).boundingRect();

                    itBox->setPos(patom.x() - rmap.width() / 2, patom.y() - rmap.height() / 2);
                    itLbl->setPos(patom.x() - rmap.width() / 2, patom.y() - rmap.height() / 2);
                }
            }
        }
    }
}

/*
 * draw/hide axes
 */
void EMapWidget2D::toggleAxes(bool b)
{
    drawAxes = b;
    if (scene->items().size()) {
        drawSceneItems();
    }
}

/*
 * draw/hide atom circles
 */
void EMapWidget2D::toggleAtoms(bool b)
{
    drawAtoms = b;
    if (scene->items().size()) {
        drawSceneItems();
    }
}

/*
 * draw/hide atom labels
 */
void EMapWidget2D::toggleAtomLabels(bool b)
{
    drawAtomLabels = b;
    if (scene->items().size()) {
        drawSceneItems();
    }
}

void EMapWidget2D::preRenderImage(int l, bool c)
{
    emit acceptsInput(false);
    if (c) imgBuffer->clear();
    imgBuffer->append(renderImage(l));
    emit acceptsInput(true);
}

void EMapWidget2D::preRenderImages()
{
    emit acceptsInput(false);

    imgBuffer->clear();
    int n = EMapDataHandler::numberOfImages(vmap, projection);

    for (int i = 0; i < n; ++i) {
        imgBuffer->append(renderImage(i));
    }

    emit acceptsInput(true);
}

QImage EMapWidget2D::renderImage(int l)
{
    if (!EMapDataHandler::checkEMapSize(vmap)) return QImage();
    int n = EMapDataHandler::numberOfImages(vmap, projection);
    if (n == 0) return QImage();

    emit acceptsInput(false);

    calcC2FMatrix(mf2c, float(l)/float(n));

    QImage img(imageWidth * superSampling, imageHeight * superSampling, QImage::Format_ARGB32);
    img.fill(QColor(0, 0, 0, 0));

    double start = 0.0;
    double end = double(img.height()) / double(nCpus);

    QList<QThread*> threadList;

    for (int i = 0; i < nCpus; ++i) {
        ThreadRenderEmap *emThread = new ThreadRenderEmap(this);

        // set up the thread parameters
        emThread->setLut(lut);
        emThread->setRange(qMax(fabs(dmax), fabs(dmin)));
        emThread->setProjection(projection);
        emThread->setImage(&img);
        emThread->setVmap(vmap);
        emThread->setInterpolation(interpolation);
        emThread->setFractLevel(float(l)/float(n));
        emThread->setMatrix(mf2c, mc2f);
        emThread->setImageSection(int(start), int(end + 0.5));
        emThread->setBackground(palette().window().color());

        threadList.append(emThread);
        emThread->start();

        start = end;
        end = start + double(img.height()) / double(nCpus);
    }

    for (int i = 0; i < threadList.size(); ++i) {
        threadList.at(i)->wait(120000);
    }

    while (threadList.size()) {
        QThread *t = threadList.takeFirst();
        delete t;
    }

    if (lut.contour) {
        if (lut.type == 1) ImageEffects::contourLinesBlackOnColor(&img, lut.contour);
        if (lut.type == 2) ImageEffects::contourLinesBlackOnWhite(&img, lut.contour);
        if (lut.type == 3) ImageEffects::contourLinesColor(&img, lut.contour);
    }

    emit acceptsInput(true);

    if (superSampling == 1) return img;
    return img.scaled(imageWidth, imageHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
}

QImage EMapWidget2D::getImage()
{
    QImage img(bRectD.width(), bRectD.height(), image.format());
    img.fill(QColor(0, 0, 0, 0));
    QPainter painter(&img);
    painter.setRenderHint(QPainter::Antialiasing);
    scene->render(&painter);
    return img;
}

void EMapWidget2D::wheelEvent(QWheelEvent *e)
{
    double factor = 1.05;

    if (e->angleDelta().y() < 0) {
        factor = 1.0 / factor;
    }

    scale(factor, factor);

    e->accept();
}

void EMapWidget2D::mousePressEvent(QMouseEvent *e)
{
    if (e->button() == Qt::RightButton) {
        zoomToContent();
        e->accept();
        return;
    }

    QGraphicsView::mousePressEvent(e);
}

void EMapWidget2D::mouseMoveEvent(QMouseEvent *e)
{
    // find the position of the mouse pointer as a fraction of the image size and position
    // (fractional cartesian coordinates)
    QPointF epos = mapToScene(QPoint(e->pos().x(), e->pos().y()));

    lastMousePos = QPointF(float(epos.x() - bRectP.x())/float(bRectP.width()),
                           float(epos.y() - bRectP.y())/float(bRectP.height()));

    updateCoordinates();
    QGraphicsView::mouseMoveEvent(e);
}

/*
 * emits the mouse coordinates
 */
void EMapWidget2D::updateCoordinates()
{
    // get fractional unit cell coordinates from the fractional cartesian coordinates
    QVector3D vfrac = mc2f.map(QVector3D(lastMousePos.x(), lastMousePos.y(), 0.0));

    if (projection == AB_PLANE) emit coordinates(vfrac.x(), vfrac.y(), flevel);
    if (projection == AC_PLANE) emit coordinates(vfrac.x(), flevel, vfrac.y());
    if (projection == BC_PLANE) emit coordinates(flevel, vfrac.x(), vfrac.y());
}

void EMapWidget2D::zoomToContent()
{
    fitInView(scene->sceneRect(), Qt::KeepAspectRatio);
    drawSceneItems();
}
