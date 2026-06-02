/***************************************************************************
                          asciixyexport.cpp  -  description
                             -------------------
    begin                : Sun Oct 04 14:16:07 CEST 2009
    copyright            : (C) 2009 by Nicola Doebelin
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

#include "asciitxtexport.h"
#include "bgmnfileio.h"

AsciiTxtExport::AsciiTxtExport(QObject *parent) :
    GenericExport(parent)
{
    uId = "ASCII_TXT";
}

int AsciiTxtExport::save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &flags)
{
    bool fix = flags.value("fixBgmnZero", false).toBool();
    QString sep(flags.value("fieldSeparator", " ").toString());

    bool b = BgmnFileIO::writeTextFile(file, getData(scan, sep, fix));

    if (b) return 1;
    return 0;
}

int AsciiTxtExport::save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags)
{
    bool fix = flags.value("fixBgmnZero", false).toBool();
    QString sep(flags.value("fieldSeparator", " ").toString());

    bool b = BgmnFileIO::writeTextFile(file, getData(scanHeap, sep, fix));

    if (b) return scanHeap.size();
    return 0;
}

QString AsciiTxtExport::getData(const Scan &scan, const QString &sep, bool fix)
{
    QString data;
    QTextStream stream(&data);
    stream.setRealNumberNotation(QTextStream::FixedNotation);
    stream.setRealNumberPrecision(precision);

    bool addXoffset = !qFuzzyIsNull(scan.xOffset());
    bool addYoffset = !qFuzzyIsNull(scan.yOffset());
    bool addScale = !qFuzzyCompare(scan.scaleFactor(), 1.0);

    for (int i = 0; i < scan.size(); i++) {
        double a = scan.angle(i);
        double c = scan.intensity(i);

        if (addXoffset) a += scan.xOffset();
        if (addScale)   c *= scan.scaleFactor();
        if (addYoffset) c += scan.yOffset();

        stream << a << sep << (fix ? fixZero(c) : c) << "\n";
    }

    return data;
}

QString AsciiTxtExport::getData(const QVector<Scan> &scanHeap, const QString &sep, bool fix)
{
    QString data;
    QTextStream stream(&data);
    stream.setRealNumberNotation(QTextStream::FixedNotation);
    stream.setRealNumberPrecision(6);

    // determine the longest scan
    int n = 0;

    for (int i = 0; i < (int)scanHeap.size(); ++i) {
        n = qMax(n, scanHeap[i].size());
    }

    // collect pointers to all data vectors of all scans
    QVector< QVector<double> > vecs;
    for (int i = 0; i < (int)scanHeap.size(); ++i) {
        vecs.push_back(scanHeap[i].pDataAngle());
        vecs.push_back(scanHeap[i].pDataIntensity());
    }

    // loop through the data points
    for (int i = 0; i < n; ++i) {
        // loop over all scans
        for (int s = 0; s < (int)vecs.size(); s += 2) {
            QVector<double> &dang = vecs[s];
            QVector<double> &dint = vecs[s+1];

            if (dang[i] == (vecs[0])[i]) {
                // if the angular values are all the same, we will store the file in format "x y y y y "
                if (s == 0) {
                    // write the angle only for the first scan
                    stream << dang[i] << sep << (fix ? fixZero(dint[i]) : dint[i]);
                } else {
                    // only write intensities for all other scans
                    stream << (fix ? fixZero(dint[i]) : dint[i]);
                }

                // append a field separator only if it was not the last field
                if (s < vecs.size() - 1) {
                    stream << sep;
                }
            } else {
                // if the angular values are NOT the same, we will store the file in format "x y x y x y "
                stream << dang[i] << sep << (fix ? fixZero(dint[i]) : dint[i]);

                // append a field separator only if it was not the last field
                if (s < vecs.size() - 1) {
                    stream << sep;
                }
            }
        }

        stream << "\n";
    }

    return data;
}

double AsciiTxtExport::fixZero(double d)
{
    if (d < 0.0) return zeroVal;
    return qFuzzyIsNull(d) ? zeroVal : d;
}
