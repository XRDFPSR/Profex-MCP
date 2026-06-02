/***************************************************************************
                          convolutiondata.h  -  description
                             -------------------
    begin                : Tue Feb 12 21:10:00 CEST 2018
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

#ifndef CONVOLUTIONDATA_H
#define CONVOLUTIONDATA_H

#include <QObject>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT ConvolutionData
{
public:
    ConvolutionData();

    static QVector<double> convolute(const QVector<double> &f, const QVector<double> &h);
};

#endif // CONVOLUTIONDATA_H
