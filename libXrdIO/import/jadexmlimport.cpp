/***************************************************************************
                          jadexmlimport.cpp  -  description
                             -------------------
    begin                : Sun Aug 26 16:00:00 CEST 2013
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



#include "jadexmlimport.h"
#include <QtXml>

JadeXmlImport::JadeXmlImport(QObject *parent)
    : GenericImport(parent)
{
    extens << "xml";
    descr = QLatin1String("MDI Jade XML Scan");
}

bool JadeXmlImport::isSupported(const QByteArray &ba)
{
    if (ba.contains("<MDI ")) {
        return true;
    }

    return false;
}

int JadeXmlImport::load(const QString &filename, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    QStringList content(BgmnFileIO::readTextFileLines(filename));

    if (content.isEmpty()) {
        qDebug() << QString("JadeXmlImport::load(): No scans found in file %1").arg(filename);
        return -1;
    }

    // create a domdocument with the name of the file's basename
    QFileInfo finfo(filename);
    QDomDocument doc(finfo.completeBaseName());
    doc.setContent(content.join("\n"));

    // check if the file is of type "xrdMeasurements", abort if not so
    QDomElement root = doc.documentElement();
    if (root.tagName() != "MDI") {
        qDebug() << QString("JadeXmlImport::load(): Not a XRDML file: %1").arg(filename);
        return -1;
    }

    QDomNodeList traces = root.elementsByTagName("ScanTrace");

    for (int i = 0; i < traces.count(); ++i) {
        QDomNode traceNode = traces.at(i);
        QDomNode startAngNode = traceNode.namedItem("StartAngle");
        QDomNode endAngNode = traceNode.namedItem("EndAngle");
        QDomNode stepSizeNode = traceNode.namedItem("AngleStep");
        QDomNode stepCTimeNode = traceNode.namedItem("StepCountTime");

        double startang = 0.0;
        double endang = 0.0;
        double stepsize = 0.0;
        double timePerStep = -1.0;

        if (startAngNode.isElement()) {
            startang = startAngNode.toElement().text().toDouble();
        }

        if (endAngNode.isElement()) {
            endang = endAngNode.toElement().text().toDouble();
        }

        if (stepSizeNode.isElement()) {
            stepsize = stepSizeNode.toElement().text().toDouble();
        }

        if (stepCTimeNode.isElement()) {
            timePerStep = stepCTimeNode.toElement().text().toDouble();
            qDebug() << QString("JadeXmlImport::load(): Read timePerStep = %1").arg(timePerStep);
        }

        qDebug() << QString("JadeXmlImport::load(): Read startang = %1, endang = %2, stepsize = %3").arg(startang).arg(endang).arg(stepsize);

        if ((startang <= 0.0) || (endang <= 0.0) || (stepsize <= 0.0)) {
            qDebug() << QString("JadeXmlImport::load(): Could not read startAngle, endAngle, or stepSize of scan %1").arg(i);
            continue;
        }

        // read the unit of the x-axis
        QString xaxis = startAngNode.toElement().attribute("unit");

        // create a new scan
        Scan scan(QString("%1_%2").arg(filename).arg(i), QColor(), 1);
        scan.setSourceFileName(filename);
        QVector<double> vec_a;
        QVector<double> vec_i;
        scan.setXAxisLabel(xaxis);
        scan.setTimePerStep(timePerStep);

        // locate the "Data" node of the ScanTrace
        QDomNode dataNode = traceNode.namedItem("Data");


        // extracting data from the <Data> node is a bit complicated, because according to the
        // Jade user manual, there are different ways of storing intensities and angles.
        // here we have to consider all varieties.

        if (dataNode.isElement()) {
            QString format = dataNode.toElement().attribute("type");

            // is the data stored as binary base64 block?
            if (dataNode.toElement().attribute("dt:dt") == "bin.base64") {
                // convert the data to QByteArray and decode from base64 format
                qDebug() << QString("JadeXmlImport::load(): Reading data in bin.base64 format");
                QByteArray data = QByteArray::fromBase64(dataNode.toElement().text().toLatin1());
                getBinaryScan(data, format, startang, stepsize, vec_a, vec_i);
            } else {
                // data in ascii text format?
                // we have to read attribute "n" and use two different approaches, one for n=0, and another
                // one for n>0
                bool ok;
                int n = dataNode.toElement().attribute("n").toInt(&ok);

                if (!ok) {
                    qDebug() << QString("JadeXmlImport::load(): Could not extract intensities from ScanTrace %1").arg(i);
                    continue;
                }

                qDebug() << QString("JadeXmlImport::load(): Reading data in ascii format with attribute n=%1").arg(n);

                // all intensity values stored in the data tag?
                if (n == 0) {
                    QStringList values = dataNode.toElement().text().split(" ", Qt::SkipEmptyParts);

                    double ang = startang;
                    for (int v = 0; v < values.size(); v++) {
                        vec_i.push_back(values.at(v).toDouble());
                        vec_a.push_back(ang);
                        ang += stepsize;
                    }
                } else {
                    // one or several values stored in a tag of type <y ...>val1 val2 val3 ... </y>
                    QDomNodeList values = dataNode.toElement().elementsByTagName("y");

                    for (int v = 0; v < values.count(); v++) {
                        // split the line into single values (for n=1 this will only be one entry per line)
                        QStringList vl = values.at(v).toElement().text().split(" ", Qt::SkipEmptyParts);
                        int p = vl.size();
                        for (int z = 0; z < p; z++) {
                            double ang = startang + (double(z) / double(p - 1)) * (endang - startang);
                            double its = vl.at(z).toDouble();

                            vec_i.push_back(its);
                            vec_a.push_back(ang);
                        }
                    }
                }
            }
        } else {
            qDebug() << QString("JadeXmlImport::load(): Could not extract data from scan %1").arg(i);
            continue;
        }


        scan.setDataAng(vec_a);
        scan.setDataInt(vec_i);
        scan.setTypes(Scan::XY | Scan::MEASURED);
        scanHeap.push_back(scan);
    }

    return traces.count();
}

bool JadeXmlImport::getBinaryScan(const QByteArray &data, const QString &format, double startAng, double stepSize, QVector<double> &vec_a, QVector<double> &vec_i)
{
    int skip;
    int mode = 0;

    if (format.toLower() == "float") {
        skip = sizeof(float);
        mode = 1;
    } else if (format.toLower() == "int") {
        skip = sizeof(int);
        mode = 2;
    } else if (format.toLower() == "double") {
        skip = sizeof(double);
        mode = 3;
    } else return false;

    int p = data.size() / skip;
    double endAng = startAng + double(p) * stepSize;
    if (p < 2) return false;

    // if it's float values, read them
    if (mode == 1) {
        for (int n = 0; n < p; ++n) {
            double ang = startAng + (double(n) / double(p - 1)) * (endAng - startAng);
            double its = double(BgmnFileIO::hex2float(data.mid(n, skip)));
            vec_i.push_back(its);
            vec_a.push_back(ang);
        }
    }

    // not sure if other number formats than float (here int) even exist
    // but let's try to capture them
    if (mode == 2) {
        for (int n = 0; n < p; ++n) {
            double ang = startAng + (double(n) / double(p - 1)) * (endAng - startAng);
            double its = double(BgmnFileIO::hex2int(data.mid(n, skip)));
            vec_i.push_back(its);
            vec_a.push_back(ang);
        }
    }

    // not sure if other number formats than float (here double) even exist
    // but let's try to capture them
    if (mode == 3) {
        for (int n = 0; n < p; ++n) {
            double ang = startAng + (double(n) / double(p - 1)) * (endAng - startAng);
            double its = double(BgmnFileIO::hex2double(data.mid(n, skip)));
            vec_i.push_back(its);
            vec_a.push_back(ang);
        }
    }

    return mode > 0;
}
