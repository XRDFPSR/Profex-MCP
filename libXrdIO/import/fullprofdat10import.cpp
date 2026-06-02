/***************************************************************************
                          fullprofdat10import.cpp  -  description
                             -------------------
    begin                : Sat Oct 03, 2009
    copyright            : (C) 2009 by Nicola Doebelin
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


#include "fullprofdat10import.h"

FullprofDat10Import::FullprofDat10Import(QObject *parent) :
    GenericImport(parent)
{
    extens << "dat";
    descr = QLatin1String("Fullprof DAT version 10");
}

bool FullprofDat10Import::isSupported(const QByteArray &ba)
{
    // not sure how to test for this file format. Usually
    // the first 6 lines are comments starting with "!"
    QStringList header(QString(ba).split(global::rxLineEnding));

    // we don't know how many lines header contains, so lets
    // iterate either up to 6, or to the number of lines header contains
    for (int i = 0; i < qMin(6, header.size()); ++i) {
        if (header.at(i).left(1) != "!") {
            return false;
        }
    }

    return true;
}


int FullprofDat10Import::load( const QString &file, QVector<Scan> &scanHeap, bool minimal )
{
    Q_UNUSED(minimal);

    qDebug() << QString("FullprofDat10Import::load(): Loading file %1").arg(file);
    QStringList content(BgmnFileIO::readTextFileLines(file));

    if (content.isEmpty()) {
        qDebug() << QString("FullprofDat10Import::load(): Could not open file %1").arg(file);
        return -1;
    }

    QVector<double> vec_a;       // angle
    QVector<double> vec_i;       // intensity

    QRegularExpression sep("[\\s;,]+");

    for (int n = 0; n < content.size(); ++n) {
        QString line = content.at(n).trimmed();

        // check for comment lines
        if (line.left(1) == "!" || line.left(1) == "%" || line.left(1) == "&" || line.left(2) == "//") {
            continue;
        }

        // check for empty lines
        if (line.isEmpty()) {
            continue;
        }

        QStringList vals = line.split(sep);

        // check for x and y values
        if (vals.size() < 2) {
            qDebug() << QString("FullprofDat10Import::load(): Incomplete line found in file %1").arg(file);
            continue;
        }

        vec_a.push_back(vals.at(0).toDouble());
        vec_i.push_back(vals.at(1).toDouble());
    }

    Scan scan(file, QColor(), 1);
    scan.setSourceFileName(file);
    scan.setDataAng(vec_a);
    scan.setDataInt(vec_i);

    scan.setTypes(Scan::XY | Scan::MEASURED);
    scanHeap.push_back(scan);

    return 1;
}
