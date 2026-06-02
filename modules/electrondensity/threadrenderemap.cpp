/***************************************************************************
                          renderemap.cpp  -  description
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
#include "threadrenderemap.h"
#include "emapdatahandler.h"
#include <QImage>
#include <QDebug>
#include <QVector3D>
#include <QMutex>

ThreadRenderEmap::ThreadRenderEmap(QObject *parent) :
    QThread(parent)
{
    // initialization
    flevel = 0.0;
    interpolate = false;
    mf2c = QMatrix4x4();
    mc2f = QMatrix4x4();
    elRange = 1.0;
    projection = AB_PLANE;
    image = nullptr;
    vmap = nullptr;
    lineStart = 0;
    lineEnd = 0;
}

/*
 * set the matrix fractional unit cell -> fractional cartesian, and the
 * matrix cartesian pixels -> fractional unit cell
 */
void ThreadRenderEmap::setMatrix(const QMatrix4x4 &f2c, const QMatrix4x4 &c2f)
{
    mf2c = f2c;
    mc2f = c2f;
}

/*
 * set the first and last line of the image to render
 */
void ThreadRenderEmap::setImageSection(int s, int e)
{
    lineStart = s;
    lineEnd = e;
}

/*
 * run the thread
 */
void ThreadRenderEmap::run()
{
    if (!EMapDataHandler::checkEMapSize(vmap)) {
        return;
    }

    // the color for pixelValue = 0.0 is in the center of the lookup table
    float zero = float(lut.data.size()) / 2.0;

    // will hold the point in fractional coordinates
    QVector3D fvec;

    // prepare for locking the image while writing the pixel value
    QMutex mutex;
    for (int iy = lineStart; iy < lineEnd; ++iy) {
        for (int ix = 0; ix < image->width(); ++ix) {
            // get cartesian pixel coordinates in fractional coordinates
            QVector3D v = mc2f.map(QVector3D(float(ix) / float(image->width()), float(iy) / float(image->height()), 0.0));

            if (projection == AB_PLANE) fvec = QVector3D(v.x(), v.y(), flevel);
            if (projection == AC_PLANE) fvec = QVector3D(v.x(), flevel, v.y());
            if (projection == BC_PLANE) fvec = QVector3D(flevel, v.x(), v.y());

            if (fvec.x() < 0.0) continue;
            if (fvec.y() < 0.0) continue;
            if (fvec.z() < 0.0) continue;

            if (fvec.x() > 1.0) continue;
            if (fvec.y() > 1.0) continue;
            if (fvec.z() > 1.0) continue;

            // read the electron density at pixel fvec, using interpolation of required
            float pixelValue = getPixelValue(fvec, interpolate);

            // transform to an index in the lookup table
            float rgb = zero + (float(lut.data.size()) - zero) * pixelValue / elRange;

            // keep index within the range of the lut (some interpolation functions
            // overshoot, even though the nodes are within the range)
            int idx = int(rgb) >= lut.data.size() ? lut.data.size() - 1 : int(rgb);
            idx = idx < 0 ? 0 : idx;

            // lock the image and write the pixel
            mutex.lock();
            image->setPixel(ix, iy, lut.data.at(idx));
            mutex.unlock();
        }
    }
}

/*
 * vector coordinates are floats, but calculated electron densities
 * are only available at integer positions.
 * ipol = 0: no interpolation, float coordinates are floored
 * ipol = 1: trilinear interpolation
 * ipol = 2: tricubic interpolation
 */
float ThreadRenderEmap::getPixelValue(const QVector3D &v, int ipol)
{
    if (!vmap) return 0.0;

    if (v.x() <  0.0 || v.y() <  0.0 || v.z() <  0.0) return 0.0;
    if (v.x() >= 1.0 || v.y() >= 1.0 || v.z() >= 1.0) return 0.0;

    int szz = vmap->size();
    int szy = vmap->at(0).size();
    int szx = vmap->at(0).at(0).size();

    // calculations without interpolation
    if (ipol == 0) {
        int iz = int(0.5 + v.z() * float(szz));
        int iy = int(0.5 + v.y() * float(szy));
        int ix = int(0.5 + v.x() * float(szx));

        iz = iz >= szz ? 0 : iz;
        iy = iy >= szy ? 0 : iy;
        ix = ix >= szx ? 0 : ix;

        return vmap->at(iz).at(iy).at(ix);
    }

    // the following values are used for all interpolation methods

    // indexes in vmap, as float values
    float z = v.z() * float(szz);
    float y = v.y() * float(szy);
    float x = v.x() * float(szx);

    // fractional value between neighbouring integer coordinates
    float xd = fractValue(x);
    float yd = fractValue(y);
    float zd = fractValue(z);

    // integer values before and after the fractional values
    int px = int(x);
    int py = int(y);
    int pz = int(z);

    int nx = px + 1;
    int ny = py + 1;
    int nz = pz + 1;

    if (ipol == 1) {
        // values exceeding QVector::size() will start reading
        // at the beginning of the vectors again
        nx = nx >= szx ? nx - szx : nx;
        ny = ny >= szy ? ny - szy : ny;
        nz = nz >= szz ? nz - szz : nz;

        // get function values of vertices
        float arr[2][2][2];
        arr[0][0][0] = vmap->at(pz).at(py).at(px);
        arr[1][0][0] = vmap->at(pz).at(py).at(nx);
        arr[0][1][0] = vmap->at(pz).at(ny).at(px);
        arr[1][1][0] = vmap->at(pz).at(ny).at(nx);
        arr[0][0][1] = vmap->at(nz).at(py).at(px);
        arr[1][0][1] = vmap->at(nz).at(py).at(nx);
        arr[0][1][1] = vmap->at(nz).at(ny).at(px);
        arr[1][1][1] = vmap->at(nz).at(ny).at(nx);

        return trilinearInterpolate(arr, xd, yd, zd);
    }

    if (ipol == 2) {
        int ppx = px - 1;
        int ppy = py - 1;
        int ppz = pz - 1;

        int nnx = nx + 1;
        int nny = ny + 1;
        int nnz = nz + 1;

        // values exceeding QVector::size() will start reading
        // at the beginning of the vectors again
        ppx = ppx < 0 ? ppx + szx : ppx;
        ppy = ppy < 0 ? ppy + szy : ppy;
        ppz = ppz < 0 ? ppz + szz : ppz;

        nnx = nnx >= szx ? nnx - szx : nnx;
        nny = nny >= szy ? nny - szy : nny;
        nnz = nnz >= szz ? nnz - szz : nnz;

        // do this after calculating nnx, which is derived from nx
        nx = nx >= szx ? nx - szx : nx;
        ny = ny >= szy ? ny - szy : ny;
        nz = nz >= szz ? nz - szz : nz;

        float arr[4][4][4];
        arr[0][0][0] = vmap->at(ppz).at(ppy).at(ppx);
        arr[1][0][0] = vmap->at(ppz).at(ppy).at( px);
        arr[2][0][0] = vmap->at(ppz).at(ppy).at( nx);
        arr[3][0][0] = vmap->at(ppz).at(ppy).at(nnx);

        arr[0][1][0] = vmap->at(ppz).at( py).at(ppx);
        arr[1][1][0] = vmap->at(ppz).at( py).at( px);
        arr[2][1][0] = vmap->at(ppz).at( py).at( nx);
        arr[3][1][0] = vmap->at(ppz).at( py).at(nnx);

        arr[0][2][0] = vmap->at(ppz).at( ny).at(ppx);
        arr[1][2][0] = vmap->at(ppz).at( ny).at( px);
        arr[2][2][0] = vmap->at(ppz).at( ny).at( nx);
        arr[3][2][0] = vmap->at(ppz).at( ny).at(nnx);

        arr[0][3][0] = vmap->at(ppz).at(nny).at(ppx);
        arr[1][3][0] = vmap->at(ppz).at(nny).at( px);
        arr[2][3][0] = vmap->at(ppz).at(nny).at( nx);
        arr[3][3][0] = vmap->at(ppz).at(nny).at(nnx);

        arr[0][0][1] = vmap->at(pz).at(ppy).at(ppx);
        arr[1][0][1] = vmap->at(pz).at(ppy).at( px);
        arr[2][0][1] = vmap->at(pz).at(ppy).at( nx);
        arr[3][0][1] = vmap->at(pz).at(ppy).at(nnx);

        arr[0][1][1] = vmap->at(pz).at( py).at(ppx);
        arr[1][1][1] = vmap->at(pz).at( py).at( px);
        arr[2][1][1] = vmap->at(pz).at( py).at( nx);
        arr[3][1][1] = vmap->at(pz).at( py).at(nnx);

        arr[0][2][1] = vmap->at(pz).at( ny).at(ppx);
        arr[1][2][1] = vmap->at(pz).at( ny).at( px);
        arr[2][2][1] = vmap->at(pz).at( ny).at( nx);
        arr[3][2][1] = vmap->at(pz).at( ny).at(nnx);

        arr[0][3][1] = vmap->at(pz).at(nny).at(ppx);
        arr[1][3][1] = vmap->at(pz).at(nny).at( px);
        arr[2][3][1] = vmap->at(pz).at(nny).at( nx);
        arr[3][3][1] = vmap->at(pz).at(nny).at(nnx);

        arr[0][0][2] = vmap->at(nz).at(ppy).at(ppx);
        arr[1][0][2] = vmap->at(nz).at(ppy).at( px);
        arr[2][0][2] = vmap->at(nz).at(ppy).at( nx);
        arr[3][0][2] = vmap->at(nz).at(ppy).at(nnx);

        arr[0][1][2] = vmap->at(nz).at( py).at(ppx);
        arr[1][1][2] = vmap->at(nz).at( py).at( px);
        arr[2][1][2] = vmap->at(nz).at( py).at( nx);
        arr[3][1][2] = vmap->at(nz).at( py).at(nnx);

        arr[0][2][2] = vmap->at(nz).at( ny).at(ppx);
        arr[1][2][2] = vmap->at(nz).at( ny).at( px);
        arr[2][2][2] = vmap->at(nz).at( ny).at( nx);
        arr[3][2][2] = vmap->at(nz).at( ny).at(nnx);

        arr[0][3][2] = vmap->at(nz).at(nny).at(ppx);
        arr[1][3][2] = vmap->at(nz).at(nny).at( px);
        arr[2][3][2] = vmap->at(nz).at(nny).at( nx);
        arr[3][3][2] = vmap->at(nz).at(nny).at(nnx);

        arr[0][0][3] = vmap->at(nnz).at(ppy).at(ppx);
        arr[1][0][3] = vmap->at(nnz).at(ppy).at( px);
        arr[2][0][3] = vmap->at(nnz).at(ppy).at( nx);
        arr[3][0][3] = vmap->at(nnz).at(ppy).at(nnx);

        arr[0][1][3] = vmap->at(nnz).at( py).at(ppx);
        arr[1][1][3] = vmap->at(nnz).at( py).at( px);
        arr[2][1][3] = vmap->at(nnz).at( py).at( nx);
        arr[3][1][3] = vmap->at(nnz).at( py).at(nnx);

        arr[0][2][3] = vmap->at(nnz).at( ny).at(ppx);
        arr[1][2][3] = vmap->at(nnz).at( ny).at( px);
        arr[2][2][3] = vmap->at(nnz).at( ny).at( nx);
        arr[3][2][3] = vmap->at(nnz).at( ny).at(nnx);

        arr[0][3][3] = vmap->at(nnz).at(nny).at(ppx);
        arr[1][3][3] = vmap->at(nnz).at(nny).at( px);
        arr[2][3][3] = vmap->at(nnz).at(nny).at( nx);
        arr[3][3][3] = vmap->at(nnz).at(nny).at(nnx);

        return tricubicInterpolate(arr, xd, yd, zd);
    }

    return 0.0;
}

float ThreadRenderEmap::linearInterpolate(float p[2], float x)
{
    return p[0] * (1.0 - x) + p[1] * x;
}

float ThreadRenderEmap::bilinearInterpolate(float p[2][2], float x, float y)
{
    float arr[2];
    arr[0] = linearInterpolate(p[0], y);
    arr[1] = linearInterpolate(p[1], y);

    return linearInterpolate(arr, x);
}

float ThreadRenderEmap::trilinearInterpolate(float p[2][2][2], float x, float y, float z)
{
    float arr[2];
    arr[0] = bilinearInterpolate(p[0], y, z);
    arr[1] = bilinearInterpolate(p[1], y, z);
    return linearInterpolate(arr, x);
}

float ThreadRenderEmap::cubicInterpolate (float p[4], float x) {
    return p[1] + 0.5 * x*(p[2] - p[0] + x*(2.0*p[0] - 5.0*p[1] + 4.0*p[2] - p[3] + x*(3.0*(p[1] - p[2]) + p[3] - p[0])));
}

float ThreadRenderEmap::bicubicInterpolate (float p[4][4], float x, float y) {
    float arr[4];
    arr[0] = cubicInterpolate(p[0], y);
    arr[1] = cubicInterpolate(p[1], y);
    arr[2] = cubicInterpolate(p[2], y);
    arr[3] = cubicInterpolate(p[3], y);
    return cubicInterpolate(arr, x);
}

float ThreadRenderEmap::tricubicInterpolate (float p[4][4][4], float x, float y, float z) {
    float arr[4];
    arr[0] = bicubicInterpolate(p[0], y, z);
    arr[1] = bicubicInterpolate(p[1], y, z);
    arr[2] = bicubicInterpolate(p[2], y, z);
    arr[3] = bicubicInterpolate(p[3], y, z);
    return cubicInterpolate(arr, x);
}
