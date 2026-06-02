/***************************************************************************
                          bgmnlamparser.h  -  description
                             -------------------
    begin                : Apr 07 18:43:07 CEST 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#ifndef BGMNLAMPARSER_H
#define BGMNLAMPARSER_H

#include <QString>
#include <QStringList>
#include <QMap>
#include <QVector>
#include "../libXrdIO/structs.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif


/**********************************************************************
 *    Class L1Curve
 *
 * The x-axis unit of L2 curves in the LAM file is:
 *
 *   1/lambda [1/nm]
 *
 * All calculations are done on that axis, conversions to other
 * x-axis scales must be done elsewhere
 **********************************************************************/

class XRDIO_EXPORT L1Curve
{
public:
    explicit L1Curve();

    void append(QString, double, double, double);
    void clear();
    inline int subCurveCount() const            {return _subCurves.size();}

    double xmin() const;
    double xmax() const;
    inline double ymin() const                  {return 0.0;}
    inline double ymax() const                  {return _ymax;}

    inline global::ProfileL1SubCurve & pSubCurve(int i)        {return _subCurves[i];}
    inline global::ProfileL1SubCurve & pSumCurve()             {return _sumCurve;}

    inline const global::ProfileL1SubCurve & pSubCurve(int i) const {return _subCurves[i];}
    inline const global::ProfileL1SubCurve & pSumCurve()      const {return _sumCurve;}

    QVector<double> computeSubCurve(int, const QVector<double> &);
    void integrateSubCurves();

private:
    double _ymax;
    QVector<global::ProfileL1SubCurve> _subCurves;
    global::ProfileL1SubCurve _sumCurve;
    double l1value(double g, double e, double q, double x);

};

/**********************************************************************
 * LAM Parser class
 **********************************************************************/

class XRDIO_EXPORT BgmnLamParser
{
public:
    BgmnLamParser();
    BgmnLamParser(const QString &);

    bool load(const QString &);
    bool parse(const QString &);
    QMap<QString, double> getWavelengths();
    L1Curve getCurve();

private:
    QStringList _content;
    QMap<QString, double> _wavelengths;
    L1Curve _curve;

    int parseContent();
};

#endif // BGMNLAMPARSER_H
