/***************************************************************************
                          bgmnprotocolparser.cpp  -  description
                             -------------------
    begin                : Thu Jun 06 21:16:07 CEST 2019
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

#include "bgmnprotocolparser.h"
#include <QtMath>
#include <cmath>
#include "../libXrdIO/structs.h"
#include <QDebug>

BgmnProtocolParser::BgmnProtocolParser()
{
    bgmnFirstValue = 0.0;
    rexpM = 0;
    rexpP = 0;
    rexpDenom = 1.0;
}

void BgmnProtocolParser::setRexpDenom(int m, double d)
{
    rexpM = m;
    rexpDenom = d;
}

int BgmnProtocolParser::parse(const QString &s)
{
    QStringList outputChunk = s.split(global::rxLineEnding);
    int n = 0;

    for (int i = 0; i < outputChunk.size(); ++i) {
        n += parseBgmnOutput(outputChunk.at(i));
    }

    return n;
}

int BgmnProtocolParser::parseBgmnOutput(const QString &s)
{
    static QRegularExpression rxBgmnP("N=\\s*(\\d+)\\s+IT=\\s*\\d+");
    QRegularExpressionMatch rmBgmnP = rxBgmnP.match(s);

    if (rmBgmnP.hasMatch()) {
        rexpP = rmBgmnP.captured(1).toInt();
        return 0;
    }

    static QRegularExpression rxBgmnFirst("^\\s*0\\s+(\\d\\.\\d{6}E[\\+-]\\d{2})\\s*$");

    if (qFuzzyIsNull(bgmnFirstValue)) {
        QRegularExpressionMatch rmBgmnFirst = rxBgmnFirst.match(s);

        if (rmBgmnFirst.hasMatch()) {
            bgmnFirstValue = qSqrt(rmBgmnFirst.captured(1).toDouble());
            return 0;
        }
    }

    static QRegularExpression rxBgmn("^\\d+\\s+(\\d\\.\\d{6}E[\\+-]\\d{2})(?:\\s+\\d\\.\\d{3}E[\\+-]\\d{2}){2}\\s+\\d\\.\\d+\\s*$");
    QRegularExpressionMatch rmBgmn = rxBgmn.match(s);

    if (rmBgmn.hasMatch()) {
        double d = std::sqrt(rmBgmn.captured(1).toDouble());
        rwp = 100.0 * d / bgmnFirstValue;
        rex = 100.0 * std::sqrt(double(rexpM - rexpP) / rexpDenom);
        chi2 = rwp / rex;
        return 1;
    }

    return 0;
}

void BgmnProtocolParser::reset()
{
    bgmnFirstValue = 0.0;
    rexpM = 0;
    rexpP = 0;
    rexpDenom = 1.0;
}
