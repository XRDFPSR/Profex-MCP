/***************************************************************************
                          bgmngerparser.cpp  -  description
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

#include "bgmngerparser.h"
#include <QDebug>

BgmnGerParser::BgmnGerParser(const QString &f)
{
    _file = f;
    _content = BgmnFileIO::readTextFileLines(_file);
    parsePeakData();
}

bool BgmnGerParser::save(const QString &f)
{
    QStringList out;

    out.append(getHeader());
    QMapIterator<double, QStringList> it(_peaks);
    while (it.hasNext()) {
        it.next();
        out.append(it.value());
    }

    return BgmnFileIO::writeTextFile(f, out.join("\n"));
}

void BgmnGerParser::addPeak(const QStringList &l)
{
    if (!l.size()) return;
    static QRegularExpression rx("THETA=(\\d+\\.?\\d*)\\s+N=\\d+.*");
    QRegularExpressionMatch rm = rx.match(l.first());

    if (rm.hasMatch()) {
        double theta = rm.captured(1).toDouble();
        _peaks[theta] = l;
    }
}

void BgmnGerParser::addPeaks(const QMap<double, QStringList> &m)
{
    QMapIterator<double, QStringList> it(m);
    while (it.hasNext()) {
        it.next();
        // we use this function because it makes us independent
        // of whether it.key() holds theta or 2theta
        addPeak(it.value());
    }
}

double BgmnGerParser::getWmin() const
{
    if (!_peaks.size()) return -1.0;
    return _peaks.firstKey();
}

double BgmnGerParser::getWmax() const
{
    if (!_peaks.size()) return -1.0;
    return _peaks.lastKey();
}

QList<double> BgmnGerParser::getZweiTheta() const
{
    QList<double> _zt;

    QMapIterator<double, QStringList> it(_peaks);
    while (it.hasNext()) {
        it.next();
        _zt.append(2.0 * it.key());
    }

    return _zt;
}

QStringList BgmnGerParser::getHeader() const
{
    return _header;
}

QMap<double, QStringList> BgmnGerParser::getPeaks() const
{
    return _peaks;
}

void BgmnGerParser::parsePeakData()
{
    static QRegularExpression rx("THETA=(\\d+\\.?\\d*)\\s+N=\\d+.*");
    QRegularExpressionMatch rm;

    _header.clear();
    _peaks.clear();

    for (int i = 0; i < _content.size(); ++i) {
        if (_content.at(i).left(6) != "THETA=") {
            _header.append(_content.at(i));
        } else {
            break;
        }
    }

    int n = _content.indexOf(rx, 0);

    while (n > 0) {
        bool ok = false;
        double theta = -1.0;
        QStringList pk(_content.at(n));

        rm = rx.match(_content.at(n));

        if (rm.hasMatch()) {
            theta = rm.captured(1).toDouble(&ok);
        }

        for (int i = n+1; i < _content.size(); ++i) {
            if (_content.at(i).left(6) == "THETA=") {
                break;
            } else {
                pk.append(_content.at(i));
            }
        }

        if (ok) _peaks[theta] = pk;
        n = _content.indexOf(rx, n+1);
    }
}
