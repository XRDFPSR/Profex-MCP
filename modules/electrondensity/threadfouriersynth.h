/***************************************************************************
                          fouriersynth.h  -  description
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

#ifndef THREADFOURIERSYNTH_H
#define THREADFOURIERSYNTH_H

#include <QThread>
#include <QVector>
#include "emapstructs.h"

class ThreadFourierSynth : public QThread
{
    Q_OBJECT
public:
    explicit ThreadFourierSynth(QObject *parent = 0);

    inline void setMode(int f)                                     {mode = f;}
    void setResolutionLevel(int res_x, int res_y, int res_z, int z);
    inline void setVoxelMap(EDataMap *v) {vmap = v;}
    inline void setHklList(const QVector<QVector<float> > &h)      {hklList = h;}
    inline void setVolume(float v)                                 {volume = v;}

private:
    float volume;
    int zLevel;
    int resolutionX, resolutionY, resolutionZ;
    int mode;
    QVector< QVector<float> > hklList;
    EDataMap *vmap;

    void run();

signals:
    void newDmax(float);
    void newDmin(float);
};

#endif // THREADFOURIERSYNTH_H
