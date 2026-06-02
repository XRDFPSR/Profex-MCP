/***************************************************************************
                          asciihklexport.cpp  -  description
                             -------------------
    begin                : Sat Jan 16, 2016
    copyright            : (C) 2016 by Nicola Doebelin
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

#include "asciihklexport.h"
#include "bgmnfileio.h"
#include "functions.h"

AsciiHklExport::AsciiHklExport(QObject *parent) :
    GenericExport(parent)
{
    uId = "ASCII_HKL";
}

int AsciiHklExport::save(const QString &file, const Scan &scan, const QMap<QString, QVariant> &flags)
{
    QString data = getData(scan, flags.value("fieldSeparator", " ").toString(), true, 0);
    bool b = BgmnFileIO::writeTextFile(file, data);

    if (b) return 1;
    return 0;
}

int AsciiHklExport::save(const QString &file, const QVector<Scan> &scanHeap, const QMap<QString, QVariant> &flags)
{
    QString data = getData(scanHeap, flags.value("fieldSeparator", " ").toString());
    bool b = BgmnFileIO::writeTextFile(file, data);

    if (b) return qMax(scanHeap.size() - 4, 1);
    return 0;
}


QString AsciiHklExport::getData(const Scan &scan, const QString &sep, bool header, int phNumber)
{
    QString data;

    if (!scan.pDataHkl().size()) {
        qDebug() << QString("AsciiHklExport::save(): Scan %1 does not contain HKL data").arg(scan.name());
        return data;
    }

    QTextStream stream(&data);
    stream.setRealNumberNotation(QTextStream::FixedNotation);
    stream.setRealNumberPrecision(6);

    if (header) {
        stream << QStringLiteral("\"Phase Name\"") << sep;
        stream << QString("\"Diffraction Angle [%1%2%3]\"").arg(global::degree, "2", global::theta) << sep;
        stream << QStringLiteral("\"Phase No.\"") << sep;
        stream << QStringLiteral("\"HKL\"") << sep;
        stream << QStringLiteral("\"Texture\"\n");
    }

    QString name = QString("\"%1\"").arg(scan.name());

    for (int i = 0; i < scan.pDataHkl().size(); i++) {
        stream << name << sep;
        stream << global::Functions::dToTwoTheta(scan.pDataHkl().at(i).position(), 0.1*scan.waveLength()) << sep;
        stream << QString("%1").arg(phNumber) << sep;
        stream << QString("\"%1\"").arg(scan.pDataHkl().at(i).hkl()) << sep;
        stream << scan.pDataHkl().at(i).texture() << "\n";
    }

    return data;
}

QString AsciiHklExport::getData(const QVector<Scan> &scanHeap, const QString &sep)
{
    QString data;
    QTextStream stream(&data);
    stream.setRealNumberNotation(QTextStream::FixedNotation);
    stream.setRealNumberPrecision(6);

    stream << QStringLiteral("\"Phase Name\"") << sep;
    stream << QStringLiteral("\"Diffraction Angle (deg)\"") << sep;
    stream << QStringLiteral("\"Phase No.\"") << sep;
    stream << QStringLiteral("\"HKL\"") << sep;
    stream << QStringLiteral("\"Texture\"\n");

    int n = 0;

    for (int j = 0; j < scanHeap.size(); ++j) {
        if (!scanHeap.at(j).pDataHkl().size()) continue;

        QString name = QString("\"%1\"").arg(scanHeap[j].name());

        for (int i = 0; i < scanHeap.at(j).pDataHkl().size(); i++) {
            stream << name << sep;
            stream << global::Functions::dToTwoTheta(scanHeap.at(j).pDataHkl().at(i).position(), 0.1*scanHeap.at(j).waveLength()) << sep;
            stream << n << sep;
            stream << QString("\"%1\"").arg(scanHeap.at(j).pDataHkl().at(i).hkl()) << sep;
            stream << scanHeap.at(j).pDataHkl().at(i).texture() << "\n";
        }
    }

    return data;
}

