/***************************************************************************
                          scan.cpp  -  description
                             -------------------
    begin                : Mon Jun 24 14:16:07 CEST 2005
    copyright            : (C) 2005 by Nicola Doebelin
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

#include "scan.h"
#include "structs.h"
#include "functions.h"
#include <QtMath>
#include <QDebug>
#include <math.h>
#include <algorithm>
#include <random>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

Scan::Scan(const QString &name, const QColor &col, int linewdth)
{
    _uid = QUuid::createUuid();
    _name = name;
    _overrideName = QString();
    _sourceFile = QString();
    _comment = QString();
    _xAxisLabel = "Diffraction Angle [" + QString(global::degree) + "2" + QString(global::theta) + "]";
    _yAxisLabel = "Intensity [counts]";
    _hklXunit = "tt";

    _flags = ScanTypes();

    _dataInt = QVector<double>();
    _dataAng = QVector<double>();
    _dataHkl = QVector<Hkl>();

    _color = col;

    _offset_x = 0.0;
    _offset_y = 0.0;
    _scale_factor = 1.0;
    _wavelength = 0.0;
    _wavelength2 = 0.0;
    _wavelength3 = 0.0;
    _stepsize = 0.02;
    _tPerStep = -1.0;
    _minIntens = 0.0;
    _maxIntens = 0.0;

    _is_visible = true;
    _is_active = false;
    _linewidth = uint(linewdth);
    _pointmode = -1;
    _hklmode = 0;

    _auxInfo = QVariantHash();
    _displayPosition = std::numeric_limits<uint>::max();
}

Scan::Scan(const Scan &sc) :
    _uid(sc._uid),
    _name(sc._name),
    _overrideName(sc._overrideName),
    _sourceFile(sc._sourceFile),
    _comment(sc._comment),
    _xAxisLabel(sc._xAxisLabel),
    _yAxisLabel(sc._yAxisLabel),
    _hklXunit(sc._hklXunit),

    _flags(sc._flags),

    _dataInt(sc._dataInt),
    _dataAng(sc._dataAng),
    _dataHkl(sc._dataHkl),

    _color(sc._color),

    _offset_x(sc._offset_x),
    _offset_y(sc._offset_y),
    _scale_factor(sc._scale_factor),
    _wavelength(sc._wavelength),
    _wavelength2(sc._wavelength2),
    _wavelength3(sc._wavelength3),
    _stepsize(sc._stepsize),
    _tPerStep(sc._tPerStep),
    _minIntens(sc._minIntens),
    _maxIntens(sc._maxIntens),

    _is_visible(sc._is_visible),
    _is_active(sc._is_active),

    _linewidth(sc._linewidth),
    _pointmode(sc._pointmode),
    _hklmode(sc._hklmode),

    _displayPosition(sc._displayPosition),
    _auxInfo(sc._auxInfo)
{
}

Scan Scan::operator=(const Scan &sc)
{
    _uid = sc._uid;
    _name = sc._name;
    _overrideName = sc._overrideName;
    _sourceFile = sc._sourceFile;
    _comment = sc._comment;
    _xAxisLabel = sc._xAxisLabel;
    _yAxisLabel = sc._yAxisLabel;
    _hklXunit = sc._hklXunit;

    _flags = sc._flags;

    _dataInt = sc._dataInt;
    _dataAng = sc._dataAng;
    _dataHkl = sc._dataHkl;

    _color = sc._color;

    _offset_x = sc._offset_x;
    _offset_y = sc._offset_y;
    _scale_factor = sc._scale_factor;
    _wavelength = sc._wavelength;
    _wavelength2 = sc._wavelength2;
    _wavelength3 = sc._wavelength3;
    _stepsize = sc._stepsize;
    _tPerStep = sc._tPerStep;
    _minIntens = sc._minIntens;
    _maxIntens = sc._maxIntens;

    _is_visible = sc._is_visible;
    _is_active = sc._is_active;

    _linewidth = sc._linewidth;
    _pointmode = sc._pointmode;
    _hklmode = sc._hklmode;

    _displayPosition = sc._displayPosition;
    _auxInfo = sc._auxInfo;
    return *this;
}

Scan Scan::clone() const
{
    Scan clone = *this;
    clone.setUid(QUuid::createUuid());
    return clone;
}

Scan::~Scan()
{}

bool Scan::isEmpty() const
{
    return ((_dataInt.size() == 0) && (_dataAng.size() == 0) && (_dataHkl.size() == 0));
}

QString Scan::name(bool withFileName) const
{
    if (withFileName) {
        if (_overrideName.isEmpty()) {
            QFileInfo fi(_sourceFile);
            if (!fi.exists()) {
                return _name;
            } else {
                return _name + " (" + fi.fileName() + ")";
            }
        } else {
            return _overrideName;
        }
    }

    if (_overrideName.isEmpty()) {
        return _name;
    }

    return _overrideName;
}

double Scan::stepSize() const
{
  if (!_dataInt.size()) return 0.0;
  if (!_dataAng.size()) return 0.0;

  double start = startAngle();
  double end   = endAngle();

  if (end == 0.0)   return 0.0;
  if (end <= start) return 0.0;

  return double(end - start) / double(_dataAng.size() - 1);
}

double Scan::at(int i) const
{
    if ((i < 0) || (i >= _dataInt.size())) return 0.0;
    return _dataInt.at(i);
}

double Scan::intensity(int i) const
{
    return at(i);
}

double Scan::intensity(double a) const
{
    double d = 0.0;

    int n = indexOfAngle(a, 0);
    if (n < 0) return d;

    return n < _dataInt.size() ? _dataInt.at(n) : 0.0;
}

double Scan::minIntensity(double start, double end) const
{
    if (!_dataInt.size()) return -1.0;

    int st = indexOfAngle(qMin(start, end), 1);
    int ed = indexOfAngle(qMax(start, end), 2);

    st = st < 0 ? 0 : st;
    ed = ed < 0 ? _dataInt.size() - 1 : ed;
    ed = ed >= _dataInt.size() ? _dataInt.size() - 1 : ed;

    return *std::min_element(_dataInt.constBegin() + st, _dataInt.constBegin() + ed);
}

double Scan::minIntensity() const
{
    if (!_dataInt.size()) return -1.0;
    return *std::min_element(_dataInt.constBegin(), _dataInt.constEnd());
}

double Scan::maxIntensity(double start, double end) const
{
    if (!_dataInt.size()) return -1.0;

    int st = indexOfAngle(qMin(start, end), 1);
    int ed = indexOfAngle(qMax(start, end), 2);

    st = st < 0 ? 0 : st;
    ed = ed < 0 ? _dataInt.size() - 1 : ed;
    ed = ed >= _dataInt.size() ? _dataInt.size() - 1 : ed;

    return *std::max_element(_dataInt.constBegin() + st, _dataInt.constBegin() + ed);
}

double Scan::maxIntensity() const
{
    if (!_dataInt.size()) return -1.0;
    return *std::max_element(_dataInt.constBegin(), _dataInt.constEnd());
}

double Scan::startAngle() const
{
    if (!_dataAng.size()) return -1.0;
    return *std::min_element(_dataAng.constBegin(), _dataAng.constEnd());
}

double Scan::endAngle() const
{
    if (!_dataAng.size()) return -1.0;
    return *std::max_element(_dataAng.constBegin(), _dataAng.constEnd());
}

double Scan::angleOfMaxIntensity() const
{
    if (!_dataAng.size()) return -1.0;
    int n = indexOfMaxIntensity();
    if (n < 0) return -1.0;
    return _dataAng.at(n);
}

int Scan::indexOfMaxIntensity() const
{
    if (!_dataInt.size()) return -1;

    double y = 0.0;
    int x = 0;

    for (int i = 0; i < _dataInt.size(); ++i) {
        if (_dataInt.at(i) > y) {
            y = _dataInt.at(i);
            x = i;
        }
    }

    return x;
}

double Scan::angleOfMaxIntensity(double start, double end) const
{
    if (!_dataAng.size()) return -1.0;
    int n = indexOfMaxIntensity(start, end);
    if (n < 0) return -1.0;
    return _dataAng.at(n);
}

int Scan::indexOfMaxIntensity(double start, double end) const
{
    if (!_dataInt.size()) return -1;
    int st = indexOfAngle(qMin(start, end), 1);
    int ed = indexOfAngle(qMax(start, end), 2);

    st = st < 0 ? 0 : st;
    ed = ed < 0 ? _dataInt.size() - 1 : ed;
    ed = ed >= _dataInt.size() ? _dataInt.size() - 1 : ed;

    double y = 0.0;
    int x = 0;

    for (int i = st; i < ed; ++i) {
        if (_dataInt.at(i) > y) {
            y = _dataInt.at(i);
            x = i;
        }
    }

    return x;
}

double Scan::angleOfMinIntensity() const
{
    if (!_dataAng.size()) return -1.0;
    int n = indexOfMinIntensity();
    if (n < 0) return -1.0;
    return _dataAng.at(n);
}

int Scan::indexOfMinIntensity() const
{
    if (!_dataInt.size()) return -1;

    double y = std::numeric_limits<double>::max();
    int x = 0;

    for (int i = 0; i < _dataInt.size(); ++i) {
        if (_dataInt.at(i) < y) {
            y = _dataInt.at(i);
            x = i;
        }
    }

    return x;
}

double Scan::angleOfMinIntensity(double start, double end) const
{
    if (!_dataAng.size()) return -1.0;
    int n = indexOfMinIntensity(start, end);
    if (n < 0) return -1.0;
    return _dataAng.at(n);
}

int Scan::indexOfMinIntensity(double start, double end) const
{
    if (!_dataInt.size()) return -1;
    int st = indexOfAngle(qMin(start, end), 1);
    int ed = indexOfAngle(qMax(start, end), 2);

    st = st < 0 ? 0 : st;
    ed = ed < 0 ? _dataInt.size() - 1 : ed;
    ed = ed >= _dataInt.size() ? _dataInt.size() - 1 : ed;

    double y = std::numeric_limits<double>::max();
    int x = 0;

    for (int i = st; i < ed; ++i) {
        if (_dataInt.at(i) < y) {
            y = _dataInt.at(i);
            x = i;
        }
    }

    return x;
}

double Scan::hklMaxIntensity() const
{
    double iMax = 0.0;

    for (int i = 0; i < _dataHkl.size(); ++i) {
        iMax = qMax(iMax, _dataHkl.at(i).intensity());
    }

    return iMax;
}

double Scan::angle(int i) const
{
    if (i < 0) return 0.0;
    if (i >= _dataAng.size()) return 0.0;

    return _dataAng[i];
}

int Scan::indexOfHkl(const QUuid &u) const
{
    if (u.isNull()) return -1;

    for (int i = 0; i < _dataHkl.size(); ++i) {
        if (_dataHkl.at(i).uid() == u) return i;
    }

    return -1;
}

void Scan::insertHkl(const Hkl &h)
{
    pDataHkl().append(h);
}

bool Scan::removeHkl(const QUuid &u)
{
    if (u.isNull()) return false;

    for (int i = 0; i < _dataHkl.size(); ++i) {
        if (_dataHkl.at(i).uid() == u) {
            _dataHkl.remove(i);
            return true;
        }
    }

    return false;
}

bool Scan::removeHkl(int i)
{
    if (i < 0) return false;
    if (i >= _dataHkl.count()) return false;
    _dataHkl.remove(i);
    return true;
}

Hkl * Scan::getHkl(const QUuid &u)
{
    int i = indexOfHkl(u);
    if (i >= 0) return &(_dataHkl[i]);
    return nullptr;
}

Hkl * Scan::getHkl(int i)
{
    if ((i >= 0) && (i < _dataHkl.size())) return &(_dataHkl[i]);
    return nullptr;
}

const Hkl * Scan::getHkl(const QUuid &u) const
{
    int i = indexOfHkl(u);
    if (i >= 0) return &(_dataHkl[i]);
    return nullptr;
}

const Hkl * Scan::getHkl(int i) const
{
    if ((i >= 0) && (i < _dataHkl.size())) return &(_dataHkl[i]);
    return nullptr;
}

double Scan::getHklMaxIntensity() const
{
    double d = 0.0;

    for (int i = 0; i < _dataHkl.size(); ++i) {
        d = qMax(d, _dataHkl.at(i).intensity());
    }

    return d;
}

void Scan::setAllHklDisplayStatus(int prev, int cur)
{
    for (int i = 0; i < _dataHkl.size(); ++i) {
        if ((_dataHkl.at(i).status() == prev) || (prev < 0)) {
            _dataHkl[i].setStatus(cur);
        }
    }
}

/*
 * returns the scan's data as a map
 */
QMap<double, double> Scan::dataMap(bool withOperations) const
{
    QMap<double, double> map;

    bool offX = withOperations ? !qFuzzyIsNull(_offset_x) : false;
    bool offY = withOperations ? !qFuzzyIsNull(_offset_y) : false;
    bool sclY = withOperations ? !qFuzzyCompare(_scale_factor, 1.0) : false;

    for (int i = 0; i < qMin(_dataAng.size(), _dataInt.size()); ++i) {
        double x = offX ? _dataAng.at(i) + _offset_x : _dataAng.at(i);
        double y = sclY ? _dataInt.at(i) * _scale_factor : _dataInt.at(i);
        if (offY) y += _offset_y;

        map[x] = y;
    }

    return map;
}

/*
 * returns the index in vector _dataAng of the value v.
 * if no data point at v is found, it will return the index depending
 * on mode:
 * mode = 0: returns the nearest index (round)
 * mode = 1: returns the next lower index (floor)
 * mode = 2: returns the next greater index (ceil)
 *
 * if the angle v is out of range of the scan, return -1;
 */
int Scan::indexOfAngle(double v, int mode) const
{
    if (!_dataAng.size())     return -1;
    if (v < _dataAng.first()) return -1;
    if (v > _dataAng.last())  return -1;

    QVector<double>::const_iterator it;
    it = std::upper_bound(_dataAng.constBegin(), _dataAng.constEnd(), v);


    int idxU = it - _dataAng.constBegin(); // points to the element right of the element with value v
    int idxL = idxU - 1;                   // points to the element left of the element with value v

    if (mode == 2) {
        if (idxU >= _dataAng.size()) return -1;
        if (idxU < 0)                return -1;
        return idxU;
    }

    if (mode == 1) {
        if (idxL >= _dataAng.size()) return -1;
        if (idxL < 0)                return -1;
        return idxL;
    }

    if (mode == 0) {
        if (idxU >= _dataAng.size()) return -1;
        if (idxL < 0)                return -1;

        double dLow = abs(v - _dataAng.at(idxL));
        double dUp  = abs(v - _dataAng.at(idxU));

        return dLow < dUp ? idxL : idxU;
    }

    return -1;
}

Scan Scan::mid(double a, double b) const
{
    Scan out = *this;
    out.setUid(QUuid::createUuid());
    out.pDataAngle().clear();
    out.pDataIntensity().clear();

    int idxA = indexOfAngle(a, 1);
    int idxB = indexOfAngle(b, 2);

    if (idxA < 0) idxA = 0;
    if (idxB < 0) idxB = size() - 1;

    if (idxB == idxA) return out;

    int idxStart = qMin(idxA, idxB);
    int idxEnd   = qMax(idxA, idxB);

    out.pDataAngle()     = _dataAng.mid(idxStart, idxEnd - idxStart);
    out.pDataIntensity() = _dataInt.mid(idxStart, idxEnd - idxStart);

    return out;
}

void Scan::reset()
{
    _name = QString();
    _overrideName = QString();
    _comment = QString();
    _dataInt.clear();
    _dataAng.clear();
    _offset_x = 0.0;
    _offset_y = 0.0;
    _scale_factor = 1.0;
    _is_visible = true;
    _linewidth = 1;
    _auxInfo.clear();
}

QVariant Scan::auxInfo(const QString &s, bool &ok) const
{
    if (_auxInfo.contains(s)) {
        ok = true;
        return _auxInfo.value(s);
    } else {
        ok = false;
        return QVariant();
    }
}

void Scan::setNamedWaveLength(const QString &s)
{
    // values taken from Hoelzer et al., Phys. Rev. A56 (1997)
    // and Bearden, Rev. Mod. Phys 39(1) (1967)

    if (s.isEmpty()) {
        return;
    }

    _wavelength = 0.0;
    _wavelength2 = 0.0;
    _wavelength3 = 0.0;

    if (s.toLower() == "cu") { // Hoelzer
        _wavelength = 1.540598;
        _wavelength2 = 1.544426;
        _wavelength3 = 1.392250;
    }
    if (s.toLower() == "cr") { // Hoelzer
        _wavelength = 2.289760;
        _wavelength2 = 2.293663;
        _wavelength3 = 2.084920;
    }
    if (s.toLower() == "fe") { // Bearden
        _wavelength = 1.936042;
        _wavelength2 = 1.93998;
        _wavelength3 = 1.75661;
    }
    if (s.toLower() == "co") { // Hoelzer
        _wavelength = 1.789010;
        _wavelength2 = 1.792900;
        _wavelength3 = 1.620830;
    }
    if (s.toLower() == "ni") { // Bearden
        _wavelength = 1.65791;
        _wavelength2 = 1.661747;
        _wavelength3 = 1.48862;
    }
    if (s.toLower() == "mo") { // Hoelzer
        _wavelength = 0.709319;
        _wavelength2 = 0.713609;
        _wavelength3 = 0.632305;
    }
    if (s.toLower() == "ag") { // Bearden
        _wavelength = 0.5594075;
        _wavelength2 = 0.563798;
        _wavelength3 = 0.497069;
    }
    if (s.toLower() == "w") { // Bearden
        _wavelength = 0.20901;
        _wavelength2 = 0.213828;
        _wavelength3 = 0.184374;
    }
}

/*
 *  adapted from BGMN apx63.c
 */
QPair<int, double> Scan::getRexpDenom(double wmin, double wmax, bool tubeTails)
{
    double _wmin = wmin < 0.0 ? _dataAng.first() : wmin;
    double _wmax = wmax < 0.0 ? _dataAng.last()  : wmax;
    double vorwahlwert = _tPerStep > 0.0 ? _tPerStep : 1.0;
    bool cps = !qFuzzyCompare(vorwahlwert, 1.0);

    QVector<double> _dataI(_dataAng.size(), 0.0);
    QVector<double> _dataW(_dataAng.size(), 0.0);

    for (int i = 0; i < _dataInt.size(); ++i) {
        double counts = _dataInt.at(i);

        if (cps) { // using "Vorwahlart=T" formula from apx63.c
            counts *= vorwahlwert;
            _dataI[i] = (counts > 0.5 ? counts + 1.0 : counts) / vorwahlwert;
            _dataW[i] = vorwahlwert / sqrt(counts + 1.0);
        } else { // using FXY without E equation from apx63.c
            _dataI[i] = (counts > 0.5 ? counts + 1.0 : counts);
            _dataW[i] = 1.0 / sqrt(counts + 1.0);
        }
    }    

    _dataW = korrw(_dataW, tubeTails);

    double sum = 0.0;
    int m = 0;

    for (int i = 0; i < _dataAng.size(); ++i) {
        if (_dataAng.at(i) < _wmin) continue;
        if (_dataAng.at(i) > _wmax) break;

        sum += pow(_dataW.at(i) * _dataI.at(i), 2.0);
        ++m;
    }

    return QPair<int, double>(m, sum);
}

/*
 *  adapted from BGMN apx63.c
 */
QVector<double> Scan::getScanWeighing(bool tubeTails)
{
    double vorwahlwert = _tPerStep > 0.001 ? _tPerStep : 1.0;
    bool cps = !qFuzzyCompare(vorwahlwert, 1.0);

    QVector<double> _dataI(_dataAng.size(), 0.0);
    QVector<double> _dataW(_dataAng.size(), 0.0);

    for (int i = 0; i < _dataInt.size(); ++i) {
        double counts = _dataInt.at(i);

        if (cps) { // using "Vorwahlart=T" formula from apx63.c
            counts *= vorwahlwert;
            _dataI[i] = (counts > 0.5 ? counts + 1.0 : counts) / vorwahlwert;
            _dataW[i] = vorwahlwert / sqrt(counts + 1.0);
        } else { // using FXY without E equation from apx63.c
            _dataI[i] = (counts > 0.5 ? counts + 1.0 : counts);
            _dataW[i] = 1.0 / sqrt(counts + 1.0);
        }
    }

    return korrw(_dataW, tubeTails);
}

/*
 * Computation of the intensity weighed w.
 * Adapted from BGMN source file apx63.c.
 *
 * using variable names from apx63.c
 */
QVector<double> Scan::korrw(const QVector<double> &w, bool tubeTails)
{
    double PEAK_WIDTH = 0.5;
    double PRECISION_PEAKTAILS = tubeTails ? 0.007 : 0.02;
    int m = w.size();
    int j0 = 0;
    double vorwahlwert = _tPerStep > 0.001 ? _tPerStep : 1.0;
    double variation = 0.0;

    QVector<double> wout(m, 0.0);

    for(int i = 0; i < m; ++i) {
        double thetai = _dataAng.at(i);

        while(thetai - 5.0 * PEAK_WIDTH > _dataAng.at(j0)) {
            ++j0;
        }

        variation = 0.0;

        double intensitaet_i = _dataInt.at(i) * vorwahlwert;
        intensitaet_i = (intensitaet_i > 0.5 ? intensitaet_i + 1.0 : intensitaet_i) / vorwahlwert;

        for(int j = j0; (j < m) && (thetai + 5.0 * PEAK_WIDTH > _dataAng.at(j)); ++j) {
            double thetaj = _dataAng.at(j);

            double intensitaet_j = _dataInt.at(j) * vorwahlwert;
            intensitaet_j = (intensitaet_j > 0.5 ? intensitaet_j + 1.0 : intensitaet_j) / vorwahlwert;

            double x = 0.5 * pow((thetai - thetaj) / PEAK_WIDTH, 2.0);
            double y = exp(-x) * (intensitaet_j - intensitaet_i);

            if (variation < y) variation = y;
        }

        double invw = 1.0 / w.at(i);
        if (invw < PRECISION_PEAKTAILS * variation) {
            invw = PRECISION_PEAKTAILS * variation;
        }

        wout[i] = 1.0 / invw;
    }

    return wout;
}

bool Scan::hasHklData() const
{
    return (_dataHkl.size() > 0);
}

bool Scan::hasScanData() const
{
    return ((_dataAng.size() > 0) && (_dataInt.size() > 0));
}

QVector<double> Scan::dataDspacing(bool &ok) const
{
    QVector<double> d_data(_dataAng.size(), 0.0);

    if (qFuzzyIsNull(_wavelength)) {
        qDebug() << QString("Scan::dataDspacing: no wavelength, returning zero data");
        ok = false;
        return d_data;
    }

    for (int i = 0; i < _dataAng.size(); ++i) {
        d_data[i] = global::Functions::twoThetaToD(_dataAng.at(i), _wavelength);
    }

    ok = true;
    return d_data;
}

bool Scan::isTemporary() const
{
    return _flags.testFlag(Scan::TEMPORARY);
}

void Scan::addNoise()
{
    std::random_device rd;
    std::mt19937 gen(rd());

    for (int i = 0; i < _dataInt.size(); ++i) {
        std::poisson_distribution<int> distribution(_dataInt.at(i));
        _dataInt[i] = distribution(gen);
    }
}

void Scan::setActive(bool b)
{
     _is_active = b;
}

void Scan::applyOffsetsPermanently()
{
     if (!qFuzzyIsNull(_offset_x)) {
        for (int i = 0; i < _dataAng.size(); ++i) {
            _dataAng[i] += _offset_x;
        }

        _offset_x = 0.0;
     }

     // scale before applying y-offset
     if (!qFuzzyCompare(_scale_factor, 1.0)) {
        for (int i = 0; i < _dataInt.size(); ++i) {
            _dataInt[i] *= _scale_factor;
        }

        _scale_factor = 1.0;
     }

     if (!qFuzzyIsNull(_offset_y)) {
        for (int i = 0; i < _dataInt.size(); ++i) {
            _dataInt[i] += _offset_y;
        }

        _offset_y = 0.0;
     }
}

QPointF Scan::first() const
{
    return QPointF(_dataAng.first(), _dataInt.first());
}

QPointF Scan::last() const
{
    return QPointF(_dataAng.last(), _dataInt.last());
}

QPointF Scan::point(int i, bool &b) const
{
    if ((i >= 0) && (i < qMin(_dataInt.size(), _dataAng.size()))) {
        b = true;
        return QPointF(_dataAng.at(i), _dataInt.at(i));
    }

    b = false;
    return QPointF();
}


/** EOF **/

