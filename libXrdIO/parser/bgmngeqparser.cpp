/***************************************************************************
                          bgmngeqparser.cpp  -  description
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

#include <QtMath>
#include <cmath>
#include "bgmngeqparser.h"
#include "../bgmnfileio.h"

#define sizeFloat 4
#define sizeDouble 8
#define sizeUshort 2
#define sizeInt 4
#define sizeLong 4

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

BgmnGeqParser::BgmnGeqParser()
{
}

BgmnGeqParser::BgmnGeqParser(const QString &f)
{
    loadFile(f);
}

void BgmnGeqParser::loadFile(const QString &f)
{
    QByteArray _content;
    BgmnFileIO::readBinaryFile(f, _content);

    if (_content.size() < 144) {
        qDebug()  << QString("BgmnGeqParser::loadFile(): Not enough data found in file %1").arg(f);
        return;
    }

    _header = parseHeader(_content);
    _l2curves = parseDataBlock(_content);
}

Header BgmnGeqParser::parseHeader(const QByteArray &b)
{
    QString _id(b.mid(0, 8));
    bool _tt = QString(b.mid(5, 1)) == "2"; // this is part of the id. 2=tubetails, 1=no tubetails
    float _ddivr = BgmnFileIO::hex2float(b.mid(8, sizeFloat));
    float _tdivd = BgmnFileIO::hex2float(b.mid(12, sizeFloat));
    float _xmin = 2.0 * qRadiansToDegrees(qAsin(BgmnFileIO::hex2float(b.mid(16, sizeFloat))));
    float _xmax = 2.0 * qRadiansToDegrees(qAsin(BgmnFileIO::hex2float(b.mid(20, sizeFloat))));
    QString _comment(b.mid(24, 100));

    return Header(_id, _ddivr, _tdivd, _xmin, _xmax, _comment, _tt);
}

QVector<L2Curve> BgmnGeqParser::parseDataBlock(const QByteArray &b)
{
    QVector<L2Curve> _curves;
    int i = 124;

    while (i < b.size()) {
        float _sinx = BgmnFileIO::hex2float(b.mid(i, sizeFloat));
        i += sizeFloat;

        int _n = BgmnFileIO::hex2int(b.mid(i, sizeInt));
        i += sizeInt;

        QVector<double> g(_n, 0.0);
        QVector<double> e(_n, 0.0);
        QVector<double> q(_n, 0.0);

        for (int m = 0; m < _n; ++m) {
            g[m] = (double)BgmnFileIO::hex2float(b.mid(i, sizeFloat));
            i += sizeFloat;
        }

        for (int m = 0; m < _n; ++m) {
            e[m] = (double)BgmnFileIO::hex2float(b.mid(i, sizeFloat));
            i += sizeFloat;
        }

        for (int m = 0; m < _n; ++m) {
            q[m] = (double)BgmnFileIO::hex2float(b.mid(i, sizeFloat));
            i += sizeFloat;
        }

        L2Curve _l2c(_sinx);
        for (int i = 0; i < _n; ++i) {
            _l2c.append(global::ProfileL2SubCurve(g[i], e[i], q[i]));
        }
        _curves.append(_l2c);
    }

    return _curves;
}

bool BgmnGeqParser::mergeGeqFiles(const QStringList &inFiles, const QString &outFile)
{
    QList<QByteArray> _headers;
    QMap<double, QByteArray> _dataBlocks;
    float _sinxmin = 1.0;
    float _sinxmax = 0.0;

    for (int i = 0; i < inFiles.size(); ++i) {
        QByteArray _if;
        if (!BgmnFileIO::readBinaryFile(inFiles.at(i), _if)) {
            qDebug() << QString("BgmnGeqParser::mergeGeqFiles(): Could not read file %1, exiting.").arg(inFiles.at(i));
            return false;
        }

        _sinxmin = qMin(_sinxmin, BgmnFileIO::hex2float(_if.mid(16, sizeFloat)));
        _sinxmax = qMax(_sinxmax, BgmnFileIO::hex2float(_if.mid(20, sizeFloat)));
        _headers.append(_if.mid(0, 124));

        int step = 4 * sizeFloat + sizeInt;

        for (int p = 124; p < _if.size(); p += step) {
            float _sinx = BgmnFileIO::hex2float(_if.mid(p, sizeFloat));
            _dataBlocks.insert(_sinx, _if.mid(p, step));
        }
    }

    QFile file(outFile);
    if (!file.open(QIODevice::WriteOnly)) {
        qDebug() << QString("BgmnGeqParser::mergeGeqFiles(): Could not write to file %1, exiting.").arg(outFile);
        return false;
    }

    QDataStream out(&file);
    out << _headers.first().mid(0, 16) << _sinxmin << _sinxmax << _headers.first().mid(24, -1);

    QMapIterator<double, QByteArray> it(_dataBlocks);
    while (it.hasNext()) {
        it.next();
        out << it.value();
    }

    file.close();
    return true;
}

/**********************************************************************
 *    Class L2Curve
 **********************************************************************/

L2Curve::L2Curve(double _t)
{
    _gsum = 0.0;
    _sinTheta = _t;
    _ymax = 0.0;
}

void L2Curve::append(const global::ProfileL2SubCurve &sc)
{
    _subCurves.append(sc);
    _gsum += sc.g;
}

/*
 * x values in -theta [rad]
 */
QVector<double> L2Curve::computeSubCurve(int n, const QVector<double> &x)
{
    QVector<double> y(x.size(), 0.0);

    for (int i = 0; i < x.size(); ++i) {
        y[i] = l2value(_subCurves[n].g, _subCurves[n].e, _subCurves[n].q, x[i]);
    }

    return y;
}

void L2Curve::integrateSubCurves()
{
    QVector<double> ysum(_subCurves[0].y);

    for (int i = 1; i < _subCurves.size(); ++i) {
        for (int x = 0; x < _subCurves[i].y.size(); ++x) {
            ysum[x] += _subCurves[i].y[x];
            _ymax = qMax(_ymax, ysum[x]);
        }
    }

    _sumCurve.g = 0.0;
    _sumCurve.e = 0.0;
    _sumCurve.q = 0.0;
    _sumCurve.x = _subCurves[0].x;
    _sumCurve.y = ysum;
}

/*
 * borrowed from gertest.c
 */
double L2Curve::l2value(double g, double e, double q, double x)
{
    return 2.0 * g * std::pow(q, 3.0) / std::pow(std::pow(q, 2.0) + std::pow(e - x, 2.0) , 2.0);
}

/*
 * borrowed from gertest.c
 */
double L2Curve::sqrt3(double d) const
{
    if (qFuzzyIsNull(d)) return 0.0;
    return qExp(qLn(d) / 3.0);
}

double L2Curve::xmin() const
{
    double _epsmin = _subCurves[0].e;
    double _g, _e, _q;
    int n = _subCurves.size();

    for (int i = 0; i < n; ++i) {
        _g = _subCurves[i].g;
        _e = _subCurves[i].e;
        _q = _subCurves[i].q;
        _epsmin = qMin(_epsmin, _e - _q * sqrt3(100.0 * n * _g / _gsum));
    }

    return _epsmin;
}

double L2Curve::xmax() const
{
    double _epsmax = _subCurves[0].e;
    double _g, _e, _q;
    int n = _subCurves.size();

    for (int i = 0; i < n; ++i) {
        _g = _subCurves[i].g;
        _e = _subCurves[i].e;
        _q = _subCurves[i].q;
        _epsmax = qMax(_epsmax, _e + _q * sqrt3(100.0 * n * _g / _gsum));
    }

    return _epsmax;
}

