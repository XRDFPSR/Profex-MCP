/***************************************************************************
                          sobelfilter.cpp  -  description
                             -------------------
    begin                : Sat Jan 06 13:00:00 CEST 2018
    copyright            : (C) 2018 by Nicola Doebelin
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

#include "sobelfilter.h"

SobelFilter::SobelFilter(QMutex *m)
{
    mutex = m;
}

void SobelFilter::setRange(int s, int e, const QImage *si, QImage *ti)
{
    line_start = s;
    line_end = e;
    sourceImage = si;
    targetImage = ti;
}

void SobelFilter::run()
{
    generateContourLines();
}


void SobelFilter::generateContourLines()
{
    if (!sourceImage) return;
    if (!targetImage) return;

    float kernelx[3][3] = {{-1.0, 0.0, 1.0},
                           {-2.0, 0.0, 2.0},
                           {-1.0, 0.0, 1.0}};

    float kernely[3][3] = {{-1.0, -2.0, -1.0},
                           { 0.0,  0.0,  0.0},
                           { 1.0,  2.0,  1.0}};

    int mx = sourceImage->width();

    for (int y = line_start; y < qMin(line_end, sourceImage->height()); ++y) {
        int pRowY = (y <= 0 ? 0 : y - 1);
        int cRowY = y;
        int nRowY = (y >= sourceImage->height() - 1 ? sourceImage->height() - 1 : y + 1);

        QRgb *pRow = (QRgb*)sourceImage->constScanLine(pRowY); // previous row (y-1)
        QRgb *cRow = (QRgb*)sourceImage->constScanLine(cRowY); // current row (y)
        QRgb *nRow = (QRgb*)sourceImage->constScanLine(nRowY); // next row (y+1)

        mutex->lock();
        QRgb *tRow = (QRgb*)targetImage->scanLine(cRowY);
        mutex->unlock();

        for (int x = 0; x < sourceImage->width(); ++x) {
            if (qAlpha(cRow[x]) == 0) continue;
            int px = (kernelx[0][0] * pixelValue(x-1, mx, pRow)) + (kernelx[0][1] * pixelValue(x, mx, pRow)) + (kernelx[0][2] * pixelValue(x+1, mx, pRow)) +
                     (kernelx[1][0] * pixelValue(x-1, mx, cRow)) + (kernelx[1][1] * pixelValue(x, mx, cRow)) + (kernelx[1][2] * pixelValue(x+1, mx, cRow)) +
                     (kernelx[2][0] * pixelValue(x-1, mx, nRow)) + (kernelx[2][1] * pixelValue(x, mx, nRow)) + (kernelx[2][2] * pixelValue(x+1, mx, nRow));

            int py = (kernely[0][0] * pixelValue(x-1, mx, pRow)) + (kernely[0][1] * pixelValue(x, mx, pRow)) + (kernely[0][2] * pixelValue(x+1, mx, pRow)) +
                     (kernely[1][0] * pixelValue(x-1, mx, cRow)) + (kernely[1][1] * pixelValue(x, mx, cRow)) + (kernely[1][2] * pixelValue(x+1, mx, cRow)) +
                     (kernely[2][0] * pixelValue(x-1, mx, nRow)) + (kernely[2][1] * pixelValue(x, mx, nRow)) + (kernely[2][2] * pixelValue(x+1, mx, nRow));

            float val = ceil(sqrt(float(px * px) + float(py * py)));
            int pval = val < 1.0 ? 255 : 0;

            tRow[x] = qRgb(pval, pval, pval);
        }
    }
}

int SobelFilter::pixelValue(int x, int mx, const QRgb *line)
{
    return qGray(line[qMin(qMax(0, x), mx - 1)]);
}
