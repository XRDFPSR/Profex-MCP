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


#include "xyeimport.h"

XyeImport::XyeImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "xye";
    descr = QLatin1String("ASCII XYE scan");
}

bool XyeImport::isSupported(const QByteArray &ba)
{
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

int XyeImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.isEmpty()) {
        qDebug() << QString("XyeImport::load(): Could not open file %1").arg(file);
        return -1;
    }

    // field separators. Consecutive \\s are merged, but consecutive other signs
    // (e.g. ;; or ,,) are treated as empty fields
    static QRegularExpression sep("[\\s;:,]");
    static QRegularExpression com("[!%&/#C'\"]");

    QStringList headers = getHeader(content.at(0).simplified().split(sep));

    QVector< QVector<double> > vec_i;       // all data vectors (vec_i[0] contains the x-axis)
    int l = 0;                              // counting lines that actually contain data values
    double zero = 0.000001;                 // value used instead of 0.0, because BGMN doesn't accept intensities or ESDs of 0.0
    bool ok;
    QStringList fields;


    for (int n = 0; n < content.size(); ++n) {
        const QString &line = content.at(n);

        if (line.isEmpty()) continue;

        // skip anything that looks like a comment sign
        if (line.left(1).contains(com)) continue;

        // trim and split (trim to remove empty fields at the beginning or end of the line)
        fields = line.simplified().split(sep);

        // not enough entries
        if (fields.size() < 3) continue;

        // in xye format, we assume the following column structure:
        // angle int1 esd1 int2 esd2 int3 esd3 ...
        // we skip esd columns and only read angle and int columns

        double ang = fields.at(0).toDouble(&ok);
        if (!ok) continue;

        // here's a trick: if we find a longer line (i.e. more data fields than in previous lines) we will add a new
        // intensity vector and initialize all previous points with 0.0.
        while (vec_i.size() < 1 + (fields.size() / 2)) {
            vec_i.push_back(QVector<double> (l, zero));
        }

        vec_i[0].push_back(ang);
        int c = 1;

        for (int i = 1; i < fields.size(); i += 2) {
            double v = fields.at(i).toDouble(&ok);
            // replace 0.0 with zero for direct compatibility with BGMN
            v = qFuzzyIsNull(v) ? zero : v;

            if (ok) vec_i[c].push_back(v);
            else    vec_i[c].push_back(zero);

            ++c;
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

QStringList XyeImport::getHeader(const QStringList &l)
{
    QStringList h;
    if (l.size() == 0) return h;

    bool ok;
    l.at(0).toDouble(&ok);
    if (ok) return h; // first field is a number, thus l is not the header line

    for (int i = 1; i < l.size(); i += 2) {
        h.append(l.at(i));
    }

    return h;
}
