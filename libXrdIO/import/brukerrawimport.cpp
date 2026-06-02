/***************************************************************************
                          brukerrawimport.cpp  -  description
                             -------------------
    begin                : Thu Jun 05, 2013
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

#include "brukerrawimport.h"
#include <QDateTime>
#include <QStringBuilder>
#include <QLatin1Char>

// these must be hard coded, because the sizes must match the
// RAW file, not the current platform
#define sizeFloat 4
#define sizeDouble 8
#define sizeUshort 2
#define sizeInt 4
#define sizeLong 4

BrukerRawImport::BrukerRawImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "raw";
    descr = QLatin1String("Bruker RAW scan version (DIFFRAC-AT V1 RAW, DIFFRAC-AT V2 RAW, DIFFRACplus V3 RAW, DIFFRACplus V4 RAW");
}

bool BrukerRawImport::isSupported(const QByteArray &ba)
{
    static QRegularExpression rx("RAW[\\s\\d]");
    if (QString(ba.left(4)).contains(rx)) {
        return true;
    }

    return false;
}

int BrukerRawImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    qDebug() << QString("BrukerRawImport::load(): Loading file %1").arg(file);
    QByteArray content;
    BgmnFileIO::readBinaryFile(file, content);

    if (content.isEmpty()) {
        qDebug() << QString("BrukerRawImport::load(): byte array is empty in file %1").arg(file);
        return -1;
    }

    if (content.size() < 252) {
        qDebug() << QString("BrukerRawImport::load(): No valid data found in file %1").arg(file);
        return -1;
    }

    // tests passed, so continue with parsing

    int ret = -1;

    QFileInfo fi(file);
    QString version = QString(content.mid(0, 4));
    if (version == "RAW ") {
        qDebug() << QString("BrukerRawImport::load(): Reading in Bruker RAW V1 format");
        ret = readV1(content, scanHeap, fi.absoluteFilePath());
    } else if (version == "RAW2") {
        qDebug() << QString("BrukerRawImport::load(): Reading in Bruker RAW V2 format");
        ret = readV2(content, scanHeap, fi.absoluteFilePath());
    } else if (version == "RAW1" && content.mid(4, 3) == ".01") {
        qDebug() << QString("BrukerRawImport::load(): Reading in Bruker RAW V3 format");
        ret = readV3(content, scanHeap, fi.absoluteFilePath());
    } else if (version == "RAW4" && content.mid(4, 3) == ".00") {
        qDebug() << QString("BrukerRawImport::load(): Reading in Bruker RAW V4 format");
        ret = readV4(content, scanHeap, fi.absoluteFilePath(), minimal);
    }

    return ret;
}

/* TODO */
int BrukerRawImport::readV1(const QByteArray &a, QVector<Scan> &scanHeap, const QString &fname)
{
    int i = 0;
    int readNextRange = 1;
    int offset = 0;

    // somewhere in the following block we will find the information whether
    // another scan block follows this one
    while (readNextRange) {
        // some version of the file format repeat "RAW " for each block, others don't
        if (a.mid(offset, 4) == "RAW ") {
            offset += 4;
        }

        int steps = BgmnFileIO::hex2int(a.mid(offset, sizeInt));
        if (steps < 2) break;
        offset += sizeInt;

        float tperstep = BgmnFileIO::hex2float(a.mid(offset, sizeFloat));
        offset += sizeFloat;

        double stepsize = (double)BgmnFileIO::hex2float(a.mid(offset, sizeFloat));
        offset += sizeFloat;

        int scanmode = BgmnFileIO::hex2int(a.mid(offset, sizeInt));
        offset += sizeInt;
        offset += 4;

        double startang = (double)BgmnFileIO::hex2float(a.mid(offset, sizeFloat));
        offset += sizeFloat;

        float thetastart = BgmnFileIO::hex2float(a.mid(offset, sizeFloat));
        offset += sizeFloat;
        Q_UNUSED(thetastart);

        float chistart = BgmnFileIO::hex2float(a.mid(offset, sizeFloat));
        offset += sizeFloat;
        Q_UNUSED(chistart);

        float phistart = BgmnFileIO::hex2float(a.mid(offset, sizeFloat));
        offset += sizeFloat;
        Q_UNUSED(phistart);

        QString name = baToString(a, offset, 32);
        offset += 32;

        if (name.isEmpty()) {
            QFileInfo fi(fname);
            name = fi.fileName();
        }

        float lambda1 = BgmnFileIO::hex2float(a.mid(offset, sizeFloat));
        offset += sizeFloat;

        float lambda2 = BgmnFileIO::hex2float(a.mid(offset, sizeFloat));
        offset += sizeFloat;
        offset += 72;

        readNextRange = BgmnFileIO::hex2int(a.mid(offset, sizeInt));
        offset += sizeInt;

        // initialize the vectors
        QVector<double> vec_a;       // angle
        QVector<double> vec_i;       // intensity

        double maxang = startang + steps * stepsize;

        for (int j = 0; j < steps; ++j) {
            double ang = startang + (double(j)/double(steps - 1)) * (maxang - startang);
            double its = double(BgmnFileIO::hex2float(a.mid(offset, sizeFloat)));

            vec_a.push_back(ang);
            vec_i.push_back(its);
            offset += sizeFloat;
        }

        // if more than one scan is stored in the file, we append the number to the scan name
        QString sname = name;
        if (readNextRange) {
            sname = QString("%1-%2").arg(name).arg(i);
        }

        Scan scan(sname, QColor(), 1);
        scan.setSourceFileName(fname);
        scan.setAuxInfo("FormatVersion", QVariant("RAW "));

        scan.setWaveLength(lambda1);
        scan.setWaveLength2(lambda2);
        scan.setTimePerStep((double)tperstep);
        scan.setAuxInfo("ScanMode", QVariant(scanmode));

        scan.setDataAng(vec_a);
        scan.setDataInt(vec_i);

        scan.setTypes(Scan::XY | Scan::MEASURED);
        scanHeap.push_back(scan);

        i++;

        // here we add an exit from the loop, to avoid endless loops
        if (i > 1024) {
            // hopefully noone stores more than 1024 scans in one file...
            qDebug() << QString("RawImport::readV1: More than 1024 scans read. Assuming an endless loop. Exiting...");
            return -1;
        }
    }

    return i;
}


int BrukerRawImport::readV2(const QByteArray &a, QVector<Scan> &scanHeap, const QString &fname)
{
    QString name = baToString(a, 8, 32);
    QString comment = baToString(a, 40, 128);

    if (name.isEmpty()) {
        QFileInfo fi(fname);
        name = fi.fileName();
    }

    QDateTime date = QDateTime::fromString(baToString(a, 168, 10), "dd-MM-yyyy");
    QDateTime time = QDateTime::fromString(baToString(a, 178, 10), "hh:mm");

    unsigned short nscans = BgmnFileIO::hex2ushort(a.mid(4, sizeUshort));
    // qDebug() << QString("RawImport::readV2: Found %1 scans to read").arg(nscans);

    QString nwl = baToString(a, 188, 2);

    // qDebug() << QString("RawImport::readV2: File was measured %1 %2").arg(date.toString("dd.MM.yyyy")).arg(time.toString("hh:mm"));
    // qDebug() << QString("RawImport::readV2: Name of wavelength is %1").arg(nwl);

    unsigned short idx = 190;

    float lambda1 = BgmnFileIO::hex2float(a.mid(idx, sizeFloat));
    idx += sizeFloat;

    float lambda2 = BgmnFileIO::hex2float(a.mid(idx, sizeFloat));
    idx += sizeFloat;

    float lambdaratio = BgmnFileIO::hex2float(a.mid(idx, sizeFloat));
    idx += sizeFloat;
    idx += 8;

    float totaltime = BgmnFileIO::hex2float(a.mid(idx, sizeFloat));
    idx += sizeFloat;
    idx += 42;

    Q_UNUSED(totaltime);
    Q_UNUSED(lambdaratio);
    Q_UNUSED(nwl);

    // qDebug() << QString("RawImport::readV2: lambda1=%1 lambda2=%2 ratio=%3 totaltime=%4").arg(lambda1).arg(lambda2).arg(lambdaratio).arg(totaltime);

    int i;

    for (i = 0; i < nscans; ++i) {
        Scan scan(fname, QColor(), 1);
        scan.setSourceFileName(fname);
        scan.setName(name);
        scan.setComment(comment);
        scan.setAuxInfo("FormatVersion", QVariant("RAW2"));
        scan.setAuxInfo("MeasureDate", QVariant(date));
        scan.setAuxInfo("MeasureTime", QVariant(time));
        scan.setWaveLength(lambda1);
        scan.setWaveLength2(lambda2);

        // initialize the vectors
        QVector<double> vec_a;       // angle
        QVector<double> vec_i;       // intensity

        if (idx >= (a.size() + 2 * sizeof(unsigned short) + 3 * sizeFloat + 30)) {
            qDebug() << QString("RawImport::readV2: Reached the end of the input array.");
            return i;
        }

        unsigned short hlength = BgmnFileIO::hex2ushort(a.mid(idx, sizeUshort));
        idx += sizeof(unsigned short);

        unsigned short steps = BgmnFileIO::hex2ushort(a.mid(idx, sizeUshort));
        idx += sizeof(unsigned short);
        idx += 4;

        double tstep = (double)BgmnFileIO::hex2float(a.mid(idx, sizeFloat));
        idx += sizeFloat;

        double stepsize = (double)BgmnFileIO::hex2float(a.mid(idx, sizeFloat));
        idx += sizeFloat;

        double startang = (double)BgmnFileIO::hex2float(a.mid(idx, sizeFloat));
        double endang = startang + double(steps) * stepsize;
        idx += sizeFloat;
        idx += 26;

        unsigned short temp = BgmnFileIO::hex2ushort(a.mid(idx, sizeUshort));
        idx += sizeof(unsigned short);

        scan.setTimePerStep((double)tstep);
        scan.setStepSize(stepsize);
        scan.setAuxInfo("Temperature_K", QVariant(temp));

        idx += (hlength - 48);

        for (unsigned short j = 0; j < steps; ++j) {
            if (idx >= a.size()) {
                qDebug() << QString("RawImport::readV2: Reached the end of the input array.");
                return i;
            }

            double ang = startang + (double(j) / double(steps - 1)) * (endang - startang);
            double its = double(BgmnFileIO::hex2float(a.mid(idx, sizeFloat)));

            vec_a.push_back(ang);
            vec_i.push_back(its);

            idx += sizeFloat;
        }

        scan.setDataAng(vec_a);
        scan.setDataInt(vec_i);
        scan.setTypes(Scan::XY | Scan::MEASURED);
        scanHeap.push_back(scan);
    }

    return i;
}

int BrukerRawImport::readV3(const QByteArray &a, QVector<Scan> &scanHeap, const QString &fname)
{
    QString output;
    int stat = BgmnFileIO::hex2int(a.mid(8, sizeInt));

    QString status = "unknown";

    switch (stat) {
        case 0: status = "done"; break;
        case 1: status = "active"; break;
        case 2: status = "aborted"; break;
        case 3: status = "interrupted"; break;
    }

    int nscans = BgmnFileIO::hex2int(a.mid(12, sizeInt));

    output += QString("BrukerRawImport::readV3(): Status = %1\n").arg(status);
    output += QString("BrukerRawImport::readV3(): Number of Scans = %1\n").arg(nscans);

    if (nscans > 255) {
        qDebug() << QString("BrukerRawImport::readV3(): Number of scans seems to be > 255 (%1). Not valid, exiting!").arg(nscans);
        return 0;
    }

    QDateTime date = QDateTime::fromString(baToString(a, 16, 10), "dd-MM-yyyy");
    QDateTime time = QDateTime::fromString(baToString(a, 26, 10), "hh:mm:ssss");

    QString userName = baToString(a, 36, 72);
    QString companyName = baToString(a, 108, 218);

    QString sampleId = baToString(a, 326, 60);
    QString comment = baToString(a, 386, 160);

    if (sampleId.isEmpty()) {
        QFileInfo fi(fname);
        sampleId = fi.fileName();
    }

    int gonioCode = BgmnFileIO::hex2int(a.mid(548, sizeInt));
    int gonioStageCode = BgmnFileIO::hex2int(a.mid(552, sizeInt));
    int sampleLoaderCode = BgmnFileIO::hex2int(a.mid(556, sizeInt));
    int gonioControllerCode = BgmnFileIO::hex2int(a.mid(560, sizeInt));
    float gonioRadius = BgmnFileIO::hex2float(a.mid(564, sizeFloat));
    float fdsDeg = BgmnFileIO::hex2float(a.mid(568, sizeFloat));
    float fssDeg = BgmnFileIO::hex2float(a.mid(572, sizeFloat));
    int pColCode = BgmnFileIO::hex2int(a.mid(576, sizeInt));
    int pMonCode = BgmnFileIO::hex2int(a.mid(580, sizeInt));
    float assDeg = BgmnFileIO::hex2float(a.mid(584, sizeFloat));
    float dsDeg = BgmnFileIO::hex2float(a.mid(588, sizeFloat));
    int sColCode = BgmnFileIO::hex2int(a.mid(592, sizeInt));
    int tfaCode = BgmnFileIO::hex2int(a.mid(596, sizeInt));
    int bFilterCode = BgmnFileIO::hex2int(a.mid(600, sizeInt));
    int sMonCode = BgmnFileIO::hex2int(a.mid(604, sizeInt));

    QString gonioCodeStr = "unknown";
    QString gonioStageCodeStr = "unknown";
    QString sampleLoaderCodeStr = "unknown";
    QString gonioControllerCodeStr = "unknown";
    QString pColCodeStr = "unknown";
    QString pMonCodeStr = "unknown";
    QString sColCodeStr = "unknown";
    QString tfaCodeStr = "unknown";
    QString bFilterCodeStr = "unknown";
    QString sMonCodeStr = "unknown";

    switch (gonioCode) {
        case 0: gonioCodeStr = "D5000 Theta/2Theta"; break;
        case 1: gonioCodeStr = "D5000 Theta/Theta"; break;
        case 2: gonioCodeStr = "D5000 matic"; break;
        case 3: gonioCodeStr = "D5000 Alpha-Theta"; break;
    }

    switch (gonioStageCode) {
        case 0: gonioStageCodeStr = "standard"; break;
        case 1: gonioStageCodeStr = "Synchronous rotation"; break;
        case 2: gonioStageCodeStr = "Rotation reflection"; break;
        case 3: gonioStageCodeStr = "Rotation transmission"; break;
        case 4: gonioStageCodeStr = "Open cradle"; break;
        case 5: gonioStageCodeStr = "Closed cradle"; break;
        case 6: gonioStageCodeStr = "Phi drive"; break;
        case 7: gonioStageCodeStr = "Chi drive"; break;
        case 8: gonioStageCodeStr = "XYZ"; break;
        case 9: gonioStageCodeStr = "Low temperature"; break;
        case 10: gonioStageCodeStr = "High temperature"; break;
        case 11: gonioStageCodeStr = "External temperature control"; break;
        case 12: gonioStageCodeStr = "other"; break;
    }

    switch (sampleLoaderCode) {
        case 0: sampleLoaderCodeStr = "None"; break;
        case 1: sampleLoaderCodeStr = "40 positions"; break;
        case 2: sampleLoaderCodeStr = "Y-matic"; break;
        case 3: sampleLoaderCodeStr = "XY-matic"; break;
    }

    switch (gonioControllerCode) {
        case 0: gonioControllerCodeStr = "standard"; break;
        case 1: gonioControllerCodeStr = "std + TC"; break;
        case 2: gonioControllerCodeStr = "std + FDC"; break;
        case 3: gonioControllerCodeStr = "std + TC + FDC"; break;
    }

    if (fdsDeg == 9999.0) fdsDeg = -1.0;
    if (fssDeg == 9999.0) fssDeg = -1.0;
    if (assDeg == 9999.0) assDeg = -1.0;
    if (dsDeg == 9999.0) dsDeg = -1.0;

    switch (pColCode) {
        case 0: pColCodeStr = "not present"; break;
        case 1: pColCodeStr = "present"; break;
    }

    switch (pMonCode) {
        case 0: pMonCodeStr = "none"; break;
        case 1: pMonCodeStr = "Transmission"; break;
        case 2: pMonCodeStr = "Reflection"; break;
        case 3: pMonCodeStr = "Ge220 2 bounce"; break;
        case 4: pMonCodeStr = "Ge220 4 bounce"; break;
        case 5: pMonCodeStr = "Ge440 4 bounce"; break;
    }

    switch (sColCode) {
        case 0: sColCodeStr = "not present"; break;
        case 1: sColCodeStr = "present"; break;
    }

    switch (tfaCode) {
        case 0: tfaCodeStr = "not present"; break;
        case 1: tfaCodeStr = "present"; break;
    }

    switch (bFilterCode) {
        case 0: bFilterCodeStr = "not present"; break;
        case 1: bFilterCodeStr = "present"; break;
    }

    switch (sMonCode) {
        case 0: sMonCodeStr = "none"; break;
        case 1: sMonCodeStr = "graphite"; break;
        case 2: sMonCodeStr = "LiF200"; break;
        case 3: sMonCodeStr = "Ge220 channel cut"; break;
    }

    output += QString("BrukerRawImport::readV3(): goniometer code = %1\n").arg(gonioCodeStr);
    output += QString("BrukerRawImport::readV3(): goniometer stage code = %1\n").arg(gonioStageCodeStr);
    output += QString("BrukerRawImport::readV3(): sample loader code = %1\n").arg(sampleLoaderCodeStr);
    output += QString("BrukerRawImport::readV3(): goniometer controller code = %1\n").arg(gonioControllerCodeStr);
    output += QString("BrukerRawImport::readV3(): goniometer radius = %1 mm\n").arg(gonioRadius, 0, 'f', 2);
    output += QString("BrukerRawImport::readV3(): fixed divergence slit = %1 degrees\n").arg(fdsDeg, 0, 'f', 3);
    output += QString("BrukerRawImport::readV3(): fixed sample slit = %1 degrees\n").arg(fssDeg, 0, 'f', 3);
    output += QString("BrukerRawImport::readV3(): incident Soller slit = %1\n").arg(pColCodeStr);
    output += QString("BrukerRawImport::readV3(): primary monochromator = %1\n").arg(pMonCodeStr);
    output += QString("BrukerRawImport::readV3(): fixed antiscatter slit = %1 degrees\n").arg(assDeg, 0, 'f', 3);
    output += QString("BrukerRawImport::readV3(): fixed detector slit = %1 degrees\n").arg(dsDeg, 0, 'f', 3);
    output += QString("BrukerRawImport::readV3(): diffracted Soller slit = %1\n").arg(sColCodeStr);
    output += QString("BrukerRawImport::readV3(): fixed thin film attachment = %1\n").arg(tfaCodeStr);
    output += QString("BrukerRawImport::readV3(): kbeta filter = %1\n").arg(bFilterCodeStr);
    output += QString("BrukerRawImport::readV3(): secondary monochromator = %1\n").arg(sMonCodeStr);

    QString nwl = baToString(a, 608, 4);
    double wlAvg = BgmnFileIO::hex2double(a.mid(616, sizeDouble));
    double wl1 = BgmnFileIO::hex2double(a.mid(624, sizeDouble));
    double wl2 = BgmnFileIO::hex2double(a.mid(632, sizeDouble));
    double wl3 = BgmnFileIO::hex2double(a.mid(640, sizeDouble));
    double wlRatio = BgmnFileIO::hex2double(a.mid(648, sizeDouble));
    QString wlUnit = baToString(a, 656, 4);

    output += QString("BrukerRawImport::readV3(): Wavelength: Wl1=%1 %5, Wl2=%2 %5, WL3=%3 %5, Average=%4 %5\n").arg(wl1).arg(wl2).arg(wl3).arg(wlAvg).arg(wlUnit);

    float totalTime = BgmnFileIO::hex2float(a.mid(664, sizeFloat));
    QString hwDep = baToString(a, 711, 1);

    Q_UNUSED(nwl);
    Q_UNUSED(wlRatio);
    Q_UNUSED(totalTime);
    Q_UNUSED(hwDep);

    int blkstart = 712;
    int i = 0;

    for (i = 0; i < nscans; ++i) {
        if (a.size() <= int(blkstart + 280 + sizeInt)) {
            qDebug()  << QString("BrukerRawImport::readV3(): Size of byte array too short. Skipping this scan.");
            break;
        }

        int hdrlen = BgmnFileIO::hex2int(a.mid(blkstart, sizeInt));
        int steps = BgmnFileIO::hex2int(a.mid(blkstart + 4, sizeInt));
        double startang = BgmnFileIO::hex2double(a.mid(blkstart + 16, sizeDouble));
        double vdsRange = BgmnFileIO::hex2double(a.mid(blkstart + 64));
        QString vdsRangeCode = baToString(a, blkstart + 72, 6);
        double assRange = BgmnFileIO::hex2double(a.mid(blkstart + 80));
        QString assRangeCode = baToString(a, blkstart + 88, 6);
        int detectorRange = BgmnFileIO::hex2int(a.mid(blkstart + 96, sizeInt));
        QString detSlitRangeCode = baToString(a, blkstart + 136, 5);
        int scanMode = BgmnFileIO::hex2int(a.mid(blkstart + 168, sizeInt));

        QString detectorRangeCode = "unknown";
        QString scanModeCode = "unknown";

        switch (detectorRange) {
            case 0: detectorRangeCode = "none"; break;
            case 1: detectorRangeCode = "SC"; break;
            case 2: detectorRangeCode = "SPC"; break;
            case 3: detectorRangeCode = "Solid State"; break;
            case 4: detectorRangeCode = "Other"; break;
            case 5: detectorRangeCode = "PSD"; break;
        }

        switch (scanMode) {
            case 0: scanModeCode = "step-scan"; break;
            case 1: scanModeCode = "continuous"; break;
        }

        double stepsize = BgmnFileIO::hex2double(a.mid(blkstart + 176, sizeDouble));
        double endang = startang + double(steps) * stepsize;
        float tperstep = BgmnFileIO::hex2float(a.mid(blkstart + 192, sizeFloat));
        wl1 = BgmnFileIO::hex2double(a.mid(blkstart + 240, sizeDouble));
        int suplheader = BgmnFileIO::hex2int(a.mid(blkstart + 256, sizeInt));

        output += QString("BrukerRawImport::readV3(): Scan %1 - Variable Divergence Slit Position = %2 deg\n").arg(i).arg(vdsRange);
        output += QString("BrukerRawImport::readV3(): Scan %1 - Variable Divergence Slit Code = %2\n").arg(i).arg(vdsRangeCode);
        output += QString("BrukerRawImport::readV3(): Scan %1 - Anti Scatter Slit Position = %2 deg\n").arg(i).arg(assRange);
        output += QString("BrukerRawImport::readV3(): Scan %1 - Anti Scatter Slit Code = %2\n").arg(i).arg(assRangeCode);
        output += QString("BrukerRawImport::readV3(): Scan %1 - Detector Slit Code = %2\n").arg(i).arg(detSlitRangeCode);
        output += QString("BrukerRawImport::readV3(): Scan %1 - Detector = %2\n").arg(i).arg(detectorRangeCode);
        output += QString("BrukerRawImport::readV3(): Scan %1 - Scan Mode = %2\n").arg(i).arg(scanModeCode);

        output += QString("BrukerRawImport::readV3(): Scan %1 - Number of Steps = %2\n").arg(i).arg(steps);
        output += QString("BrukerRawImport::readV3(): Scan %1 - Start Angle = %2\n").arg(i).arg(startang);
        output += QString("BrukerRawImport::readV3(): Scan %1 - Step Size = %2\n").arg(i).arg(stepsize);
        output += QString("BrukerRawImport::readV3(): Scan %1 - Time Per Step = %2\n").arg(i).arg(tperstep);
        output += QString("BrukerRawImport::readV3(): Scan %1 - WaveLength1 = %2\n").arg(i).arg(wl1);

        // initialize the vectors
        QVector<double> vec_a;       // angle
        QVector<double> vec_i;       // intensity

        // if more than one scan is read, append the number to the scan name
        QString sname = sampleId;
        if (nscans) {
            sname = QString("%1-%2").arg(sampleId).arg(i);
        }

        Scan scan(sname, QColor(), 1);
        scan.setSourceFileName(fname);
        scan.setAuxInfo("FormatVersion", QVariant("RAW1.01"));

        scan.setComment(comment);
        scan.setWaveLength(wl1);
        scan.setWaveLength2(wl2);
        scan.setWaveLength3(wl3);
        scan.setTimePerStep((double)tperstep);
        scan.setAuxInfo("Status", QVariant(status));
        scan.setAuxInfo("MeasureDate", QVariant(date));
        scan.setAuxInfo("MeasureTime", QVariant(time));

        int pos = blkstart + hdrlen + suplheader;

        for (int j = 0; j < steps; ++j) {
            if (pos >= a.size()) {
                qDebug()  << QString("BrukerRawImport::readV3(): Size of byte array too short. Aborting reading of this scan.");
                break;
            }

            double its = double(BgmnFileIO::hex2float(a.mid(pos, sizeFloat)));
            double ang = startang + (double(j) / double(steps - 1)) * (endang - startang);

            vec_a.push_back(ang);
            vec_i.push_back(its);
            pos += sizeFloat;
        }

        scan.setDataAng(vec_a);
        scan.setDataInt(vec_i);

        scan.setTypes(Scan::XY | Scan::MEASURED);
        scanHeap.push_back(scan);
        blkstart = pos;
    }

    // qDebug() << output;
    return i;
}

/*
 * NOTE: V4 support is based on the original Bruker header file V4_RAW_FORMAT.H,
 *       which was kindly provided by Bruker staff. Bruker's header file
 *       contains some wrong information (wrong array positions in the comments).
 *       These were fixed here by reverse engineering.
 */
int BrukerRawImport::readV4(const QByteArray &a, QVector<Scan> &scanHeap, const QString &fname, bool minimal)
{
    QString dbg;
    long pos = 0; // position pointer

    /* V4_RAW_FILE_HEADER */

    QDateTime szDate = QDateTime::fromString(baToString(a, 12, 12), "MM/dd/yyyy");
    QDateTime szTime = QDateTime::fromString(baToString(a, 24, 12), "hh:mm:ss");

    long iNoOfRanges = BgmnFileIO::hex2long(a.mid(40, sizeLong));
    long iNoOfMeasuredRanges = BgmnFileIO::hex2long(a.mid(44, sizeLong));
    long iExtraRecordSize = BgmnFileIO::hex2long(a.mid(56, sizeLong));

    bool szFurther_dql_reading = (QChar(a.at(60)) == QChar(0) ? false : true);

    // V4_RAW_FILE_HEADER debug output
    qDebug() << QString("BrukerRawImport::readV4(): File %1, measured %2, %3").arg(fname).arg(szDate.toString("dd.MM.yyyy")).arg(szTime.toString("hh:mm"));
    qDebug() << QString("BrukerRawImport::readV4(): Number of ranges: %1").arg(iNoOfRanges);
    qDebug() << QString("BrukerRawImport::readV4(): Number of measured ranges: %1").arg(iNoOfMeasuredRanges);
    qDebug() << QString("BrukerRawImport::readV4(): Contains further DQL data? %1").arg(szFurther_dql_reading ? QString("Yes") : QString("No"));

    // from now on, the position pointer is variable
    pos = 61;

    /* V4_RAW_VISUAL_EDIT_DQL */
    // if present, skip it

    if (szFurther_dql_reading) {
        pos += (int)BgmnFileIO::hex2long(a.mid(pos + sizeLong));
    }

    /* V4_VARIABLE_INFO */
    // loop through extra records

    QVariantHash varInfoMap;
    QVariantHash hwConfMap;

    // count the records, just for debug output
    int r = 1;

    while (pos < 61 + iExtraRecordSize) {
        // range check, give headroom to read two long variables
        if (pos >= a.size() - 8) {
            break;
        }

        int iRecordType = (int)BgmnFileIO::hex2long(a.mid(pos, sizeLong));
        int iRecordLength = (int)BgmnFileIO::hex2long(a.mid(pos + sizeLong, sizeLong));

        qDebug() << QString("BrukerRawImport::readV4(): Extra record %1 parsed").arg(r);

        // parse V4_VARIABLE_INFO records
        if (iRecordType == 10) {
            qDebug() << QString("BrukerRawImport::readV4(): Variable record at %1").arg(pos);
            varInfoMap.insert(readV4ExtraRecord10(a.mid(pos, iRecordLength), minimal));
        }

        // parse V4_RAW_HARDWARE_CONF
        if (iRecordType == 30) {
            qDebug() << QString("BrukerRawImport::readV4(): Hardware record at %1").arg(pos);
            hwConfMap.insert(readV4ExtraRecord30(a.mid(pos, iRecordLength), minimal));
        }

        // move to beginning of next block
        pos += iRecordLength;
        r++;
    }

    // here we dump the auxilary data for debug purpose
    QVariantHash::const_iterator itv = varInfoMap.constBegin();
    while (itv != varInfoMap.constEnd()) {
        // qDebug() << QString("BrukerRawImport::readV4(): Range Header aux info: %1 = %2\n").arg(itv.key(), itv.value().toString());
        dbg = dbg % QString("BrukerRawImport::readV4(): VAR aux info: %1 = %2\n").arg(itv.key()).arg(itv.value().toString());
        ++itv;
    }

    QVariantHash::const_iterator ith = hwConfMap.constBegin();
    while (ith != hwConfMap.constEnd()) {
        // qDebug() << QString("BrukerRawImport::readV4(): Range Header aux info: %1 = %2\n").arg(ith.key(), ith.value().toString());
        dbg = dbg % QString("BrukerRawImport::readV4(): HW aux info: %1 = %2\n").arg(ith.key()).arg(ith.value().toString());
        ++ith;
    }

    // extract information obtained from extra records
    QFileInfo fi(fname);
    QString name = varInfoMap.value("SAMPLEID", QVariant(fi.fileName())).toString();
    if (name.isEmpty()) name = fi.fileName();
    QString comment = varInfoMap.value("COMMENT", QVariant(QString())).toString();

    // extract information obtained from hardware config record
    double lambda1 = hwConfMap.value("fAlpha1", QVariant(1.540598)).toDouble();
    double lambda2 = hwConfMap.value("fAlpha2", QVariant(1.544426)).toDouble();

    // not sure whether iNoOfMeasuredRanges or iNoOfRanges actually describes
    // the number of ranges in the file. They should probably be identicaly anyway.
    // to be on the safe side, just loop over the smaller number of the two

    int cRanges = 0;

    for (int n = 0; n < qMin(iNoOfMeasuredRanges, iNoOfRanges); ++n) {
        /* V4_RAW_RANGE_HEADER */
        if (pos + 160 >= a.size()) {
            break;
        }

        QVariantHash rangeHeaderMap = readV4RangeHeader(a.mid(pos, 160), minimal);

        // here we dump the auxilary data for debug purpose
        QVariantHash::const_iterator it = rangeHeaderMap.constBegin();

        while (it != rangeHeaderMap.constEnd()) {
            // qDebug() << QString("BrukerRawImport::readV4(): Range Header aux info: %1 = %2\n").arg(it.key(), it.value().toString());
            dbg = dbg % QString("BrukerRawImport::readV4(): Range Header aux info: %1 = %2\n").arg(it.key(), it.value().toString());
            ++it;
        }

        double fStart = rangeHeaderMap.value("fStart", QVariant(-1.0)).toDouble();
        double fIncrement = rangeHeaderMap.value("fIncrement", QVariant(-1.0)).toDouble();
        long iSteps = (long)rangeHeaderMap.value("iSteps", QVariant(-1)).toInt();
        long iExtraRecordSize = (long)rangeHeaderMap.value("iExtraRecordSize",  QVariant(-1)).toInt();
        double fStepTime = rangeHeaderMap.value("fStepTime", -1.0).toDouble();
        double fEnd = fStart + double(iSteps) * fIncrement;
        int iNoCounts = rangeHeaderMap.value("iNoCounts", QVariant(-1)).toInt();

        if (fIncrement < 0.0) {
            qDebug() << QString("BrukerRawImport::readV4(): Illegal stepsize (%1)"
                              "read from file %2")
                      .arg(fIncrement)
                      .arg(fname);
            return cRanges;
        }

        pos += 160 + iExtraRecordSize;

        /* V4 Intensities */
        QList<Scan> scans;
        scans.reserve(iNoCounts);
        for (int j = 0; j < iNoCounts; ++j) {
            Scan s(name, QColor(), 1);
            s.setSourceFileName(fname);
            s.setAuxInfo("FormatVersion", QVariant("RAW4"));
            s.setName(name + (iNoCounts > 1 ? QString(" - %1").arg(j, 2, 10, QLatin1Char('0')) : QString()));
            s.setComment(comment);
            s.setAuxInfo("MeasureDate", QVariant(szDate));
            s.setAuxInfo("MeasureTime", QVariant(szTime));
            s.setWaveLength(lambda1);
            s.setWaveLength2(lambda2);
            s.setTimePerStep(fStepTime);
            s.pDataAngle().reserve(iSteps);
            s.pDataIntensity().reserve(iSteps);
            s.setStepSize(fIncrement);
            s.addAuxInfo(varInfoMap);
            s.addAuxInfo(hwConfMap);
            s.addAuxInfo(rangeHeaderMap);
            s.setTypes(Scan::XY | Scan::MEASURED);
            scans.append(s);
        }

        // read data
        for (long j = 0; j < iSteps; ++j) {
            for (int k = 0; k < iNoCounts; ++k) {
                if (pos >= a.size()) {
                    qDebug() << QString("BrukerRawImport::readV4(): Reached the end of the input array.");
                    return 0;
                }

                double ang = fStart + (double(j) / double(iSteps - 1)) * (fEnd - fStart);
                double its = double(BgmnFileIO::hex2float(a.mid(pos, sizeFloat)));

                scans[k].pDataAngle().push_back(ang);
                scans[k].pDataIntensity().push_back(its);

                pos += sizeFloat;
            }
        }

        for (int j = 0; j < scans.size(); ++j) {
            scanHeap.push_back(scans[j]);
        }

        cRanges += scans.size();
    }

    //qDebug() << dbg;
    return cRanges;
}

/*
 * parse V4_VARIABLE_INFO records
 */
QVariantHash BrukerRawImport::readV4ExtraRecord10(const QByteArray &a, bool minimal)
{
    QVariantHash auxInfo;

    if (a.size() < 36) {
        qDebug() << QString("BrukerRawImport::readV4ExtraRecord10(): Illegal array size: %1").arg(a.size());
        return auxInfo;
    }

    if (!minimal) {
        int iFlags = (int)BgmnFileIO::hex2long(a.mid(8, sizeLong));
        QString szType = baToString(a, 12, 24);

        // read data only if it is in ASCII format, ignore binary format
        QString data = (iFlags == 0 ? baToString(a, 36, a.size() - 36) : QString());
        auxInfo.insert(szType, QVariant(data));
    }

    return auxInfo;
}

/*
 * parse V4_VARIABLE_INFO records
 * if minimal=true, only the necessary information (e.g. wavelength) will be
 * parsed, other hardware info will be ignored
 */
QVariantHash BrukerRawImport::readV4ExtraRecord30(const QByteArray &a, bool minimal)
{
    QVariantHash auxInfo;

    if (a.size() < 136) {
        qDebug() << QString("BrukerRawImport::readV4ExtraRecord30(): Illegal array size: %1").arg(a.size());
        return auxInfo;
    }

    const int sizeFlags = 8 * sizeLong;

    if (!minimal) {
        // store the bits of iGoniomModel in an array of bools:
        // flagsGoniomModel[0] = "0x1"
        // flagsGoniomModel[1] = "0x2"
        // flagsGoniomModel[2] = "0x4"
        // flagsGoniomModel[3] = "0x8"
        // flagsGoniomModel[4] = "0x10"
        // flagsGoniomModel[5] = "0x20"
        // ...

        long iGoniomModel = BgmnFileIO::hex2long(a.mid(8, sizeLong));
        bool flagsGoniomModel[sizeFlags];

        for (int j = 0;  j < sizeFlags;  ++j) {
            flagsGoniomModel[j] = 0 != (iGoniomModel & (1 << j));
        }

        QStringList lGoniomModel;
        if (flagsGoniomModel[0]) lGoniomModel.append("D5000_TYPE");
        if (flagsGoniomModel[1]) lGoniomModel.append("D5005_TYPE");
        if (flagsGoniomModel[2]) lGoniomModel.append("D8_TYPE");
        if (flagsGoniomModel[3]) lGoniomModel.append("D500_TYPE");
        if (flagsGoniomModel[4]) lGoniomModel.append("OTHER_TYPE");
        if (flagsGoniomModel[5]) lGoniomModel.append("D4_TYPE");
        if (flagsGoniomModel[8]) lGoniomModel.append("THETA_2THETA");
        if (flagsGoniomModel[9]) lGoniomModel.append("THETA_THETA");
        if (flagsGoniomModel[10]) lGoniomModel.append("ALPHA_THETA");
        if (flagsGoniomModel[11]) lGoniomModel.append("MATIC");
        if (flagsGoniomModel[16]) lGoniomModel.append("GADDS");
        if (flagsGoniomModel[17]) lGoniomModel.append("SAXS");
        if (flagsGoniomModel[18]) lGoniomModel.append("SMART");
        if (flagsGoniomModel[19]) lGoniomModel.append("OTHER_SYSTEM");

        auxInfo.insert(QString("iGoniomModel"), QVariant(lGoniomModel));

        long iGoniomStage = BgmnFileIO::hex2long(a.mid(12, sizeLong));
        if (iGoniomStage == 0) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("STANDARD_STAGE")));
        if (iGoniomStage == 1) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("SYNCHR_ROT")));
        if (iGoniomStage == 2) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("ROT_REFLECTION")));
        if (iGoniomStage == 3) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("ROT_TRANSMISSION")));
        if (iGoniomStage == 4) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("OPEN_CRADLE")));
        if (iGoniomStage == 5) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("CLOSED_CRADLE")));
        if (iGoniomStage == 6) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("QUARTER_CRADLE")));
        if (iGoniomStage == 7) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("PHI_STAGE")));
        if (iGoniomStage == 8) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("CHI_STAGE")));
        if (iGoniomStage == 9) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("XYZ_STAGE")));
        if (iGoniomStage == 10) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("LOW_TEMP")));
        if (iGoniomStage == 11) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("HIGH_TEMP")));
        if (iGoniomStage == 12) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("EXTERNAL_TEMP")));
        if (iGoniomStage == 13) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("PHI_AT_FIXED_CHI")));
        if (iGoniomStage == 14) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("FOUR_CYCLE")));
        if (iGoniomStage == 15) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("SMALL_XYZ_STAGE")));
        if (iGoniomStage == 16) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("LARGE_XYZ_STAGE")));
        if (iGoniomStage == 17) auxInfo.insert(QString("iGoniomStage"), QVariant(QString("UNKNOWN")));

        long iSampleChanger = BgmnFileIO::hex2long(a.mid(16, sizeLong));
        if (iSampleChanger == 0) auxInfo.insert(QString("iSampleChanger"), QVariant(QString("NONE")));
        if (iSampleChanger == 1) auxInfo.insert(QString("iSampleChanger"), QVariant(QString("FOURTY_POSITION")));
        if (iSampleChanger == 2) auxInfo.insert(QString("iSampleChanger"), QVariant(QString("Y_MATIC")));
        if (iSampleChanger == 3) auxInfo.insert(QString("iSampleChanger"), QVariant(QString("XY_MATIC")));
        if (iSampleChanger == 4) auxInfo.insert(QString("iSampleChanger"), QVariant(QString("MANUAL")));
        if (iSampleChanger == 5) auxInfo.insert(QString("iSampleChanger"), QVariant(QString("UNKNOWN")));

        long iGoniomCtrl = BgmnFileIO::hex2long(a.mid(20, sizeLong));
        bool flagsGoniomCtrl[sizeFlags]; // 8 * sizeof(long)

        for (int j = 0;  j < sizeFlags;  ++j) {
            flagsGoniomCtrl[j] = 0 != (iGoniomCtrl & (1 << j));
        }

        QStringList lGoniomCtrl;
        if (flagsGoniomCtrl[0]) lGoniomCtrl.append("DIFF_CONT");
        if (flagsGoniomCtrl[1]) lGoniomCtrl.append("TC_SOC");
        if (flagsGoniomCtrl[2]) lGoniomCtrl.append("FDC_SOC");
        if (flagsGoniomCtrl[3]) lGoniomCtrl.append("TC_OTHER");
        if (flagsGoniomCtrl[4]) lGoniomCtrl.append("FDC_OTHER");
        if (flagsGoniomCtrl[5]) lGoniomCtrl.append("GGCS");
        if (flagsGoniomCtrl[6]) lGoniomCtrl.append("UNKNOWN");
        auxInfo.insert(QString("iGoniomCtrl"), QVariant(lGoniomCtrl));

        auxInfo.insert(QString("fGoniomDiameter"), QVariant(BgmnFileIO::hex2float(a.mid(24, sizeFloat))));

        long iSyncAxis = BgmnFileIO::hex2long(a.mid(28, sizeLong));
        if (iSyncAxis == 0) auxInfo.insert(QString("iSyncAxis"), QVariant(QString("NONE")));
        if (iSyncAxis == 1) auxInfo.insert(QString("iSyncAxis"), QVariant(QString("REFLECTION_PHI")));
        if (iSyncAxis == 2) auxInfo.insert(QString("iSyncAxis"), QVariant(QString("TRANSMISSION_PHI")));
        if (iSyncAxis == 3) auxInfo.insert(QString("iSyncAxis"), QVariant(QString("X_CLOSED_CRADLE")));

        long iBeamOpticsFlags = BgmnFileIO::hex2long(a.mid(32, sizeLong));
        bool flagsBeamOpticsFlags[sizeFlags]; // 8 * sizeof(long)

        for (int j = 0;  j < sizeFlags;  ++j) {
            flagsBeamOpticsFlags[j] = 0 != (iBeamOpticsFlags & (1 << j));
        }

        QStringList lBeamOpticsFlags;
        if (flagsBeamOpticsFlags[0]) lBeamOpticsFlags.append("DIVSLIT_SET");
        if (flagsBeamOpticsFlags[1]) lBeamOpticsFlags.append("NEAR_SAMPLE_SLIT_SET");
        if (flagsBeamOpticsFlags[2]) lBeamOpticsFlags.append("PRIM_SOLLER_SLIT_SET");
        if (flagsBeamOpticsFlags[3]) lBeamOpticsFlags.append("ANTISC_SLIT_SET");
        if (flagsBeamOpticsFlags[4]) lBeamOpticsFlags.append("DET_SLIT_SET");
        if (flagsBeamOpticsFlags[5]) lBeamOpticsFlags.append("SEC_SOLLER_SLIT_SET");
        if (flagsBeamOpticsFlags[6]) lBeamOpticsFlags.append("THINFILM_ATT_SET");
        if (flagsBeamOpticsFlags[7]) lBeamOpticsFlags.append("BETA_FILTER_SET");
        if (flagsBeamOpticsFlags[8]) lBeamOpticsFlags.append("MOT_SLIT_CHANGER_SET");
        if (flagsBeamOpticsFlags[9]) lBeamOpticsFlags.append("MOT_ABS_CHANGER_SET");
        if (flagsBeamOpticsFlags[10]) lBeamOpticsFlags.append("MOT_ROTARY_ABSORBER_SET");
        auxInfo.insert(QString("iBeamOpticsFlags"), QVariant(lBeamOpticsFlags));

        auxInfo.insert(QString("fDivSlit"), QVariant(BgmnFileIO::hex2float(a.mid(36, sizeFloat))));
        auxInfo.insert(QString("fNearSampleSlit"), QVariant(BgmnFileIO::hex2float(a.mid(40, sizeFloat))));
        auxInfo.insert(QString("fPrimSollerSlit"), QVariant(BgmnFileIO::hex2float(a.mid(44, sizeFloat))));

        long iMonochromator = BgmnFileIO::hex2long(a.mid(48, sizeLong));
        if (iMonochromator == 0) auxInfo.insert(QString("iMonochromator"), QVariant(QString("NONE")));
        if (iMonochromator == 1) auxInfo.insert(QString("iMonochromator"), QVariant(QString("TRANSMISSION_MONO")));
        if (iMonochromator == 2) auxInfo.insert(QString("iMonochromator"), QVariant(QString("REFLECTION_MONO")));
        if (iMonochromator == 3) auxInfo.insert(QString("iMonochromator"), QVariant(QString("GE220_2_BOUNCE")));
        if (iMonochromator == 4) auxInfo.insert(QString("iMonochromator"), QVariant(QString("GE220_4_BOUNCE")));
        if (iMonochromator == 5) auxInfo.insert(QString("iMonochromator"), QVariant(QString("GE440_4_BOUNCE")));
        if (iMonochromator == 6) auxInfo.insert(QString("iMonochromator"), QVariant(QString("FLAT_GRAPHITE_MONO")));
        if (iMonochromator == 7) auxInfo.insert(QString("iMonochromator"), QVariant(QString("SINGLE_GOEBEL_MIRROR")));
        if (iMonochromator == 8) auxInfo.insert(QString("iMonochromator"), QVariant(QString("CROSSED_GOEBEL_MIRROR")));
        if (iMonochromator == 9) auxInfo.insert(QString("iMonochromator"), QVariant(QString("FLAT_GERMANIUM_111")));
        if (iMonochromator == 10) auxInfo.insert(QString("iMonochromator"), QVariant(QString("FLAT_SILICON_111")));
        if (iMonochromator == 11) auxInfo.insert(QString("iMonochromator"), QVariant(QString("GE_REFLECTION")));
        if (iMonochromator == 12) auxInfo.insert(QString("iMonochromator"), QVariant(QString("ASYM_GE_4_BOUNCE")));
        if (iMonochromator == 13) auxInfo.insert(QString("iMonochromator"), QVariant(QString("UNKNOWN")));

        auxInfo.insert(QString("fAntiScSlit"), QVariant(BgmnFileIO::hex2float(a.mid(52, sizeFloat))));
        auxInfo.insert(QString("fDetSlit"), QVariant(BgmnFileIO::hex2float(a.mid(56, sizeFloat))));
        auxInfo.insert(QString("fSecondSollerSlit"), QVariant(BgmnFileIO::hex2float(a.mid(60, sizeFloat))));
        auxInfo.insert(QString("fThinFilmAtt"), QVariant(BgmnFileIO::hex2float(a.mid(64, sizeFloat))));

        long iAnalyzer = BgmnFileIO::hex2long(a.mid(68, sizeLong));
        if (iAnalyzer == 0) auxInfo.insert(QString("iAnalyzer"), QVariant(QString("NONE")));
        if (iAnalyzer == 1) auxInfo.insert(QString("iAnalyzer"), QVariant(QString("GRAPHITE_ANALYZER")));
        if (iAnalyzer == 2) auxInfo.insert(QString("iAnalyzer"), QVariant(QString("LIF_ANALYZER")));
        if (iAnalyzer == 3) auxInfo.insert(QString("iAnalyzer"), QVariant(QString("GE220_CHANNEL_CUT")));
        if (iAnalyzer == 4) auxInfo.insert(QString("iAnalyzer"), QVariant(QString("GOEBEL_MIRROR_ANALYZER")));
        if (iAnalyzer == 5) auxInfo.insert(QString("iAnalyzer"), QVariant(QString("UNKNOWN")));

        // wavelengths are read outside the "minimal" condition

        auxInfo.insert(QString("fAlphaRatio"), QVariant(BgmnFileIO::hex2double(a.mid(104, sizeDouble))));
        auxInfo.insert(QString("fBetaRelInt"), QVariant(BgmnFileIO::hex2float(a.mid(112, sizeFloat))));
        auxInfo.insert(QString("szAnode"), QVariant(baToString(a, 116, 4)));
        auxInfo.insert(QString("szWaveUnit"), QVariant(baToString(a, 120, 4)));
        auxInfo.insert(QString("fActivateAbsorber"), QVariant(BgmnFileIO::hex2float(a.mid(124, sizeFloat))));
        auxInfo.insert(QString("fDeactivateAbsorber"), QVariant(BgmnFileIO::hex2float(a.mid(128, sizeFloat))));
        auxInfo.insert(QString("fAbsFactor"), QVariant(BgmnFileIO::hex2float(a.mid(132, sizeFloat))));
    }

    // wavelengths
    auxInfo.insert(QString("fAlphaAverage"), QVariant(BgmnFileIO::hex2double(a.mid(72, sizeDouble))));
    auxInfo.insert(QString("fAlpha1"), QVariant(BgmnFileIO::hex2double(a.mid(80, sizeDouble))));
    auxInfo.insert(QString("fAlpha2"), QVariant(BgmnFileIO::hex2double(a.mid(88, sizeDouble))));
    auxInfo.insert(QString("fBeta"), QVariant(BgmnFileIO::hex2double(a.mid(96, sizeDouble))));

    return auxInfo;
}

QVariantHash BrukerRawImport::readV4RangeHeader(const QByteArray &a, bool minimal)
{
    QVariantHash auxInfo;

    if (a.size() < 160) {
        qDebug() << QString("BrukerRawImport::readV4RangeHeader(): Illegal array size: %1").arg(a.size());
        return auxInfo;
    }

    const int sizeFlags = 8 * sizeLong;

    if (!minimal) {
        // optional data
        auxInfo.insert(QString("iDataLength"), QVariant(BgmnFileIO::hex2long(a.mid(0, sizeLong))));
        auxInfo.insert(QString("iNoOfMeasuredData"), QVariant(BgmnFileIO::hex2long(a.mid(4, sizeLong))));
        auxInfo.insert(QString("iNoOfCompletedData"), QVariant(BgmnFileIO::hex2long(a.mid(8, sizeLong))));
        auxInfo.insert(QString("iNoOfConfDrives"), QVariant(BgmnFileIO::hex2long(a.mid(12, sizeLong))));

        long iMotSlitChangerIn = BgmnFileIO::hex2long(a.mid(16, sizeLong));
        if (iMotSlitChangerIn == 0) auxInfo.insert(QString("iMotSlitChangerIn"), QVariant(QString("MOT_CHANGER_OUT")));
        if (iMotSlitChangerIn == 1) auxInfo.insert(QString("iMotSlitChangerIn"), QVariant(QString("MOT_CHANGER_IN")));
        if (iMotSlitChangerIn == 2) auxInfo.insert(QString("iMotSlitChangerIn"), QVariant(QString("MOT_CHANGER_AUTO")));

        auxInfo.insert(QString("iNoOfDetectors"), QVariant(BgmnFileIO::hex2long(a.mid(20, sizeLong))));

        long iAdditionalDetectorFlags = BgmnFileIO::hex2long(a.mid(24, sizeLong));
        bool flagsAdditionalDetectorFlags[sizeFlags];

        for (int j = 0;  j < sizeFlags;  ++j) {
            flagsAdditionalDetectorFlags[j] = 0 != (iAdditionalDetectorFlags & (1 << j));
        }

        QStringList lAdditionalDetectorFlags;
        if (flagsAdditionalDetectorFlags[0]) lAdditionalDetectorFlags.append("PSD_SET");
        if (flagsAdditionalDetectorFlags[1]) lAdditionalDetectorFlags.append("AD_SET");
        if (flagsAdditionalDetectorFlags[2]) lAdditionalDetectorFlags.append("PSD_MEASURED");
        if (flagsAdditionalDetectorFlags[3]) lAdditionalDetectorFlags.append("AD_MEASURED");
        if (flagsAdditionalDetectorFlags[4]) lAdditionalDetectorFlags.append("PSD_SAVED");
        if (flagsAdditionalDetectorFlags[5]) lAdditionalDetectorFlags.append("AD_SAVED");
        if (flagsAdditionalDetectorFlags[6]) lAdditionalDetectorFlags.append("NONE");
        auxInfo.insert(QString("iAdditionalDetectorFlags"), QVariant(lAdditionalDetectorFlags));

        long iScanMode = BgmnFileIO::hex2long(a.mid(28, sizeLong));
        if (iScanMode == 0) auxInfo.insert(QString("iScanMode"), QVariant(QString("STEPSCAN")));
        if (iScanMode == 1) auxInfo.insert(QString("iScanMode"), QVariant(QString("CONTINUOUSSCAN")));
        if (iScanMode == 2) auxInfo.insert(QString("iScanMode"), QVariant(QString("CONTINUOUSSTEPSCAN")));

        auxInfo.insert(QString("szScanType"), QVariant(baToString(a, 32, 24)));
        auxInfo.insert(QString("iSynchRotation"), QVariant(BgmnFileIO::hex2long(a.mid(56, sizeLong))));
        auxInfo.insert(QString("fMeasDelayTime"), QVariant(BgmnFileIO::hex2float(a.mid(60, sizeFloat))));
        auxInfo.insert(QString("iEstScanTime"), QVariant(BgmnFileIO::hex2long(a.mid(64, sizeLong))));
        auxInfo.insert(QString("fRangeSampleStarted"), QVariant(BgmnFileIO::hex2float(a.mid(68, sizeFloat))));
        // fStart, fIncrement, iSteps, iExtraRecordSize, fStepTime are required
        auxInfo.insert(QString("fRotationSpeed"), QVariant(BgmnFileIO::hex2float(a.mid(96, sizeFloat))));
        auxInfo.insert(QString("fGeneratorVoltage"), QVariant(BgmnFileIO::hex2float(a.mid(100, sizeFloat))));
        auxInfo.insert(QString("fGeneratorCurrent"), QVariant(BgmnFileIO::hex2float(a.mid(104, sizeFloat))));
        auxInfo.insert(QString("iDisplayPlaneNumber"), QVariant(BgmnFileIO::hex2long(a.mid(108, sizeLong))));
        auxInfo.insert(QString("fActUsedLambda"), QVariant(BgmnFileIO::hex2double(a.mid(112, sizeDouble))));
        auxInfo.insert(QString("iNoOfVaryingParams"), QVariant(BgmnFileIO::hex2long(a.mid(120, sizeLong))));
        auxInfo.insert(QString("iNoEncoderDrives"), QVariant(BgmnFileIO::hex2long(a.mid(128, sizeLong))));


        long iExtraParamFlags = BgmnFileIO::hex2long(a.mid(132, sizeLong));
        bool flagsExtraParamFlags[sizeFlags]; // 8 * 4, 4 is the size of long on a 32bit system

        for (int j = 0;  j < sizeFlags;  ++j) {
            flagsExtraParamFlags[j] = 0 != (iExtraParamFlags & (1 << j));
        }

        QStringList lExtraParamFlags;

        if (flagsExtraParamFlags[0]) lExtraParamFlags.append("VARIABLE_TIME_PER_STEP");
        auxInfo.insert(QString("iExtraParamFlags"), QVariant(lExtraParamFlags));

        auxInfo.insert(QString("iDataRecordLength"), QVariant(BgmnFileIO::hex2long(a.mid(136, sizeLong))));
        // iExtraRecordSize is required
        auxInfo.insert(QString("fSmoothingWidth"), QVariant(BgmnFileIO::hex2float(a.mid(144, sizeFloat))));
        auxInfo.insert(QString("iSimMeasCond"), QVariant(BgmnFileIO::hex2long(a.mid(148, sizeLong))));
        auxInfo.insert(QString("fIncrement_3"), QVariant(BgmnFileIO::hex2double(a.mid(152, sizeDouble))));
    }

    // required data
    auxInfo.insert(QString("fStart"), QVariant(BgmnFileIO::hex2double(a.mid(72, sizeDouble))));
    auxInfo.insert(QString("fIncrement"), QVariant(BgmnFileIO::hex2double(a.mid(80, sizeDouble))));
    auxInfo.insert(QString("iSteps"), QVariant(BgmnFileIO::hex2long(a.mid(88, sizeLong))));
    auxInfo.insert(QString("fStepTime"), QVariant(BgmnFileIO::hex2float(a.mid(92, sizeFloat))));
    auxInfo.insert(QString("iExtraRecordSize"), QVariant(BgmnFileIO::hex2long(a.mid(140, sizeLong))));
    auxInfo.insert(QString("iNoCounts"), QVariant(BgmnFileIO::hex2long(a.mid(124, sizeLong))));

    return auxInfo;
}
