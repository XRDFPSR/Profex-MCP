/***************************************************************************
                          bgmnresparser.h  -  description
                             -------------------
    begin                : Sat Aug 22 21:04:07 CEST 2015
    copyright            : (C) 2015 by Nicola Doebelin
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

#ifndef BGMNRESPARSER_H
#define BGMNRESPARSER_H

#include <QString>
#include <QStringList>
#include "../crystal/crystalstructure.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

class XRDIO_EXPORT BgmnResParser
{
public:
    explicit BgmnResParser(const QString &);

    inline CrystalStructure structure() const {return _structure;}

private:
    CrystalStructure _structure;
    QStringList _content;
    double PI;

    void readRes(const QString &);

    double bisoFromUiso(double uiso);
    double bisoFromUaniso(double alpha, double beta, double gamma,
                          double u11, double u22, double u33,
                          double u12, double u13, double u23);
};

#endif // BGMNRESPARSER_H
