/***************************************************************************
                          bgmnparparser.cpp  -  description
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

#include "bgmnparparser.h"
#include "bgmnfileio.h"
#include "functions.h"
#include <math.h>
#include <QFile>
#include <QTextStream>
#include <QDebug>
#include <QtConcurrentMap>

#ifndef M_PI
    #define M_PI acos(-1.0)
#endif

BgmnParParser::BgmnParParser()
    : BgmnDelayedParser()
{
}

BgmnParParser::BgmnParParser(const QString &s, bool &ok)
    : BgmnDelayedParser(s, ok)
{
    ok = load(s);
}

bool BgmnParParser::privateLoad(const QString &s)
{
    if (!QFile::exists(s)) {
        qDebug() << QString("BgmnParParser::load(): File %1 doesnt exist").arg(s);
        return false;
    }

    QList<QByteArray> baLst;
    if (!readSafely(s, baLst)) {
        return false;
    }

    if (baLst.size() < 2) {
        return false;
    }

    double _wavelength = parseWavelength(baLst.first());
    double _polarization = parsePolarization(baLst.first());

    QList<ParLine> plLst;
    for (int i = 0; i < baLst.size(); ++i) {
        plLst.append(ParLine(baLst.at(i), _wavelength, _polarization));
    }

    QList<ParLine>::const_iterator itStart = plLst.constBegin();
    QList<ParLine>::const_iterator itEnd   = plLst.constEnd();
    ++itStart; // skip title line
    --itEnd;   // skip background coefficients

    QFuture<QMap<QString, QVector<Hkl> > > f = QtConcurrent::mappedReduced(itStart, itEnd, parseDataLine, sortByPhase, QtConcurrent::OrderedReduce | QtConcurrent::SequentialReduce);
    f.waitForFinished();
    _data = f.result();

    return (_data.size() > 0);
}

QVector<Hkl> BgmnParParser::parseDataLine(const ParLine &pl)
{
    if (pl.byteArray.size() < 33) return QVector<Hkl>();

    // hard coded positions
    double hk    = pl.byteArray.mid(2, 14).trimmed().toDouble();  // this variable is called hk in bgmn source code
    double sk    = pl.byteArray.mid(17, 10).trimmed().toDouble(); // called sk in bgmn source code. corresponds to 1/d
    double dpos  = qFuzzyIsNull(sk) ? 0.0 : 1.0 / sk;
    double sinx2 = std::pow(0.5 * sk * pl.waveLength, 2.0);

    double b1        = 0.0;
    double b2        = 0.0;
    double gsum      = 1.0;
    QString phase;

    static QRegularExpression rxProfile("([234]).{27}([\\d\\.]+)?\\s+([\\d\\.]+)?");
    static QRegularExpression rxPhase("(?:GSUM=([\\d\\.]+)\\s+)?(?:PHASE=([^ ]+))");
    static QRegularExpression rxHkl("H=\\d+\\s+(?:TEXTUR=([\\d\\.]+)\\s+)?(-?\\d+ -?\\d+ -?\\d+)");

    QRegularExpressionMatch rm = rxProfile.match(pl.byteArray);

    if (rm.hasMatch()) {
        int rp = rm.captured(1).toInt();
        if (rp > 2) b1 = rm.captured(2).toDouble();
        // the documentation is unclear about the definition of b2, sometimes this parameter
        // is defined as b2^2 (PDF manual, output.exe), and sometimes as b2 (website, context help).
        // here we stick with assuming the par file contains b2^2
        if (rp > 3) b2 = std::sqrt(rm.captured(3).toDouble());
        // if (rp > 3) b2 = rm.captured(3).toDouble();
    }

    QVector<Hkl> vec;
    int idx = 27;
    rm = rxPhase.match(pl.byteArray, idx);

    if (rm.hasMatch()) {
        if (!rm.captured(1).isEmpty()) gsum = rm.captured(1).toDouble();
        phase = rm.captured(2);
        idx = rm.capturedEnd(0) + 1;
    }

    // taken from output.c line 506ff
    double intens = gsum * 360.0 * hk * 0.5 / (M_PI * std::sqrt(1.0 - sinx2) / pl.waveLength);
    if (pl.polarization > 0.0) intens *= (0.5 * (1.0 + pl.polarization * std::pow(1.0 - 2.0 * sinx2, 2.0)));

    QRegularExpressionMatchIterator it = rxHkl.globalMatch(pl.byteArray, idx);

    while (it.hasNext()) {
        QRegularExpressionMatch m = it.next();

        double tex = m.captured(1).isEmpty() ? 1.0 : m.captured(1).toDouble();
        QString hkl = m.captured(2);

        vec.append(Hkl(dpos, hkl, 0, phase, QColor(), intens, tex, 1, b1, b2, gsum));
    }

    // correct b1 by factor 1/1.50 if centrosymmetric equivalent peaks are found. It is an oddity of
    // BGMN par files to multiply B1 by 1.5 in that case. We must revert it here.
    if (vec.size() > 1) {
        for (int i = 0; i < vec.size(); ++i) {
            vec[i].setB1(vec.at(i).B1() / 1.50);
        }
    }

    return vec;
}

void BgmnParParser::sortByPhase(QMap<QString, QVector<Hkl> > &map, const QVector<Hkl> &vhkl)
{
    for (int i = 0; i < vhkl.size(); ++i) {
        if (!vhkl.at(i).isEmpty()) map[vhkl.at(i).phase()].append(vhkl.at(i));
    }
}

QVector<Hkl> BgmnParParser::getReflections(const QString &pname)
{
    if (pname.isEmpty()) {
        QVector<Hkl> vec;
        QMapIterator<QString, QVector<Hkl> > it(_data);

        while (it.hasNext()) {
            it.next();
            vec.append(it.value());
        }

        return vec;
    }

    return _data.value(pname);
}

const QMap<QString, QVector<Hkl> > * BgmnParParser::reflections()
{
    return &_data;
}

QVector<double> BgmnParParser::getBackgroundCoefficients(const QString &f)
{
    QVector<double> vec;
    if (!QFile::exists(f)) return vec;

    QByteArray ba;
    BgmnFileIO::readBinaryFile(f, ba);

    int idxLast = ba.lastIndexOf('\n');

    if (idxLast < 0) return vec;

    QList<QByteArray> lastLine = ba.mid(idxLast).split(' ');

    for (int i = 1; i < lastLine.size(); ++i) {
        vec.append(lastLine.at(i).toDouble());
    }

    return vec;
}

double BgmnParParser::parseWavelength(const QByteArray &ba) const
{
    double wl = -1.0;

    QList<global::CharWaveLength> allWl = global::Functions::getAllWavelengths();

    static QRegularExpression rxlam("LAMBDA=(\\S+)");
    static QRegularExpression rxsyn("SYNCHROTRON=(\\d+\\.\\d+)");

    QRegularExpressionMatch rm = rxlam.match(QString(ba));

    if (rm.hasMatch()) {
        for (int i = 0; i < allWl.size(); ++i) {
            if (allWl.at(i).element.toLower() == rm.captured(1).left(2).toLower()) {
                wl = allWl.at(i).ka1;
                break;
            }
        }
    } else {
        rm = rxsyn.match(QString(ba));
        if (rm.hasMatch()) {
            wl = rm.captured(1).toDouble();
        }
    }

    return wl;
}

double BgmnParParser::parsePolarization(const QByteArray &ba) const
{
    double pol = -1.0;

    static QRegularExpression rxpol("POL=(\\d+\\.\\d+)");

    QRegularExpressionMatch rm = rxpol.match(QString(ba));
    if (rm.hasMatch()) {
        pol = rm.captured(1).toDouble();
    }

    return pol;
}

