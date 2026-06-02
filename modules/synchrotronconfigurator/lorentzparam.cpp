/***************************************************************************
                          lorentzparam.cpp  -  description
                             -------------------
    begin                : Thu Jan 02 14:32:00 CEST 2025
    copyright            : (C) 2025 by Nicola Doebelin
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

#include "lorentzparam.h"

LorentzParam::LorentzParam()
    : _g(0.0), _e(0.0), _q(0.0)
{}

LorentzParam::LorentzParam(double g, double e, double q)
    : _g(g), _e(e), _q(q)
{}

/* class LorentzParams */

LorentzParams::LorentzParams()
    : _s(0.0), _t(0.0)
{}

LorentzParams::LorentzParams(double s, double t)
    : _s(s), _t(t)
{}

LorentzParams::LorentzParams(const QList<LorentzParam> &l, double s, double t)
    : QList<LorentzParam>(l), _s(s), _t(t)
{}

QList<double> LorentzParams::fktG() const
{
    QList<double> l;

    for (int i = 0; i < size(); ++i) {
        l.append(at(i).g());
    }

    return l;
}

QList<double> LorentzParams::fktE() const
{
    QList<double> l;

    for (int i = 0; i < size(); ++i) {
        l.append(at(i).e());
    }

    return l;
}

QList<double> LorentzParams::fktQ() const
{
    QList<double> l;

    for (int i = 0; i < size(); ++i) {
        l.append(at(i).q());
    }

    return l;
}
