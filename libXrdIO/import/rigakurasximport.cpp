/***************************************************************************
                          brukerbrmlimport.cpp  -  description
                             -------------------
    begin                : Sun Aug 25 19:00:00 CEST 2013
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



#include "rigakurasximport.h"
#include <QtXml>
#include <QDomNamedNodeMap>
#include <QList>
#include <QBuffer>
#include "../../quazip/quazip.h"
#include "../../quazip/quazipfile.h"
#include "../../quazip/quaziodevice.h"
#include "../../zlib/zlib.h"

RigakuRasxImport::RigakuRasxImport(QObject *parent)
    : GenericImport(parent)
{
    extens << "rasx";
    descr = QLatin1String("Rigaku RASX archive");
    verbose = false;
}

bool RigakuRasxImport::isSupported(const QByteArray &ba)
{
    // magic number for zip archives
    if (ba.left(4).toHex() == "504b0304") return true;

    // neither of the two above
    return false;
}

int RigakuRasxImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    QFile f(file);
    f.open(QIODevice::ReadOnly);
    QDataStream in(&f);
    QByteArray buffer;
    buffer.resize(128);

    in.readRawData(buffer.data(), 128);

    // if it is a zip archive, use the corresponding function
    if (buffer.left(4).toHex() == "504b0304") return loadCompressedArchive(file, scanHeap, minimal);

    return 0;
}

int RigakuRasxImport::loadCompressedArchive( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);
    qDebug() << QString("RigakuRasxImport::loadCompressedArchive(): Loading file %1").arg(file);
    QuaZip zipFile(file);

    if (!zipFile.open(QuaZip::mdUnzip)) {
        qDebug() << QString("RigakuRasxImport::loadCompressedArchive(): Could not uncompress file %1").arg(file);
        zipFile.close();
        return 0;
    }

    QStringList zFiles = zipFile.getFileNameList();
    QStringList measCondFiles = zFiles.filter(QRegularExpression("MesurementConditions\\d+.xml"));
    QStringList profileFiles = zFiles.filter(QRegularExpression("Profile\\d+.txt"));

    if ((measCondFiles.size() <= 0) || (profileFiles.size() <= 0)) return 0;

    QVector<Scan> tScanHeap;
    tScanHeap.append(parseMetaData(file, measCondFiles.first()));

    // append scans that contain the correct metadata, but no scan data yet
    for (int i = 1; i < profileFiles.size(); ++i) {
        if (i >= measCondFiles.size()) {
            tScanHeap.append(tScanHeap.first());
        } else {
            tScanHeap.append(parseMetaData(file, measCondFiles.at(i)));
        }
    }

    for (int i = 0; i < tScanHeap.size(); ++i) {
        parseProfile(file, profileFiles.at(i), tScanHeap[i]);
    }

    zipFile.close();
    scanHeap.append(tScanHeap);

    return tScanHeap.size();
}

void RigakuRasxImport::parseProfile(const QString &zipArchive, const QString &file, Scan &s)
{
    static QRegularExpression rx("^(\\d+\\.?\\d*)\\s+(\\d+\\.?\\d*)(?:\\s+|$)");
    QuaZipFile dataFile(zipArchive, file, QuaZip::csSensitive, this);
    QTextStream stream(&dataFile);

    if (dataFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        while (!stream.atEnd()) {
            QString line = stream.readLine();
            QRegularExpressionMatch rm = rx.match(line);

            if (rm.hasMatch()) {
                s.pDataAngle().append(rm.captured(1).toDouble());
                s.pDataIntensity().append(rm.captured(2).toDouble());
            }
        }

        dataFile.close();
    }
}

Scan RigakuRasxImport::parseMetaData(const QString &zipArchive, const QString &file)
{
    QFileInfo finfo(file);
    QDomDocument doc(finfo.completeBaseName());

    // opening the file inside the archive and reading the content
    QuaZipFile dataFile(zipArchive, file, QuaZip::csSensitive, this);
    dataFile.open(QIODevice::ReadOnly);
    doc.setContent(dataFile.readAll());
    dataFile.close();

    // check if the file is of type "RawData", skip if not so
    QDomElement root = doc.documentElement();
    if (root.tagName() != "MeasurementConditions") {
        qDebug() << QString("RigakuRasxImport::loadCompressedArchive(): The file is not a RASX XML file %1").arg(file);
        return Scan();
    }

    QString label;
    QDomNodeList nodes = doc.elementsByTagName("SampleName");
    if (nodes.size()) label = nodes.at(0).toElement().text();

    Scan s(label, QColor(), 1);
    s.setSourceFileName(zipArchive);
    s.setTypes(Scan::XY | Scan::MEASURED);
    s.setXAxisLabel(QString("Diffraction Angle [%1%2%3]").arg(global::degree).arg(2).arg(global::theta));

    s.setWaveLength(getDoubleValue(doc.elementsByTagName("WavelengthKalpha1")));
    s.setWaveLength2(getDoubleValue(doc.elementsByTagName("WavelengthKalpha2")));
    s.setWaveLength3(getDoubleValue(doc.elementsByTagName("WavelengthKbeta")));

    double scStep  = getDoubleValue(doc.elementsByTagName("ScanInformation"), "Step");
    double scSpeed = getDoubleValue(doc.elementsByTagName("ScanInformation"), "Speed");
    QString scSpeedUnit = getStringValue(doc.elementsByTagName("ScanInformation"), "SpeedUnit");

    if (!qFuzzyIsNull(scSpeed)) {
        if (scSpeedUnit == "deg/min") s.setTimePerStep(scStep * 60.0 / scSpeed);
        else                          s.setTimePerStep(-1.0);
    } else {
        s.setTimePerStep(-1.0);
    }

    return s;
}

double RigakuRasxImport::getDoubleValue(const QDomNodeList &nodes)
{
    if (nodes.size()) {
        QDomElement e = nodes.at(0).toElement();
        if (!e.isNull()) return e.text().toDouble();
    }

    return -1.0;
}

double RigakuRasxImport::getDoubleValue(const QDomNodeList &nodes, const QString &child)
{
    if (nodes.size()) {
        QDomElement e = nodes.at(0).firstChildElement(child);
        if (!e.isNull()) return e.text().toDouble();
    }

    return -1.0;
}

QString RigakuRasxImport::getStringValue(const QDomNodeList &nodes)
{
    if (nodes.size()) {
        QDomElement e = nodes.at(0).toElement();
        if (!e.isNull()) return e.text();
    }

    return QString();
}

QString RigakuRasxImport::getStringValue(const QDomNodeList &nodes, const QString &child)
{
    if (nodes.size()) {
        QDomElement e = nodes.at(0).firstChildElement(child);
        if (!e.isNull()) return e.text();
    }

    return QString();
}
