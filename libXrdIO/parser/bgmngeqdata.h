/***************************************************************************
                          bgmngeqdata.h  -  description
                             -------------------
    begin                : Tue Feb 08 21:10:00 CEST 2018
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

#ifndef GEQDATA_H
#define GEQDATA_H

#include <QObject>
#include <QtMath>
#include "bgmngeqparser.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BgmnGeqData
{
public:
    BgmnGeqData();
    BgmnGeqData(const BgmnGeqData &); // copy constructor

    int load(const QString &);

    // x-values in -theta [rad]
    void computeCurvesNative(const QVector<double> &);

    // x-values in 2theta [deg]
    void computeCurvesTwoTheta(const QVector<double> &);

    inline int count() const {return _curves.size();}
    inline int size()  const {return _curves.size();}
    inline bool hasData() const {return _curves.size() > 0;}

    int subCurveCount(int) const;

    inline global::ProfileL2SubCurve & pSubCurve(int i, int j)      {return _curves[i].pSubCurve(j);}
    inline global::ProfileL2SubCurve & pSumCurve(int i)             {return _curves[i].pSumCurve();}

    inline const global::ProfileL2SubCurve & pSubCurve(int i, int j) const {return _curves[i].pSubCurve(j);}
    inline const global::ProfileL2SubCurve & pSumCurve(int i)        const {return _curves[i].pSumCurve();}

    double sinTheta(int) const;

    inline double xmin(int i) const           {return _curves[i].xmin();}
    inline double xmax(int i) const           {return _curves[i].xmax();}
    inline double ymin(int i) const           {return _curves[i].ymin();}
    double ymax(int i) const;

    inline void setXLabel(const QString &s)   {_xlabel = s;}
    inline QString getXLabel() const          {return _xlabel;}

    inline double twoThetaToGeqNative(double tt)                const {return -qDegreesToRadians(tt / 2.0);}
    inline double geqNativeToTwoTheta(double negrad)            const {return -qRadiansToDegrees(negrad) * 2.0;}

private:
    QVector<L2Curve> _curves;
    double _ymax;
    QString _xlabel;
};

#endif // GEQDATA_H
