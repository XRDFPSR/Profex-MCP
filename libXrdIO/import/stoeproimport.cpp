/***************************************************************************
                          stoeproimport.cpp  -  description
                             -------------------
    begin                : Thu Jun 18, 2013
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

#include "stoeproimport.h"

StoeProImport::StoeProImport(QObject *parent) :
    GenericImport(parent)
{
    extens << "pro";
    descr = QLatin1String("Stoe PRO scan");
}

bool StoeProImport::isSupported(const QByteArray &ba)
{
    QStringList header = QString(ba).split(global::rxLineEnding);

    if (header.size() < 2) {
        return false;
    }

    QRegularExpression rxh("^(?:\\s*\\d+\\.\\d+){2,2}\\s*$");
    QRegularExpression rxd("^(?:\\s*\\d+\\.\\d*){10,10}\\s*$");
    QRegularExpressionMatch rmh = rxh.match(header.at(0));
    QRegularExpressionMatch rmd = rxd.match(header.at(1));

    return (rmh.hasMatch() && rmd.hasMatch());
}

int StoeProImport::load(const QString &file, QVector<Scan> &scanHeap, bool minimal)
{
    Q_UNUSED(minimal);

    qDebug() << QString("StoeProImport::load(): Loading file %1").arg(file);
    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.size() < 2) {
        qDebug() << QString("StoeProImport::load(): No valid content found in file %1").arg(file);
        return -1;
    }

    Scan scan(QString(), QColor(), 1);
    scan.setSourceFileName(file);
    QVector<double> vec_a;
    QVector<double> vec_i;

    QStringList line = content.at(0).trimmed().split(QRegularExpression("\\s+"));

    if (line.size() < 2) {
        qDebug() << QString("StoeProImport::load(): No valid first line found in file %1").arg(file);
        return -1;
    }

    double startAng = line.at(0).toDouble();
    double stepSize = line.at(1).toDouble();

    QRegularExpression split("\\.\\s+");

    for (int k = 1; k < content.size(); ++k) {
        line = content.at(k).split(split);

        for (int j = 0; j < line.size(); ++j) {
            vec_i.push_back(line.at(j).toDouble());
        }
    }

    int p = vec_i.size();
    double endAng = startAng + p * stepSize;

    for (int i = 0; i < p; ++i) {
        vec_a.push_back(startAng + (double(i) / double(p - 1)) * (endAng - startAng));
    }

    scan.setDataAng(vec_a);
    scan.setDataInt(vec_i);
    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);

    return scanHeap.size();
}
