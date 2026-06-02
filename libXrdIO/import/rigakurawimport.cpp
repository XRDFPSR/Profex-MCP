/***************************************************************************
                          rigakurawimport.cpp  -  description
                             -------------------
    begin                : Thu July 27, 2013
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

#include "rigakurawimport.h"

// maybe these should be hard coded, because the sizes must match the
// RAW file, not the current platform
#define sizeFloat 4
#define sizeDouble 8
#define sizeUshort 2
#define sizeInt 4
#define sizeLong 4

RigakuRawImport::RigakuRawImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "raw";
    descr = QLatin1String("Rigaku RAW scan");
}


bool RigakuRawImport::isSupported(const QByteArray &ba)
{
    return ba.left(2) == "FI";
}

int RigakuRawImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    QByteArray content;
    BgmnFileIO::readBinaryFile(file, content);

    if (content.isEmpty()) {
        qDebug() << QString("RigakuRawImport::load(): Empty file read from %1").arg(file);
        return -1;
    }

    if (baToString(content, 24, 2) == "BU") {
        if (!loadNew(content, scanHeap, QFileInfo(file))) return -1;
    }

    if (baToString(content, 22, 2) == "BU") {
        if (!loadOld(content, scanHeap, QFileInfo(file))) return -1;
    }

    return scanHeap.size();
}

bool RigakuRawImport::loadNew(const QByteArray &content, QVector<Scan> &scanHeap, const QFileInfo &fi)
{
    Scan scan(fi.completeBaseName(), QColor(), 1);
    scan.setSourceFileName(fi.absoluteFilePath());

    scan.setWaveLength(BgmnFileIO::hex2double(content.mid(1212, sizeDouble)));
    scan.setWaveLength2(BgmnFileIO::hex2double(content.mid(1220, sizeDouble)));
    scan.setWaveLength3(BgmnFileIO::hex2double(content.mid(1228, sizeDouble)));

    QVector<double> &vec_a = scan.pDataAngle();
    QVector<double> &vec_i = scan.pDataIntensity();

    double startang = double(BgmnFileIO::hex2float(content.mid(2962, sizeFloat)));
    double endang = double(BgmnFileIO::hex2float(content.mid(2966, sizeFloat)));
    double stepsize = double(BgmnFileIO::hex2float(content.mid(2970, sizeFloat)));

    scan.setStepSize(stepsize);

    int count = BgmnFileIO::hex2int(content.mid(3154, sizeInt));
    int startIdx = 3158;

    for (int j = 0; j < count; ++j) {
        double ang = startang + (double(j) / double(count - 1)) * (endang - startang);
        double its = double(BgmnFileIO::hex2float(content.mid(startIdx + j * sizeFloat, sizeFloat)));

        vec_a.push_back(ang);
        vec_i.push_back(its);
    }

    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);
    return true;
}

bool RigakuRawImport::loadOld(const QByteArray &content, QVector<Scan> &scanHeap, const QFileInfo &fi)
{
    Scan scan(fi.completeBaseName(), QColor("#000000"), 1);
    scan.setSourceFileName(fi.absoluteFilePath());

    scan.setWaveLength(BgmnFileIO::hex2double(content.mid(984, sizeDouble)));
    scan.setWaveLength2(BgmnFileIO::hex2double(content.mid(992, sizeDouble)));
    scan.setWaveLength3(BgmnFileIO::hex2double(content.mid(1000, sizeDouble)));

    QVector<double> &vec_a = scan.pDataAngle();
    QVector<double> &vec_i = scan.pDataIntensity();

    double startang = (double)BgmnFileIO::hex2float(content.mid(3004, sizeFloat));
    double endang = (double)BgmnFileIO::hex2float(content.mid(3008, sizeFloat));
    double stepsize = (double)BgmnFileIO::hex2float(content.mid(3012, sizeFloat));

    scan.setStepSize(stepsize);

    int count = BgmnFileIO::hex2int(content.mid(3054, sizeInt));
    int startIdx = 3058;

    for (int j = 0; j < count; ++j) {
        double ang = startang + (double(j) / double(count - 1)) * (endang - startang);
        double its = double(BgmnFileIO::hex2float(content.mid(startIdx + j * sizeFloat, sizeFloat)));

        vec_a.push_back(ang);
        vec_i.push_back(its);
    }

    scanHeap.push_back(scan);
    return true;
}
