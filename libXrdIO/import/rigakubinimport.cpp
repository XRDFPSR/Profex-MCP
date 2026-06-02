/***************************************************************************
                          rigakubinimport.cpp  -  description
                             -------------------
    begin                : Thu Aug 26, 2013
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

#include "rigakubinimport.h"

// maybe these should be hard coded, because the sizes must match the
// BIN file, not the current platform
#define sizeFloat sizeof(float)
#define sizeInt sizeof(int)

RigakuBinImport::RigakuBinImport(QObject *parent)
    : GenericImport(parent)
{
    extens << "bin";
    descr = QLatin1String("Rigaku BIN scan");
}

bool RigakuBinImport::isSupported(const QByteArray &ba)
{
    // is this a valid test????
    if (ba.left(2) == "K7") {
        return true;
    }

    return false;
}

int RigakuBinImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    qDebug() << QString("RigakuBinImport::load(): Loading file %1").arg(file);
    QByteArray content;
    BgmnFileIO::readBinaryFile(file, content);

    if (content.isEmpty()) {
        qDebug() << QString("RigakuBinImport::load(): byte array is empty in file %1").arg(file);
        return -1;
    }

    int pos = 0;
    Scan scan(file, QColor(), 1);
    scan.setSourceFileName(file);
    scan.setAuxInfo("Date", QVariant(QDateTime::fromString(baToString(content, 2, 8), "dd.MM.yy")));
    scan.setComment(baToString(content, 10, 64));
    scan.setNamedWaveLength(baToString(content, 74, 10));

    pos = 84;
    double startAng = (double)BgmnFileIO::hex2float(content.mid(pos, sizeFloat));
    pos += sizeFloat;

    double endAng = (double)BgmnFileIO::hex2float(content.mid(pos, sizeFloat));
    pos += sizeFloat;

    double stepSize = (double)BgmnFileIO::hex2float(content.mid(pos, sizeFloat));
    pos += sizeFloat;

    double stepCountTime = (double)BgmnFileIO::hex2float(content.mid(pos, sizeFloat));
    pos += sizeFloat;

    int dataPointCount = BgmnFileIO::hex2int(content.mid(pos, sizeInt));
    pos += sizeInt;

    double maximumCount = (double)BgmnFileIO::hex2float(content.mid(pos, sizeFloat));
    pos += sizeFloat;

    Q_UNUSED(endAng);
    Q_UNUSED(stepCountTime);
    Q_UNUSED(maximumCount);
    Q_UNUSED(stepSize);

    // unknown float
    pos += sizeFloat;

    QVector<double> vec_a;
    QVector<double> vec_i;

    for (int j = 0; j < dataPointCount; ++j) {
        double ang = startAng + (double(j) / double(dataPointCount - 1)) * (endAng - startAng);
        double its = double(BgmnFileIO::hex2float(content.mid(pos, sizeFloat)));

        vec_a.push_back(ang);
        vec_i.push_back(its);

        pos += sizeFloat;
        if (pos >= content.size() - int(sizeFloat)) break;
    }

    scan.setDataAng(vec_a);
    scan.setDataInt(vec_i);
    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);

    return 1;
}
