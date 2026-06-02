/***************************************************************************
                          thermorawimport.cpp  -  description
                             -------------------
    begin                : Thu Apr 25, 2019
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

#include "thermorawimport.h"

#define sizeFloat 4
#define sizeDouble 8
#define sizeUshort 2
#define sizeInt 4
#define sizeLong 4

ThermoRawImport::ThermoRawImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "raw";
    descr = QLatin1String("Thermo Fischer RAW scan");
}


bool ThermoRawImport::isSupported(const QByteArray &ba)
{
    return ba.left(1) == QByteArray("\x01");
}

int ThermoRawImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    QByteArray content;
    BgmnFileIO::readBinaryFile(file, content);

    if (content.isEmpty()) {
        qDebug() << QString("ThermoRawImport::load(): Empty file read from %1").arg(file);
        return -1;
    }

    QFileInfo fi(file);
    QString name = fi.completeBaseName();

    name = baToString(content, 1441, 12);
    if (name.isEmpty()) {
        name = fi.completeBaseName();
    }

    double lambda1 = BgmnFileIO::hex2double(content.mid(864, sizeDouble));
    double lambda2 = BgmnFileIO::hex2double(content.mid(872, sizeDouble));
    double lambdaM = BgmnFileIO::hex2double(content.mid(880, sizeDouble));
    double lambda3 = BgmnFileIO::hex2double(content.mid(888, sizeDouble));

    qDebug() << QString("ThermoRawImport::load(): Lambda1 = %1").arg(lambda1);
    qDebug() << QString("ThermoRawImport::load(): Lambda2 = %1").arg(lambda2);
    qDebug() << QString("ThermoRawImport::load(): Lambda3 = %1").arg(lambda3);
    qDebug() << QString("ThermoRawImport::load(): LambdaM = %1").arg(lambdaM);

    int npoints = BgmnFileIO::hex2int(content.mid(1548, sizeInt));
    int blkstart = 1552;

    QVector<double> vec_a(npoints, 0.0);
    QVector<double> vec_i(npoints, 0.0);

    for (int i = 0; i < npoints; ++i) {
        if (2 * i * sizeDouble + blkstart + sizeDouble >= content.size()) {
            break;
        }

        vec_a[i] = BgmnFileIO::hex2double(content.mid(blkstart + 2 * i * sizeDouble, sizeDouble));
        vec_i[i] = double(BgmnFileIO::hex2int(content.mid(blkstart + 2 * i * sizeDouble + 2 * sizeFloat, sizeInt)));
    }

    Scan scan(name, QColor(), 1);
    scan.setSourceFileName(file);
    scan.setDataAng(vec_a);
    scan.setDataInt(vec_i);
    scan.setWaveLength(lambda1);
    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);

    return scanHeap.size();
}
