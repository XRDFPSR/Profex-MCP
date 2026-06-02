/***************************************************************************
                          emappainter2d.h  -  description
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

#ifndef EMAPWIDGET2D_H
#define EMAPWIDGET2D_H

#include "threadrenderemap.h"
#include "../libXrdIO/colorMaps/lutstructs.h"
#include "emapstructs.h"
#include <QGraphicsView>
#include <QImage>
#include <QEvent>
#include <QMouseEvent>
#include <QMatrix4x4>
#include <QRectF>
#include <QHash>
#include <QSettings>
#include <QWheelEvent>
#include <QMouseEvent>

class EMapWidget2D : public QGraphicsView
{
    Q_OBJECT
public:
    explicit EMapWidget2D(QWidget *parent = 0);
    ~EMapWidget2D();

    void drawLevel(int);
    QImage getImage();
    bool initLevel(int i);
    void setLut(const colorMaps::Lut &, float, float);
    void zoomToContent();
    void showDefaultText();

    void setVmap(const EDataMap *);
    inline void setMatrix(const QMatrix4x4 &f2c)                {mf2c = f2c;}
    inline void setProjection(projection_t p)                   {projection = p;}
    inline void setNCpus(int i)                                 {nCpus = i;}
    inline void setImageSize(int i)                             {imageSize = i;}
    inline void setInterpolation(int i)                         {interpolation = i;}
    inline void setAtomList(const QList<Atom> &l)               {atoms = l;}
    inline void setSuperSamplingFactor(int i)                   {superSampling = i;}

    // renders image at level l and stores it in the buffer. if c = true, the buffer is cleared first
    void preRenderImage(int l, bool c);

    // renders all images and stores them in the buffer
    void preRenderImages();

    // render images, specify the level to be rendered
    QImage renderImage(int);

    // return a reference to the current image
    const QImage * currentImage()                               {return &image;}

    // return the number of images available
    inline int numberOfLevels()                                 {return imgBuffer->size();}

private:
    QGraphicsScene *scene;
    QGraphicsPixmapItem *imgIt;
    QMatrix4x4 mf2c;  // fractional unit cell -> cartesian Angstrom
    QMatrix4x4 mc2f;  // fractional cartesian -> fractional unit cell
    QMatrix4x4 mf2px; // fractional unit cell -> cartesian pixel
    int level;
    float flevel;
    float dmin, dmax;
    QRectF bRectC; // bounding rect of the unit cell in cartesian Angstrom
    QRect  bRectP; // bounding rect of the unit cell in cartesian pixels
    QRectF bRectD; // bounding rect of the drawing
    projection_t projection;
    float axLength;
    bool drawAxes;
    bool drawAtoms;
    bool drawAtomLabels;
    int interpolation;
    double sfact;
    int superSampling;

    const EDataMap *vmap;
    QList<QImage> *imgBuffer;
    QImage image;
    QImage simg;
    colorMaps::Lut lut;
    QHash<QRgb, float> iLut;
    QList<Atom> atoms;
    QColor atomCircleColor;
    QColor atomTextColor;
    QPointF lastMousePos;
    int atomCircleWidth;
    int atomCircleFill;

    int activeThreads;
    int nCpus;
    int imageSize;
    int imageHeight;
    int imageWidth;

    void drawSceneItems();
    void drawPixmap();
    void drawOverlay(const QRectF &);

    QMatrix4x4 calcC2FMatrix(const QMatrix4x4 &, float);
    void updateCoordinates();
    void updateSceneRect();

    void wheelEvent(QWheelEvent *);
    void mousePressEvent(QMouseEvent *);
    void mouseMoveEvent(QMouseEvent *);

public slots:
    void toggleAxes(bool);
    void toggleAtoms(bool);
    void toggleAtomLabels(bool);

private slots:

signals:
    void coordinates(float, float, float);
    void acceptsInput(bool);
    void zoomTo(QPoint);
};

#endif // EMAPWIDGET2D_H
