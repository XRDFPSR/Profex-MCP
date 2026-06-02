/***************************************************************************
                          lutgenerator.cpp  -  description
                             -------------------
    begin                : Fri Sep 26 10:24:00 CEST 2014
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
#include "lutgenerator.h"
#include <QDebug>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

LutGenerator::LutGenerator(int s, bool t)
{
    size = s;
    transparent = t;
}

/*
 * calculates various lookup tables.
 *
 * Note: in case of grayscale map, the lut will only be of size "size",
 * because the dmin = black, and dmax = white
 * all other maps will have size "2*size", because 0.0 = black, and one
 * full color range extens to the negative range, and one to the positive range
 */
QMap<QString, colorMaps::Lut> LutGenerator::getLuts(bool smth, bool cntr, bool cntl, bool ccol, bool colc)
{
    lutList.clear();

    int n = 1;

    // smooth
    if (smth) {
        lutList.insert(QString("%1 Grayscale").arg(n,      2, 10, QChar('0')), lutGrayScale(1, 0, 0));
        lutList.insert(QString("%1 Blue-Red").arg(n+1,     2, 10, QChar('0')), lutBlueRed());
        lutList.insert(QString("%1 Bright Glow").arg(n+2,  2, 10, QChar('0')), lutBrightGlow());
        lutList.insert(QString("%1 Dark Glow").arg(n+3,    2, 10, QChar('0')), lutDarkGlow());
        lutList.insert(QString("%1 False Colors").arg(n+4, 2, 10, QChar('0')), lutFalseColor(1, 0, 0));
        n += 5;
    }

    // contour
    if (cntr) {
        lutList.insert(QString("%1 Gray Contour").arg(n,          2, 10, QChar('0')), lutGrayScale(16, 1, 0));
        lutList.insert(QString("%1 False Color Contour").arg(n+1, 2, 10, QChar('0')), lutFalseColor(16, 1, 0));
        n += 2;
    }

    // contour lines
    if (cntl) {
        lutList.insert(QString("%1 Coarse Contour Lines").arg(n,        2, 10, QChar('0')), lutGrayScale(16, 2, 1));
        lutList.insert(QString("%1 Coarse Contour Lines Bold").arg(n+1, 2, 10, QChar('0')), lutGrayScale(16, 2, 2));
        lutList.insert(QString("%1 Fine Contour Lines").arg(n+2,        2, 10, QChar('0')), lutGrayScale(8, 2, 1));
        lutList.insert(QString("%1 Fine Contour Lines Bold").arg(n+3,   2, 10, QChar('0')), lutGrayScale(8, 2, 2));
        n += 4;
    }

    // contour color and lines
    if (ccol) {
        lutList.insert(QString("%1 Gray with Contour Lines").arg(n,               2, 10, QChar('0')), lutGrayScale(16, 1, 1));
        lutList.insert(QString("%1 Gray with Contour Lines Bold").arg(n+1,        2, 10, QChar('0')), lutGrayScale(16, 1, 2));
        lutList.insert(QString("%1 False Color with Contour Lines").arg(n+2,      2, 10, QChar('0')), lutFalseColor(16, 1, 1));
        lutList.insert(QString("%1 False Color with Contour Lines Bold").arg(n+3, 2, 10, QChar('0')), lutFalseColor(16, 1, 2));
        n += 4;
    }

    // colored contour lines
    if (colc) {
        lutList.insert(QString("%1 Colored Contour Lines").arg(n,        2, 10, QChar('0')), lutFalseColor(16, 3, 1));
        lutList.insert(QString("%1 Colored Contour Lines Bold").arg(n+1, 2, 10, QChar('0')), lutFalseColor(16, 3, 2));
    }

    return lutList;
}

colorMaps::Lut LutGenerator::lutGrayScale(int div, int type, int con)
{
    colorMaps::Lut lut("Grayscale", type, con);
    lut.bits = 8;
    lut.hasNegativeRange = false;

    int sz = size / div;

    for (int i = 0; i < sz; ++i) {
        int g = int(255.0 * float(i)/float(sz));
        lut.data.append(qRgb(g, g, g));
    }

    return lut;
}

colorMaps::Lut LutGenerator::lutBlueRed()
{
    colorMaps::Lut lut("Blue-Red", 0, 0);
    lut.bits = 24;
    lut.hasNegativeRange = true;

    // negative range
    for (int i = 0; i <= size; ++i) {
        float f = float(size - i)/float(size);
        int r = int(255.0 * f);
        int g = int(255.0 * f);
        int b = int(255.0 * sqrt(f));
        lut.data.append(qRgb(r, g, b));
    }

    // positive range
    for (int i = 1; i <= size; ++i) {
        float f = float(i)/float(size);
        int r = int(255.0 * sqrt(f));
        int g = int(255.0 * f);
        int b = int(255.0 * f);
        lut.data.append(qRgb(r, g, b));
    }

    return lut;
}

colorMaps::Lut LutGenerator::lutBrightGlow()
{
    colorMaps::Lut lut("Bright Glow", 0, 0);
    lut.bits = 24;
    lut.hasNegativeRange = true;

    // negative range
    for (int i = 0; i <= size; ++i) {
        float f = float(size - i)/float(size);
        float r = 255.0 * 1.0 * f >= 255.0 ? 255.0 : 255.0 * 1.0 * f;
        float g = 255.0 * 2.0 * f >= 255.0 ? 255.0 : 255.0 * 2.0 * f;
        float b = 255.0 * 3.0 * f >= 255.0 ? 255.0 : 255.0 * 3.0 * f;
        lut.data.append(qRgb(int(r), int(g), int(b)));
    }

    // positive range
    for (int i = 1; i <= size; ++i) {
        float f = float(i)/float(size);
        float r = 255.0 * 3.0 * f >= 255.0 ? 255.0 : 255.0 * 3.0 * f;
        float g = 255.0 * 2.0 * f >= 255.0 ? 255.0 : 255.0 * 2.0 * f;
        float b = 255.0 * 1.0 * f >= 255.0 ? 255.0 : 255.0 * 1.0 * f;
        lut.data.append(qRgb(int(r), int(g), int(b)));
    }

    return lut;
}

colorMaps::Lut LutGenerator::lutDarkGlow()
{
    colorMaps::Lut lut("Dark Glow", 0, 0);
    lut.bits = 24;
    lut.hasNegativeRange = true;

    // negative range
    for (int i = 0; i <= size; ++i) {
        float f = float(size - i)/float(size) * float(M_PI/2.0);
        // float r = 0.0;
        float g = 255.0 * (1.0 - cos(f));
        float b = 255.0 * sin(f);
        lut.data.append(qRgb(0, int(g), int(b)));
    }

    // positive range
    for (int i = 1; i <= size; ++i) {
        float f = float(i)/float(size) * float(M_PI/2.0);
        float r = 255.0 * sin(f);
        float g = 255.0 * pow(float(sin(f)), float(2.0));
        float b = 255.0 * (1.0 - cos(f));
        lut.data.append(qRgb(int(r), int(g), int(b)));
    }

    return lut;
}

colorMaps::Lut LutGenerator::lutFalseColor(int div, int type, int con)
{
    colorMaps::Lut lut("False Colors", type, con);
    lut.bits = 24;
    lut.hasNegativeRange = false;

    int sz = size / div;

    int r = 0;
    int g = 0;
    int b = 0;

    for (int i = 0; i < sz; ++i) {
        float f = float(i) / float(sz - 1);
        r = int(f * 1024.0 - 512.0);
        g = f < 0.5 ? int(f * 1024.0) : int(1024.0 - f * 1024.0);
        b = int(512.0 - f * 1024.0);

        r = r > 255 ? 255 : r;
        g = g > 255 ? 255 : g;
        b = b > 255 ? 255 : b;

        r = r < 0 ? 0 : r;
        g = g < 0 ? 0 : g;
        b = b < 0 ? 0 : b;

        lut.data.append(qRgb(r, g, b));
    }

    return lut;
}
