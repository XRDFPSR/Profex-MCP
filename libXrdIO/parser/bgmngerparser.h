/***************************************************************************
                          bgmngerparser.h  -  description
                             -------------------
    begin                : Mon Feb 04 21:04:07 CEST 2019
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

#ifndef BGMNGERPARSER_H
#define BGMNGERPARSER_H

#include <QString>
#include <QMap>
#include <QStringList>
#include "../bgmnfileio.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif


class XRDIO_EXPORT BgmnGerParser
{
public:
    explicit BgmnGerParser(const QString &);

    double getWmin() const;
    double getWmax() const;
    QList<double> getZweiTheta() const;
    QStringList getHeader() const;
    QMap<double, QStringList> getPeaks() const;
    bool save(const QString &);

    inline void setHeader(const QStringList &l) {_header = l;}
    void addPeak(const QStringList &);
    void addPeaks(const QMap<double, QStringList> &);

private:
    QStringList _content;
    QString _file;
    QStringList _header;
    QMap<double, QStringList> _peaks;

    void parsePeakData();
};

#endif // BGMNGERPARSER_H
