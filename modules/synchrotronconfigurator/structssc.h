/***************************************************************************
                          structs.h  -  description
                             -------------------
    begin                : Sun Jan 19 12:48:00 CEST 2025
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

#ifndef STRUCTSSC_H
#define STRUCTSSC_H

#include <QString>
#include <QList>

namespace synchro {

enum ParameterKey {
    CHERNYSHOV_D,
    CHERNYSHOV_D_CHECKED,
    CHERNYSHOV_P,
    CHERNYSHOV_P_CHECKED,
    CHERNYSHOV_C,
    CHERNYSHOV_C_CHECKED,
    CHERNYSHOV_T,
    CHERNYSHOV_T_CHECKED,
    CHERNYSHOV_PHI,
    CHERNYSHOV_PHI_CHECKED,
    CHERNYSHOV_PHI_FOCUSED,
    CHERNYSHOV_SHAPE,
    CHERNYSHOV_ALPHA,

    RANGE_MEASURED_START,
    RANGE_MEASURED_END,
    RANGE_MEASURED_STEP,
    RANGE_PROFILE_WFACTOR,
    RANGE_PROFILE_START,
    RANGE_PROFILE_END,
    RANGE_PROFILE_STEP,

    POSITION_CORRECTION_MODE,
    WAVELENGTH_NM,
    DETECTOR_MATERIAL,
    DETECTOR_DENSITY,
    DETECTOR_LAC_MM,
    DETECTOR_TRANSPARENCY_CUTOFF,

    PROFILE_FWHM_RAD,
    PROFILE_FWHM_DEG,
    PROFILE_AREA,
    TWOTHETA_DEG,
    PROFILE_NUMBER,
    GEOMETRY
};

struct SupportPeak {
    int number;
    double position;
    double fwhm;
    double shape;

    SupportPeak(): number(-1), position(-1.0), fwhm(-1.0), shape(-1.0) {}
    SupportPeak(int _n, double _p, double _f, double _s): number(_n), position(_p), fwhm(_f), shape(_s) {}
};

struct L2Curve {
    double g;
    double e;
    double q;

    L2Curve(): g(-1.0), e(-1.0), q(-1.0) {}
    L2Curve(double _g, double _e, double _q): g(_g), e(_e), q(_q) {}
};

struct Profile {
    int number;
    double position;
    double fwhm;
    double shape;
    double area;
    QList<double> pseudoVoigtX;
    QList<double> pseudoVoigtY;
    QList<double> convolvedX;
    QList<double> convolvedY;
    QList<synchro::L2Curve> l2curves;

    Profile(): number(-1), position(-1.0), fwhm(-1.0), shape(-1.0), area(-1.0),
        pseudoVoigtX(QList<double>()), pseudoVoigtY(QList<double>()),
        convolvedX(QList<double>()), convolvedY(QList<double>()), l2curves(QList<synchro::L2Curve>()) {}

    Profile(int _n, double _p, double _f, double _s, double _a, QList<double> _pvX, QList<double> _pvY, QList<double> _convX, QList<double> _convY, QList<synchro::L2Curve> _l):
        number(_n), position(_p), fwhm(_f), shape(_s), area(_a),
        pseudoVoigtX(_pvX), pseudoVoigtY(_pvY), convolvedX(_convX), convolvedY(_convY), l2curves(_l) {}
};

}

#endif // STRUCTSSC_H
