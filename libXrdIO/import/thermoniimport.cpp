/***************************************************************************
                          thermoniimport.cpp  -  description
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

#include "thermoniimport.h"

#define sizeFloat 4
#define sizeDouble 8
#define sizeUshort 2
#define sizeInt 4
#define sizeLong 4

ThermoNiImport::ThermoNiImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "ni";
    descr = QLatin1String("Thermo Fischer NI scan");
}


bool ThermoNiImport::isSupported(const QByteArray &ba)
{
    // return QString(ba.at(0)) == QString("\u0002");
    qDebug()  << ba.left(1);
    return ba.left(1) == QByteArray("\x02");
}

int ThermoNiImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    QByteArray content;
    BgmnFileIO::readBinaryFile(file, content);

    if (content.isEmpty()) {
        qDebug() << QString("ThermoNiImport::load(): Empty file read from %1").arg(file);
        return -1;
    }

    QFileInfo fi(file);
    QString name = fi.completeBaseName();

    name = QString(content.mid(1506, 12));
    if (name.isEmpty()) {
        name = fi.completeBaseName();
    }

    double lambda1 = BgmnFileIO::hex2double(content.mid(945, sizeDouble));
    double lambda2 = BgmnFileIO::hex2double(content.mid(953, sizeDouble));
    double lambdaM = BgmnFileIO::hex2double(content.mid(961, sizeDouble));
    double lambda3 = BgmnFileIO::hex2double(content.mid(969, sizeDouble));

    qDebug() << QString("ThermoNiImport::load(): Lambda1 = %1").arg(lambda1);
    qDebug() << QString("ThermoNiImport::load(): Lambda2 = %1").arg(lambda2);
    qDebug() << QString("ThermoNiImport::load(): Lambda3 = %1").arg(lambda3);
    qDebug() << QString("ThermoNiImport::load(): LambdaM = %1").arg(lambdaM);

    int npoints = BgmnFileIO::hex2int(content.mid(1617, sizeInt));
    int blkstart = 1621;

    QVector<double> vec_a(npoints, 0.0);
    QVector<double> vec_i(npoints, 0.0);

    for (int i = 0; i < npoints; ++i) {
        if (2 * i * sizeDouble + blkstart + sizeDouble >= content.size()) {
            break;
        }

        vec_a[i] = BgmnFileIO::hex2double(content.mid(blkstart + 2 * i * sizeDouble, sizeDouble));
        vec_i[i] = double(BgmnFileIO::hex2int(content.mid(blkstart + 2 * i * sizeDouble + 3 * sizeFloat, sizeInt)));

        qDebug() << QString("%1   %2").arg(vec_a[i]).arg(vec_i[i]);
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
