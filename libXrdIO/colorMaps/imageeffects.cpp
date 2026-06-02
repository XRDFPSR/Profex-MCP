/***************************************************************************
                          imageeffects.h  -  description
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

#include "imageeffects.h"
#include "sobelfilter.h"
#include "math.h"
#include <QDebug>
#include <QColor>
#include <QImage>
#include <QVector>
#include <QThread>
#include <QPainter>

ImageEffects::ImageEffects()
{
}

void ImageEffects::contourLinesBlackOnColor(QImage *image, int iterations)
{
    if (!image) return;

    QPainter p(image);
    p.setCompositionMode(QPainter::CompositionMode_Multiply);
    p.drawImage(image->rect(), renderContourLines(image, iterations));
}

void ImageEffects::contourLinesBlackOnWhite(QImage *image, int iterations)
{
    if (!image) return;

    QPainter p(image);
    p.drawImage(image->rect(), renderContourLines(image, iterations));
}

void ImageEffects::contourLinesColor(QImage *image, int iterations)
{
    if (!image) return;

    QPainter p(image);
    p.setCompositionMode(QPainter::CompositionMode_Lighten);
    p.drawImage(image->rect(), renderContourLines(image, iterations));
}

QImage ImageEffects::renderContourLines(const QImage *image, int iterations)
{
    if (!image) return QImage();

    QImage contours(image->width(), image->height(), QImage::Format_ARGB32);
    contours.fill(qRgba(255, 255, 255, 0));

    QList<SobelFilter *> threadList;
    QMutex mutex;

    int nthreads = QThread::idealThreadCount();
    double l = double(image->height()) / double(nthreads);

    for (int i = 0; i < nthreads; ++i) {
        int start = int(double(i) * l + 0.5);
        int end   = int(double(i+1) * l + 0.5);
        threadList.append(new SobelFilter(&mutex));
        threadList.last()->setRange(start, end, image, &contours);
        threadList.last()->start();
    }

    for (int i = 0; i < threadList.size(); ++i) {
        threadList.at(i)->wait(20000);
    }

    // additional iterations of the sobel filter to make the lines bolder
    for (int iter = 1; iter < iterations; ++iter) {
        QImage oldContours(contours);
        for (int i = 0; i < nthreads; ++i) {
            int start = int(double(i) * l + 0.5);
            int end   = int(double(i+1) * l + 0.5);
            threadList.at(i)->setRange(start, end, &oldContours, &contours);
            threadList.at(i)->start();
        }

        for (int i = 0; i < threadList.size(); ++i) {
            threadList.at(i)->wait(20000);
        }
    }

    while (threadList.size()) {
        delete threadList.takeLast();
    }

    return contours;
}
