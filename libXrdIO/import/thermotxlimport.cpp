/***************************************************************************
                          thermotxlimport.cpp  -  description
                             -------------------
    begin                : Wed Nov 15 14:37:00 CEST 2023
    copyright            : (C) 2023 by Nicola Doebelin
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



#include "thermotxlimport.h"
#include "../functions.h"
#include <QtXml>
#include <QList>
#include <QBuffer>
#include <wchar.h>
#include <string.h>

ThermoTxlImport::ThermoTxlImport(QObject *parent)
    : GenericImport(parent)
{
    extens << "txl";
    descr = QLatin1String("Thermo Fisher XML file");
}

bool ThermoTxlImport::isSupported(const QByteArray &ba)
{
    static QRegularExpression rxTag("<TXL ");
    static QRegularExpression rxMaj("major=\"(\\d)\"");
    static QRegularExpression rxMin("minor=\"(\\d)\"");

    int major = -1;
    int minor = -1;

    QRegularExpressionMatch rm = rxTag.match(QString(ba));
    if (!rm.hasMatch()) return false;

    rm = rxMaj.match(QString(ba));
    if (rm.hasMatch()) major = rm.captured(1).toInt();

    rm = rxMin.match(QString(ba));
    if (rm.hasMatch()) minor = rm.captured(1).toInt();

    qDebug() << QString("ThermoTxlImport::isSupported(): Version %1.%2").arg(major).arg(minor);
    return ((major >= 1) && (minor >= 0));
}

int ThermoTxlImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    qDebug() << QString("ThermoTxlImport::load(): Loading file %1").arg(file);
    QString content(BgmnFileIO::readTextFile(file));

    if (content.isEmpty()) {
        qDebug() << QString("ThermoTxlImport::load(): No data found in file %1").arg(file);
        return -1;
    }

    QFileInfo finfo(file);
    _globalFileName = finfo.fileName();
    QDomDocument doc(finfo.completeBaseName());
    doc.setContent(content);

    QDomElement rootEl = doc.documentElement();
    QDomElement globalHeaderEl = rootEl.firstChildElement("Header");

    if (globalHeaderEl.isNull()) {
        qDebug() << QString("ThermoTxlImport::load(): Error finding global header in %1").arg(file);
        return -1;
    } else {
        parseGlobalHeader(globalHeaderEl);
    }

    QDomNodeList scanNodeList = rootEl.elementsByTagName("Scan");

    int n = 0;

    for (int i = 0; i < scanNodeList.size(); ++i) {
        Scan scan(QString(), QColor(), 1);
        scan.setSourceFileName(finfo.absoluteFilePath());

        if (parseScan(scanNodeList.at(i).toElement(), scan, scanNodeList.size() < 2 ? -1 : i)) {
            scanHeap.push_back(scan);
            ++n;
        }
    }

    return n;
}

void ThermoTxlImport::parseGlobalHeader(const QDomElement &el)
{
    parseGlobalOrigFileName(el);
    parseGlobalOperatorName(el);
    parseGlobalWaveLength(el);
}

bool ThermoTxlImport::parseScan(const QDomElement &el, Scan &scan, int n)
{
    int err = 0;

    err += parseScanHeader(el.firstChildElement("Header"), scan) ? 0 : 1;
    checkScanId(scan, _globalOrigFileName, n);

    err += parseScanData(el.firstChildElement("ScanData"), scan) ? 0 : 1;

    parseScanPeakData(el.firstChildElement("PeakSearch"), scan);

    return err == 0;
}

bool ThermoTxlImport::parseScanHeader(const QDomElement &elHeader, Scan &scan)
{
    if (elHeader.isNull()) return false;

    scan.setName(parseScanId(elHeader));
    scan.setWaveLength(_globalWaveLength);
    scan.setTypes(Scan::XY | Scan::MEASURED);

    return true;
}

void ThermoTxlImport::parseGlobalOrigFileName(const QDomElement &el)
{
    QDomElement gOFN = el.firstChildElement("OrigFilename");
    if (!gOFN.isNull()) _globalOrigFileName = gOFN.text();
    else                _globalOrigFileName = _globalFileName;
}

void ThermoTxlImport::parseGlobalOperatorName(const QDomElement &el)
{
    QDomElement gOPN = el.firstChildElement("OperatorName");
    if (!gOPN.isNull()) _globalOperatorName = gOPN.text();
    else                _globalOperatorName = QString();
}

void ThermoTxlImport::parseGlobalWaveLength(const QDomElement &el)
{
    QDomElement gWL = el.firstChildElement("Wavelength");

    if (gWL.isNull())  {
        _globalWaveLength = 1.54056;
        qDebug() << QString("ThermoTxlImport::parseGlobalWaveLength(): Wavelength element not found, falling back to 1.54056");
    } else {
        if (!xmlToDouble(gWL.text(), _globalWaveLength)) {
            _globalWaveLength = 1.54056;
            qDebug() << QString("ThermoTxlImport::parseGlobalWaveLength(): Reading wavelength failed, falling back to 1.54056");
        } else {
            qDebug() << QString("ThermoTxlImport::parseGlobalWaveLength(): Wavelength set to %1").arg(_globalWaveLength);
        }
    }
}

QString ThermoTxlImport::parseScanId(const QDomElement &el)
{
    QDomNodeList idList = el.elementsByTagName("SampleId");

    for (int i = 0; i < idList.size(); ++i) {
        QDomElement elId = idList.at(i).toElement();

        if (elId.isNull()) continue;

        if (elId.attribute("label") == "Name") {
            return elId.text();
        }
    }

    return _globalOrigFileName;
}

bool ThermoTxlImport::parseScanData(const QDomElement &elData, Scan &scan)
{
    if (elData.isNull()) return false;

    QDomElement elXdata = elData.firstChildElement("Xvalues");
    QDomElement elYdata = elData.firstChildElement("Yvalues");

    if (elXdata.isNull() || elYdata.isNull()) {
        qDebug() << QString("ThermoTxlImport::parseScanData(): Could not parse elements Xvalues or Yvalues. Exiting.");
        return false;
    }

    int nXdata = elXdata.attribute("count").toInt();
    int nYdata = elYdata.attribute("count").toInt();

    if ((nXdata < 2) || (nYdata < 2)) {
        qDebug() << QString("ThermoTxlImport::parseScanData(): No data values found (nXdata=%1, nYdata=%2). Exiting.").arg(nXdata).arg(nYdata);
        return false;
    }

    int n = qMin(nXdata, nYdata);
    qDebug() << QString("hermoTxlImport::parseScanData(): Parsing %1 data points").arg(n);

    QString dataStrX = elXdata.text().toUtf8();
    QString dataStrY = elYdata.text().toUtf8();

    QVector<double> vAng(n, 0.0);
    QVector<double> vInt(n, 0.0);

    int idx = 0;

    for (int i = 0; i < n; ++i) {
        if (idx > qMin(dataStrX.size(), dataStrY.size()) - 16) break;

        double dAng = 0.0;
        double dInt = 0.0;

        bool angOk = xmlToDoubleMapped(dataStrX.mid(idx, 16), dAng);
        bool intOk = xmlToDoubleMapped(dataStrY.mid(idx, 16), dInt);

        if (angOk && intOk) {
            vAng[i] = dAng;
            vInt[i] = dInt;
        }

        idx += 16;
    }

    scan.setDataAng(vAng);
    scan.setDataInt(vInt);

    return true;
}

bool ThermoTxlImport::parseScanPeakData(const QDomElement &elData, Scan &scan)
{
    if (elData.isNull()) return false;

    QDomElement elXdata = elData.firstChildElement("XPos");
    QDomElement elYdata = elData.firstChildElement("YPos");

    if (elXdata.isNull() || elYdata.isNull()) {
        qDebug() << QString("ThermoTxlImport::parseScanPeakData(): Could not parse elements XPos or YPos. Exiting.");
        return false;
    }

    int nXPos = elXdata.attribute("count").toInt();
    int nYPos = elYdata.attribute("count").toInt();

    if ((nXPos < 2) || (nYPos < 2)) {
        qDebug() << QString("ThermoTxlImport::parseScanPeakData(): No peaks found (nXPos=%1, nYPos=%2). Exiting.").arg(nXPos).arg(nYPos);
        return false;
    }

    int n = qMin(nXPos, nYPos);
    qDebug() << QString("ThermoTxlImport::parseScanPeakData(): Parsing %1 data points").arg(n);

    QString posStrX = elXdata.text().toUtf8();
    QString posStrY = elYdata.text().toUtf8();

    QVector<Hkl> vHkl(n);

    int idx = 0;

    for (int i = 0; i < n; ++i) {
        if (idx > qMin(posStrX.size(), posStrY.size()) - 16) break;

        double dAng = 0.0;
        double dInt = 0.0;

        bool angOk = xmlToDoubleMapped(posStrX.mid(idx, 16), dAng);
        bool intOk = xmlToDoubleMapped(posStrY.mid(idx, 16), dInt);

        if (angOk && intOk) {
            vHkl[i] = Hkl(dAng, QString(), 0, scan.name(), QColor(), dInt);
        }

        idx += 16;
    }

    scan.setHklData(vHkl);

    return true;
}

/*
 * Based on BEA-XML document provided by Thermo Fisher.
 * They use swscanf_s(), which is not available on Linux,
 * hence using swscanf() here.
 */
bool ThermoTxlImport::xmlToDouble(const QString &s, double &d)
{
    int n = swscanf(s.toStdWString().c_str(), L"%la", &d);

    if (n == 1) {
        return true;
    }

    return false;
}

/*
 * Based on BEA-XML document provided by Thermo Fisher.
 * This code should work on both little-endian and
 * big-endian architectures.
 */
bool ThermoTxlImport::xmlToDoubleMapped(const QString &s, double &d)
{
    union {
        double doubleValue;
        uint64_t intValue;
    } unionStruct = {0};

    int n = swscanf(s.toStdWString().c_str(), L"%llx", &unionStruct.intValue);

    if (n == 1) {
        d = unionStruct.doubleValue;
        return true;
    }

    return false;
}

void ThermoTxlImport::checkScanId(Scan &scan, const QString &fn, int n)
{
    if (scan.name().isEmpty()) {
        QFileInfo fi(fn);
        QString str = fi.completeBaseName();
        if (n >= 0) str += QString(" Scan %1").arg(n+1);
        scan.setName(str);
    }
}
