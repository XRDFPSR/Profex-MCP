/***************************************************************************
                          hkl.cpp  -  description
                             -------------------
    begin                : Wed Mar 04, 2009
    copyright            : (C) 2009 by Nicola Doebelin
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

#include <QDataStream>
#include "hkl.h"

Hkl::Hkl()
{
    _uid = QUuid::createUuid();
    _phase = QStringLiteral("");
    _col = Qt::black;
    _pos = 0.0;
    _hkl = "- - -";
    _vpos = 0;
    _intensity = 1.0;
    _texture = 1.0;
    _status = 1;
    _b1 = 0.0;
    _b2 = 0.0;
    _gsum = 1.0;
}

Hkl::Hkl(const Hkl &h)
    : _pos(h._pos), _vpos(h._vpos), _hkl(h._hkl), _phase(h._phase), _col(h._col),
      _intensity(h._intensity), _texture(h._texture), _splitter(h._splitter),
      _status(h._status), _b1(h._b1), _b2(h._b2), _uid(h._uid), _gsum(h._gsum)
{}

Hkl::Hkl(double pos, const QString &hkl, int vpos, const QString &ph, const QColor &c, double intens, double tex, int stat, double b1, double b2, double gs)
{
    _uid = QUuid::createUuid();
    _splitter = "!";
    _pos = pos;
    _hkl = hkl;
    _vpos = vpos;
    _phase = ph;
    _col = c;
    _intensity = intens;
    _texture = tex;
    _status = stat;
    _b1 = b1;
    _b2 = b2;
    _gsum = gs;

    // make sure the splitter used for string format does not appear in the phase name
    _phase.replace(_splitter, "_");
}

Hkl::Hkl(const QString &p, const QString &c, const QJsonObject &json)
{
    _uid = QUuid::createUuid();

    _phase     = p;
    _col.setNamedColor(c);

    QJsonValue jv_pos = json["position"];
    _pos       = jv_pos.isUndefined() ? 0.0 : jv_pos.toDouble();

    QJsonValue jv_hkl = json["hkl"];
    _hkl       = jv_hkl.isUndefined() ? QString() : jv_hkl.toString();

    QJsonValue jv_vpos = json["vposition"];
    _vpos      = jv_vpos.isUndefined() ? 0 : json["vposition"].toInt();

    QJsonValue jv_intens = json["intensity"];
    _intensity = jv_intens.isUndefined() ? 0.0 : json["intensity"].toDouble();

    QJsonValue jv_tex = json["texture"];
    _texture   = jv_tex.isUndefined() ? 1.0 : json["texture"].toDouble();

    QJsonValue jv_status = json["status"];
    _status    = jv_status.isUndefined() ? 1 : jv_status.toInt();

    QJsonValue jv_b1 = json["b1"];
    _b1        = jv_b1.isUndefined() ? 0.0 : jv_b1.toDouble();

    QJsonValue jv_k2 = json["k2"];
    _b1        = jv_k2.isUndefined() ? 0.0 : jv_k2.toDouble();

    QJsonValue jv_gsum = json["gsum"];
    _b1        = jv_gsum.isUndefined() ? 1.0 : jv_gsum.toDouble();
}

Hkl Hkl::operator=(const Hkl &h)
{
    _pos = h._pos;
    _vpos = h._vpos;
    _hkl = h._hkl;
    _phase = h._phase;
    _col = h._col;
    _intensity = h._intensity;
    _texture = h._texture;
    _splitter = h._splitter;
    _status = h._status;
    _b1 = h._b1;
    _b2 = h._b2;
    _uid = h._uid;
    _gsum = h._gsum;
    return *this;
}

QJsonObject Hkl::getJsonData() const
{
    QJsonObject json;
    json["position"]  = _pos;
    json["hkl"]       = _hkl;
    json["vposition"] = _vpos;
    json["intensity"] = _intensity;
    json["texture"]   = _texture;
    json["status"]    = _status;
    json["b1"]        = _b1;
    json["b2"]        = _b2;
    json["gsum"]      = _gsum;
    return json;
}
