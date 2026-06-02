/***************************************************************************
                          convolutiondata.cpp  -  description
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

#include <QVector>
#include "convolutiondata.h"
#include "3rdparty/alglib/src/ap.h"
#include "3rdparty/alglib/src/fasttransforms.h"

ConvolutionData::ConvolutionData()
{
}

QVector<double> ConvolutionData::convolute(const QVector<double> &f, const QVector<double> &h)
{
    int sizeF = f.size();
    int sizeH = h.size();

    if (!sizeF) {
        if (!sizeH) return QVector<double>();
        return h;
    }

    if (!sizeH) return f;

    QVector<double> r(sizeF + sizeH - 1, 0.0);

    alglib::real_1d_array mF;
    alglib::real_1d_array mH;
    alglib::real_1d_array mR;

    mF.setcontent(sizeF, f.data());
    mH.setcontent(sizeH, h.data());

    alglib::convr1d(mF, sizeF, mH, sizeH, mR);

    for (int i = 0; i < mR.length(); ++i) {
        r[i] = mR[i];
    }

    return r;
}
