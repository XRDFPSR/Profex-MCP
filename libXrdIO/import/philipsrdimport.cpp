/***************************************************************************
                          philipsrdimport.cpp  -  description
                             -------------------
    begin                : Thu May 30, 2013
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

#include "philipsrdimport.h"

// maybe these should be hard coded, because the sizes must match the
// RD file, not the current platform
#define sizeFloat 4
#define sizeDouble 8
#define sizeUshort 2
#define sizeInt 4
#define sizeLong 4

PhilipsRdImport::PhilipsRdImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "rd";
    descr = QLatin1String("Philips RD scan");
}

bool PhilipsRdImport::isSupported(const QByteArray &ba)
{
    if ((ba.left(4) == "V3RD") || (ba.left(4) == "V5RD")) {
        return true;
    }

    return false;
}

int PhilipsRdImport::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    qDebug() << QString("PhilipsRdImport::load(): Loading file %1").arg(file);
    QByteArray content;
    BgmnFileIO::readBinaryFile(file, content);

    if (content.isEmpty()) {
        qDebug() << QString("PhilipsRdImport::load(): byte array is empty in file %1").arg(file);
        return -1;
    }

    if (content.size() < 250) {
        qDebug() << QString("PhilipsRdImport::load(): No valid data found in file %1").arg(file);
        return -1;
    }

    // tests passed, so continue with parsing

    // translate some indexes into human readable values
    QStringList anode;
    anode << "Cu" << "Mo" << "Fe" << "Cr" << "Other";

    QStringList focus;
    focus << "BF" << "NF" << "FF" << "LFF";

    QStringList instrument;
    instrument << "PW1800" << "PW1710 based system"
               << "PW1840" << "PW3710 based system"
               << "Undefined" << "X'Pert MPD";

    // initialize the vectors
    QVector<double> vec_a;       // angle
    QVector<double> vec_i;       // intensity

    Scan scan(file, QColor(), 1);
    scan.setSourceFileName(file);

    QString version = baToString(content, 0, 4);
    scan.setAuxInfo("FileFormatVersion", version);

    int dtype = BgmnFileIO::char2int(content.mid(84, 1));
    if (dtype < instrument.size()) {
        scan.setAuxInfo("Instrument", instrument.at(dtype));
        qDebug() << QString("PhilipsRdImport::load(): Instrument = %1 (%2)").arg(instrument.at(dtype)).arg(dtype);
    }

    int ano = BgmnFileIO::char2int(content.mid(85, 1));
    if (ano < anode.size() - 1) {
        scan.setNamedWaveLength(anode.at(ano));
        qDebug() << QString("PhilipsRdImport::load(): Anode = %1 (%2)").arg(anode.at(ano)).arg(ano);
    }

    int foc = BgmnFileIO::char2int(content.mid(86, 1));
    if (foc < focus.size()) {
        scan.setAuxInfo("FocusType", focus.at(foc));
        qDebug() << QString("PhilipsRdImport::load(): FocusType = %1 (%2)").arg(focus.at(foc)).arg(foc);
    }

    scan.setName(baToString(content, 138, 8));
    scan.setComment(baToString(content, 146, 20));

    double stepSize = BgmnFileIO::hex2double(content.mid(214, sizeDouble));
    double startAng = BgmnFileIO::hex2double(content.mid(222, sizeDouble));
    double endAng = BgmnFileIO::hex2double(content.mid(230, sizeDouble));

    bool b;

    qDebug() << QString("PhilipsRdImport::load(): Version = %1").arg(scan.auxInfo("FileFormatVersion", b).toString());
    qDebug() << QString("PhilipsRdImport::load(): Name = %1").arg(scan.name());
    qDebug() << QString("PhilipsRdImport::load(): Comment = %1").arg(scan.comment());
    qDebug() << QString("PhilipsRdImport::load(): stepSize = %1").arg(stepSize, 0, 'f', 8);
    qDebug() << QString("PhilipsRdImport::load(): startAng = %1").arg(startAng, 0, 'f', 8);
    qDebug() << QString("PhilipsRdImport::load(): endAng = %1").arg(endAng, 0, 'f', 8);

    // check if any of the read values is 0 or negative
    if (stepSize * startAng * endAng <= 0.0) {
        qDebug() << QString("PhilipsRdImport::load(): No valid stepSize, startAng, or endAng found in file %1").arg(file);
        return -1;
    }

    int count = (endAng - startAng)/stepSize + 1;

    int i = 250;
    if (version.left(2).toUpper() == "V5") {
        i = 810;
    }

    for (int n = 0; n < count; ++n) {

        // make sure we don't run over the end of the bytearray
        if (i > content.size() - sizeUshort) {
            break;
        }

        double ang = startAng + (double(n) / double(count - 1)) * (endAng - startAng);
        double val = double(BgmnFileIO::hex2ushort(content.mid(i, sizeUshort)));

        // linear intensity is val²/100
        val *= val/100.0;

        vec_a.push_back(ang);
        vec_i.push_back(val);

        // move forward in the bytearray
        i += sizeUshort;
    }

    scan.setDataAng(vec_a);
    scan.setDataInt(vec_i);
    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);

    return scanHeap.size();
}
