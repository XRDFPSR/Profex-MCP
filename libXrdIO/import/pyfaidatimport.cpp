/***************************************************************************
                          pyfaidatimport.cpp  -  description
                             -------------------
    begin                : Mon Jun 10, 2024
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


#include "pyfaidatimport.h"

PyFaiDatImport::PyFaiDatImport(QObject *parent)
    : GenericImport{parent}
{
    extens << "dat";
    descr = QLatin1String("pyFAI calibrated DAT");
}

bool PyFaiDatImport::isSupported(const QByteArray &ba)
{
    if (ba.first(25) == QByteArray("# == pyFAI calibration ==")) return true;
    if (ba.first(17) == QByteArray("# pyfai_version =")) return true;
    return false;
}

int PyFaiDatImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.isEmpty()) {
        qDebug() << QString("PyFaiDatImport::load(): Could not open file %1").arg(file);
        return -1;
    }

    QFileInfo fi(file);
    QVector<double> x;
    QVector<double> y;

    double wl = parseWaveLength(content);
    parseData(content, x, y);

    Scan scan(fi.fileName(), QColor(), 1);
    scan.setSourceFileName(file);
    scan.setDataAng(x);
    scan.setDataInt(y);
    scan.setWaveLength(wl);
    scan.setWavelengthMode(Scan::WavelengthMode::SYNCHROTRON);
    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);

    return 1;
}

double PyFaiDatImport::parseWaveLength(const QStringList &l)
{
    static QRegularExpression rx("^# [Ww]avelength(?::|\\s=) ([\\d\\.\\+-eE]+) m");

    for (int i = 0; i < l.size(); ++i) {
        if (l.at(i).first(1) != "#") {
            return 0.0;
        }

        QRegularExpressionMatch rm = rx.match(l.at(i));
        if (rm.hasMatch()) {
            double wl = rm.captured(1).toDouble();
            return 10e+9 * wl; // meter to nm
        }
    }

    return 0.0;
}

void PyFaiDatImport::parseData(const QStringList &l, QVector<double> &x, QVector<double> &y)
{
    static QRegularExpression rx("^\\s*([\\d\\.\\+-eE]+)\\s+([\\d\\.\\+-eE]+)");

    for (int i = 0; i < l.size(); ++i) {
        if (l.at(i).first(1) == "#") continue;
        QRegularExpressionMatch rm = rx.match(l.at(i));
        if (rm.hasMatch()) {
            x.push_back(rm.captured(1).toDouble());
            y.push_back(rm.captured(2).toDouble());
        }
    }
}
