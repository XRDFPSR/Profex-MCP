/***************************************************************************
                          curvehandler.cpp  -  description
                             -------------------
    begin                : Thu Jan 16 19:20:03 CEST 2020
    copyright            : (C) 2020 by Nicola Doebelin
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

#include "curvehandler.h"
#include "../libXrdIO/curveFitting/linearcurve.h"
#include "../libXrdIO/curveFitting/gaussiancurve.h"
#include "../libXrdIO/curveFitting/lorentziancurve.h"
#include "../libXrdIO/curveFitting/bgmnl2curve.h"
#include "../libXrdIO/curveFitting/pseudovoigtcurve.h"
#include "../libXrdIO/curveFitting/pearsoncurve.h"
#include "../libXrdIO/curveFitting/gaussiansplitcurve.h"
#include "../libXrdIO/curveFitting/lorentziansplitcurve.h"
#include "../libXrdIO/curveFitting/pseudovoigtsplitcurve.h"
#include "../libXrdIO/curveFitting/pearsonsplitcurve.h"
#include "../libXrdIO/curveFitting/quadraticcurve.h"
#include "../libXrdIO/curveFitting/cubiccurve.h"
#include "../libXrdIO/curveFitting/polynom4curve.h"

CurveHandler::CurveHandler()
{

}

std::shared_ptr<GenericCurve> CurveHandler::createCurve(int type)
{
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LINEAR)
        return std::make_shared<LinearCurve>();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::GAUSSIAN)
        return std::make_shared<GaussianCurve>();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LORENTZIAN)
        return std::make_shared<LorentzianCurve>();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PSEUDOVOIGT)
        return std::make_shared<PseudoVoigtCurve>();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PEARSONVII)
        return std::make_shared<PearsonCurve>();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::BGMNL2)
        return std::make_shared<BgmnL2Curve>();

    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::GAUSSIANSPLIT)
        return std::make_shared<GaussianSplitCurve>();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LORENTZIANSPLIT)
        return std::make_shared<LorentzianSplitCurve>();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PSEUDOVOIGTSPLIT)
        return std::make_shared<PseudoVoigtSplitCurve>();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PEARSONSPLIT)
        return std::make_shared<PearsonSplitCurve>();

    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::QUADRATIC)
        return std::make_shared<QuadraticCurve>();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::CUBIC)
        return std::make_shared<CubicCurve>();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::POLY4)
        return std::make_shared<Polynom4Curve>();

    return nullptr;  // If no match, return a valid empty shared_ptr
}

QString CurveHandler::description(int type)
{
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LINEAR)           return LinearCurve::descriptionStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::GAUSSIAN)         return GaussianCurve::descriptionStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LORENTZIAN)       return LorentzianCurve::descriptionStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PSEUDOVOIGT)      return PseudoVoigtCurve::descriptionStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PEARSONVII)       return PearsonCurve::descriptionStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::BGMNL2)           return BgmnL2Curve::descriptionStatic();

    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::GAUSSIANSPLIT)    return GaussianSplitCurve::descriptionStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LORENTZIANSPLIT)  return LorentzianSplitCurve::descriptionStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PSEUDOVOIGTSPLIT) return PseudoVoigtSplitCurve::descriptionStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PEARSONSPLIT)     return PearsonSplitCurve::descriptionStatic();

    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::QUADRATIC)        return QuadraticCurve::descriptionStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::CUBIC)            return CubicCurve::descriptionStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::POLY4)            return Polynom4Curve::descriptionStatic();
    return GenericCurve::descriptionStatic();
}

QString CurveHandler::equationText(int type)
{
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LINEAR)           return LinearCurve::equationTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::GAUSSIAN)         return GaussianCurve::equationTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LORENTZIAN)       return LorentzianCurve::equationTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PSEUDOVOIGT)      return PseudoVoigtCurve::equationTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PEARSONVII)       return PearsonCurve::equationTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::BGMNL2)           return BgmnL2Curve::equationTextStatic();

    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::GAUSSIANSPLIT)    return GaussianSplitCurve::equationTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LORENTZIANSPLIT)  return LorentzianSplitCurve::equationTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PSEUDOVOIGTSPLIT) return PseudoVoigtSplitCurve::equationTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PEARSONSPLIT)     return PearsonSplitCurve::equationTextStatic();

    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::QUADRATIC)        return QuadraticCurve::equationTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::CUBIC)            return CubicCurve::equationTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::POLY4)            return Polynom4Curve::equationTextStatic();
    return GenericCurve::equationTextStatic();
}

QStringList CurveHandler::parameterText(int type)
{
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LINEAR)           return LinearCurve::parameterTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::GAUSSIAN)         return GaussianCurve::parameterTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LORENTZIAN)       return LorentzianCurve::parameterTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PSEUDOVOIGT)      return PseudoVoigtCurve::parameterTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PEARSONVII)       return PearsonCurve::parameterTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::BGMNL2)           return BgmnL2Curve::parameterTextStatic();

    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::GAUSSIANSPLIT)    return GaussianSplitCurve::parameterTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::LORENTZIANSPLIT)  return LorentzianSplitCurve::parameterTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PSEUDOVOIGTSPLIT) return PseudoVoigtSplitCurve::parameterTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::PEARSONSPLIT)     return PearsonSplitCurve::parameterTextStatic();

    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::QUADRATIC)        return QuadraticCurve::parameterTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::CUBIC)            return CubicCurve::parameterTextStatic();
    if (static_cast<GenericCurve::CurveType>(type) == GenericCurve::POLY4)            return Polynom4Curve::parameterTextStatic();
    return GenericCurve::parameterTextStatic();
}
