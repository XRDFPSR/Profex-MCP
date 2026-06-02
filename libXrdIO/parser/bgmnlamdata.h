/***************************************************************************
                          bgmnlamdata.h  -  description
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

#ifndef LAMDATA_H
#define LAMDATA_H

#include <QObject>
#include <QtMath>
#include "bgmnlamparser.h"
#include "../libXrdIO/structs.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BgmnLamData
{
public:
    BgmnLamData();
    BgmnLamData(const BgmnLamData &);

    int load(const QString &);
    int parse(const QString &);

    // values in lambda [A]
    void computeCurvesLambda(const QVector<double> &);

    // values in 2theta [degrees] at position d [nm]
    void computeCurvesTwoTheta(const QVector<double> &, double);

    inline int count() const                  {return _curve.subCurveCount();}
    inline int size()  const                  {return _curve.subCurveCount();}
    inline bool hasData() const               {return _curve.subCurveCount() > 0;}

    inline global::ProfileL1SubCurve & pSubCurve(int i)      {return _curve.pSubCurve(i);}
    inline global::ProfileL1SubCurve & pSumCurve()           {return _curve.pSumCurve();}

    inline const global::ProfileL1SubCurve & pSubCurve(int i) const {return _curve.pSubCurve(i);}
    inline const global::ProfileL1SubCurve & pSumCurve()      const {return _curve.pSumCurve();}

    double wavelength() const;

    inline double xmin() const                {return _curve.xmin();}
    inline double xmax() const                {return _curve.xmax();}
    inline double ymin() const                {return _curve.ymin();}
    inline double ymax() const                {return _curve.ymax() * 1.05;}

    inline void setXLabel(const QString &s)   {_xlabel = s;}
    inline QString getXLabel() const          {return _xlabel;}

    // converts a wavelength [Angstrom] to the native unit [1/nm]
    inline double angstromToNative(double lamang)          const {return 10.0 / lamang;}

    // converts a twotheta angle [degrees] to the native unit [1/nm] for a specific d value in [nm]
    inline double twothetaToNative(double ttang, double d) const {return 1.0 / (2.0 * d * qSin(qDegreesToRadians(0.5 * ttang)));}

    inline double nativeToAngstrom(double laminvnm)           const {return 10.0 / laminvnm;}
    inline double nativeToTwoTheta(double laminvnm, double d) const {return 2.0 * qRadiansToDegrees(qAsin(1.0 / (2.0 * laminvnm * d)));}

private:
    L1Curve _curve;
    QString _xlabel;
};

#endif // LAMDATA_H
