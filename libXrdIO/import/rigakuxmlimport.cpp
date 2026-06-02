/***************************************************************************
                          rigakuxmlimport.cpp  -  description
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



#include "rigakuxmlimport.h"
#include <QtXml>
#include <QDomNamedNodeMap>
#include <QList>
#include <QBuffer>

RigakuXmlImport::RigakuXmlImport(QObject *parent)
    : GenericImport(parent)
{
    extens << "xml";
    descr = QLatin1String("Rigaku SmartLab XML file");
}

bool RigakuXmlImport::isSupported(const QByteArray &ba)
{
    return ba.contains("SmartLabFileInfo");
}

int RigakuXmlImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    // read the text file
    qDebug() << QString("RigakuXmlImport::load(): Loading file %1").arg(file);
    QString content(BgmnFileIO::readTextFile(file));

    if (content.isEmpty()) {
        qDebug() << QString("RigakuXmlImport::load(): No data found in file %1").arg(file);
        return -1;
    }

    // create a domdocument with the name of the file's basename and set the content
    QFileInfo finfo(file);
    QDomDocument doc(finfo.completeBaseName());
    doc.setContent(content);

    QDomNodeList dnLstMeasFile = doc.elementsByTagName("MeasFile");
    //QDomNodeList dnLstMeasCond = doc.elementsByTagName("MeasCondFile");
    //QDomNodeList dnLstMeasPDF  = doc.elementsByTagName("MeasPDFFile");

    int n = 0;

    for (int i = 0; i < dnLstMeasFile.size(); ++i) {
        QDomElement e = dnLstMeasFile.at(i).toElement();
        QByteArray binString = e.text().toUtf8();

        QString sdata = QString(QByteArray::fromBase64(binString));
        QStringList dataLst(sdata.split("\n"));

        QList<QStringList> header = getHeaderBlocks(dataLst);
        QList<QStringList> data = getDataBlocks(dataLst);

        for (int j = 0; j < qMin(data.size(), header.size()); ++j) {
            QString label(getComment(header.at(j)));

            double wlKa1 = getWaveLength(header.at(j), 0);
            double wlKa2 = getWaveLength(header.at(j), 1);
            double wlKb  = getWaveLength(header.at(j), 2);

            Scan scan(label, QColor(), 1);
            scan.setSourceFileName(finfo.absoluteFilePath());

            scan.setWaveLength(wlKa1);
            scan.setWaveLength2(wlKa2);
            scan.setWaveLength3(wlKb);

            parseDataBlocks(scan, data.at(j));

            scan.setTypes(Scan::XY | Scan::MEASURED);
            scanHeap.push_back(scan);
        }
    }

    return n;
}

QList<QStringList> RigakuXmlImport::getHeaderBlocks(const QStringList &c)
{
    QList<QStringList> l;

    int start = c.indexOf(QRegularExpression("\\*RAS_HEADER_START"), 0);
    int end   = c.indexOf(QRegularExpression("\\*RAS_HEADER_END"), 0);

    while (start >= 0) {
        l.append(c.mid(start, end - start + 1));

        start = c.indexOf(QRegularExpression("\\*RAS_HEADER_START"), end + 1);
        end   = c.indexOf(QRegularExpression("\\*RAS_HEADER_END"), end + 1);
    }

    return l;
}

QList<QStringList> RigakuXmlImport::getDataBlocks(const QStringList &c)
{
    QList<QStringList> l;

    int start = c.indexOf(QRegularExpression("\\*RAS_INT_START"), 0);
    int end   = c.indexOf(QRegularExpression("\\*RAS_INT_END"), 0);

    while (start >= 0) {
        l.append(c.mid(start + 1, end - start - 1));

        start = c.indexOf(QRegularExpression("\\*RAS_INT_START"), end + 1);
        end   = c.indexOf(QRegularExpression("\\*RAS_INT_END"), end + 1);
    }

    return l;
}

double RigakuXmlImport::getWaveLength(const QStringList &h, int n)
{
    QRegularExpression rxKa1("\\*HW_XG_WAVE_LENGTH_ALPHA1\\s+\"(\\d+\\.?\\d*)\"");
    QRegularExpression rxKa2("\\*HW_XG_WAVE_LENGTH_ALPHA2\\s+\"(\\d+\\.?\\d*)\"");
    QRegularExpression rxKb("\\*HW_XG_WAVE_LENGTH_BETA\\s+\"(\\d+\\.?\\d*)\"");

    QRegularExpression rx;
    if (n == 0) rx = rxKa1;
    if (n == 1) rx = rxKa2;
    if (n == 2) rx = rxKb;

    int i = h.indexOf(rx);
    if (i < 0) return 0.0;

    QRegularExpressionMatch rm = rx.match(h.at(i));
    if (rm.hasMatch()) return rm.captured(1).toDouble();
    return 0.0;
}

QString RigakuXmlImport::getComment(const QStringList &h)
{
    static QRegularExpression rxfs("\\*FILE_SAMPLE\\s+\"([^\"]+)\"");
    static QRegularExpression rxfm("\\*FILE_MEMO\\s+\"([^\"]+)\"");
    static QRegularExpression rxfc("\\*FILE_COMMENT\\s+\"([^\"]+)\"");
    static QRegularExpression rxdn("\\*DISP_NOTE\\s+\"([^\"]+)\"");

    int idxfs = h.indexOf(rxfs);
    int idxfm = h.indexOf(rxfm);
    int idxfc = h.indexOf(rxfc);
    int idxdn = h.indexOf(rxdn);

    QRegularExpressionMatch rm;
    QStringList out;

    rm = rxfs.match(h.at(idxfs));
    if (rm.hasMatch()) out.append(rm.captured(1));

    rm = rxfm.match(h.at(idxfm));
    if (rm.hasMatch()) out.append(rm.captured(1));

    rm = rxfc.match(h.at(idxfc));
    if (rm.hasMatch()) out.append(rm.captured(1));

    rm = rxdn.match(h.at(idxdn));
    if (rm.hasMatch()) out.append(rm.captured(1));

    return out.join(" ");
}

void RigakuXmlImport::parseDataBlocks(Scan &scan, const QStringList &data)
{
    qDebug() << QString("RigakuRasImport::parseDataBlocks(): Parsing data block with %1 lines").arg(data.size());
    QVector<double> angle;
    QVector<double> intens;

    QString dpattern("[+-]?\\d+\\.?\\d*(?:[eE][+-]?\\d+)?");
    QRegularExpression rx(QString("^\\s*(%1)\\s+(%1)(?:\\s+%1)\\s*$").arg(dpattern));
    QRegularExpressionMatch rm;

    for (int i = 0; i < data.size(); ++i) {
        rm = rx.match(data.at(i));
        if (rm.hasMatch()) {
            angle.append(rm.captured(1).toDouble());
            intens.append(rm.captured(2).toDouble());
        }
    }

    qDebug() << QString("RigakuRasImport::parseDataBlocks(): %1 data points appended to scan").arg(angle.size());

    scan.pDataAngle() = angle;
    scan.pDataIntensity() = intens;
}
