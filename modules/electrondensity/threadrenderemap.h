/***************************************************************************
                          renderemap.h  -  description
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

#ifndef THREADRENDEREMAP_H
#define THREADRENDEREMAP_H

#include "../libXrdIO/colorMaps/lutstructs.h"
#include "emapstructs.h"
#include <QThread>
#include <QMatrix4x4>
#include <QRgb>
#include <QColor>
#include <QImage>

class ThreadRenderEmap : public QThread
{
    Q_OBJECT
public:
    explicit ThreadRenderEmap(QObject *parent = 0);

    inline void setLut(const colorMaps::Lut &l)             {lut = l;}
    inline void setRange(float f)                {elRange = f;}
    inline void setProjection(projection_t p)    {projection = p;}
    inline void setImage(QImage *i)              {image = i;}
    inline void setVmap(const QVector<QVector<QVector<float> > > *m) {vmap = m;}
    inline void setInterpolation(int i)          {interpolate = i;}
    inline void setBackground(const QColor &c)   {bgColor = c;}
    inline void setFractLevel(float f)           {flevel = f;}
    void setImageSection(int, int);
    void setMatrix(const QMatrix4x4 &, const QMatrix4x4 &);

private:
    float flevel;
    int interpolate;
    QMatrix4x4 mf2c;
    QMatrix4x4 mc2f;
    colorMaps::Lut lut;
    float elRange;
    projection_t projection;
    QImage *image;
    const QVector<QVector<QVector<float > > > *vmap;
    QColor bgColor;
    int lineStart, lineEnd;

    float getPixelValue(const QVector3D &, int);

    /*
     * returns the fraction of x between its neighbouring integer values
     *
     * e.g. x=3.25 will return 0.25
     */
    inline float fractValue(float x) {return ceil(x) == floor(x) ? 0.0 : float((x - floor(x)) / (ceil(x) - floor(x)));}

    float linearInterpolate(float p[2], float x);
    float bilinearInterpolate(float p[2][2], float x, float y);
    float trilinearInterpolate(float p[2][2][2], float x, float y, float z);

    float cubicInterpolate (float p[4], float x);
    float bicubicInterpolate (float p[4][4], float x, float y);
    float tricubicInterpolate (float p[4][4][4], float x, float y, float z);

    void run();
};

#endif // THREADRENDEREMAP_H
