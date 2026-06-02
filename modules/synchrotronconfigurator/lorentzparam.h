/***************************************************************************
                          lorentzparam.h  -  description
                             -------------------
    begin                : Sat Dec 28 09:32:00 CEST 2024
    copyright            : (C) 2024 by Nicola Doebelin
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

#ifndef LORENTZPARAM_H
#define LORENTZPARAM_H

#include <QList>

enum GEOMETRY{REFLEXION, TRANSMISSION, CAPILLARY};

class LorentzParam {
public:
    LorentzParam();
    LorentzParam(double, double, double);

    void setParameters(double, double, double);

    inline void setG(double d) {_g = d;}
    inline void setE(double d) {_e = d;}
    inline void setQ(double d) {_q = d;}

    inline double g() const {return _g;}
    inline double e() const {return _e;}
    inline double q() const {return _q;}

private:
    double _g; // Height
    double _e; // Position in rad
    double _q; // Width in rad
};

class LorentzParams : public QList<LorentzParam>
{
public:
    LorentzParams();
    LorentzParams(double t, double s);
    LorentzParams(const QList<LorentzParam> &, double t, double s);

    inline void setS(double s) {_s = s;}
    inline void setT(double t) {_t = t;}

    QList<double> fktG() const;
    QList<double> fktE() const;
    QList<double> fktQ() const;
    inline double s() const {return _s;}
    inline double t() const {return _t;}

private:
    double _s; // Scale factor
    double _t; // Theta in degrees
};

#endif // LORENTZPARAM_H
