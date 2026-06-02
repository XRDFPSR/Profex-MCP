/***************************************************************************
                          fouriersynth.cpp  -  description
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
#include "threadfouriersynth.h"
#include "emapstructs.h"
#include <QMutexLocker>
#include <QDebug>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

ThreadFourierSynth::ThreadFourierSynth(QObject *parent) :
    QThread(parent)
{
    zLevel = 0;
    volume = 0.0;
    resolutionX = 0;
    resolutionY = 0;
    resolutionZ = 0;
    mode = 0;
    vmap = Q_NULLPTR;
}

void ThreadFourierSynth::run()
{
    // check valid pointer
    if (!vmap) {
        return;
    }

    // check ranges
    if ((zLevel > vmap->size()) || (!hklList.size())) {
        return;
    }

    float dmax = 0.0;
    float dmin = 0.0;

    QVector<QVector<float> > face;

    for (int iy = 0; iy < resolutionY; ++iy) {      // loop y from 0 to RES
        QVector<float> line;
        for (int ix = 0; ix < resolutionX; ++ix) {  // loop x from 0 to RES
            // initialize rho for the voxel at position xyz
            float rho = 0.0;

            for (int i = 0; i < hklList.size(); ++i) { // all hkl indices in the refined file
                // check if enough variables are available
                if (hklList.at(i).size() < 6) continue;

                // read variables from hklList
                // using doubles because the cos function later requires doubles
                double h  = hklList[i][0];         // h index
                double k  = hklList[i][1];         // k index
                double l  = hklList[i][2];         // l index
                double Fo = hklList[i][3];         // Fo, amplitude of observed structure factor
                double Fc = hklList[i][4];         // Fc, amplitude of calculated structure factor
                double f  = hklList[i][5] * M_PI / 180.0; // f, phase of calculated structure factor, converted to radians

                double x = double(ix)     / double(resolutionX);    // fractional coordinate x
                double y = double(iy)     / double(resolutionY);    // fractional coordinate y
                double z = double(zLevel) / double(resolutionZ);    // fractional coordinate z

                // calculate rho (electron density) for xyz and index hkl
                if (mode == 0) rho += float(Fc        * cos(2.0 * M_PI * (h*x + k*y + l*z) - f));
                if (mode == 1) rho += float(Fo        * cos(2.0 * M_PI * (h*x + k*y + l*z) - f));
                if (mode == 2) rho += float((Fo - Fc) * cos(2.0 * M_PI * (h*x + k*y + l*z) - f));
            }

            // write rho normalized to the cell volume to the map
            line.append(rho / volume);

            // determine the maximum and minimum rho, for normalization to 1 later
            dmax = qMax(rho / volume, dmax);
            dmin = qMin(rho / volume, dmin);
        }
        face.append(line);
    }


    QMutex mutex;
    QMutexLocker locker(&mutex);
    (*vmap)[zLevel] = face;

    emit newDmax(dmax);
    emit newDmin(dmin);
}

void ThreadFourierSynth::setResolutionLevel(int res_x, int res_y, int res_z, int z)
{
    resolutionX = res_x;
    resolutionY = res_y;
    resolutionZ = res_z;
    zLevel = z;
}
