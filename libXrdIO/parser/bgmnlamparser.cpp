/***************************************************************************
                          bgmnlamparser.cpp  -  description
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

#include "bgmnlamparser.h"
#include "bgmnfileio.h"
#include "structs.h"
#include <QFile>
#include <QTextStream>
#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QtMath>
#include <QDebug>


BgmnLamParser::BgmnLamParser()
{
}

BgmnLamParser::BgmnLamParser(const QString &s)
{
    load(s);
}

bool BgmnLamParser::load(const QString &s)
{
    _content = BgmnFileIO::readTextFileLines(s);
    return parseContent() > 0;
}

bool BgmnLamParser::parse(const QString &s)
{
    _content = s.split(global::rxLineEnding);
    return parseContent() > 0;
}

int BgmnLamParser::parseContent()
{
    _wavelengths.clear();
    _curve.clear();

    QString pFloat("(-?\\d+\\.?\\d*E?\\+?-?\\d*)");
    QString pIf("(?:\\s*\\*ifthenelse\\(.*\\))?");
    QString pComment("(?:\\s*%(.*))?");

    QRegularExpression rxData(QString("^\\s*%1%2\\s+%1\\s+%1%3\\s*$").arg(pFloat).arg(pIf).arg(pComment));
    QRegularExpressionMatch rmData;

    for (int i = 0; i < _content.size(); ++i) {
        rmData = rxData.match(_content.at(i));
        if (rmData.hasMatch()) {
            QStringList l(rmData.capturedTexts());

            if (l.size() > 3) {
                double g = l.at(1).toDouble();
                double e = l.at(2).toDouble();
                double q = l.at(3).toDouble();
                QString lbl = QString("ILAM=%1").arg(_curve.subCurveCount() + 1);

                if (!qFuzzyIsNull(e) && !qFuzzyIsNull(q)) {
                    if (l.size() > 4) {
                        _wavelengths[l.at(4).trimmed()] = 10.0 / e;
                        lbl = l.at(4).trimmed();
                    }

                    _curve.append(lbl, g, e, q);
                }
            }
        }
    }

    return _wavelengths.size();
}

QMap<QString, double> BgmnLamParser::getWavelengths()
{
    return _wavelengths;
}

L1Curve BgmnLamParser::getCurve()
{
    return _curve;
}

/**************************************************************
 *    Class L1Curve
 **************************************************************/

L1Curve::L1Curve()
{
    _ymax = 0.0;
}

void L1Curve::append(QString l, double g, double e, double q)
{
    _subCurves.append(global::ProfileL1SubCurve(l, g, e, q));
}

void L1Curve::clear()
{
    _subCurves.clear();
    _ymax = 0.0;
}

double L1Curve::xmin() const
{
    if (!_subCurves.size()) return 0.0;

    double _mi = _subCurves[0].e;

    for (int i = 0; i < _subCurves.size(); ++i) {
        _mi = qMin(_mi, _subCurves[i].e - 25.0 * _subCurves[i].q);
    }

    return _mi;
}

double L1Curve::xmax() const
{
    if (!_subCurves.size()) return 0.0;

    double _ma = _subCurves[0].e;

    for (int i = 0; i < _subCurves.size(); ++i) {
        _ma = qMax(_ma, _subCurves[i].e + 25.0 * _subCurves[i].q);
    }

    return _ma;
}

QVector<double> L1Curve::computeSubCurve(int n, const QVector<double> &x)
{
    int j = x.size();

    QVector<double> v(j, 0.0);
    if (n > _subCurves.size()) return v;

    for (int i = 0; i < j; ++i) {
        v[i] = l1value(_subCurves[n].g, _subCurves[n].e, _subCurves[n].q, x[i]);
        _ymax = qMax(_ymax, v[i]);
    }

    return v;
}

void L1Curve::integrateSubCurves()
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

double L1Curve::l1value(double g, double e, double q, double x)
{
    return g * q / (qPow(q, 2.0) + pow(e - x, 2.0));
}
