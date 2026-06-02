/***************************************************************************
                          stoerawimport.cpp  -  description
                             -------------------
    begin                : Thu Feb 20, 2014
    copyright            : (C) 2014 by Nicola Doebelin
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

#include "stoerawimport.h"

// maybe these should be hard coded, because the sizes must match the
// RAW file, not the current platform
#define sizeFloat 4
#define sizeDouble 8
#define sizeUshort 2
#define sizeInt 4
#define sizeLong 4

StoeRawImport::StoeRawImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "raw";
    descr = QLatin1String("Stoe RAW scan");
}

bool StoeRawImport::isSupported(const QByteArray &ba)
{
    return ba.left(14).toLower() == "raw_1.06powdat";
}

int StoeRawImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    QByteArray content;
    BgmnFileIO::readBinaryFile(file, content);

    if (content.isEmpty()) {
        qDebug() << QString("StoeRawImport::load(): Empty file read from %1").arg(file);
        return -1;
    }

    QFileInfo fi(file);
    QString name = fi.completeBaseName();

    // what comment to use as name?
/*
    name = QString(content.mid(40, 36));
    if (name.isEmpty()) {
        name = fi.completeBaseName();
    }
*/

    int nscans = 0;

    // read the size in bytes of the data block of each scan
    QVector<uint> blksizes;
    for (int idx = 495; idx < 511; idx += sizeUshort) {
        uint s = BgmnFileIO::hex2ushort(content.mid(idx, sizeUshort));
        blksizes.push_back(s);
        if (s) {
            ++nscans;
            qDebug() << QString("StoeRawImport::load(): Found scan no %1 with block size %2").arg(nscans).arg(s);
        }
    }

    float lambda1 = BgmnFileIO::hex2float(content.mid(322, sizeFloat));
    float lambda2 = BgmnFileIO::hex2float(content.mid(326, sizeFloat));

    qDebug() << QString("StoeRawImport::load(): Lambda1 = %1").arg(lambda1);
    qDebug() << QString("StoeRawImport::load(): Lambda2 = %1").arg(lambda2);

    int blkstart = 2048;
    int blkhdr = 512;

    // this one is a wild guess, I have no idea whether position 493 really stores the data format for the data points.
    // but so far it seems to work...
    //int prec = hex2ushort(content.mid(493, sizeUshort)); // prec = 0: data points as ushort, prec = 1: data points as int
    // UPDATE: No, pos 493 was wrong. Ask the user as long as we don't know the true location

    // use this block to ask the user for the format
    // QStringList lst;
    // lst << "uint16" << "int32";
    // bool ok;
    // int prec = lst.indexOf(QInputDialog::getItem(0, tr("Intensity Data Format"), tr("Format of intensity values"), lst, 0, false, &ok));
    // if (!ok) return -1;

    for (int n = 0; n < nscans; ++n) {
        qDebug() << QString("StoeRawImport::load(): Reading scan no %1 at position %2").arg(n+1).arg(blkstart);

        if (nscans) {
            scanHeap.push_back(Scan(QString("%1 %2").arg(name).arg(n+1)));
        } else {
            scanHeap.push_back(Scan(name));
        }

        scanHeap[n].setTypes(Scan::XY | Scan::MEASURED);
        scanHeap[n].setSourceFileName(file);
        scanHeap[n].setWaveLength(lambda1);
        scanHeap[n].setWaveLength2(lambda2);

        QVector<double> &vec_a = scanHeap[n].pDataAngle();
        QVector<double> &vec_i = scanHeap[n].pDataIntensity();

        int count = BgmnFileIO::hex2ushort(content.mid(blkstart + 34, sizeUshort));
        double startang = (double)BgmnFileIO::hex2float(content.mid(blkstart + 44, sizeFloat));
        double endang = (double)BgmnFileIO::hex2float(content.mid(blkstart + 52, sizeFloat));
        double stepsize = (double)BgmnFileIO::hex2float(content.mid(blkstart + 60, sizeFloat));

        qDebug() << QString("StoeRawImport::load(): Start = %1, End = %2, StepSize = %3, Steps = %4").arg(startang).arg(endang).arg(stepsize).arg(count);

        Q_UNUSED(endang);

        scanHeap[n].setStepSize(stepsize);

        blkstart += blkhdr;

        // read data points, either as ushort or int
        if (double(blksizes.at(n))/double(count) > 2.0) {
            for (int j = 0; j < count; ++j) {
                double ang = startang + (double(j) / double(count - 1)) * (endang - startang);
                double its = double(BgmnFileIO::hex2int(content.mid(blkstart + j * int(sizeInt), sizeInt)));

                vec_a.push_back(ang);
                vec_i.push_back(its);
            }
        } else {
            for (int j = 0; j < count; ++j) {
                double ang = startang + (double(j) / double(count - 1)) * (endang - startang);
                double its = double(BgmnFileIO::hex2ushort(content.mid(blkstart + j * int(sizeUshort), sizeUshort)));

                vec_a.push_back(ang);
                vec_i.push_back(its);
            }
        }

        blkstart += 2*blksizes.at(n);
    }
    return scanHeap.size();
}
