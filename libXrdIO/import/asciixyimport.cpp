/***************************************************************************
                          asciixyimport.cpp  -  description
                             -------------------
    begin                : Sat Oct 03, 2009
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


#include "asciixyimport.h"

AsciiXyImport::AsciiXyImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "xy" << "xyp" << "csv" << "dat" << "asc" << "txt";
    descr = QLatin1String("Free format XY scan");
}

bool AsciiXyImport::isSupported(const QByteArray &ba)
{
    // ignore some other ASCII xy formats for which separate import filters exist
    if (ba.left(25) == QByteArray("# == pyFAI calibration ==")) return false;

    QStringList header = QString(ba).simplified().split(global::rxLineEnding);

    static QRegularExpression rxc("[!%&/#C'\"]");
    static QRegularExpression rxsep("[\\s;:,]");

    int dl = 0;

    for (int i = 0; i < header.size(); ++i) {
        // skip all comment lines
        QRegularExpressionMatch match = rxc.match(header.at(i).trimmed().left(1));
        if (match.hasMatch()) {
            continue;
        }

        // skip empty lines
        if (header.at(i).isEmpty()) {
            continue;
        }

        // data lines must contain numbers and separators.
        // if not, return false
        QStringList line = header.at(i).split(rxsep);

        bool okfirst, oksecond;

        if (line.size() < 2) {
            return false;
        } else {
            line.at(0).toDouble(&okfirst);
            line.at(1).toDouble(&oksecond);

            if (!(okfirst && oksecond)) {
                return false;
            }

            dl++;

            // stop after 2 successful lines, to keep the function call fast
            if (dl > 2) {
                return true;
            }
        }
    }

    // if the header was too long to reach data lines,
    // we optimistically return true.
    return true;
}

int AsciiXyImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.isEmpty()) {
        qDebug() << QString("AsciiXyImport::load(): Could not open file %1").arg(file);
        return -1;
    }

    QVector< QVector<double> > vec_i;       // all data vectors (vec_i[0] contains the x-axis)
    QStringList headers;                  // if we find a header line, store it here
    int l = 0;                            // counting lines that actually contain data values
    bool headerFound = false;
    bool ok;
    QStringList vals;

    // comment signs
    static QRegularExpression com("[!%&/#C'\"]");

    // field separators. Consecutive \\s are merged, but consecutive other signs
    // (e.g. ;; or ,,) are treated as empty fields
    static QRegularExpression sep("[\\s;:,]");

    for (int n = 0; n < content.size(); ++n) {
        const QString &line = content.at(n);

        if (line.isEmpty()) {
            continue;
        }

        // skip anything that looks like a comment sign
        if (line.left(1).contains(com)) {
            continue;
        }

        // trim and split (trim to remove empty fields at the beginning or end of the line)
        vals = line.simplified().split(sep);

        // not enough entries
        if (vals.size() < 2) {
            continue;
        }

        // check the first line not starting with a comment sign for numeric values. If the first
        // entry is not a number, assume it's a header string
        if (!headerFound) {
            vals.at(0).toDouble(&ok);

            // not a number, assume it's a header
            if (!ok) {
                headers = vals;
                headerFound = true;
                continue;
            }

            // in any case, we skip this test from now on.
            headerFound = true;
        }

        // here's a trick: if we find a longer line (i.e. more data fields than in previous lines) we will add a new
        // intensity vector and initialize all previous points with 0.0.
        while (vals.size() > vec_i.size()) {
            vec_i.push_back(QVector<double> (l, 0.0));
        }

        // loop over all columns
        for (int i = 0; i < vec_i.size(); ++i) {
            // try to convert element to number
            if (i < vals.size()) {
                double v = vals.at(i).toDouble(&ok);

                // if it failed, reset the value to 0.0. In that case, empty field
                // (e.g. ";;" will be filled with 0.0
                if (!ok) {
                    v = 0.0;
                }

                // write value to vector
                vec_i[i].push_back(v);
            }
         }

        l++;
    }

    QFileInfo fi(file);

    for (int i = 1; i < vec_i.size(); ++i) {
        QString label = QString("%1 Data %2").arg(fi.fileName()).arg(i);

        // try to find a header for the scan
        if (i < headers.size()) {
            label = headers.at(i);
        }

        Scan scan(label, QColor(), 1);
        scan.setSourceFileName(file);
        scan.setDataAng(vec_i.at(0));
        scan.setDataInt(vec_i.at(i));
        scan.setWaveLength(0.0);
        scan.setTypes(Scan::XY | Scan::MEASURED);
        scanHeap.push_back(scan);
    }

    return 1;
}
