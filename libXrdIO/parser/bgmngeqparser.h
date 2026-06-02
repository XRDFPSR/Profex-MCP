/***************************************************************************
                          bgmngeqparser.h  -  description
                             -------------------
    begin                : Feb 07 17:31:07 CEST 2018
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

#ifndef BGMNGEQPARSER_H
#define BGMNGEQPARSER_H

#include <QObject>
#include <QVector>
#include "../libXrdIO/structs.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

struct Header
{
    QString id;
    float ddivr;
    float tdivd;
    float tthetamin; // in degrees 2theta
    float tthetamax; // in degrees 2theta
    QString comment;
    bool tubeTails;

    Header(): id(), ddivr(), tdivd(), tthetamin(), tthetamax(), comment(), tubeTails() {}
    Header(QString _id, float _dd, float _td, float _tmi, float _tma, QString _cm, bool _tt)
        : id(_id), ddivr(_dd), tdivd(_td), tthetamin(_tmi), tthetamax(_tma), comment(_cm), tubeTails(_tt) {}
};


/**********************************************************************
 *    Class L2Curve
 *
 * The x-axis unit of L2 curves in the GEQ file is:
 *
 *   -theta [radians]
 *
 * This class provides functions to return the L2 curves on different
 * x-scales, including:
 *
 *   2theta [degrees]
 *   1/d    [1/Angstrom] for a given wavelength
 **********************************************************************/

class XRDIO_EXPORT L2Curve
{
public:
    explicit L2Curve() {}
    explicit L2Curve(double);

    void append(const global::ProfileL2SubCurve &);

    inline double sinTheta() const                  {return _sinTheta;}
    inline double gsum()     const                  {return _gsum;}

    // values in -theta [rad]
    double xmin() const;
    double xmax() const;

    inline int subCurveCount() const                {return _subCurves.size();}
    inline double ymin() const                      {return 0.0;}
    inline double ymax() const                      {return _ymax;}

    inline global::ProfileL2SubCurve & pSubCurve(int i)            {return _subCurves[i];}
    inline global::ProfileL2SubCurve & pSumCurve()                 {return _sumCurve;}

    inline const global::ProfileL2SubCurve & pSubCurve(int i) const {return _subCurves[i];}
    inline const global::ProfileL2SubCurve & pSumCurve()      const {return _sumCurve;}

    QVector<double> computeSubCurve(int, const QVector<double> &);
    void integrateSubCurves();

private:
    double _sinTheta;
    double _gsum;
    double _ymax;
    QVector<global::ProfileL2SubCurve> _subCurves;
    global::ProfileL2SubCurve _sumCurve;

    // x in -theta [rad]
    double l2value(double g, double e, double q, double x);
    double sqrt3(double) const;
};

/**********************************************************************
 * GEQ Parser Class
 **********************************************************************/

class XRDIO_EXPORT BgmnGeqParser
{
public:
    explicit BgmnGeqParser();
    explicit BgmnGeqParser(const QString &);
    void loadFile(const QString &);

    inline QVector<L2Curve> curves()   {return _l2curves;}
    inline bool hasTubeTails() const   {return _header.tubeTails;}

    bool mergeGeqFiles(const QStringList &inFiles, const QString &outFile);

private:
    Header _header;
    QVector<L2Curve> _l2curves;

    Header parseHeader(const QByteArray &);
    QVector<L2Curve> parseDataBlock(const QByteArray &);
};

#endif // BGMNGEQPARSER_H
