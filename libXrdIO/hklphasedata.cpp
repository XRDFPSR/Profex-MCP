/***************************************************************************
                          hklphasedata.cpp  -  description
                             -------------------
    begin                : Mon Feb 04, 2019
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

#include "hklphasedata.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QIODevice>
#include <QDataStream>
#include <algorithm>

HklPhaseData::HklPhaseData()
{
    _file      = QString();
    _md5hash   = QString();
    _sourceDir = QString();
    _comment   = QString();
    _phase     = QString();
    _color     = QString();
    _formula   = QString();
    _doIndex    = true;
    _favorite   = 0;
    _strongest  = -1.0;
    _strongest2 = -1.0;
    _strongest3 = -1.0;
    _longest    = -1.0;
    _longest2   = -1.0;
    _longest3   = -1.0;
    _density    = -1.0;
    _hklData = QByteArray();
    _auxData = QMap<QString, QVariant>();
    _xyData  = QByteArray();
}

HklPhaseData::HklPhaseData(const QString &f, const QString &md5, const QString &sdir,
                           const QString &com, const QString &ph, const QString &col, const QString &form,
                           int fav, double strA, double strB, double strC, double lngA, double lngB, double lngC,
                           double dns, const QByteArray &hkl, const QMap<QString, QVariant> &aux, const QByteArray &xy,
                           const QStringList &el, bool di)
    : _file(f), _md5hash(md5), _sourceDir(sdir), _comment(com), _phase(ph), _color(col), _formula(form),
      _favorite(fav), _strongest(strA), _strongest2(strB), _strongest3(strC), _longest(lngA), _longest2(lngB), _longest3(lngC), _density(dns),
      _hklData(hkl), _auxData(aux), _xyData(xy), _elements(el), _doIndex(di)
{}

HklPhaseData::HklPhaseData(const HklPhaseData &p)
    : _file(p._file), _md5hash(p._md5hash), _sourceDir(p._sourceDir), _comment(p._comment), _phase(p._phase), _color(p._color),
      _formula(p._formula), _favorite(p._favorite), _strongest(p._strongest), _strongest2(p._strongest2), _strongest3(p._strongest3),
      _longest(p._longest), _longest2(p._longest2), _longest3(p._longest3), _density(p._density),
      _hklData(p._hklData), _auxData(p._auxData), _xyData(p._xyData), _elements(p._elements), _doIndex(p._doIndex)
{}

HklPhaseData HklPhaseData::operator=(const HklPhaseData &h)
{
    _file = h._file;
    _md5hash = h._md5hash;
    _sourceDir = h._sourceDir;
    _comment = h._comment;
    _phase = h._phase;
    _color = h._color;
    _formula = h._formula;
    _favorite = h._favorite;
    _strongest = h._strongest;
    _strongest2 = h._strongest2;
    _strongest3 = h._strongest3;
    _longest = h._longest;
    _longest2 = h._longest2;
    _longest3 = h._longest3;
    _density = h._density;
    _doIndex = h._doIndex;
    _hklData = h._hklData;
    _auxData = h._auxData;
    _xyData = h._xyData;
    _elements = h._elements;
    return *this;
}

QVector<Hkl> HklPhaseData::hklData() const
{
    QVector<Hkl> vec;
    QJsonArray jarray(QJsonDocument::fromJson(_hklData).array());

    for (int i = 0; i < jarray.size(); ++i) {
        vec.append(Hkl(_phase, _color, jarray.at(i).toObject()));
    }

    return vec;
}

void HklPhaseData::setHklData(const QVector<Hkl> &vec)
{
    if (!vec.size()) {
        _hklData.clear();
        _strongest  = -1.0;
        _strongest2 = -1.0;
        _strongest3 = -1.0;
        _longest    = -1.0;
        _longest2   = -1.0;
        _longest3   = -1.0;
        return;
    }

    getDMetrics(vec, _strongest, _strongest2, _strongest3, _longest, _longest2, _longest3);

    QJsonArray jarray;
    for (int i = 0; i < vec.count(); ++i) {
        jarray.append(vec.at(i).getJsonData());
    }

    QJsonDocument jdoc;
    jdoc.setArray(jarray);
    _hklData = jdoc.toJson();
}

void HklPhaseData::setXyData(const QVector<QVector<double> > &v)
{
    _xyData.clear();

    if (v.size() < 2) return;
    double max = *std::max_element(v.at(1).constBegin(), v.at(1).constEnd());

    QByteArray ba;
    int n = qMin(v.at(0).size(), v.at(1).size());

    QDataStream stream(&ba, QIODevice::WriteOnly);

    for (int i = 0; i < n; ++i) {
        stream << v.at(0).at(i) << 100.0 * v.at(1).at(i) / max;
    }

    _xyData = qCompress(ba);
}

QVector<QVector<double> > HklPhaseData::xyData() const
{
    QVector<QVector<double> > vec(2, QVector<double>());
    if (_xyData.isEmpty()) return vec;

    double x, y;
    QByteArray ba = qUncompress(_xyData);
    QDataStream stream(&ba, QIODevice::ReadOnly);

    while (!stream.atEnd()) {
        stream >> x;
        stream >> y;
        vec[0].append(x);
        vec[1].append(y);
    }

    return vec;
}

void HklPhaseData::getDMetrics(const QVector<Hkl> &v, double &strA, double &strB, double &strC, double &lngA, double &lngB, double &lngC)
{
    if (!v.size()) {
        strA = -1.0;
        strB = -1.0;
        strC = -1.0;
        lngA = -1.0;
        lngB = -1.0;
        lngC = -1.0;
        return;
    }

    QMap<double, double> stro;
    QList<double> lgst;

    for (int i = 0; i < v.size(); ++i) {
        stro[v.at(i).intensity()]  = v.at(i).position();
        lgst.append(v.at(i).position());
    }

    std::sort(lgst.begin(), lgst.end());

    lngA = lgst.size() ? lgst.takeLast() : -1.0;
    lngB = lgst.size() ? lgst.takeLast() : -1.0;
    lngC = lgst.size() ? lgst.takeLast() : -1.0;

    strA = stro.size() ? stro.take(stro.lastKey()) : -1.0;
    strB = stro.size() ? stro.take(stro.lastKey()) : -1.0;
    strC = stro.size() ? stro.take(stro.lastKey()) : -1.0;
}
