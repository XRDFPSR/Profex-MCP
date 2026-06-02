/***************************************************************************
                          curvefittingmanager.cpp  -  description
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

#include "curvefittingmanager.h"
#include "3rdparty/alglib/src/interpolation.h"
#include <QDebug>

CurveFitValue::CurveFitValue()
{}

CurveFitValue::CurveFitValue(const QString &s, double v, double e)
    :_name(s), _value(v), _esd(e)
{}

CurveFitValue::~CurveFitValue()
{}

CurveFitValueList::CurveFitValueList()
{}

CurveFitValueList::~CurveFitValueList()
{}

QStringList CurveFitValueList::names() const
{
    QStringList l;
    l.resize(_values.size());

    for (int i = 0; i < _values.size(); ++i) {
        l[i] = _values.at(i).name();
    }

    return l;
}

QList<double> CurveFitValueList::values() const
{
    QList<double> l;
    l.resize(_values.size());

    for (int i = 0; i < _values.size(); ++i) {
        l[i] = _values.at(i).value();
    }

    return l;
}

QList<double> CurveFitValueList::esds() const
{
    QList<double> l;
    l.resize(_values.size());

    for (int i = 0; i < _values.size(); ++i) {
        l[i] = _values.at(i).esd();
    }

    return l;
}

CurveFittingManager::CurveFittingManager()
    : maxits(0), epsx(0.0), diffstep(0.0001)
{}

CurveFittingManager::~CurveFittingManager()
{}

void CurveFittingManager::setControls(alglib::ae_int_t m, double e, double d)
{
    maxits = m;
    epsx = e;
    diffstep = d;
}

void CurveFittingManager::setScanData(const Scan &_scan)
{
    dataRange = _scan;
    dataRange.pDataHkl().clear();
}

void CurveFittingManager::setCurves(const QList<std::shared_ptr<GenericCurve> > &_clist)
{
    curvesList.clear();

    for (const auto &curve : _clist) {
        curvesList.append(curve); // store as weak_ptr
    }
}

void CurveFittingManager::setValues(const QStringList &_varNames, const QVector<double> &_initVal, const QVector<double> &_loLimits, const QVector<double> &_upLimits)
{
    variableNames = _varNames;
    initValues    = _initVal;
    lowerLimits   = _loLimits;
    upperLimits   = _upLimits;
}

QVector<double> CurveFittingManager::fit()
{
    int numParams = 0;
    m_abort.store(false);

    for (int i = 0; i < curvesList.size(); ++i) {
        auto curve = curvesList.at(i).lock(); // lock weak_ptr to get shared_ptr
        if (!curve) continue;
        numParams += curve->nParameters();
    }

    if ((numParams != initValues.size()) || (numParams != lowerLimits.size()) || (numParams != upperLimits.size())) {
        qDebug() << QString("CurveFittingManager::fit(): Wrong parameter count (expected: %1 received: %2 / %3 / %4). Exiting.")
                        .arg(numParams)
                        .arg(initValues.size())
                        .arg(lowerLimits.size())
                        .arg(upperLimits.size());
        return QVector<double>();
    } else {
        qDebug() << QString("CurveFittingManager::fit(): Parameter count passed (expected and received %1).")
                        .arg(numParams);
    }

    iteration = 1;

    alglib::real_2d_array xdata;
    alglib::real_1d_array ydata;
    alglib::real_1d_array curveParams;
    alglib::real_1d_array bndl;
    alglib::real_1d_array bndu;

    curveParams.setlength(numParams);
    bndl.setlength(numParams);
    bndu.setlength(numParams);

    xdata.setcontent(dataRange.pDataAngle().size(), 1, dataRange.pDataAngle().data());
    ydata.setcontent(dataRange.pDataIntensity().size(), dataRange.pDataIntensity().data());
    curveParams.setcontent(initValues.size(), initValues.data());
    bndl.setcontent(lowerLimits.size(), lowerLimits.data());
    bndu.setcontent(upperLimits.size(), upperLimits.data());

    try {
        alglib::ae_int_t info;
        alglib::lsfitstate state;
        alglib::lsfitreport rep;

        alglib::lsfitcreatef(xdata, ydata, curveParams, diffstep, state);
        alglib::lsfitsetbc(state, bndl, bndu);
        alglib::lsfitsetcond(state, epsx, maxits);

        // use the overload whose callback returns a bool
        alglib::lsfitfit(state, fitCurve, stepOutput, this);

        // if we returned false from stepOutput, user aborted
        if (isAborted()) {
            qDebug() << "CurveFittingManager::fit(): aborted by user.";
            return QVector<double>();
        }

        alglib::lsfitresults(state, curveParams, rep);
        info = rep.terminationtype;

        if (int(info) < 0) {
            qDebug() << QString("CurveFittingManager::fit(): info is < 0, returning empty string");
            return QVector<double>();
        }

        CurveFitValueList vlist;
        applyResults(vlist, variableNames, curveParams, rep);

        initValues = vlist.values();
        fitReport = generateReportString(curvesList, info, rep);
        qDebug() << QString("CurveFittingManager::fit(): Returning %1 fitted parameters").arg(initValues.size());

        return initValues;
    } catch (const alglib::ap_error &e) {
        // Catch and handle the ALGLIB exception
        qDebug() << QString("CurveFittingManager::fit(): ALGLIB error: %1").arg(QString::fromStdString(e.msg));
        return QVector<double>();
    }
}

QVector<double> CurveFittingManager::getXvalues(double stepSize)
{
    QVector<double> xcalc;

    // if stepsize < 0, use the stepsize of the data range. Else use the
    // specified stepsize for the calculated curves
    if (stepSize < 0.0) {
        xcalc = dataRange.pDataAngle();
    } else {
        double curang = dataRange.pDataAngle().constFirst();
        double ss = qFuzzyIsNull(stepSize) ? 0.0001 : stepSize;

        xcalc = QVector<double>((dataRange.pDataAngle().constLast() - curang)/ss, 0.0);

        for (int i = 0; i < xcalc.size(); ++i) {
            xcalc[i] = curang;
            curang += ss;
        }
    }

    return xcalc;
}

void CurveFittingManager::fitCurve(const alglib::real_1d_array &c, const alglib::real_1d_array &x, double &func, void *context)
{
    // Retrieve the CurveFittingManager instance from the context pointer
    auto *manager = static_cast<CurveFittingManager *>(context);
    if (!manager) {
        qDebug() << "CurveFittingManager::fitCurve(): Invalid context pointer";
        return;
    }

    QMutexLocker locker(&manager->curvesListMutex); // Ensure thread safety

    func = 0.0;

    for (int i = 0; i < manager->curvesList.size(); ++i) {
        auto curve = manager->curvesList[i].lock(); // lock weak_ptr to get a shared_ptr
        if (!curve) continue;
        func += curve->fit(c, x);

    }
}

void CurveFittingManager::stepOutput(const alglib::real_1d_array &, double, void *context)
{
    auto *mgr = static_cast<CurveFittingManager*>(context);
    if (mgr->isAborted()) {
        throw alglib::ap_error("UserAbort");
    }
}

QList<Scan> CurveFittingManager::calculateCurve(double stepSize, const QString &sumName, const QUuid &sumUid)
{
    QVector<double> x = getXvalues(stepSize);

    alglib::real_1d_array c;
    c.setcontent(initValues.size(), initValues.data());

    QList<Scan> curves;
    QVector<double> sumY(x.size(), 0.0);

    for (int i = 0; i < curvesList.size(); ++i) {
        auto curve = curvesList.at(i).lock(); // lock weak_ptr to get shared_ptr
        QVector<double> y(x.size(), 0.0);
        curve->compute(c, x, y, sumY);

        Scan sc = dataRange.clone();
        sc.setUid(curve->uid());
        sc.setSourceFileName(QString());
        sc.setColor(QColor());
        sc.setName(curve->displayName());
        sc.setTypes(Scan::XY | Scan::SYNTHETIC | Scan::TEMPORARY);
        sc.setXoffset(0.0);
        sc.setDataAng(x);
        sc.setDataInt(y);

        curves.append(sc);
    }

    Scan sum = dataRange.clone();
    sum.setUid(sumUid);
    sum.setSourceFileName(QString());
    sum.setColor(QColor());
    sum.setName(QString("#S %1").arg(sumName));
    sum.setTypes(Scan::XY | Scan::SYNTHETIC | Scan::TEMPORARY);
    sum.setXoffset(0.0);
    sum.setDataAng(x);
    sum.setDataInt(sumY);

    curves.append(sum);
    return curves;
}

double CurveFittingManager::getArea(int n, QUuid &uid, bool &ok)
{
    if (n >= curvesList.size()) {
        ok = false;
        return -1.0;
    }

    ok = true;
    auto curve = curvesList.at(n).lock(); // lock weak_ptr to get shared_ptr
    if (!curve) return -1.0;

    uid = curve->uid();
    return curve->area();
}

QMap<QString, QVariant> CurveFittingManager::generateReportString(const QList<std::weak_ptr<GenericCurve>> &, const alglib::ae_int_t &i, const alglib::lsfitreport &r)
{
    QMap<QString, QVariant> rep;
    rep["RangeName"] = rangeName;
    rep["VariableNames"] = variableNames;

    int errorCode = int(i);
    QString errorString;

    switch (errorCode) {
    case -8:
        errorString = QString("optimizer detected NAN/INF");
        break;
    case -7:
        errorString = QString("gradient verification failed");
        break;
    case -3:
        errorString = QString("inconsistent restraints");
        break;
    case 2:
        errorString = QString("converged");
        break;
    case 5:
        errorString = QString("maximum iterations reached");
        break;
    case 7:
        errorString = QString("stopping conditions are too stringent");
        break;
    default:
        errorString = QString("unknown error code").arg(i);
    }

    rep["ExitCode"] = errorCode;
    rep["ExitString"] = errorString;
    rep["R2"] = r.r2;

    // Number of parameters
    int K = static_cast<int>(r.errpar.length());

    // Extract standard deviations into a QList<double>
    QVariantList stdDevs;
    stdDevs.reserve(K);

    for(int i = 0; i < K; ++i) {
        stdDevs.append(r.errpar[i]);
    }

    rep["StdDevs"] = stdDevs;
    rep["CorrMatrix"] = correlationMatrix(r);

    return rep;
}

void CurveFittingManager::applyResults(CurveFitValueList &vlist, const QStringList &vnames, const alglib::real_1d_array &vals, const alglib::lsfitreport &rep)
{
    vlist.clear();

    int k = qMin<int>(rep.errpar.length(), vals.length());

    for (int i = 0; i < k; ++i) {
        QString name = (i < vnames.size())
        ? vnames.at(i)
        : QString("p%1").arg(i+1);
        vlist.append(CurveFitValue(name, vals[i], rep.errpar[i]));
    }
}

QVariantList CurveFittingManager::correlationMatrix(const alglib::lsfitreport &rep)
{
    // Number of parameters
    int k = static_cast<int>(rep.errpar.length());

    // Build Pearson‑style correlation coefficients into a QList<QList<double>>
    QVariantList corrMatrix;
    corrMatrix.reserve(k);

    for(int i = 0; i < k; ++i) {
        QVariantList row;
        row.reserve(k);

        for(int j = 0; j < k; ++j) {
            double si = rep.errpar[i];
            double sj = rep.errpar[j];
            double cov = rep.covpar[i][j];
            double corr = 0.0;

            if(si > 0.0 && sj > 0.0) {
                corr = cov / (si * sj);
            }

            row.append(corr);
        }

        // the QVariant() wrapper is absolutely required, else it will merge all QVariantLists into one
        corrMatrix.append(QVariant(row));
    }

    return corrMatrix;
 }
