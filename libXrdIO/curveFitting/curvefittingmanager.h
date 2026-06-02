/***************************************************************************
                          curvefittingmanager.h  -  description
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

#ifndef CURVEFITTINGMANAGER_H
#define CURVEFITTINGMANAGER_H

#include <QtCore/QtGlobal>
#include <QMutex>
#include "genericcurve.h"
#include "../scan.h"
#include "../3rdparty/alglib/src/interpolation.h"
#include <atomic>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT CurveFitValue
{
public:
    explicit CurveFitValue();
    explicit CurveFitValue(const QString &n, double v, double e);
    ~CurveFitValue();

    inline void setName(const QString &s) {_name = s;}
    inline void setValue(double d)        {_value = d;}
    inline void setEsd(double d)          {_esd = d;}

    inline QString name() const {return _name;}
    inline double value() const {return _value;}
    inline double esd()   const {return _esd;}

private:
    QString _name;
    double _value;
    double _esd;
};

class XRDIO_EXPORT CurveFitValueList
{
public:
    CurveFitValueList();
    ~CurveFitValueList();

    inline int size() const {return _values.size();}
    inline void clear() {_values.clear();}
    inline void append(const CurveFitValue &v) {_values.append(v);}
    inline void append(const QString &s, double v, double e) {_values.append(CurveFitValue(s, v, e));}

    inline void setName(int i, const QString &s) {_values[i].setName(s);}
    inline void setValue(int i, double d) {_values[i].setValue(d);}
    inline void setEsd(int i, double d) {_values[i].setValue(d);}

    inline QString name(int i) const {return _values.at(i).name();}
    inline double value(int i) const {return _values.at(i).value();}
    inline double esd(int i) const {return _values.at(i).esd();}

    QStringList names() const;
    QList<double> values() const;
    QList<double> esds() const;

private:
    QList<CurveFitValue> _values;
};

class XRDIO_EXPORT CurveFittingManager
{
public:
    CurveFittingManager();
    ~CurveFittingManager();

    inline void setRangeName(const QString &s) {rangeName = s;}
    void setControls(alglib::ae_int_t m, double e, double d);
    void setScanData(const Scan &_scan);
    void setCurves(const QList<std::shared_ptr<GenericCurve>> &_clist);
    void setValues(const QStringList &_varNames, const QVector<double> &_initVal, const QVector<double> &_loLimits, const QVector<double> &_upLimits);
    QVector<double> fit();
    inline QMap<QString, QVariant> getReport() const {return fitReport;}
    QList<Scan> calculateCurve(double stepSize, const QString &sumName, const QUuid &sumUid);
    double getArea(int, QUuid &, bool &ok);
    inline int numCurves() const {return curvesList.size();}

    void abortFit()        { m_abort.store(true); }
    bool isAborted() const { return m_abort.load(); }

    static void stepOutput(const alglib::real_1d_array &, double, void *context);

private:
    QString rangeName;
    Scan dataRange;
    QStringList variableNames;
    QVector<double> initValues;
    QVector<double> lowerLimits;
    QVector<double> upperLimits;
    QMap<QString, QVariant> fitReport;
    alglib::ae_int_t maxits;
    double epsx;
    double diffstep;
    int iteration;
    QList<std::weak_ptr<GenericCurve>> curvesList;
    QMutex curvesListMutex;
    std::atomic<bool> m_abort{false};

    static void fitCurve(const alglib::real_1d_array &c, const alglib::real_1d_array &x, double &func, void *context);
    QMap<QString, QVariant> generateReportString(const QList<std::weak_ptr<GenericCurve> > &, const alglib::ae_int_t &, const alglib::lsfitreport &);
    QVector<double> getXvalues(double);
    void applyResults(CurveFitValueList &vlist, const QStringList &vnames, const alglib::real_1d_array &vals, const alglib::lsfitreport &rep);
    QVariantList correlationMatrix(const alglib::lsfitreport &rep);
};


#endif // CURVEFITTINGMANAGER_H
