/***************************************************************************
                          bgmnsampledata.h  -  description
                             -------------------
    begin                : Wed May 23 21:35:00 CEST 2018
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

#ifndef BGMSAMPLEDATA_H
#define BGMSAMPLEDATA_H

#include <QObject>
#include <QVector>
#include <QtMath>
#include "../libXrdIO/structs.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

class XRDIO_EXPORT BgmnSampleData
{
public:
    BgmnSampleData();

    inline bool hasData() const  {return _curveLc.x.size() > 0;}

    // values in 1/d [1/nm]
    void computeCurvesNative(const QVector<double> &);

    // values in 2theta [degrees] at position d [nm]
    void computeCurvesTwoTheta(const QVector<double> &, double);

    inline global::ProfileL12PCurve & pCurveL1()      {return _curveL1;}
    inline global::ProfileL12PCurve & pCurveL2()      {return _curveL2;}
    inline global::ProfileL12PCurve & pCurveLc()      {return _curveLc;}

    inline const global::ProfileL12PCurve & pCurveL1()     const {return _curveL1;}
    inline const global::ProfileL12PCurve & pCurveL2()     const {return _curveL2;}
    inline const global::ProfileL12PCurve & pCurveLc()     const {return _curveLc;}

    double getCurveWidthL1(double);
    double getCurveWidthL2(double);

    void setCurveParameters(double b1, double k1, double k2, double di, double wl);

    double twothetaToNative(double ttang, double lam) const;
    double nativeToTwoTheta(double dinv, double lam) const;

private:
    global::ProfileL12PCurve _curveL1;
    global::ProfileL12PCurve _curveL2;
    global::ProfileL12PCurve _curveLc;
    QString _xlabel;

    void computeLc();

    double l1Value(double b1, double x0, double x);
    double l2Value(double b1, double k1, double k2, double d, double x);
    QVector<double> xConvoluted(const QVector<double> &, const QVector<double> &);
};

#endif // BGMSAMPLEDATA_H
