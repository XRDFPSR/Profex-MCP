/***************************************************************************
                          reynoldspltimport.cpp  -  description
                             -------------------
    begin                : Thu May 21, 2013
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

#include "reynoldspltimport.h"

ReynoldsPltImport::ReynoldsPltImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "plt";
    descr = QLatin1String("Reynolds PLT data");
}

bool ReynoldsPltImport::isSupported(const QByteArray &ba)
{
    QStringList header = QString(ba).split(global::rxLineEnding);

    // we need at least 10 lines to reach the data block
    if (header.size() < 10) {
        return false;
    }

    int h = header.at(5).toInt() + 10; // line 5 describes the number of additional header lines

    // we want enough lines to read some data lines
    if (header.size() <= h) {
        return false;
    }

    QRegularExpression rx("^\\s+\\d+\\.\\d+\\s*$");
    QRegularExpressionMatch rm;

    // parse from pos h to the end of the header. If any of the lines doesn't contain
    // a numerical value, return false.

    for (int i = h; i < header.size(); ++i) {
        rm = rx.match(header.at(i));
        if (!rm.hasMatch()) return false;
    }

    return true;
}

int ReynoldsPltImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    qDebug() << QString("ReynoldsPltImport::load(): Loading file %1").arg(file);
    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.size() < 11) {
        qDebug() << QString("ReynoldsPltImport::load(): No valid data found in file %1").arg(file);
        return -1;
    }

    QVector<double> vec_a;       // angle
    QVector<double> vec_i;       // intensity

    Scan scan(content.at(0), QColor(), 1);
    scan.setSourceFileName(file);
    scan.setComment(QString("%1, %2").arg(content.at(1)).arg(content.at(2)));
    scan.setWaveLength(content.at(3).toDouble());

    bool ok = false;
    int auxLines = content.at(5).toInt(&ok);
    if (!ok) auxLines = 0;

    double startAng = content.at(7 + auxLines).toDouble();
    double endAng = content.at(8 + auxLines).toDouble();
    double stepSize = content.at(9 + auxLines).toDouble();
    int startIdx = 10 + auxLines;
    int p = content.size() - startIdx;

    Q_UNUSED(stepSize);

    for (int j = 0; j < p; ++j) {
        double ang = startAng + (double(j) / double(p - 1)) * (endAng - startAng);
        double its = content.at(j + startIdx).toDouble();
        vec_a.push_back(ang);
        vec_i.push_back(its);
    }

    scan.setDataAng(vec_a);
    scan.setDataInt(vec_i);
    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);

    return scanHeap.size();
}
