/***************************************************************************
                          structs.h  -  description
                             -------------------
    begin                : Mon Oct 07 12:46:00 CEST 2014
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

#ifndef LUTSTRUCTS_H
#define LUTSTRUCTS_H

#include <QString>
#include <QRgb>
#include <QList>

namespace colorMaps {

/*
 * type = 0: no contour lines
 * type = 1: black lines on image background
 * type = 2: black lines on white background
 * type = 3: color lines on white background
 *
 * contour = 0: no contour lines
 * contour = 1: thin lines
 * contour = 2: bold lines
 */
struct Lut {
    QString name;
    QList<QRgb> data;
    int type;
    int contour;
    int bits;
    bool hasNegativeRange;

    Lut() : name(), data(), type(), contour() {}
    Lut(QString n, int t, int c) : name(n), type(t), contour(c) {}
    Lut(QString n, QList<QRgb> l, int t, int c) : name(n), data(l), type(t), contour(c) {}
};
} // end of namespace colorMaps

#endif // LUTSTRUCTS_H
