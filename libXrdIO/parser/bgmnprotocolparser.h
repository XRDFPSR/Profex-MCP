/***************************************************************************
                          bgmnprotocolparser.h  -  description
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


#ifndef BGMNPROTOCOLPARSER_H
#define BGMNPROTOCOLPARSER_H

#include <QRegularExpression>

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BgmnProtocolParser
{
public:
    explicit BgmnProtocolParser();

    int parse(const QString &);
    void setRexpDenom(int, double);

    inline double getRwp() const        {return rwp;}
    inline double getRexp() const       {return rex;}
    inline double getFirst() const      {return bgmnFirstValue;}
    inline int getNumParameters() const {return rexpP;}

    void reset();

private:
    double bgmnFirstValue;
    int rexpM;
    int rexpP;
    double rexpDenom;

    double rwp;
    double rex;
    double chi2;

    int parseBgmnOutput(const QString &);
};

#endif // BGMNPROTOCOLPARSER_H
