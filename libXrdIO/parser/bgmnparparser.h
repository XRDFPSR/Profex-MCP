/***************************************************************************
                          bgmnparparser.h  -  description
                             -------------------
    begin                : Oct 26 18:20:07 CEST 2013
    copyright            : (C) 2013 by Nicola Doebelin
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

#ifndef BGMNPARPARSER_H
#define BGMNPARPARSER_H

#include <QString>
#include <QStringList>
#include <QRegularExpression>
#include <QVector>
#include <QColor>
#include <QMap>
#include "bgmndelayedparser.h"
#include "../hkl.h"

#if defined XRDIO
 #define XRDIO_EXPORT Q_DECL_EXPORT
#else
 #define XRDIO_EXPORT Q_DECL_IMPORT
#endif

using namespace std;

struct ParLine {
    QByteArray byteArray;
    double waveLength;
    double polarization;

    ParLine() : byteArray(), waveLength(), polarization() {}
    ParLine(QByteArray _b, double _w, double _p) : byteArray(_b), waveLength(_w), polarization(_p) {}
};

class XRDIO_EXPORT BgmnParParser : public BgmnDelayedParser
{
public:
    explicit BgmnParParser();
    explicit BgmnParParser(const QString &, bool &ok);

    QVector<Hkl> getReflections(const QString &phame = QString());
    static QVector<double> getBackgroundCoefficients(const QString &);
    const QMap<QString, QVector<Hkl> > * reflections();

protected:
    bool privateLoad(const QString &) override;
    bool privateReload() override {return true;}
    QMap<QString, QVector<Hkl> > _data;

    static QVector<Hkl> parseDataLine(const ParLine &pl);
    static void sortByPhase(QMap<QString, QVector<Hkl> > &, const QVector<Hkl> &);
    double parseWavelength(const QByteArray &) const;
    double parsePolarization(const QByteArray &) const;
};

#endif // BGMNPARPARSER_H
