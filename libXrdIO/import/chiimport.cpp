/***************************************************************************
                          chiqimport.cpp  -  description
                             -------------------
    begin                : Tue Jan 16, 2024
    copyright            : (C) 2024 by Nicola Doebelin
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

#include "chiimport.h"
#include "functions.h"

ChiImport::ChiImport(QObject *parent)
    : GenericImport{parent}
{
    extens << "chi";
    descr = QLatin1String("Chi ASCII scans");
}


bool ChiImport::isSupported(const QByteArray &ba)
{
    if (ba.left(13) == QByteArray("# chi_Q chi_I")) return true;
    else if (ba.left(18) == QByteArray("# chi_2theta chi_I")) return true;
    else return false;
}

int ChiImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.isEmpty()) {
        qDebug() << QString("ChiImport::load(): Could not open file %1").arg(file);
        return -1;
    }

    QVector<double> vec_a;
    QVector<double> vec_i;
    double wl = 0.0;
    double wlFallback = 0.20; // in Angstrom

    // comment signs
    static QRegularExpression com("[!%&/#C'\"]");

    // field separator
    static QRegularExpression sep("\\s+");

    // parse header
    static QRegularExpression head("^# (chi_Q|chi_2theta) chi_I\\s*(\\d+\\.?\\d*)?(nm|NM|A|AA)?");

    bool convertQtoTT = false;
    bool wlNmToAA = false;

    QRegularExpressionMatch rm = head.match(content.first());

    if (!rm.hasMatch()) {
        qDebug() << QString("ChiImport::load(): Unsupported format: %1").arg(content.first());
        return -1;
    }

    qDebug() << QString("*** unit detected: %1").arg(rm.captured(3));
    if (rm.captured(3).toLower() == "nm") {
        wlNmToAA = true;
    }

    if (rm.captured(1) == "chi_Q") {
        if (rm.captured(2).isNull()) {
            qDebug() << QString("ChiImport::load(): Cannot convert Q indexed files to 2theta scale without knowing the wavelength.");
            qDebug() << QString("                   Append the wavelength in Angstrom to the header line and try again.");
            return -1;
        } else {
            wl = rm.captured(2).toDouble() * (wlNmToAA ? 10.0 : 1.0);
            convertQtoTT = true;
        }
    } else if (rm.captured(1) == "chi_2theta") {
        if (rm.captured(2).isNull()) {
            qDebug() << QString("ChiImport::load(): Could not find the wavelength. Falling back to %1 A").arg(wlFallback);
            qDebug() << QString("                   Append the wavelength in Angstrom to the header line to fix this issue.");

            wl = wlFallback;
            convertQtoTT = false;
        } else {
            wl = rm.captured(2).toDouble() * (wlNmToAA ? 10.0 : 1.0);
            convertQtoTT = false;
        }
    } else {
        qDebug() << QString("ChiImport::load(): Unsupported format: %1").arg(content.first());
        return -1;
    }

    for (int n = 1; n < content.size(); ++n) {
        const QString &line = content.at(n);

        if (line.isEmpty()) {
            continue;
        }

        // skip anything that looks like a comment sign
        if (line.left(1).contains(com)) {
            continue;
        }

        // split the records
        QStringList vals = line.split(sep);

        // not enough entries
        if (vals.size() < 2) {
            continue;
        }

        bool aok, iok;
        double ang = vals.at(0).toDouble(&aok);
        double cts = vals.at(1).toDouble(&iok);

        if (aok && iok) {
            if (convertQtoTT) vec_a.push_back(global::Functions::qToTwoTheta(ang, wl));
            else              vec_a.push_back(ang);
            vec_i.push_back(cts);
        }
    }

    QFileInfo fi(file);

    Scan scan(fi.fileName(), QColor(), 1);
    scan.setSourceFileName(file);
    scan.setDataAng(vec_a);
    scan.setDataInt(vec_i);
    scan.setWaveLength(wl);
    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);

    return 1;
}
