/***************************************************************************
                          genericcurve.h  -  description
                             -------------------
    begin                : Tue Nov 12 19:20:03 CEST 2019
    copyright            : (C) 2019 by Nicola Doebelin
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

#ifndef GENERICCURVE_H
#define GENERICCURVE_H

#include <QObject>
#include <QtCore/QtGlobal>
#include <QStringList>
#include <QVector>
#include <QUuid>
#include <math.h>
#include "../3rdparty/alglib/src/ap.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

class XRDIO_EXPORT GenericCurve
{
public:
    GenericCurve();
    ~GenericCurve();

    enum CurveType {
        DUMMY,
        LINEAR,
        GAUSSIAN,
        LORENTZIAN,
        PSEUDOVOIGT,
        PEARSONVII,
        GAUSSIANSPLIT,
        LORENTZIANSPLIT,
        PSEUDOVOIGTSPLIT,
        PEARSONSPLIT,
        QUADRATIC,
        CUBIC,
        POLY4,
        BGMNL2,
        FWHMCHERNYSHOV,
        FWHMCHERNYSHOVFOCUSED
    };

    virtual inline CurveType type() const {return DUMMY;}

    static inline QString descriptionStatic() {return "Dummy";}
    static inline QString equationTextStatic() {return QString();}
    static inline QStringList parameterTextStatic() {return QStringList();}

    virtual inline QString description() const {return descriptionStatic();}
    virtual inline QString equationText() const {return equationTextStatic();}
    virtual QStringList parameterText() const {return parameterTextStatic();}

    inline void setUid(const QUuid &u) {_uid = u;}
    inline QUuid uid() {return _uid;}
    virtual inline int nParameters() const {return 0;}
    virtual inline bool hasArea() const {return false;}
    inline double area() const {return _area;}

    virtual QStringList parameterNames() const {return QStringList();}
    virtual QList<double> defaultValues() const {return QList<double>();}

    /* provide range start, range end, and step size as arguments, because some functions
     * use them as limits */
    virtual QStringList lowerLimits(double, double, double) const {return QStringList();}
    virtual QStringList upperLimits(double, double, double) const {return QStringList();}

    inline void setIndices(const QVector<int> &v) {_idx = v;}
    inline QVector<int> indices() const           {return _idx;}

    inline void setDisplayName(const QString &s) {_displayName = s;}
    inline QString displayName() const {return _displayName;}

    /*
     * c:   array of curve parameters (can contain paramters of more than one curve)
     * x:   data array
     */
    virtual double fit(const alglib::real_1d_array &c, const alglib::real_1d_array &x);

    /*
     * c:   array of curve parameters (can contain paramters of more than one curve)
     * x:   data array
     * y:   resulting y values
     * sy:  sum of y values (resulting y values are added to values previously contained in sy)
     */
    virtual void compute(const alglib::real_1d_array &c, const QVector<double> &x, QVector<double> &y, QVector<double> &sy);

protected:
    QUuid _uid;
    QVector<int> _idx;
    double _area;
    QString _displayName;

    virtual double calculateArea(const alglib::real_1d_array &) const {return -1.0;}
};

#endif // GENERICCURVE_H
